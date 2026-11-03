#include "SettingsPanel.h"
#include "LookAndFeel.h"

namespace freesound
{

SettingsPanel::SettingsPanel (AuthManager& a, SessionImporter& i, PreviewPlayer& p)
    : auth (a), importer (i), previewPlayer (p)
{
    heading.setFont (juce::FontOptions (18.0f, juce::Font::bold));
    heading.setColour (juce::Label::textColourId, AppLookAndFeel::textPrimary());
    addAndMakeVisible (heading);

    apiHelp.setText (
        "Paste your Freesound API key as Token. OAuth Client ID/Secret + Auth Code "
        "are only needed for original-file Import downloads.",
        juce::dontSendNotification);
    apiHelp.setFont (juce::FontOptions (12.0f));
    apiHelp.setColour (juce::Label::textColourId, AppLookAndFeel::textMuted());
    apiHelp.setJustificationType (juce::Justification::topLeft);
    addAndMakeVisible (apiHelp);

    auto prepEditor = [this] (juce::TextEditor& e, bool password)
    {
        e.setFont (juce::FontOptions (13.0f));
        if (password)
            e.setPasswordCharacter (0x2022);
        addAndMakeVisible (e);
    };

    for (auto* l : { &tokenLabel, &clientIdLabel, &clientSecretLabel, &oauthCodeLabel,
                     &sessionLabel, &ptFolderLabel, &audioLabel })
    {
        l->setColour (juce::Label::textColourId, AppLookAndFeel::textMuted());
        l->setFont (juce::FontOptions (12.0f));
        addAndMakeVisible (*l);
    }

    prepEditor (tokenEditor, true);
    prepEditor (clientIdEditor, false);
    prepEditor (clientSecretEditor, true);
    prepEditor (oauthCodeEditor, false);
    prepEditor (ptFolderEditor, false);
    ptFolderEditor.setReadOnly (true);

    sampleRateBox.addItem ("44100 Hz", 1);
    sampleRateBox.addItem ("48000 Hz", 2);
    sampleRateBox.addItem ("96000 Hz", 3);
    sampleRateBox.setSelectedId (2, juce::dontSendNotification);

    bitDepthBox.addItem ("16-bit", 1);
    bitDepthBox.addItem ("24-bit", 2);
    bitDepthBox.addItem ("32-bit", 3);
    bitDepthBox.setSelectedId (2, juce::dontSendNotification);

    addAndMakeVisible (sampleRateBox);
    addAndMakeVisible (bitDepthBox);

    audioStatus.setFont (juce::FontOptions (12.0f));
    audioStatus.setColour (juce::Label::textColourId, AppLookAndFeel::accent());
    addAndMakeVisible (audioStatus);

    deviceSelector = std::make_unique<juce::AudioDeviceSelectorComponent> (
        previewPlayer.getDeviceManager(),
        0, 0,   // no inputs
        1, 2,   // 1-2 outputs
        false, false, false, false);
    audioViewport.setViewedComponent (deviceSelector.get(), false);
    audioViewport.setScrollBarsShown (true, false);
    addAndMakeVisible (audioViewport);

    browseFolderBtn.onClick = [this] { chooseAudioFilesFolder(); };
    saveBtn.onClick = [this] { saveToAuth(); };
    closeBtn.onClick = [this] { if (onClose) onClose(); };
    clearBtn.onClick = [this]
    {
        auth.clearSecrets();
        loadFromAuth();
        statusLabel.setText ("Secrets cleared.", juce::dontSendNotification);
    };

    clearBtn.setColour (juce::TextButton::buttonColourId, AppLookAndFeel::danger());
    addAndMakeVisible (browseFolderBtn);
    addAndMakeVisible (saveBtn);
    addAndMakeVisible (closeBtn);
    addAndMakeVisible (clearBtn);

    statusLabel.setColour (juce::Label::textColourId, AppLookAndFeel::success());
    addAndMakeVisible (statusLabel);

    loadFromAuth();
}

SettingsPanel::~SettingsPanel()
{
    audioViewport.setViewedComponent (nullptr, false);
    deviceSelector.reset();
}

void SettingsPanel::paint (juce::Graphics& g)
{
    // Fully opaque page so main UI never shows through
    g.fillAll (AppLookAndFeel::background());
    g.setColour (AppLookAndFeel::accent().withAlpha (0.45f));
    g.fillRect (0, 0, getWidth(), 2);
}

void SettingsPanel::visibilityChanged()
{
    if (isVisible())
    {
        loadFromAuth();
        ensurePreviewDevice();
    }
}

void SettingsPanel::ensurePreviewDevice()
{
    juce::String err;
    const bool ok = previewPlayer.startIndependentOutput (err);
    if (ok)
    {
        auto* dev = previewPlayer.getDeviceManager().getCurrentAudioDevice();
        audioStatus.setText (dev != nullptr
                                 ? ("Preview device ready: " + dev->getName())
                                 : "Preview device ready.",
                             juce::dontSendNotification);
        audioStatus.setColour (juce::Label::textColourId, AppLookAndFeel::success());
    }
    else
    {
        audioStatus.setText (err.isNotEmpty() ? err
                                              : "Could not open a Windows preview device.",
                             juce::dontSendNotification);
        audioStatus.setColour (juce::Label::textColourId, AppLookAndFeel::danger());
    }

    if (deviceSelector != nullptr)
        deviceSelector->resized();
}

void SettingsPanel::loadFromAuth()
{
    tokenEditor.setText (auth.getApiToken(), false);
    clientIdEditor.setText (auth.getClientId(), false);
    clientSecretEditor.setText (auth.getClientSecret(), false);
    ptFolderEditor.setText (importer.getProToolsAudioFilesFolder().getFullPathName(), false);

    const auto spec = importer.getSessionSpec();
    if (std::abs (spec.sampleRate - 44100.0) < 1.0) sampleRateBox.setSelectedId (1, juce::dontSendNotification);
    else if (std::abs (spec.sampleRate - 96000.0) < 1.0) sampleRateBox.setSelectedId (3, juce::dontSendNotification);
    else sampleRateBox.setSelectedId (2, juce::dontSendNotification);

    if (spec.bitDepth == 16) bitDepthBox.setSelectedId (1, juce::dontSendNotification);
    else if (spec.bitDepth == 32) bitDepthBox.setSelectedId (3, juce::dontSendNotification);
    else bitDepthBox.setSelectedId (2, juce::dontSendNotification);
}

void SettingsPanel::saveToAuth()
{
    auth.setApiToken (tokenEditor.getText());
    auth.setClientId (clientIdEditor.getText());
    auth.setClientSecret (clientSecretEditor.getText());
    auth.save();

    SessionImporter::SessionAudioSpec spec;
    switch (sampleRateBox.getSelectedId())
    {
        case 1: spec.sampleRate = 44100.0; break;
        case 3: spec.sampleRate = 96000.0; break;
        default: spec.sampleRate = 48000.0; break;
    }
    switch (bitDepthBox.getSelectedId())
    {
        case 1: spec.bitDepth = 16; break;
        case 3: spec.bitDepth = 32; break;
        default: spec.bitDepth = 24; break;
    }
    importer.setSessionSpec (spec);

    const auto folder = juce::File (ptFolderEditor.getText());
    if (folder.isDirectory())
        importer.setProToolsAudioFilesFolder (folder);

    ensurePreviewDevice();
    statusLabel.setText ("Saved.", juce::dontSendNotification);

    const auto code = oauthCodeEditor.getText().trim();
    if (code.isNotEmpty() && onOAuthCodeEntered)
        onOAuthCodeEntered (code);

    if (onCredentialsSaved)
        onCredentialsSaved();

    if (onClose)
        onClose();
}

void SettingsPanel::chooseAudioFilesFolder()
{
    auto chooser = std::make_shared<juce::FileChooser> (
        "Select Pro Tools session Audio Files folder",
        juce::File::getSpecialLocation (juce::File::userDocumentsDirectory),
        "*");

    chooser->launchAsync (juce::FileBrowserComponent::openMode
                          | juce::FileBrowserComponent::canSelectDirectories,
                          [this, chooser] (const juce::FileChooser& fc)
                          {
                              auto f = fc.getResult();
                              if (f.isDirectory())
                              {
                                  ptFolderEditor.setText (f.getFullPathName(), false);
                                  importer.setProToolsAudioFilesFolder (f);
                              }
                          });
}

void SettingsPanel::resized()
{
    auto r = getLocalBounds().reduced (16);
    heading.setBounds (r.removeFromTop (28));
    apiHelp.setBounds (r.removeFromTop (40));
    r.removeFromTop (6);

    auto placeField = [&r] (juce::Label& lab, juce::Component& ed, int h = 28)
    {
        lab.setBounds (r.removeFromTop (16));
        ed.setBounds (r.removeFromTop (h));
        r.removeFromTop (6);
    };

    placeField (tokenLabel, tokenEditor);
    placeField (clientIdLabel, clientIdEditor);
    placeField (clientSecretLabel, clientSecretEditor);
    placeField (oauthCodeLabel, oauthCodeEditor);

    sessionLabel.setBounds (r.removeFromTop (16));
    auto sr = r.removeFromTop (28);
    sampleRateBox.setBounds (sr.removeFromLeft (140));
    sr.removeFromLeft (8);
    bitDepthBox.setBounds (sr.removeFromLeft (120));
    r.removeFromTop (6);

    ptFolderLabel.setBounds (r.removeFromTop (16));
    auto fr = r.removeFromTop (28);
    browseFolderBtn.setBounds (fr.removeFromRight (100));
    fr.removeFromRight (6);
    ptFolderEditor.setBounds (fr);
    r.removeFromTop (8);

    audioLabel.setBounds (r.removeFromTop (16));
    audioStatus.setBounds (r.removeFromTop (18));
    r.removeFromTop (4);

    auto buttons = r.removeFromBottom (36);
    statusLabel.setBounds (r.removeFromBottom (22));
    r.removeFromBottom (6);

    saveBtn.setBounds (buttons.removeFromLeft (100));
    buttons.removeFromLeft (8);
    closeBtn.setBounds (buttons.removeFromLeft (100));
    buttons.removeFromLeft (8);
    clearBtn.setBounds (buttons.removeFromLeft (120));

    audioViewport.setBounds (r);
    if (deviceSelector != nullptr)
        deviceSelector->setBounds (0, 0, audioViewport.getWidth(), 260);
}

} // namespace freesound
