#include "PtslClient.h"

namespace freesound
{

namespace
{
    juce::File projectToolsDir()
    {
        // Dev layout: <repo>/Tools/ptsl/import_to_protools.py
        auto exe = juce::File::getSpecialLocation (juce::File::currentExecutableFile);
        juce::Array<juce::File> candidates {
            exe.getParentDirectory().getChildFile ("Tools").getChildFile ("ptsl"),
            exe.getParentDirectory().getParentDirectory().getChildFile ("Tools").getChildFile ("ptsl"),
            juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
                .getChildFile ("FreesoundBrowser").getChildFile ("Tools").getChildFile ("ptsl")
        };

        // Walk up from exe looking for Tools/ptsl
        auto dir = exe.getParentDirectory();
        for (int i = 0; i < 8 && dir.exists(); ++i)
        {
            candidates.add (dir.getChildFile ("Tools").getChildFile ("ptsl"));
            dir = dir.getParentDirectory();
        }

        for (auto& c : candidates)
            if (c.getChildFile ("import_to_protools.py").existsAsFile())
                return c;

        return {};
    }
}

juce::File PtslClient::findHelperScript()
{
    auto dir = projectToolsDir();
    if (dir.isDirectory())
        return dir.getChildFile ("import_to_protools.py");
    return {};
}

juce::File PtslClient::findPython()
{
    const auto localPrograms = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
                                   .getSiblingFile ("Local")
                                   .getChildFile ("Programs")
                                   .getChildFile ("Python");

    juce::Array<juce::File> candidates {
        // Prefer real installs (grpc lives here) over PATH / Store stubs
        localPrograms.getChildFile ("Python314").getChildFile ("python.exe"),
        localPrograms.getChildFile ("Python313").getChildFile ("python.exe"),
        localPrograms.getChildFile ("Python312").getChildFile ("python.exe"),
        juce::File ("C:/Python314/python.exe"),
        juce::File ("C:/Python312/python.exe")
    };

    // PATH lookup — append only (never prefer over known installs)
    juce::ChildProcess where;
    if (where.start ("where python"))
    {
        where.waitForProcessToFinish (3000);
        auto out = where.readAllProcessOutput().trim();
        for (auto& line : juce::StringArray::fromLines (out))
        {
            juce::File f (line.trim());
            if (f.existsAsFile()
                && ! f.getFullPathName().containsIgnoreCase ("WindowsApps")
                && ! f.getFullPathName().containsIgnoreCase ("Local\\Python\\bin"))
                candidates.add (f);
        }
    }

    // Pick the first interpreter that can import grpc
    for (auto& c : candidates)
    {
        if (! c.existsAsFile())
            continue;

        juce::ChildProcess probe;
        juce::StringArray args;
        args.add (c.getFullPathName());
        args.add ("-c");
        args.add ("import grpc");
        if (probe.start (args) && probe.waitForProcessToFinish (8000) && probe.getExitCode() == 0)
            return c;
    }

    // Last resort: any existing candidate (setup still needed)
    for (auto& c : candidates)
        if (c.existsAsFile())
            return c;

    return {};
}

bool PtslClient::isProToolsListening (int port)
{
    juce::StreamingSocket sock;
    const bool ok = sock.connect ("127.0.0.1", port, 400);
    if (ok)
        sock.close();
    return ok;
}

PtslClient::Result PtslClient::importWavToSession (const juce::File& wavFile,
                                                   const juce::String& destination,
                                                   const juce::String& location)
{
    Result result;

    if (! wavFile.existsAsFile())
    {
        result.message = "WAV missing: " + wavFile.getFullPathName();
        return result;
    }

    if (! isProToolsListening())
    {
        result.message = "Pro Tools PTSL not listening on localhost:31416. "
                         "Open Pro Tools with a session and enable scripting.";
        return result;
    }

    auto python = findPython();
    auto script = findHelperScript();

    if (! python.existsAsFile())
    {
        result.message = "Python not found. Install Python 3 and grpcio "
                         "(pip install grpcio protobuf).";
        return result;
    }

    if (! script.existsAsFile())
    {
        result.message = "PTSL helper missing: Tools/ptsl/import_to_protools.py";
        return result;
    }

    juce::StringArray args;
    args.add (python.getFullPathName());
    args.add (script.getFullPathName());
    args.add ("--file");
    args.add (wavFile.getFullPathName());
    args.add ("--destination");
    args.add (destination);
    args.add ("--location");
    args.add (location);

    juce::ChildProcess proc;
    if (! proc.start (args))
    {
        result.message = "Failed to start PTSL helper process";
        return result;
    }

    proc.waitForProcessToFinish (120000);
    const auto output = proc.readAllProcessOutput().trim();
    result.rawJson = output;

    auto json = juce::JSON::parse (output);
    if (auto* o = json.getDynamicObject())
    {
        result.ok = (bool) o->getProperty ("ok");
        result.message = o->getProperty ("message").toString();
        if (result.message.isEmpty())
            result.message = o->getProperty ("error").toString();
        if (result.message.isEmpty())
            result.message = output;
        return result;
    }

    result.ok = false;
    result.message = output.isNotEmpty() ? output : "Empty response from PTSL helper";
    return result;
}

} // namespace freesound
