#include "ResultsTable.h"
#include "LookAndFeel.h"

namespace freesound
{

ResultsTable::ResultsTable()
{
    table.setModel (this);
    table.setHeaderHeight (28);
    table.setRowHeight (34);
    table.setMultipleSelectionEnabled (false);

    auto& h = table.getHeader();
    h.addColumn ("Name",     colName,     220, 80, 400, juce::TableHeaderComponent::defaultFlags);
    h.addColumn ("Duration", colDuration,  90, 60, 120, juce::TableHeaderComponent::defaultFlags);
    h.addColumn ("Author",   colAuthor,   120, 60, 200, juce::TableHeaderComponent::defaultFlags);
    h.addColumn ("License",  colLicense,   90, 60, 140, juce::TableHeaderComponent::defaultFlags);
    h.addColumn ("Format",   colType,      70, 50, 100, juce::TableHeaderComponent::defaultFlags);
    h.addColumn ("Preview",  colPreview,   80, 70, 100, juce::TableHeaderComponent::notSortable);
    h.addColumn ("Import",   colImport,    80, 70, 100, juce::TableHeaderComponent::notSortable);

    addAndMakeVisible (table);
}

void ResultsTable::resized()
{
    table.setBounds (getLocalBounds());
}

void ResultsTable::setResults (const std::vector<SoundInfo>& sounds)
{
    data = sounds;
    table.updateContent();
    table.deselectAllRows();
    repaint();
}

void ResultsTable::clearResults()
{
    data.clear();
    table.updateContent();
}

const SoundInfo* ResultsTable::getSelectedSound() const
{
    const int row = table.getSelectedRow();
    return getSound (row);
}

const SoundInfo* ResultsTable::getSound (int row) const
{
    if (row < 0 || row >= (int) data.size())
        return nullptr;
    return &data[(size_t) row];
}

int ResultsTable::getNumRows()
{
    return (int) data.size();
}

void ResultsTable::paintRowBackground (juce::Graphics& g, int /*rowNumber*/, int width, int height,
                                       bool rowIsSelected)
{
    g.fillAll (rowIsSelected ? AppLookAndFeel::accent().withAlpha (0.22f)
                             : AppLookAndFeel::panel());
    g.setColour (juce::Colour (0xff2a3140));
    g.fillRect (0, height - 1, width, 1);
}

void ResultsTable::paintCell (juce::Graphics& g, int rowNumber, int columnId,
                              int width, int height, bool /*rowIsSelected*/)
{
    if (auto* s = getSound (rowNumber))
    {
        juce::String text;
        switch (columnId)
        {
            case colName:     text = s->name; break;
            case colDuration: text = s->getDurationDisplay(); break;
            case colAuthor:   text = s->username; break;
            case colLicense:  text = s->license; break;
            case colType:     text = s->type.toUpperCase(); break;
            default: break;
        }

        g.setColour (AppLookAndFeel::textPrimary());
        g.setFont (juce::FontOptions (13.0f));
        g.drawText (text, 6, 0, width - 10, height, juce::Justification::centredLeft, true);
    }
}

void ResultsTable::cellClicked (int rowNumber, int /*columnId*/, const juce::MouseEvent&)
{
    if (auto* s = getSound (rowNumber))
        if (onSelectionChanged)
            onSelectionChanged (*s);
}

void ResultsTable::cellDoubleClicked (int rowNumber, int /*columnId*/, const juce::MouseEvent&)
{
    if (auto* s = getSound (rowNumber))
        if (onPreview)
            onPreview (*s);
}

juce::Component* ResultsTable::refreshComponentForCell (int rowNumber, int columnId,
                                                        bool /*isRowSelected*/,
                                                        juce::Component* existing)
{
    if (columnId != colPreview && columnId != colImport)
        return nullptr;

    auto* sound = getSound (rowNumber);
    if (sound == nullptr)
        return nullptr;

    const bool isPreview = columnId == colPreview;
            auto label = isPreview ? "Play" : "To PT";

    if (existing == nullptr)
    {
        auto fn = [this, rowNumber, isPreview]
        {
            if (auto* s = getSound (rowNumber))
            {
                if (isPreview) { if (onPreview) onPreview (*s); }
                else           { if (onImport)  onImport (*s); }
            }
        };
        auto* btn = new RowActionButton (label, std::move (fn));
        btn->setColour (juce::TextButton::buttonColourId,
                        isPreview ? AppLookAndFeel::panelAlt() : AppLookAndFeel::accent());
        return btn;
    }

    if (auto* btn = dynamic_cast<RowActionButton*> (existing))
    {
        btn->setButtonText (label);
        btn->onClick = [this, rowNumber, isPreview]
        {
            if (auto* s = getSound (rowNumber))
            {
                if (isPreview) { if (onPreview) onPreview (*s); }
                else           { if (onImport)  onImport (*s); }
            }
        };
        return btn;
    }

    return existing;
}

juce::var ResultsTable::getDragSourceDescription (const juce::SparseSet<int>& rowsToDescribe)
{
    if (rowsToDescribe.size() == 0)
        return {};

    const int row = rowsToDescribe[0];
    if (auto* s = getSound (row))
    {
        auto f = getDragFileForSound (s->id);
        if (f.existsAsFile())
            return f.getFullPathName();
    }
    return {};
}

void ResultsTable::setDragFileForSound (int soundId, const juce::File& wav)
{
    dragFiles[soundId] = wav;
}

juce::File ResultsTable::getDragFileForSound (int soundId) const
{
    auto it = dragFiles.find (soundId);
    return it != dragFiles.end() ? it->second : juce::File();
}

} // namespace freesound
