#pragma once

#include <JuceHeader.h>

namespace freesound
{

class WaveformDisplay : public juce::Component,
                        private juce::ChangeListener,
                        private juce::Timer
{
public:
    WaveformDisplay (juce::AudioFormatManager& formats,
                     juce::AudioThumbnailCache& cache);
    ~WaveformDisplay() override;

    void setSource (const juce::File& file);
    void clear();
    void setTransportSource (juce::AudioTransportSource* transport);

    void paint (juce::Graphics& g) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent& e) override;
    void mouseDrag (const juce::MouseEvent& e) override;

private:
    void changeListenerCallback (juce::ChangeBroadcaster*) override;
    void timerCallback() override;
    void seekFromMouse (const juce::MouseEvent& e);

    juce::AudioThumbnail thumbnail;
    juce::AudioTransportSource* transport = nullptr;
    juce::File current;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WaveformDisplay)
};

} // namespace freesound
