#include "SessionImporter.h"

namespace freesound
{

SessionImporter::SessionImporter()
{
    getCacheRoot().createDirectory();
    getImportReadyFolder().createDirectory();
}

void SessionImporter::setSessionSpec (SessionAudioSpec spec)
{
    if (spec.sampleRate > 0.0)
        sessionSpec.sampleRate = spec.sampleRate;
    if (spec.bitDepth == 16 || spec.bitDepth == 24 || spec.bitDepth == 32)
        sessionSpec.bitDepth = spec.bitDepth;
    if (spec.numChannels > 0)
        sessionSpec.numChannels = spec.numChannels;
}

juce::File SessionImporter::getCacheRoot() const
{
    auto root = juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
                    .getChildFile ("FreesoundBrowser");
    return root.getChildFile ("Cache");
}

juce::File SessionImporter::getImportReadyFolder() const
{
    return getCacheRoot().getChildFile ("ImportReady");
}

void SessionImporter::setProToolsAudioFilesFolder (const juce::File& folder)
{
    ptAudioFiles = folder;
}

juce::File SessionImporter::makeTempDownloadPath (const SoundInfo& sound,
                                                  const juce::String& ext) const
{
    auto dir = getCacheRoot().getChildFile ("Downloads");
    dir.createDirectory();
    auto name = AudioConverter::sanitizeFileName (sound.name)
                + "_" + juce::String (sound.id)
                + "." + ext.trimCharactersAtStart (".").toLowerCase();
    return dir.getChildFile (name);
}

ConversionResult SessionImporter::prepareForProTools (const SoundInfo& sound,
                                                      const juce::File& downloaded,
                                                      const juce::String& outputBaseName)
{
    ConversionTarget target;
    target.sampleRate = sessionSpec.sampleRate;
    target.bitDepth = sessionSpec.bitDepth;
    target.numChannels = 0; // preserve source channel count

    juce::String baseName = AudioConverter::sanitizeFileName (outputBaseName);
    if (baseName.isEmpty())
        baseName = AudioConverter::sanitizeFileName (sound.name);

    if (baseName.isEmpty())
        baseName = "freesound_" + juce::String (sound.id);

    auto outWav = getImportReadyFolder().getChildFile (baseName + ".wav");
    // Avoid clobbering an existing import of the same name
    if (outWav.existsAsFile())
        outWav = getImportReadyFolder().getNonexistentChildFile (baseName, ".wav");

    // Always emit WAV for maximum Pro Tools compatibility (handles FLAC/OGG/MP3,
    // and re-samples / re-quantizes when the download is already WAV/AIFF).
    auto result = converter.convertToWav (downloaded, outWav, target);

    if (result.ok && ptAudioFiles.isDirectory())
    {
        auto copyDest = ptAudioFiles.getChildFile (outWav.getFileName());
        if (outWav.copyFileTo (copyDest))
            result.outputFile = copyDest;
    }

    return result;
}

juce::StringArray SessionImporter::pathsForDrag (const juce::File& wavFile)
{
    juce::StringArray paths;
    if (wavFile.existsAsFile())
        paths.add (wavFile.getFullPathName());
    return paths;
}

bool SessionImporter::revealInFinderOrExplorer (const juce::File& wavFile) const
{
    if (! wavFile.existsAsFile())
        return false;

    wavFile.revealToUser();
    return true;
}

} // namespace freesound
