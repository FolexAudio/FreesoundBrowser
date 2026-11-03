#include "PluginEditor.h"
#include "Import/PtslClient.h"
#include "Audio/AudioConverter.h"

namespace
{
    using EditorPtr = juce::Component::SafePointer<FreesoundBrowserAudioProcessorEditor>;
}

//==============================================================================
FreesoundBrowserAudioProcessorEditor::FreesoundBrowserAudioProcessorEditor (
    FreesoundBrowserAudioProcessor& p)
    : AudioProcessorEditor (&p),
      processor (p),
      preview (p.getPreviewPlayer()),
      settings (p.getAuth(), p.getImporter(), p.getPreviewPlayer())
{
    setLookAndFeel (&lookAndFeel);
    setResizable (true, true);
    setResizeLimits (780, 560, 1600, 1200);
    setSize (980, 720);

    searchBar.onSearch = [this] (juce::String)
    {
        currentPage = 1;
        runSearch (currentPage);
    };

    resultsTable.onPreview = [this] (const freesound::SoundInfo& s) { handlePreview (s); };
    resultsTable.onImport  = [this] (const freesound::SoundInfo& s) { handleImport (s); };
    resultsTable.onSelectionChanged = [this] (const freesound::SoundInfo& s)
    {
        preview.setSoundMeta (s);
    };

    settings.onClose = [this] { showSettings (false); };
    settings.onCredentialsSaved = [this]
    {
        setStatus ("Credentials saved.", false);
    };
    settings.onOAuthCodeEntered = [this] (juce::String code)
    {
        setStatus ("Exchanging OAuth code...", false);
        const EditorPtr safe (this);
        processor.getClient().exchangeOAuthCode (code,
            [safe] (bool ok, juce::String message)
            {
                if (safe != nullptr)
                    safe->setStatus (message, ! ok);
            });
    };

    settingsBtn.onClick = [this] { showSettings (true); };
    prevPageBtn.onClick = [this]
    {
        if (currentPage > 1)
            runSearch (currentPage - 1);
    };
    nextPageBtn.onClick = [this]
    {
        runSearch (currentPage + 1);
    };

    pageLabel.setJustificationType (juce::Justification::centred);
    pageLabel.setColour (juce::Label::textColourId, freesound::AppLookAndFeel::textMuted());
    statusLabel.setColour (juce::Label::textColourId, freesound::AppLookAndFeel::textMuted());
    resultsCountLabel.setColour (juce::Label::textColourId, freesound::AppLookAndFeel::textMuted());
    resultsCountLabel.setJustificationType (juce::Justification::centredLeft);

    addAndMakeVisible (searchBar);
    addAndMakeVisible (filterPanel);
    addAndMakeVisible (resultsTable);
    addAndMakeVisible (preview);
    addAndMakeVisible (settingsBtn);
    addAndMakeVisible (prevPageBtn);
    addAndMakeVisible (nextPageBtn);
    addAndMakeVisible (pageLabel);
    addAndMakeVisible (statusLabel);
    addAndMakeVisible (resultsCountLabel);

    addChildComponent (settings);
    settings.setVisible (false);

    addChildComponent (renameDialog);
    renameDialog.setVisible (false);
    renameDialog.onCancel = [this] { hideRenameDialog(); setStatus ("Import cancelled.", false); };
    renameDialog.onConfirm = [this] (juce::String name)
    {
        hideRenameDialog();
        beginImportWithName (pendingImportSound, std::move (name));
    };

    preview.clear();
    setStatus (processor.getAuth().hasApiToken() || processor.getAuth().hasAccessToken()
                   ? "Ready. Enter a query and press Search."
                   : "Set your Freesound API token in Settings to begin.",
               false);

    if (! processor.getAuth().hasApiToken() && ! processor.getAuth().hasAccessToken())
        showSettings (true);
}

FreesoundBrowserAudioProcessorEditor::~FreesoundBrowserAudioProcessorEditor()
{
    setLookAndFeel (nullptr);
}

void FreesoundBrowserAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (freesound::AppLookAndFeel::background());

    g.setColour (freesound::AppLookAndFeel::accent().withAlpha (0.5f));
    g.fillRect (0, 0, getWidth(), 2);
}

