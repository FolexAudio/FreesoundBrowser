import sys
sys.path.insert(0, r"C:\Users\Alex\Desktop\New folder\Tools\ptsl\repo")
from ptsl_client import PTSLClient
import json
c = PTSLClient()
c.connect()
r = c.register("FreesoundTools", "FreesoundBrowser")
print("REGISTER", json.dumps(r, indent=2)[:2000])
r2 = c.host_ready()
print("READY", json.dumps(r2, indent=2)[:1000])
r3 = c.send_command("GetSessionName")
print("SESSION", json.dumps(r3, indent=2)[:1000])
c.disconnect()
