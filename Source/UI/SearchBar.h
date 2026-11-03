#pragma once

#include <JuceHeader.h>

namespace freesound
{

class SearchBar : public juce::Component,
                  private juce::TextEditor::Listener
{
public:
    SearchBar();
    ~SearchBar() override;

    void resized() override;

    juce::String getQuery() const { return editor.getText().trim(); }
    void setQuery (const juce::String& q);

    std::function<void (juce::String query)> onSearch;

private:
    void textEditorReturnKeyPressed (juce::TextEditor&) override;
    void buttonClicked();

    juce::Label title;
    juce::TextEditor editor;
    juce::TextButton searchButton { "Search" };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SearchBar)
};

} // namespace freesound
