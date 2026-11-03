#pragma once

#include <JuceHeader.h>

namespace freesound
{

struct ConversionTarget
{
    double sampleRate = 48000.0;
    int bitDepth = 24;          // 16 or 24 (written as PCM WAV)
    int numChannels = 0;        // 0 = keep source channel count
};

struct ConversionResult
{
    bool ok = false;
    juce::File outputFile;
    juce::String error;
    double sourceSampleRate = 0.0;
    int sourceBitDepth = 0;
    juce::String sourceFormatName;
};

/**
    Decodes common web/DAW formats (WAV, AIFF, FLAC, OGG, MP3) via
    juce::AudioFormatManager and writes PCM WAV suitable for Pro Tools.
*/
class AudioConverter
{
public:
    AudioConverter();

    /** Register readers (WAV/AIFF/FLAC/Ogg + MP3 if available). */
    void registerFormats();

    juce::AudioFormatManager& getFormatManager() { return formats; }

    /**
        If `source` is already PCM WAV matching the target rate/depth (within
        tolerance), optionally copies it; otherwise transcodes to WAV.
        Always writes to `destinationWav` (.wav).
    */
    ConversionResult convertToWav (const juce::File& source,
                                   const juce::File& destinationWav,
                                   const ConversionTarget& target);

    /** Probe format without converting. */
    bool probe (const juce::File& file,
                juce::String& formatName,
                double& sampleRate,
                int& numChannels,
                int& bitsPerSample,
                juce::int64& lengthInSamples);

    static bool isProToolsNativeExtension (const juce::String& ext);
    static juce::String sanitizeFileName (const juce::String& name);

private:
    juce::AudioFormatManager formats;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioConverter)
};

} // namespace freesound