void FreesoundBrowserAudioProcessorEditor::resized()
{
    auto r = getLocalBounds().reduced (10);

    auto top = r.removeFromTop (40);
    settingsBtn.setBounds (top.removeFromRight (100).reduced (0, 4));
    searchBar.setBounds (top);

    filterPanel.setBounds (r.removeFromTop (72));
    r.removeFromTop (6);

    auto bottom = r.removeFromBottom (150);
    auto pager = r.removeFromBottom (32);
    statusLabel.setBounds (r.removeFromBottom (22));
    r.removeFromBottom (4);

    resultsCountLabel.setBounds (pager.removeFromLeft (220));
    nextPageBtn.setBounds (pager.removeFromRight (70).reduced (2));
    pageLabel.setBounds (pager.removeFromRight (80));
    prevPageBtn.setBounds (pager.removeFromRight (70).reduced (2));

    resultsTable.setBounds (r);
    preview.setBounds (bottom.reduced (0, 4));

    // Settings is a full-page opaque view
    settings.setBounds (getLocalBounds());
    renameDialog.setBounds (getLocalBounds());
}

void FreesoundBrowserAudioProcessorEditor::showSettings (bool show)
{
    const bool showMain = ! show;

    searchBar.setVisible (showMain);
    filterPanel.setVisible (showMain);
    resultsTable.setVisible (showMain);
    preview.setVisible (showMain);
    settingsBtn.setVisible (showMain);
    prevPageBtn.setVisible (showMain);
    nextPageBtn.setVisible (showMain);
    pageLabel.setVisible (showMain);
    statusLabel.setVisible (showMain);
    resultsCountLabel.setVisible (showMain);

    settings.setVisible (show);
    if (show)
        settings.toFront (true);

    resized();
}

void FreesoundBrowserAudioProcessorEditor::setStatus (const juce::String& text, bool isError)
{
    statusLabel.setText (text, juce::dontSendNotification);
    statusLabel.setColour (juce::Label::textColourId,
                           isError ? freesound::AppLookAndFeel::danger()
                                   : freesound::AppLookAndFeel::textMuted());
}

void FreesoundBrowserAudioProcessorEditor::runSearch (int page)
{
    if (busy)
        return;

    if (! processor.getAuth().hasApiToken() && ! processor.getAuth().hasAccessToken())
    {
        setStatus ("API credentials required.", true);
        showSettings (true);
        return;
    }

    busy = true;
    currentPage = juce::jmax (1, page);
    setStatus ("Searching Freesound...", false);

    auto filters = filterPanel.getFilters (searchBar.getQuery(), currentPage);
    const EditorPtr safe (this);

    processor.getClient().search (filters,
        [safe] (bool ok, freesound::SearchResult result, juce::String error)
        {
            if (safe == nullptr)
                return;

            safe->busy = false;

            if (! ok)
            {
                safe->setStatus (error.isNotEmpty() ? error : "Search failed", true);
                return;
            }

            safe->totalCount = result.count;
            safe->resultsTable.setResults (result.results);
            safe->resultsCountLabel.setText (
                juce::String (result.count) + " results",
                juce::dontSendNotification);
            safe->pageLabel.setText ("Page " + juce::String (safe->currentPage),
                                     juce::dontSendNotification);
            safe->prevPageBtn.setEnabled (result.hasPrevious() || safe->currentPage > 1);
            safe->nextPageBtn.setEnabled (result.hasNext());
            safe->setStatus ("Loaded " + juce::String ((int) result.results.size())
                                 + " sounds.", false);
        });
}

void FreesoundBrowserAudioProcessorEditor::handlePreview (const freesound::SoundInfo& sound)
{
    if (busy)
        return;

    const auto url = sound.getBestPreviewUrl();
    if (url.isEmpty())
    {
        setStatus ("No preview available for this sound.", true);
        return;
    }

    busy = true;
    setStatus ("Downloading preview...", false);

    const auto ext = sound.getBestPreviewExtension();
    auto dest = processor.getImporter().getCacheRoot()
                    .getChildFile ("Previews")
                    .getChildFile (juce::String (sound.id) + "_preview." + ext);
    dest.getParentDirectory().createDirectory();

    auto playPreviewFile = [this, sound] (const juce::File& file) -> bool
    {
        juce::String err;
        if (! processor.getPreviewPlayer().loadFile (file, err))
        {
            setStatus (err, true);
            return false;
        }

        preview.setSoundMeta (sound);
        processor.getPreviewPlayer().play();

        juce::String status = "Playing preview: " + sound.name;
        if (processor.getPreviewPlayer().isUsingIndependentOutput())
            status += " (direct output)";
        else
            status += " — press Play in Pro Tools if you hear nothing";
        setStatus (status, false);
        return true;
    };

    if (dest.existsAsFile() && dest.getSize() > 0)
    {
        if (! playPreviewFile (dest))
            dest.deleteFile();
        busy = false;
        return;
    }

    const EditorPtr safe (this);
    processor.getClient().downloadUrl (url, dest,
        [safe, playPreviewFile] (bool ok, juce::File file, juce::String error)
        {
            if (safe == nullptr)
                return;

            safe->busy = false;

            if (! ok)
            {
                safe->setStatus (error.isNotEmpty() ? error : "Preview download failed", true);
                return;
            }

            playPreviewFile (file);
        },
        false);
}

