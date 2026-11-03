#pragma once

#include <JuceHeader.h>
#include "LookAndFeel.h"

namespace freesound
{

/** In-plugin rename UI — never use OS modal AlertWindows inside Pro Tools. */
class RenameImportDialog : public juce::Component,
                           private juce::TextEditor::Listener
{
public:
    RenameImportDialog()
    {
        title.setText ("Import to Pro Tools", juce::dontSendNotification);
        title.setFont (juce::FontOptions (18.0f, juce::Font::bold));
        title.setColour (juce::Label::textColourId, AppLookAndFeel::textPrimary());
        title.setJustificationType (juce::Justification::centred);
        addAndMakeVisible (title);

        hint.setText ("Name for the new track / clip:", juce::dontSendNotification);
        hint.setFont (juce::FontOptions (13.0f));
        hint.setColour (juce::Label::textColourId, AppLookAndFeel::textMuted());
        hint.setJustificationType (juce::Justification::centredLeft);
        addAndMakeVisible (hint);

        nameEditor.setFont (juce::FontOptions (15.0f));
        nameEditor.setJustification (juce::Justification::centredLeft);
        nameEditor.addListener (this);
        addAndMakeVisible (nameEditor);

        importBtn.setButtonText ("Import");
        cancelBtn.setButtonText ("Cancel");
        importBtn.onClick = [this]
        {
            if (onConfirm)
                onConfirm (nameEditor.getText().trim());
        };
        cancelBtn.onClick = [this]
        {
            if (onCancel)
                onCancel();
        };
        addAndMakeVisible (importBtn);
        addAndMakeVisible (cancelBtn);
    }

    ~RenameImportDialog() override
    {
        nameEditor.removeListener (this);
    }

    void setSuggestedName (const juce::String& name)
    {
        nameEditor.setText (name, false);
        nameEditor.selectAll();
        nameEditor.grabKeyboardFocus();
    }

    void paint (juce::Graphics& g) override
    {
        g.fillAll (juce::Colours::black.withAlpha (0.55f));

        auto card = getCardBounds().toFloat();
        g.setColour (AppLookAndFeel::panel());
        g.fillRoundedRectangle (card, 8.0f);
        g.setColour (AppLookAndFeel::accent().withAlpha (0.7f));
        g.drawRoundedRectangle (card, 8.0f, 1.5f);
    }

    void resized() override
    {
        auto card = getCardBounds();
        auto r = card.reduced (20);

        title.setBounds (r.removeFromTop (28));
        r.removeFromTop (10);
        hint.setBounds (r.removeFromTop (22));
        r.removeFromTop (6);
        nameEditor.setBounds (r.removeFromTop (34));
        r.removeFromTop (16);

        auto buttons = r.removeFromTop (34);
        importBtn.setBounds (buttons.removeFromRight (110));
        buttons.removeFromRight (10);
        cancelBtn.setBounds (buttons.removeFromRight (110));
    }

    std::function<void (juce::String name)> onConfirm;
    std::function<void()> onCancel;

private:
    juce::Rectangle<int> getCardBounds() const
    {
        const int w = juce::jmin (440, getWidth() - 40);
        const int h = 200;
        return { (getWidth() - w) / 2, (getHeight() - h) / 2, w, h };
    }

    void textEditorReturnKeyPressed (juce::TextEditor&) override
    {
        if (onConfirm)
            onConfirm (nameEditor.getText().trim());
    }

    void textEditorEscapeKeyPressed (juce::TextEditor&) override
    {
        if (onCancel)
            onCancel();
    }

    juce::Label title;
    juce::Label hint;
    juce::TextEditor nameEditor;
    juce::TextButton importBtn;
    juce::TextButton cancelBtn;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RenameImportDialog)
};

} // namespace freesound
