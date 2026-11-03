#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
FreesoundBrowserAudioProcessor::FreesoundBrowserAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true))
{
    // Seed importer from last known host-ish defaults; editor/settings can override.
    freesound::SessionImporter::SessionAudioSpec spec;
    spec.sampleRate = 48000.0;
    spec.bitDepth = 24;
    importer.setSessionSpec (spec);
}

FreesoundBrowserAudioProcessor::~FreesoundBrowserAudioProcessor()
{
    client.cancelAll();
}

//==============================================================================
void FreesoundBrowserAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate > 0.0 ? sampleRate : 48000.0;

    freesound::SessionImporter::SessionAudioSpec spec = importer.getSessionSpec();
    spec.sampleRate = currentSampleRate;
    importer.setSessionSpec (spec);

    previewPlayer.prepareToPlay (samplesPerBlock, currentSampleRate);
    previewScratch.setSize (2, samplesPerBlock);
}

void FreesoundBrowserAudioProcessor::releaseResources()
{
    previewPlayer.releaseResources();
}

bool FreesoundBrowserAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
        && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // Output-only — no mic/line input (preview uses Settings → WASAPI out).
    return layouts.getMainInputChannelSet().isDisabled();
}

void FreesoundBrowserAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                                   juce::MidiBuffer& midi)
{
    juce::ignoreUnused (midi);
    juce::ScopedNoDenormals noDenormals;

    buffer.clear();

    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    // Host-callback preview path (fallback when independent WASAPI isn't open).
    if (! previewPlayer.shouldMixIntoHostCallback())
        return;

    if (previewScratch.getNumSamples() < numSamples || previewScratch.getNumChannels() < 2)
        previewScratch.setSize (2, numSamples, false, false, true);

    previewScratch.clear();
    juce::AudioSourceChannelInfo info (&previewScratch, 0, numSamples);
    previewPlayer.getNextAudioBlock (info);

    for (int ch = 0; ch < numChannels; ++ch)
    {
        const int srcCh = juce::jmin (ch, previewScratch.getNumChannels() - 1);
        buffer.addFrom (ch, 0, previewScratch, srcCh, 0, numSamples);
    }
}

//==============================================================================
juce::AudioProcessorEditor* FreesoundBrowserAudioProcessor::createEditor()
{
    return new FreesoundBrowserAudioProcessorEditor (*this);
}

void FreesoundBrowserAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    juce::ValueTree state ("FreesoundBrowser");
    state.setProperty ("sampleRate", importer.getSessionSpec().sampleRate, nullptr);
    state.setProperty ("bitDepth", importer.getSessionSpec().bitDepth, nullptr);
    state.setProperty ("ptFolder", importer.getProToolsAudioFilesFolder().getFullPathName(), nullptr);

    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void FreesoundBrowserAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
    {
        auto state = juce::ValueTree::fromXml (*xml);
        if (state.hasType ("FreesoundBrowser"))
        {
            freesound::SessionImporter::SessionAudioSpec spec = importer.getSessionSpec();
            spec.sampleRate = (double) state.getProperty ("sampleRate", spec.sampleRate);
            spec.bitDepth = (int) state.getProperty ("bitDepth", spec.bitDepth);
            importer.setSessionSpec (spec);

            const juce::String folder = state.getProperty ("ptFolder").toString();
            if (folder.isNotEmpty())
                importer.setProToolsAudioFilesFolder (juce::File (folder));
        }
    }
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new FreesoundBrowserAudioProcessor();
}
