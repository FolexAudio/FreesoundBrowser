#include "PreviewComponent.h"
#include "LookAndFeel.h"

namespace freesound
{

PreviewComponent::PreviewComponent (PreviewPlayer& p)
    : player (p),
      waveform (p.getFormatManager(), p.getThumbnailCache())
{
    titleLabel.setFont (juce::FontOptions (14.0f, juce::Font::bold));
    titleLabel.setColour (juce::Label::textColourId, AppLookAndFeel::textPrimary());
    metaLabel.setFont (juce::FontOptions (12.0f));
    metaLabel.setColour (juce::Label::textColourId, AppLookAndFeel::textMuted());

    waveform.setTransportSource (&player.getTransport());

    playBtn.onClick = [this]
    {
        if (player.isPlaying())
            player.pause();
        else
            player.play();
        updateTransportButtons();
    };

    stopBtn.onClick = [this]
    {
        player.stop();
        updateTransportButtons();
    };

    position.setRange (0.0, 1.0, 0.001);
    position.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    position.setSliderSnapsToMousePosition (true);
    position.onDragStart = [this] { draggingSlider = true; };
    position.onDragEnd = [this]
    {
        draggingSlider = false;
        player.seek (position.getValue() * player.getLengthSeconds());
    };
    position.onValueChange = [this]
    {
        if (draggingSlider)
            player.seek (position.getValue() * player.getLengthSeconds());
    };

    player.onTransportChanged = [this] { updateTransportButtons(); };

    addAndMakeVisible (titleLabel);
    addAndMakeVisible (metaLabel);
    addAndMakeVisible (waveform);
    addAndMakeVisible (playBtn);
    addAndMakeVisible (stopBtn);
    addAndMakeVisible (position);

    startTimerHz (20);
}

PreviewComponent::~PreviewComponent()
{
    stopTimer();
    player.onTransportChanged = nullptr;
    waveform.setTransportSource (nullptr);
}

void PreviewComponent::setSoundMeta (const SoundInfo& sound)
{
    titleLabel.setText (sound.name, juce::dontSendNotification);
    metaLabel.setText (sound.username + "  ·  " + sound.getDurationDisplay()
                           + "  ·  " + sound.license + "  ·  " + sound.type.toUpperCase(),
                       juce::dontSendNotification);

    if (player.getCurrentFile().existsAsFile())
        waveform.setSource (player.getCurrentFile());
}

void PreviewComponent::clear()
{
    titleLabel.setText ("No selection", juce::dontSendNotification);
    metaLabel.setText ({}, juce::dontSendNotification);
    waveform.clear();
}

void PreviewComponent::resized()
{
    auto r = getLocalBounds().reduced (8);
    titleLabel.setBounds (r.removeFromTop (20));
    metaLabel.setBounds (r.removeFromTop (18));
    r.removeFromTop (4);

    auto transport = r.removeFromBottom (28);
    playBtn.setBounds (transport.removeFromLeft (70));
    transport.removeFromLeft (6);
    stopBtn.setBounds (transport.removeFromLeft (70));
    transport.removeFromLeft (8);
    position.setBounds (transport.reduced (0, 4));

    r.removeFromBottom (6);
    waveform.setBounds (r);
}

void PreviewComponent::timerCallback()
{
    if (! draggingSlider && player.getLengthSeconds() > 0.0)
        position.setValue (player.getPositionSeconds() / player.getLengthSeconds(),
                           juce::dontSendNotification);

    updateTransportButtons();
}

void PreviewComponent::updateTransportButtons()
{
    playBtn.setButtonText (player.isPlaying() ? "Pause" : "Play");
}

} // namespace freesound
