#pragma once

#include <JuceHeader.h>

namespace freesound
{

class AppLookAndFeel : public juce::LookAndFeel_V4
{
public:
    AppLookAndFeel();

    void drawButtonBackground (juce::Graphics& g, juce::Button& button,
                               const juce::Colour& backgroundColour,
                               bool shouldDrawButtonAsHighlighted,
                               bool shouldDrawButtonAsDown) override;

    void drawTextEditorOutline (juce::Graphics& g, int width, int height,
                                juce::TextEditor& textEditor) override;

    void drawTableHeaderBackground (juce::Graphics& g, juce::TableHeaderComponent& header) override;

    // Palette
    static juce::Colour background()   { return juce::Colour (0xff12151a); }
    static juce::Colour panel()        { return juce::Colour (0xff1a1f28); }
    static juce::Colour panelAlt()     { return juce::Colour (0xff222833); }
    static juce::Colour accent()       { return juce::Colour (0xff3d9cf0); }
    static juce::Colour accentDim()    { return juce::Colour (0xff2a6fad); }
    static juce::Colour textPrimary()  { return juce::Colour (0xffe8ecf1); }
    static juce::Colour textMuted()    { return juce::Colour (0xff9aa3b2); }
    static juce::Colour danger()       { return juce::Colour (0xffe05c5c); }
    static juce::Colour success()      { return juce::Colour (0xff4caf82); }
};

} // namespace freesound
