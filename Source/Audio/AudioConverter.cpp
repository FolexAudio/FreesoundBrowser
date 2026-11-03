#include "AudioConverter.h"

namespace freesound
{

AudioConverter::AudioConverter()
{
    registerFormats();
}

void AudioConverter::registerFormats()
{
    formats.registerBasicFormats(); // WAV, AIFF, FLAC, Ogg Vorbis (when enabled in JUCE)

   #if JUCE_USE_MP3AUDIOFORMAT
    formats.registerFormat (new juce::MP3AudioFormat(), false);
   #endif
}

bool AudioConverter::isProToolsNativeExtension (const juce::String& ext)
{
    const auto e = ext.trimCharactersAtStart (".").toLowerCase();
    // Pro Tools natively prefers WAV / AIFF (and SD2 historically).
    // FLAC/OGG/MP3 typically need conversion for broad session compatibility.
    return e == "wav" || e == "aif" || e == "aiff" || e == "bwf";
}

juce::String AudioConverter::sanitizeFileName (const juce::String& name)
{
    auto s = name.trim();
    const juce::String illegal = "\\/:*?\"<>|";
    for (auto c : illegal)
        s = s.replaceCharacter (c, '_');
    if (s.length() > 80)
        s = s.substring (0, 80);
    if (s.isEmpty())
        s = "freesound";
    return s;
}

bool AudioConverter::probe (const juce::File& file,
                            juce::String& formatName,
                            double& sampleRate,
                            int& numChannels,
                            int& bitsPerSample,
                            juce::int64& lengthInSamples)
{
    std::unique_ptr<juce::AudioFormatReader> reader (formats.createReaderFor (file));
    if (reader == nullptr)
        return false;

    formatName = reader->getFormatName();
    sampleRate = reader->sampleRate;
    numChannels = (int) reader->numChannels;
    bitsPerSample = (int) reader->bitsPerSample;
    lengthInSamples = reader->lengthInSamples;
    return true;
}

ConversionResult AudioConverter::convertToWav (const juce::File& source,
                                               const juce::File& destinationWav,
                                               const ConversionTarget& target)
{
    ConversionResult result;
    result.outputFile = destinationWav;

    if (! source.existsAsFile())
    {
        result.error = "Source file missing: " + source.getFullPathName();
        return result;
    }

    std::unique_ptr<juce::AudioFormatReader> reader (formats.createReaderFor (source));
    if (reader == nullptr)
    {
        result.error = "Unsupported or corrupt audio: " + source.getFileName();
        return result;
    }

    result.sourceSampleRate = reader->sampleRate;
    result.sourceBitDepth = (int) reader->bitsPerSample;
    result.sourceFormatName = reader->getFormatName();

    const double outRate = target.sampleRate > 0.0 ? target.sampleRate : reader->sampleRate;
    const int outBits = (target.bitDepth == 16 || target.bitDepth == 24 || target.bitDepth == 32)
                            ? target.bitDepth : 24;
    const int outCh = target.numChannels > 0 ? target.numChannels : (int) reader->numChannels;

    destinationWav.getParentDirectory().createDirectory();
    destinationWav.deleteFile();

    juce::WavAudioFormat wavFormat;
    auto* rawStream = destinationWav.createOutputStream().release();
    if (rawStream == nullptr)
    {
        result.error = "Cannot create output: " + destinationWav.getFullPathName();
        return result;
    }

    // On success, createWriterFor takes ownership of rawStream
    std::unique_ptr<juce::AudioFormatWriter> writer (
        wavFormat.createWriterFor (rawStream,
                                   outRate,
                                   (unsigned int) outCh,
                                   outBits,
                                   {},
                                   0));

    if (writer == nullptr)
    {
        delete rawStream;
        result.error = "Failed to create WAV writer";
        return result;
    }

    const bool needsResample = std::abs (reader->sampleRate - outRate) > 0.5;
    const bool needsRemix = (int) reader->numChannels != outCh;

    if (! needsResample && ! needsRemix)
    {
        if (! writer->writeFromAudioReader (*reader, 0, reader->lengthInSamples))
        {
            result.error = "Failed writing PCM samples";
            destinationWav.deleteFile();
            return result;
        }
    }
    else
    {
        juce::AudioFormatReaderSource readerSource (reader.get(), false);
        juce::ResamplingAudioSource resampler (&readerSource, false, (int) reader->numChannels);
        resampler.setResamplingRatio (reader->sampleRate / outRate);

        const juce::int64 outLength = (juce::int64) std::llround (
            (double) reader->lengthInSamples * (outRate / reader->sampleRate));

        juce::AudioBuffer<float> buffer (outCh, 8192);
        juce::int64 written = 0;
        resampler.prepareToPlay (8192, outRate);

        // Channel mapping: upmix mono->stereo or take first N channels
        while (written < outLength)
        {
            const int toDo = (int) juce::jmin<juce::int64> (8192, outLength - written);
            juce::AudioBuffer<float> temp ((int) reader->numChannels, toDo);
            juce::AudioSourceChannelInfo info (&temp, 0, toDo);
            resampler.getNextAudioBlock (info);

            buffer.setSize (outCh, toDo, false, false, true);
            buffer.clear();

            if ((int) reader->numChannels == 1 && outCh >= 2)
            {
                buffer.copyFrom (0, 0, temp, 0, 0, toDo);
                buffer.copyFrom (1, 0, temp, 0, 0, toDo);
            }
            else
            {
                const int copyCh = juce::jmin (outCh, (int) reader->numChannels);
                for (int c = 0; c < copyCh; ++c)
                    buffer.copyFrom (c, 0, temp, c, 0, toDo);
            }

            if (! writer->writeFromAudioSampleBuffer (buffer, 0, toDo))
            {
                result.error = "Failed writing resampled PCM";
                destinationWav.deleteFile();
                return result;
            }

            written += toDo;
        }
    }

    writer.reset(); // flush header
    result.ok = destinationWav.existsAsFile() && destinationWav.getSize() > 0;
    if (! result.ok)
        result.error = "Output WAV missing after conversion";

    return result;
}

} // namespace freesound
