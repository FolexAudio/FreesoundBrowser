#pragma once

#include <JuceHeader.h>
#include "AudioConverter.h"

namespace freesound
{

/**
    Auditions preview files via an independent WASAPI/DirectSound device when
    possible (so Pro Tools / PatchWork don't need to be playing). Falls back to
    mixing into the plugin processBlock otherwise.
*/
class PreviewPlayer : public juce::AudioSource
{
public:
    PreviewPlayer();
    ~PreviewPlayer() override;

    void prepareToPlay (int samplesPerBlockExpected, double sampleRate) override;
    void releaseResources() override;
    void getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill) override;

    bool startIndependentOutput (juce::String& error);
    void stopIndependentOutput();

    bool isUsingIndependentOutput() const { return usingIndependentOutput; }
    bool shouldMixIntoHostCallback() const { return ! usingIndependentOutput; }

    juce::AudioDeviceManager& getDeviceManager() { return deviceManager; }

    bool loadFile (const juce::File& file, juce::String& error);

    void play();
    void stop();
    void pause();
    void seek (double seconds);
    void setGain (float g);

    bool isPlaying() const;
    double getLengthSeconds() const;
    double getPositionSeconds() const;
    juce::File getCurrentFile() const { return currentFile; }

    juce::AudioThumbnailCache& getThumbnailCache() { return thumbnailCache; }
    juce::AudioFormatManager& getFormatManager() { return converter.getFormatManager(); }
    juce::AudioTransportSource& getTransport() { return transport; }

    std::function<void()> onTransportChanged;

private:
    void reprepareTransport();

    AudioConverter converter;
    juce::AudioThumbnailCache thumbnailCache { 8 };

    juce::TimeSliceThread readAheadThread { "PreviewReader" };
    std::unique_ptr<juce::AudioFormatReaderSource> readerSource;
    juce::AudioTransportSource transport;
    juce::MixerAudioSource mixer;

    juce::AudioDeviceManager deviceManager;
    juce::AudioSourcePlayer devicePlayer;
    bool usingIndependentOutput = false;

    juce::File currentFile;
    double preparedRate = 44100.0;
    int preparedBlock = 512;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PreviewPlayer)
};

} // namespace freesound
