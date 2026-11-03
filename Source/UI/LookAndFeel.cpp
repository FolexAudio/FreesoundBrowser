#include "LookAndFeel.h"

namespace freesound
{

AppLookAndFeel::AppLookAndFeel()
{
    setColour (juce::ResizableWindow::backgroundColourId, background());
    setColour (juce::TextButton::buttonColourId, accent());
    setColour (juce::TextButton::buttonOnColourId, accentDim());
    setColour (juce::TextButton::textColourOffId, textPrimary());
    setColour (juce::TextButton::textColourOnId, textPrimary());
    setColour (juce::TextEditor::backgroundColourId, panelAlt());
    setColour (juce::TextEditor::textColourId, textPrimary());
    setColour (juce::TextEditor::outlineColourId, juce::Colour (0xff3a4252));
    setColour (juce::TextEditor::focusedOutlineColourId, accent());
    setColour (juce::TextEditor::highlightColourId, accent().withAlpha (0.35f));
    setColour (juce::ComboBox::backgroundColourId, panelAlt());
    setColour (juce::ComboBox::outlineColourId, juce::Colour (0xff3a4252));
    setColour (juce::ComboBox::textColourId, textPrimary());
    setColour (juce::ComboBox::arrowColourId, textMuted());
    setColour (juce::PopupMenu::backgroundColourId, panel());
    setColour (juce::PopupMenu::textColourId, textPrimary());
    setColour (juce::PopupMenu::highlightedBackgroundColourId, accent());
    setColour (juce::Label::textColourId, textPrimary());
    setColour (juce::ListBox::backgroundColourId, panel());
    setColour (juce::ListBox::outlineColourId, juce::Colour (0xff2a3140));
    setColour (juce::ScrollBar::thumbColourId, juce::Colour (0xff3a4558));
    setColour (juce::TableHeaderComponent::backgroundColourId, panelAlt());
    setColour (juce::TableHeaderComponent::textColourId, textMuted());
    setColour (juce::TableHeaderComponent::outlineColourId, juce::Colour (0xff2a3140));
    setColour (juce::Slider::thumbColourId, accent());
    setColour (juce::Slider::trackColourId, juce::Colour (0xff3a4558));
    setColour (juce::Slider::backgroundColourId, panelAlt());
}

void AppLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& button,
                                           const juce::Colour& backgroundColour,
                                           bool shouldDrawButtonAsHighlighted,
                                           bool shouldDrawButtonAsDown)
{
    auto bounds = button.getLocalBounds().toFloat().reduced (0.5f);
    auto base = backgroundColour;

    if (! button.isEnabled())
        base = base.withMultipliedAlpha (0.4f);
    else if (shouldDrawButtonAsDown)
        base = base.darker (0.15f);
    else if (shouldDrawButtonAsHighlighted)
        base = base.brighter (0.08f);

    g.setColour (base);
    g.fillRoundedRectangle (bounds, 4.0f);
}

void AppLookAndFeel::drawTextEditorOutline (juce::Graphics& g, int width, int height,
                                            juce::TextEditor& textEditor)
{
    if (textEditor.isEnabled())
    {
        g.setColour (textEditor.hasKeyboardFocus (true)
                         ? findColour (juce::TextEditor::focusedOutlineColourId)
                         : findColour (juce::TextEditor::outlineColourId));
        g.drawRoundedRectangle (0.5f, 0.5f, (float) width - 1.0f, (float) height - 1.0f, 4.0f, 1.0f);
    }
}

void AppLookAndFeel::drawTableHeaderBackground (juce::Graphics& g, juce::TableHeaderComponent& header)
{
    g.fillAll (findColour (juce::TableHeaderComponent::backgroundColourId));
    g.setColour (findColour (juce::TableHeaderComponent::outlineColourId));
    g.fillRect (0, header.getHeight() - 1, header.getWidth(), 1);
}

} // namespace freesound
