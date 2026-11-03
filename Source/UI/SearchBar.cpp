#include "SearchBar.h"
#include "LookAndFeel.h"

namespace freesound
{

SearchBar::SearchBar()
{
    title.setText ("Freesound", juce::dontSendNotification);
    title.setFont (juce::FontOptions (20.0f, juce::Font::bold));
    title.setColour (juce::Label::textColourId, AppLookAndFeel::accent());
    addAndMakeVisible (title);

    editor.setTextToShowWhenEmpty ("Search sounds, e.g. door slam, rain, whoosh...",
                                   AppLookAndFeel::textMuted());
    editor.addListener (this);
    editor.setFont (juce::FontOptions (15.0f));
    addAndMakeVisible (editor);

    searchButton.onClick = [this] { buttonClicked(); };
    addAndMakeVisible (searchButton);
}

SearchBar::~SearchBar()
{
    editor.removeListener (this);
}

void SearchBar::setQuery (const juce::String& q)
{
    editor.setText (q, false);
}

void SearchBar::resized()
{
    auto r = getLocalBounds();
    title.setBounds (r.removeFromLeft (110).reduced (0, 6));
    searchButton.setBounds (r.removeFromRight (100).reduced (4, 4));
    editor.setBounds (r.reduced (4, 4));
}

void SearchBar::textEditorReturnKeyPressed (juce::TextEditor&)
{
    buttonClicked();
}

void SearchBar::buttonClicked()
{
    if (onSearch)
        onSearch (getQuery());
}

} // namespace freesound
