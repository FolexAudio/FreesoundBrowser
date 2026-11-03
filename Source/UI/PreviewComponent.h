#pragma once

#include <JuceHeader.h>
#include "../Audio/PreviewPlayer.h"
#include "../Audio/WaveformDisplay.h"
#include "../Freesound/FreesoundModels.h"

namespace freesound
{

class PreviewComponent : public juce::Component,
                         private juce::Timer
{
public:
    explicit PreviewComponent (PreviewPlayer& player);
    ~PreviewComponent() override;

    void resized() override;
    void setSoundMeta (const SoundInfo& sound);
    void clear();

private:
    void timerCallback() override;
    void updateTransportButtons();

    PreviewPlayer& player;
    WaveformDisplay waveform;

    juce::Label titleLabel;
    juce::Label metaLabel;
    juce::TextButton playBtn { "Play" };
    juce::TextButton stopBtn { "Stop" };
    juce::Slider position;
    bool draggingSlider = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PreviewComponent)
};

} // namespace freesound
