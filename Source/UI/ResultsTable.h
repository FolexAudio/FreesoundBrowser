#pragma once

#include <JuceHeader.h>
#include <map>
#include <vector>
#include "../Freesound/FreesoundModels.h"

namespace freesound
{

class ResultsTable : public juce::Component,
                     public juce::TableListBoxModel,
                     public juce::DragAndDropContainer
{
public:
    ResultsTable();

    void resized() override;

    void setResults (const std::vector<SoundInfo>& sounds);
    void clearResults();

    const SoundInfo* getSelectedSound() const;
    const SoundInfo* getSound (int row) const;
    int getNumRows() override;

    // TableListBoxModel
    void paintRowBackground (juce::Graphics&, int rowNumber, int width, int height,
                             bool rowIsSelected) override;
    void paintCell (juce::Graphics&, int rowNumber, int columnId,
                    int width, int height, bool rowIsSelected) override;
    void cellClicked (int rowNumber, int columnId, const juce::MouseEvent&) override;
    void cellDoubleClicked (int rowNumber, int columnId, const juce::MouseEvent&) override;
    juce::Component* refreshComponentForCell (int rowNumber, int columnId, bool isRowSelected,
                                              juce::Component* existing) override;
    juce::var getDragSourceDescription (const juce::SparseSet<int>& rowsToDescribe) override;

    std::function<void (const SoundInfo&)> onPreview;
    std::function<void (const SoundInfo&)> onImport;
    std::function<void (const SoundInfo&)> onSelectionChanged;

    /** Absolute path of a ready WAV for the given sound id (enables OS drag). */
    void setDragFileForSound (int soundId, const juce::File& wav);
    juce::File getDragFileForSound (int soundId) const;

private:
    enum Columns
    {
        colName = 1,
        colDuration,
        colAuthor,
        colLicense,
        colType,
        colPreview,
        colImport
    };

    juce::TableListBox table;
    std::vector<SoundInfo> data;
    std::map<int, juce::File> dragFiles;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ResultsTable)
};

//==============================================================================
class RowActionButton : public juce::TextButton
{
public:
    RowActionButton (juce::String text, std::function<void()> fn)
        : TextButton (std::move (text))
    {
        onClick = std::move (fn);
        setMouseCursor (juce::MouseCursor::PointingHandCursor);
    }
};

} // namespace freesound