void FreesoundBrowserAudioProcessorEditor::showRenameDialog (const freesound::SoundInfo& sound)
{
    pendingImportSound = sound;
    const auto suggested = freesound::AudioConverter::sanitizeFileName (sound.name);
    renameDialog.setSuggestedName (suggested.isNotEmpty() ? suggested : "freesound");
    renameDialog.setVisible (true);
    renameDialog.toFront (true);
    resized();
}

void FreesoundBrowserAudioProcessorEditor::hideRenameDialog()
{
    renameDialog.setVisible (false);
}

void FreesoundBrowserAudioProcessorEditor::handleImport (const freesound::SoundInfo& sound)
{
    if (busy)
        return;

    if (sound.downloadUrl.isEmpty())
    {
        setStatus ("No download URL. Check API / OAuth permissions.", true);
        return;
    }

    // Never use OS-modal AlertWindows inside Pro Tools — they crash the host.
    showRenameDialog (sound);
}

void FreesoundBrowserAudioProcessorEditor::beginImportWithName (const freesound::SoundInfo& sound,
                                                                juce::String customName)
{
    if (busy)
        return;

    customName = freesound::AudioConverter::sanitizeFileName (customName.trim());
    if (customName.isEmpty())
        customName = "freesound";

    busy = true;
    setStatus ("Downloading original file...", false);

    const auto ext = sound.type.isNotEmpty() ? sound.type : "wav";
    auto downloadPath = processor.getImporter().makeTempDownloadPath (sound, ext);
    const EditorPtr safe (this);

    auto startConvert = [safe, sound, customName] (juce::File downloaded)
    {
        if (safe == nullptr)
            return;

        safe->setStatus ("Converting to session WAV...", false);
        auto* importer = &safe->processor.getImporter();

        juce::Thread::launch ([safe, sound, downloaded, importer, customName]
        {
            const auto conv = importer->prepareForProTools (sound, downloaded, customName);

            juce::MessageManager::callAsync ([safe, sound, conv]
            {
                if (safe == nullptr)
                    return;

                if (! conv.ok)
                {
                    safe->busy = false;
                    safe->setStatus (conv.error.isNotEmpty() ? conv.error : "Conversion failed", true);
                    return;
                }

                safe->resultsTable.setDragFileForSound (sound.id, conv.outputFile);
                safe->setStatus ("Importing into Pro Tools via PTSL...", false);

                juce::Thread::launch ([safe, wav = conv.outputFile]
                {
                    const auto ptsl = freesound::PtslClient::importWavToSession (
                        wav, "NewTrack", "Selection");

                    juce::MessageManager::callAsync ([safe, ptsl, wav]
                    {
                        if (safe == nullptr)
                            return;

                        safe->busy = false;

                        if (ptsl.ok)
                        {
                            safe->setStatus (
                                ptsl.message.isNotEmpty()
                                    ? ptsl.message
                                    : ("Imported into Pro Tools: " + wav.getFileNameWithoutExtension()),
                                false);
                        }
                        else
                        {
                            safe->processor.getImporter().revealInFinderOrExplorer (wav);
                            safe->setStatus (
                                "PTSL import failed: "
                                    + (ptsl.message.isNotEmpty() ? ptsl.message
                                                                 : juce::String ("unknown"))
                                    + " — WAV saved at " + wav.getFullPathName(),
                                true);
                        }
                    });
                });
            });
        });
    };

    if (downloadPath.existsAsFile() && downloadPath.getSize() > 0)
    {
        startConvert (downloadPath);
        return;
    }

    processor.getClient().downloadSound (sound, downloadPath,
        [safe, startConvert] (bool ok, juce::File file, juce::String error)
        {
            if (safe == nullptr)
                return;

            if (! ok)
            {
                safe->busy = false;
                safe->setStatus (error.isNotEmpty() ? error
                    : "Download failed. Original downloads require a valid OAuth access token.",
                    true);
                return;
            }

            startConvert (file);
        });
}
