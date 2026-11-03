#include "PreviewPlayer.h"

namespace freesound
{

PreviewPlayer::PreviewPlayer()
{
    readAheadThread.startThread (juce::Thread::Priority::normal);
    mixer.addInputSource (&transport, false);
    // Don't open a device in the constructor — plugin constructors run in
    // awkward host contexts. Open lazily from the UI / first preview.
}

PreviewPlayer::~PreviewPlayer()
{
    stop();
    stopIndependentOutput();
    transport.setSource (nullptr);
    readerSource.reset();
    readAheadThread.stopThread (2000);
}

void PreviewPlayer::stopIndependentOutput()
{
    devicePlayer.setSource (nullptr);
    deviceManager.removeAudioCallback (&devicePlayer);
    deviceManager.closeAudioDevice();
    usingIndependentOutput = false;
}

bool PreviewPlayer::startIndependentOutput (juce::String& error)
{
    error.clear();

    if (usingIndependentOutput)
    {
        if (deviceManager.getCurrentAudioDevice() != nullptr)
            return true;
        stopIndependentOutput();
    }

    // Prefer shared Windows audio so we don't fight Pro Tools ASIO.
    juce::StringArray preferred { "Windows Audio", "Windows Audio (Exclusive Mode)",
                                  "DirectSound", "WASAPI" };

    bool typed = false;
    for (const auto& want : preferred)
    {
        for (auto* type : deviceManager.getAvailableDeviceTypes())
        {
            if (type->getTypeName().containsIgnoreCase (want)
                || want.containsIgnoreCase (type->getTypeName()))
            {
                type->scanForDevices();
                deviceManager.setCurrentAudioDeviceType (type->getTypeName(), true);
                typed = true;
                break;
            }
        }
        if (typed)
            break;
    }

    // Force a setup with 0 in / 2 out on the default device of the chosen type
    juce::AudioDeviceManager::AudioDeviceSetup setup;
    deviceManager.getAudioDeviceSetup (setup);
    setup.inputChannels.clear();
    setup.outputChannels.setRange (0, 2, true);
    setup.useDefaultInputChannels = false;
    setup.useDefaultOutputChannels = true;

    juce::String result = deviceManager.initialise (0, 2, nullptr, true, {}, &setup);

    if (result.isNotEmpty())
    {
        // Retry with simplest initialise
        result = deviceManager.initialiseWithDefaultDevices (0, 2);
    }

    if (result.isNotEmpty() || deviceManager.getCurrentAudioDevice() == nullptr)
    {
        error = "Preview output unavailable"
                + (result.isNotEmpty() ? (" (" + result + ")") : juce::String())
                + ". Pick a device under Settings → Preview audio output.";
        stopIndependentOutput();
        return false;
    }

    devicePlayer.setSource (&mixer);
    deviceManager.addAudioCallback (&devicePlayer);
    usingIndependentOutput = true;

    if (auto* dev = deviceManager.getCurrentAudioDevice())
    {
        preparedRate = dev->getCurrentSampleRate();
        preparedBlock = juce::jmax (32, dev->getCurrentBufferSizeSamples());
        reprepareTransport();
    }

    return true;
}

void PreviewPlayer::prepareToPlay (int samplesPerBlockExpected, double sampleRate)
{
    preparedBlock = samplesPerBlockExpected;
    preparedRate = sampleRate;

    if (! usingIndependentOutput)
        mixer.prepareToPlay (samplesPerBlockExpected, sampleRate);
}

void PreviewPlayer::releaseResources()
{
    if (! usingIndependentOutput)
        mixer.releaseResources();
}

void PreviewPlayer::getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill)
{
    if (usingIndependentOutput)
    {
        bufferToFill.clearActiveBufferRegion();
        return;
    }

    mixer.getNextAudioBlock (bufferToFill);
}

void PreviewPlayer::reprepareTransport()
{
    if (preparedRate > 0.0 && preparedBlock > 0)
    {
        mixer.prepareToPlay (preparedBlock, preparedRate);
        transport.prepareToPlay (preparedBlock, preparedRate);
    }
}

bool PreviewPlayer::loadFile (const juce::File& file, juce::String& error)
{
    stop();
    transport.setSource (nullptr);
    readerSource.reset();
    currentFile = {};

    if (! file.existsAsFile())
    {
        error = "Preview file not found";
        return false;
    }

    juce::String deviceError;
    if (! startIndependentOutput (deviceError))
    {
        // Keep going — host-callback mix may still work if PT is playing
        juce::ignoreUnused (deviceError);
    }

    auto* reader = converter.getFormatManager().createReaderFor (file);
    if (reader == nullptr)
    {
        error = "Cannot decode preview: " + file.getFileName();
        return false;
    }

    readerSource = std::make_unique<juce::AudioFormatReaderSource> (reader, true);
    transport.setSource (readerSource.get(),
                         32768,
                         &readAheadThread,
                         reader->sampleRate);

    reprepareTransport();
    transport.setGain (1.0f);
    currentFile = file;

    if (onTransportChanged)
        onTransportChanged();

    return true;
}

void PreviewPlayer::play()
{
    transport.start();
    if (onTransportChanged)
        onTransportChanged();
}

void PreviewPlayer::stop()
{
    transport.stop();
    transport.setPosition (0.0);
    if (onTransportChanged)
        onTransportChanged();
}

void PreviewPlayer::pause()
{
    transport.stop();
    if (onTransportChanged)
        onTransportChanged();
}

void PreviewPlayer::seek (double seconds)
{
    transport.setPosition (seconds);
}

void PreviewPlayer::setGain (float g)
{
    transport.setGain (g);
}

bool PreviewPlayer::isPlaying() const { return transport.isPlaying(); }
double PreviewPlayer::getLengthSeconds() const { return transport.getLengthInSeconds(); }
double PreviewPlayer::getPositionSeconds() const { return transport.getCurrentPosition(); }

} // namespace freesound
