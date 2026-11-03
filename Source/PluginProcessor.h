#pragma once

#include <JuceHeader.h>
#include "Audio/PreviewPlayer.h"
#include "Freesound/AuthManager.h"
#include "Freesound/FreesoundClient.h"
#include "Import/SessionImporter.h"

//==============================================================================
class FreesoundBrowserAudioProcessor : public juce::AudioProcessor
{
public:
    FreesoundBrowserAudioProcessor();
    ~FreesoundBrowserAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    //==============================================================================
    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //==============================================================================
    freesound::AuthManager& getAuth() { return auth; }
    freesound::FreesoundClient& getClient() { return client; }
    freesound::PreviewPlayer& getPreviewPlayer() { return previewPlayer; }
    freesound::SessionImporter& getImporter() { return importer; }

    double getHostSampleRate() const { return currentSampleRate; }

private:
    freesound::AuthManager auth;
    freesound::FreesoundClient client { auth };
    freesound::PreviewPlayer previewPlayer;
    freesound::SessionImporter importer;

    double currentSampleRate = 48000.0;
    juce::AudioBuffer<float> previewScratch;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FreesoundBrowserAudioProcessor)
};
