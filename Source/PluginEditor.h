#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "UI/LookAndFeel.h"
#include "UI/SearchBar.h"
#include "UI/FilterPanel.h"
#include "UI/ResultsTable.h"
#include "UI/SettingsPanel.h"
#include "UI/PreviewComponent.h"
#include "UI/RenameImportDialog.h"
#include "Freesound/FreesoundModels.h"

//==============================================================================
class FreesoundBrowserAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit FreesoundBrowserAudioProcessorEditor (FreesoundBrowserAudioProcessor&);
    ~FreesoundBrowserAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void runSearch (int page);
    void handlePreview (const freesound::SoundInfo& sound);
    void handleImport (const freesound::SoundInfo& sound);
    void beginImportWithName (const freesound::SoundInfo& sound, juce::String customName);
    void showRenameDialog (const freesound::SoundInfo& sound);
    void hideRenameDialog();
    void setStatus (const juce::String& text, bool isError = false);
    void showSettings (bool show);

    FreesoundBrowserAudioProcessor& processor;
    freesound::AppLookAndFeel lookAndFeel;

    freesound::SearchBar searchBar;
    freesound::FilterPanel filterPanel;
    freesound::ResultsTable resultsTable;
    freesound::PreviewComponent preview;
    freesound::SettingsPanel settings;
    freesound::RenameImportDialog renameDialog;

    juce::TextButton settingsBtn { "Settings" };
    juce::TextButton prevPageBtn { "Prev" };
    juce::TextButton nextPageBtn { "Next" };
    juce::Label pageLabel;
    juce::Label statusLabel;
    juce::Label resultsCountLabel;

    freesound::SoundInfo pendingImportSound;
    int currentPage = 1;
    int totalCount = 0;
    bool busy = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FreesoundBrowserAudioProcessorEditor)
};
