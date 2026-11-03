#pragma once

#include <JuceHeader.h>

namespace freesound
{

/**
    Talks to Pro Tools via the PTSL helper script (gRPC localhost:31416).

    Requires:
      - Pro Tools running with an open session
      - PTSL / scripting enabled (server on port 31416)
      - Python 3 + grpcio (see Tools/ptsl)
*/
class PtslClient
{
public:
    struct Result
    {
        bool ok = false;
        juce::String message;
        juce::String rawJson;
    };

    /** Resolve python + Tools/ptsl/import_to_protools.py next to the project / exe. */
    static juce::File findHelperScript();
    static juce::File findPython();

    static bool isProToolsListening (int port = 31416);

    /**
        Import a WAV into the open session (new track at edit selection by default).
        Runs synchronously — call from a worker thread.
    */
    static Result importWavToSession (const juce::File& wavFile,
                                      const juce::String& destination = "NewTrack",
                                      const juce::String& location = "Selection");
};

} // namespace freesound
