#pragma once

#include <JuceHeader.h>
#include "../Audio/AudioConverter.h"
#include "../Freesound/FreesoundModels.h"

namespace freesound
{

/**
    Prepares downloaded Freesound assets for Pro Tools and exposes them via
    filesystem paths suitable for:

      1. Drag-and-drop from the plugin UI onto the Pro Tools timeline / Clip List
      2. An "Import" action that writes session-compatible WAV into a cache
         folder (and optionally copies into a user-configured Audio Files folder)

    Note on AAX host APIs:
      Avid does not provide a public, general-purpose AAX API to inject arbitrary
      audio files into the Clip List or onto tracks from a non-AudioSuite effect.
      The supported, reliable workflows are drag-and-drop of file paths and
      placing WAV files where Pro Tools can import them. This class implements
      those host-safe paths and documents the extension point for future AAX
      host-service hooks when available in your SDK license.
*/
class SessionImporter
{
public:
    struct SessionAudioSpec
    {
        double sampleRate = 48000.0;
        int bitDepth = 24;
        int numChannels = 2;
    };

    SessionImporter();

    void setSessionSpec (SessionAudioSpec spec);
    SessionAudioSpec getSessionSpec() const { return sessionSpec; }

    /** Root cache: ~/Documents/FreesoundBrowser/Cache (or equivalent). */
    juce::File getCacheRoot() const;

    /** Where converted WAVs ready for Pro Tools are stored. */
    juce::File getImportReadyFolder() const;

    /**
        Optional: user-configured Pro Tools session "Audio Files" folder.
        When set, Import also copies the WAV there for one-step session pickup.
    */
    void setProToolsAudioFilesFolder (const juce::File& folder);
    juce::File getProToolsAudioFilesFolder() const { return ptAudioFiles; }

    /**
        Convert `downloaded` to session-compatible WAV.
        If `outputBaseName` is non-empty it is used for the .wav / track name;
        otherwise falls back to sanitized sound name + id.
    */
    ConversionResult prepareForProTools (const SoundInfo& sound,
                                         const juce::File& downloaded,
                                         const juce::String& outputBaseName = {});

    /** Build a StringArray of absolute paths for DragAndDropContainer. */
    static juce::StringArray pathsForDrag (const juce::File& wavFile);

    /**
        Host-integration hook. Currently opens the parent folder / reveals the
        file so the user can complete import if drag is unavailable, and returns
        the absolute path for scripting / external tools.

        Replace or extend this with AAX host-service calls when your AAX SDK
        agreement exposes file-import helpers.
    */
    bool revealInFinderOrExplorer (const juce::File& wavFile) const;

    juce::File makeTempDownloadPath (const SoundInfo& sound, const juce::String& ext) const;

private:
    SessionAudioSpec sessionSpec;
    juce::File ptAudioFiles;
    AudioConverter converter;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SessionImporter)
};

} // namespace freesound
