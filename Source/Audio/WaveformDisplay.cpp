#include "WaveformDisplay.h"

namespace freesound
{

WaveformDisplay::WaveformDisplay (juce::AudioFormatManager& formats,
                                  juce::AudioThumbnailCache& cache)
    : thumbnail (512, formats, cache)
{
    thumbnail.addChangeListener (this);
}

WaveformDisplay::~WaveformDisplay()
{
    thumbnail.removeChangeListener (this);
    stopTimer();
}

void WaveformDisplay::setSource (const juce::File& file)
{
    current = file;
    thumbnail.setSource (new juce::FileInputSource (file));
    repaint();
}

void WaveformDisplay::clear()
{
    current = {};
    thumbnail.clear();
    repaint();
}

void WaveformDisplay::setTransportSource (juce::AudioTransportSource* t)
{
    transport = t;
    if (transport != nullptr)
        startTimerHz (30);
    else
        stopTimer();
}

void WaveformDisplay::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    g.setColour (juce::Colour (0xff1a1d23));
    g.fillRoundedRectangle (bounds, 4.0f);

    g.setColour (juce::Colour (0xff2a3140));
    g.drawRoundedRectangle (bounds.reduced (0.5f), 4.0f, 1.0f);

    if (thumbnail.getTotalLength() > 0.0)
    {
        g.setColour (juce::Colour (0xff5dade2));
        thumbnail.drawChannels (g, getLocalBounds().reduced (4),
                                0.0, thumbnail.getTotalLength(), 1.0f);

        if (transport != nullptr && thumbnail.getTotalLength() > 0.0)
        {
            const auto x = (float) (transport->getCurrentPosition() / thumbnail.getTotalLength())
                           * (float) getWidth();
            g.setColour (juce::Colours::white.withAlpha (0.85f));
            g.drawLine (x, 2.0f, x, (float) getHeight() - 2.0f, 1.5f);
        }
    }
    else
    {
        g.setColour (juce::Colours::white.withAlpha (0.35f));
        g.setFont (12.0f);
        g.drawText ("No waveform", getLocalBounds(), juce::Justification::centred);
    }
}

void WaveformDisplay::resized() {}

void WaveformDisplay::mouseDown (const juce::MouseEvent& e) { seekFromMouse (e); }
void WaveformDisplay::mouseDrag (const juce::MouseEvent& e) { seekFromMouse (e); }

void WaveformDisplay::seekFromMouse (const juce::MouseEvent& e)
{
    if (transport == nullptr || thumbnail.getTotalLength() <= 0.0)
        return;

    const double ratio = juce::jlimit (0.0, 1.0, (double) e.x / (double) getWidth());
    transport->setPosition (ratio * thumbnail.getTotalLength());
    repaint();
}

void WaveformDisplay::changeListenerCallback (juce::ChangeBroadcaster*)
{
    repaint();
}

void WaveformDisplay::timerCallback()
{
    if (transport != nullptr && transport->isPlaying())
        repaint();
}

} // namespace freesound
