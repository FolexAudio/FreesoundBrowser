#pragma once

#include <JuceHeader.h>
#include <optional>
#include <vector>

namespace freesound
{

//==============================================================================
struct SoundInfo
{
    int id = 0;
    juce::String name;
    juce::String description;
    juce::String username;
    juce::String license;          // e.g. "Attribution", "Creative Commons 0"
    juce::String licenseUrl;
    juce::String tags;             // space / comma joined for display
    juce::StringArray tagList;

    double durationSeconds = 0.0;
    int samplerate = 0;
    int bitrate = 0;
    int bitdepth = 0;
    int channels = 0;
    juce::String type;             // wav, flac, aiff, ogg, mp3, ...
    juce::int64 filesize = 0;

    juce::String previewHqMp3;
    juce::String previewHqOgg;
    juce::String previewLqMp3;
    juce::String previewLqOgg;

    juce::String downloadUrl;      // authenticated download
    juce::String pageUrl;

    /** Prefer OGG — JUCE decodes it reliably; MP3 support is optional/fragile. */
    juce::String getBestPreviewUrl() const
    {
        if (previewHqOgg.isNotEmpty()) return previewHqOgg;
        if (previewLqOgg.isNotEmpty()) return previewLqOgg;
        if (previewHqMp3.isNotEmpty()) return previewHqMp3;
        return previewLqMp3;
    }

    juce::String getBestPreviewExtension() const
    {
        const auto url = getBestPreviewUrl().toLowerCase();
        if (url.contains (".ogg")) return "ogg";
        if (url.contains (".mp3")) return "mp3";
        return "ogg";
    }

    juce::String getDurationDisplay() const
    {
        const int totalMs = juce::roundToInt (durationSeconds * 1000.0);
        const int mins = totalMs / 60000;
        const int secs = (totalMs / 1000) % 60;
        const int ms   = totalMs % 1000;
        return juce::String::formatted ("%d:%02d.%03d", mins, secs, ms);
    }
};

//==============================================================================
struct SearchFilters
{
    juce::String query;
    double minDuration = 0.0;
    double maxDuration = 0.0;          // 0 = no max
    juce::String licenseFilter;        // "", "cc0", "attribution", "attribution noncommercial", ...
    juce::String tags;                 // space-separated, AND-matched via Freesound filter
    juce::String sort = "score";       // score, duration_desc, downloads_desc, rating_desc, created_desc
    int page = 1;
    int pageSize = 15;

    juce::String buildFilterString() const
    {
        juce::StringArray parts;

        if (minDuration > 0.0 || maxDuration > 0.0)
        {
            const juce::String lo = juce::String (minDuration, 2);
            const juce::String hi = maxDuration > 0.0 ? juce::String (maxDuration, 2) : "*";
            parts.add ("duration:[" + lo + " TO " + hi + "]");
        }

        if (licenseFilter.isNotEmpty())
        {
            // Freesound license filter tokens
            parts.add ("license:\"" + licenseFilter + "\"");
        }

        if (tags.isNotEmpty())
        {
            auto tagTokens = juce::StringArray::fromTokens (tags, " ,;", "");
            tagTokens.trim();
            tagTokens.removeEmptyStrings();
            for (const auto& t : tagTokens)
                parts.add ("tag:" + t);
        }

        return parts.joinIntoString (" ");
    }
};

//==============================================================================
struct SearchResult
{
    int count = 0;
    juce::String nextUrl;
    juce::String previousUrl;
    std::vector<SoundInfo> results;

    bool hasNext() const { return nextUrl.isNotEmpty(); }
    bool hasPrevious() const { return previousUrl.isNotEmpty(); }
};

//==============================================================================
inline SoundInfo parseSoundObject (const juce::var& obj)
{
    SoundInfo s;
    auto* o = obj.getDynamicObject();
    if (o == nullptr)
        return s;

    s.id          = (int) o->getProperty ("id");
    s.name        = o->getProperty ("name").toString();
    s.description = o->getProperty ("description").toString();
    s.username    = o->getProperty ("username").toString();
    s.license     = o->getProperty ("license").toString();
    s.durationSeconds = (double) o->getProperty ("duration");
    s.samplerate  = (int) o->getProperty ("samplerate");
    s.bitrate     = (int) o->getProperty ("bitrate");
    s.bitdepth    = (int) o->getProperty ("bitdepth");
    s.channels    = (int) o->getProperty ("channels");
    s.type        = o->getProperty ("type").toString();
    s.filesize    = (juce::int64) (double) o->getProperty ("filesize");
    s.downloadUrl = o->getProperty ("download").toString();
    s.pageUrl     = o->getProperty ("url").toString();

    if (auto* tags = o->getProperty ("tags").getArray())
    {
        for (const auto& t : *tags)
            s.tagList.add (t.toString());
        s.tags = s.tagList.joinIntoString (", ");
    }

    if (auto* previews = o->getProperty ("previews").getDynamicObject())
    {
        s.previewHqMp3 = previews->getProperty ("preview-hq-mp3").toString();
        s.previewHqOgg = previews->getProperty ("preview-hq-ogg").toString();
        s.previewLqMp3 = previews->getProperty ("preview-lq-mp3").toString();
        s.previewLqOgg = previews->getProperty ("preview-lq-ogg").toString();
    }

    // License may be a URL; keep a short label for the table
    if (s.license.containsIgnoreCase ("creativecommons.org/publicdomain/zero")
        || s.license.containsIgnoreCase ("/zero/"))
        s.licenseUrl = s.license, s.license = "CC0";
    else if (s.license.containsIgnoreCase ("by-nc"))
        s.licenseUrl = s.license, s.license = "CC BY-NC";
    else if (s.license.containsIgnoreCase ("by"))
        s.licenseUrl = s.license, s.license = "CC BY";
    else if (s.license.containsIgnoreCase ("sampling+"))
        s.licenseUrl = s.license, s.license = "Sampling+";

    return s;
}

inline SearchResult parseSearchResponse (const juce::var& root)
{
    SearchResult out;
    auto* o = root.getDynamicObject();
    if (o == nullptr)
        return out;

    out.count       = (int) o->getProperty ("count");
    out.nextUrl     = o->getProperty ("next").toString();
    out.previousUrl = o->getProperty ("previous").toString();

    if (auto* arr = o->getProperty ("results").getArray())
    {
        out.results.reserve ((size_t) arr->size());
        for (const auto& item : *arr)
            out.results.push_back (parseSoundObject (item));
    }

    return out;
}

} // namespace freesound
