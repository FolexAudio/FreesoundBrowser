"""
PTSL gRPC Client - Python wrapper for Pro Tools Scripting Library.

Talks to Pro Tools' embedded gRPC server on localhost:31416.
Handles connection registration, session_id tracking, and command dispatch.
"""

import json
import grpc
import PTSL_pb2
import PTSL_pb2_grpc


class PTSLClient:
    """Low-level gRPC client for Pro Tools Scripting Library."""

    # Pro Tools gRPC server address
    DEFAULT_ADDRESS = "localhost:31416"

    # PTSL version for this SDK (2025.10)
    VERSION_YEAR = 2025
    VERSION_MINOR = 10
    VERSION_REVISION = 0

    def __init__(self, address: str = None):
        self.address = address or self.DEFAULT_ADDRESS
        self.session_id = None
        self._channel = None
        self._stub = None

    def connect(self):
        """Create gRPC channel to Pro Tools."""
        channel_args = [
            ("grpc.max_receive_message_length", 2147483647),
        ]
        self._channel = grpc.insecure_channel(self.address, options=channel_args)
        self._stub = PTSL_pb2_grpc.PTSLStub(self._channel)
        return self

    def disconnect(self):
        """Close gRPC channel."""
        if self._channel:
            self._channel.close()
            self._channel = None
            self._stub = None

    def __enter__(self):
        self.connect()
        return self

    def __exit__(self, *args):
        self.disconnect()

    def _build_request(self, command_id: int, body_json: str = "{}") -> PTSL_pb2.Request:
        """Build a PTSL Request protobuf message."""
        header = PTSL_pb2.RequestHeader(
            command=command_id,
            version=self.VERSION_YEAR,
            version_minor=self.VERSION_MINOR,
            version_revision=self.VERSION_REVISION,
            session_id=self.session_id or "",
        )
        return PTSL_pb2.Request(
            header=header,
            request_body_json=body_json,
        )

    def send_command(self, command_name: str, body: dict = None) -> dict:
        """
        Send a command to Pro Tools and return the response.

        Args:
            command_name: One of the CId_* command names (e.g., "GetTrackList")
            body: Request body as a dict (will be JSON-serialized)

        Returns:
            dict with keys: status, body, error (parsed from protobuf Response)
        """
        if not self._stub:
            raise RuntimeError("Not connected. Call connect() first.")

        # Resolve command name to enum value
        command_id = getattr(PTSL_pb2, f"CId_{command_name}", None)
        if command_id is None:
            raise ValueError(f"Unknown command: {command_name}. Check PTSL_pb2.CId_* names.")

        body_json = json.dumps(body) if body else "{}"
        request = self._build_request(command_id, body_json)

        try:
            response = self._stub.SendGrpcRequest(request)
        except grpc.RpcError as e:
            return {
                "status": "GRPC_ERROR",
                "body": None,
                "error": f"gRPC error: {e.code()} - {e.details()}",
            }

        result = {
            "status": PTSL_pb2.TaskStatus.Name(response.header.status),
            "body": None,
            "error": None,
        }

        if response.response_body_json:
            try:
                result["body"] = json.loads(response.response_body_json)
            except json.JSONDecodeError:
                result["body"] = response.response_body_json

        if response.response_error_json:
            try:
                result["error"] = json.loads(response.response_error_json)
            except json.JSONDecodeError:
                result["error"] = response.response_error_json

        return result

    def send_streaming_command(self, command_name: str, body: dict = None, callback=None):
        """
        Send a streaming command (for events/PollEvents).

        Yields each intermediate response, or calls callback for each.
        """
        if not self._stub:
            raise RuntimeError("Not connected. Call connect() first.")

        command_id = getattr(PTSL_pb2, f"CId_{command_name}", None)
        if command_id is None:
            raise ValueError(f"Unknown command: {command_name}")

        body_json = json.dumps(body) if body else "{}"
        request = self._build_request(command_id, body_json)

        try:
            responses = self._stub.SendGrpcStreamingRequest(request)
            for response in responses:
                result = {
                    "status": PTSL_pb2.TaskStatus.Name(response.header.status),
                    "body": None,
                    "error": None,
                }
                if response.response_body_json:
                    try:
                        result["body"] = json.loads(response.response_body_json)
                    except json.JSONDecodeError:
                        result["body"] = response.response_body_json

                if callback:
                    callback(result)
                else:
                    yield result
        except grpc.RpcError as e:
            error_result = {
                "status": "GRPC_ERROR",
                "body": None,
                "error": f"gRPC error: {e.code()} - {e.details()}",
            }
            if callback:
                callback(error_result)
            else:
                yield error_result

    def register(self, company_name: str, application_name: str) -> dict:
        """Register connection with Pro Tools. Must be called first."""
        result = self.send_command("RegisterConnection", {
            "company_name": company_name,
            "application_name": application_name,
        })
        if result["status"] == "Completed" and result["body"]:
            self.session_id = result["body"].get("session_id")
        return result

    def host_ready(self) -> dict:
        """Check if Pro Tools host is ready."""
        return self.send_command("HostReadyCheck")


def list_all_commands() -> list[str]:
    """Return all available CId_* command names."""
    cmds = []
    for name in dir(PTSL_pb2):
        if name.startswith("CId_"):
            cmds.append(name[4:])  # Strip "CId_" prefix
    return sorted(cmds)
