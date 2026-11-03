#include "FilterPanel.h"
#include "LookAndFeel.h"

namespace freesound
{

FilterPanel::FilterPanel()
{
    auto styleLabel = [] (juce::Label& l)
    {
        l.setColour (juce::Label::textColourId, AppLookAndFeel::textMuted());
        l.setFont (juce::FontOptions (12.0f));
    };

    styleLabel (durationLabel);
    styleLabel (minLabel);
    styleLabel (maxLabel);
    styleLabel (licenseLabel);
    styleLabel (tagsLabel);
    styleLabel (sortLabel);

    for (auto* e : { &minDuration, &maxDuration, &tagsEditor })
    {
        e->setInputRestrictions (32, e == &tagsEditor ? juce::String() : "0123456789.");
        e->setFont (juce::FontOptions (13.0f));
        addAndMakeVisible (*e);
    }

    minDuration.setTextToShowWhenEmpty ("0", AppLookAndFeel::textMuted());
    maxDuration.setTextToShowWhenEmpty ("any", AppLookAndFeel::textMuted());
    tagsEditor.setTextToShowWhenEmpty ("footstep metal impact...", AppLookAndFeel::textMuted());

    licenseBox.addItem ("Any", 1);
    licenseBox.addItem ("CC0 (public domain)", 2);
    licenseBox.addItem ("Attribution", 3);
    licenseBox.addItem ("Attribution NonCommercial", 4);
    licenseBox.setSelectedId (1, juce::dontSendNotification);

    sortBox.addItem ("Relevance", 1);
    sortBox.addItem ("Duration (long)", 2);
    sortBox.addItem ("Downloads", 3);
    sortBox.addItem ("Rating", 4);
    sortBox.addItem ("Created (new)", 5);
    sortBox.setSelectedId (1, juce::dontSendNotification);

    auto notify = [this]
    {
        if (onFiltersChanged)
            onFiltersChanged();
    };

    licenseBox.onChange = notify;
    sortBox.onChange = notify;

    addAndMakeVisible (durationLabel);
    addAndMakeVisible (minLabel);
    addAndMakeVisible (maxLabel);
    addAndMakeVisible (licenseLabel);
    addAndMakeVisible (tagsLabel);
    addAndMakeVisible (sortLabel);
    addAndMakeVisible (licenseBox);
    addAndMakeVisible (sortBox);
}

void FilterPanel::resized()
{
    auto r = getLocalBounds().reduced (8, 4);
    const int rowH = 28;
    const int labelW = 70;

    auto row1 = r.removeFromTop (rowH);
    durationLabel.setBounds (row1.removeFromLeft (90));
    minLabel.setBounds (row1.removeFromLeft (28));
    minDuration.setBounds (row1.removeFromLeft (56).reduced (0, 2));
    row1.removeFromLeft (6);
    maxLabel.setBounds (row1.removeFromLeft (28));
    maxDuration.setBounds (row1.removeFromLeft (56).reduced (0, 2));
    row1.removeFromLeft (12);
    sortLabel.setBounds (row1.removeFromLeft (36));
    sortBox.setBounds (row1.removeFromLeft (140).reduced (0, 2));

    r.removeFromTop (4);
    auto row2 = r.removeFromTop (rowH);
    licenseLabel.setBounds (row2.removeFromLeft (labelW));
    licenseBox.setBounds (row2.removeFromLeft (200).reduced (0, 2));
    row2.removeFromLeft (12);
    tagsLabel.setBounds (row2.removeFromLeft (40));
    tagsEditor.setBounds (row2.reduced (0, 2));
}

SearchFilters FilterPanel::getFilters (const juce::String& query, int page) const
{
    SearchFilters f;
    f.query = query;
    f.page = page;
    f.minDuration = minDuration.getText().getDoubleValue();
    f.maxDuration = maxDuration.getText().getDoubleValue();
    f.tags = tagsEditor.getText().trim();

    switch (licenseBox.getSelectedId())
    {
        case 2: f.licenseFilter = "Creative Commons 0"; break;
        case 3: f.licenseFilter = "Attribution"; break;
        case 4: f.licenseFilter = "Attribution NonCommercial"; break;
        default: break;
    }

    switch (sortBox.getSelectedId())
    {
        case 2: f.sort = "duration_desc"; break;
        case 3: f.sort = "downloads_desc"; break;
        case 4: f.sort = "rating_desc"; break;
        case 5: f.sort = "created_desc"; break;
        default: f.sort = "score"; break;
    }

    return f;
}

void FilterPanel::setPageSize (int n)
{
    juce::ignoreUnused (n);
}

} // namespace freesound
