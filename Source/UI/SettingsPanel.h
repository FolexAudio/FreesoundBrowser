#pragma once

#include <JuceHeader.h>
#include "../Audio/PreviewPlayer.h"
#include "../Freesound/AuthManager.h"
#include "../Import/SessionImporter.h"

namespace freesound
{

class SettingsPanel : public juce::Component
{
public:
    SettingsPanel (AuthManager& auth, SessionImporter& importer, PreviewPlayer& previewPlayer);
    ~SettingsPanel() override;

    void paint (juce::Graphics& g) override;
    void resized() override;
    void visibilityChanged() override;

    std::function<void()> onCredentialsSaved;
    std::function<void (juce::String code)> onOAuthCodeEntered;
    std::function<void()> onClose;

private:
    void loadFromAuth();
    void saveToAuth();
    void chooseAudioFilesFolder();
    void ensurePreviewDevice();

    AuthManager& auth;
    SessionImporter& importer;
    PreviewPlayer& previewPlayer;

    juce::Label heading { {}, "Settings" };
    juce::Label apiHelp;
    juce::Label tokenLabel { {}, "API Token" };
    juce::TextEditor tokenEditor;
    juce::Label clientIdLabel { {}, "OAuth Client ID" };
    juce::TextEditor clientIdEditor;
    juce::Label clientSecretLabel { {}, "OAuth Client Secret" };
    juce::TextEditor clientSecretEditor;
    juce::Label oauthCodeLabel { {}, "OAuth Auth Code" };
    juce::TextEditor oauthCodeEditor;
    juce::Label sessionLabel { {}, "Session sample rate / bit depth" };
    juce::ComboBox sampleRateBox;
    juce::ComboBox bitDepthBox;
    juce::Label ptFolderLabel { {}, "Pro Tools Audio Files folder (optional)" };
    juce::TextEditor ptFolderEditor;
    juce::TextButton browseFolderBtn { "Browse..." };

    juce::Label audioLabel { {}, "Preview audio output (Windows device — not Pro Tools ASIO)" };
    juce::Label audioStatus;
    std::unique_ptr<juce::AudioDeviceSelectorComponent> deviceSelector;
    juce::Viewport audioViewport;

    juce::TextButton saveBtn { "Save" };
    juce::TextButton closeBtn { "Close" };
    juce::TextButton clearBtn { "Clear secrets" };
    juce::Label statusLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SettingsPanel)
};

} // namespace freesound
