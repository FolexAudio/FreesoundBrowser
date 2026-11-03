#pragma once

#include "AuthManager.h"
#include "FreesoundModels.h"

namespace freesound
{

/**
    Asynchronous Freesound.org REST client.

    Uses juce::URL / WebInputStream on a background thread pool and delivers
    results via callbacks on the message thread.
*/
class FreesoundClient
{
public:
    static constexpr const char* kApiBase = "https://freesound.org/apiv2";

    using SearchCallback    = std::function<void (bool ok, SearchResult result, juce::String error)>;
    using DownloadCallback  = std::function<void (bool ok, juce::File localFile, juce::String error)>;
    using SoundCallback     = std::function<void (bool ok, SoundInfo sound, juce::String error)>;
    using SimpleCallback    = std::function<void (bool ok, juce::String message)>;

    explicit FreesoundClient (AuthManager& auth);
    ~FreesoundClient();

    AuthManager& getAuth() { return auth; }

    //--------------------------------------------------------------------------
    /** Text search: GET /apiv2/search/text/ */
    void search (const SearchFilters& filters, SearchCallback cb);

    /** Fetch a single sound with full fields: GET /apiv2/sounds/<id>/ */
    void getSound (int soundId, SoundCallback cb);

    /**
        Download original file (requires auth).
        Streams to `destination` (parent dirs created as needed).
    */
    void downloadSound (const SoundInfo& sound, const juce::File& destination, DownloadCallback cb);

    /** Download a preview URL (usually no auth required). */
    void downloadUrl (const juce::String& url,
                      const juce::File& destination,
                      DownloadCallback cb,
                      bool needsAuth = false);

    /**
        Exchange OAuth2 authorization code for access/refresh tokens.
        POST https://freesound.org/apiv2/oauth2/access_token/
    */
    void exchangeOAuthCode (const juce::String& code, SimpleCallback cb);

    void cancelAll();

    /** Fields requested from search endpoint. */
    static juce::String defaultSearchFields();

private:
    struct Job : juce::ThreadPoolJob
    {
        Job (const juce::String& name, std::function<void()> fn)
            : ThreadPoolJob (name), work (std::move (fn)) {}

        JobStatus runJob() override
        {
            if (work)
                work();
            return jobHasFinished;
        }

        std::function<void()> work;
    };

    void enqueue (const juce::String& name, std::function<void()> work);

    juce::String pairWithToken (juce::URL url) const;
    juce::var fetchJson (juce::URL url, bool needsAuth, juce::String& errorOut,
                         int* statusCode = nullptr) const;
    bool downloadToFile (juce::URL url, const juce::File& dest, bool needsAuth,
                         juce::String& errorOut) const;

    AuthManager& auth;
    juce::ThreadPool pool { 3 };
    std::atomic<bool> shuttingDown { false };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FreesoundClient)
};

} // namespace freesound
