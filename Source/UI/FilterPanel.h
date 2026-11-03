#pragma once

#include <JuceHeader.h>
#include "../Freesound/FreesoundModels.h"

namespace freesound
{

class FilterPanel : public juce::Component
{
public:
    FilterPanel();

    void resized() override;

    SearchFilters getFilters (const juce::String& query, int page) const;
    void setPageSize (int n);

    std::function<void()> onFiltersChanged;

private:
    juce::Label durationLabel { {}, "Duration (s)" };
    juce::Label minLabel { {}, "Min" };
    juce::Label maxLabel { {}, "Max" };
    juce::TextEditor minDuration;
    juce::TextEditor maxDuration;

    juce::Label licenseLabel { {}, "License" };
    juce::ComboBox licenseBox;

    juce::Label tagsLabel { {}, "Tags" };
    juce::TextEditor tagsEditor;

    juce::Label sortLabel { {}, "Sort" };
    juce::ComboBox sortBox;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FilterPanel)
};

} // namespace freesound
