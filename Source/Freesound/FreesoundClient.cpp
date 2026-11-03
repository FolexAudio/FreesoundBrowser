#include "FreesoundClient.h"

namespace freesound
{

namespace
{
    juce::String urlEncode (const juce::String& s)
    {
        return juce::URL::addEscapeChars (s, true);
    }
}

FreesoundClient::FreesoundClient (AuthManager& a)
    : auth (a)
{
}

FreesoundClient::~FreesoundClient()
{
    shuttingDown = true;
    pool.removeAllJobs (true, 8000);
}

juce::String FreesoundClient::defaultSearchFields()
{
    return "id,name,description,username,license,duration,samplerate,bitrate,"
           "bitdepth,channels,type,filesize,download,previews,tags,url";
}

void FreesoundClient::enqueue (const juce::String& name, std::function<void()> work)
{
    if (shuttingDown)
        return;

    pool.addJob (new Job (name, std::move (work)), true);
}

juce::String FreesoundClient::pairWithToken (juce::URL url) const
{
    // Prefer Authorization header; also append token query for endpoints that accept it
    const auto token = auth.getApiToken();
    if (token.isNotEmpty() && ! auth.hasAccessToken())
        return url.withParameter ("token", token).toString (true);

    return url.toString (true);
}

juce::var FreesoundClient::fetchJson (juce::URL url, bool needsAuth, juce::String& errorOut,
                                      int* statusCode) const
{
    errorOut.clear();

    juce::StringPairArray headers;
    const auto authHeader = auth.getAuthorizationHeader();
    if (needsAuth || authHeader.isNotEmpty())
    {
        if (authHeader.isEmpty())
        {
            errorOut = "Missing Freesound API credentials. Open Settings and enter your API token.";
            return {};
        }
        headers.set ("Authorization", authHeader);
    }

    headers.set ("Accept", "application/json");

    juce::String headerBlock;
    for (int i = 0; i < headers.size(); ++i)
        headerBlock << headers.getAllKeys()[i] << ": " << headers.getAllValues()[i] << "\n";

    const int timeoutMs = 30000;
    int localStatus = 0;
    auto opts = juce::URL::InputStreamOptions (juce::URL::ParameterHandling::inAddress)
                    .withExtraHeaders (headerBlock)
                    .withConnectionTimeoutMs (timeoutMs)
                    .withNumRedirectsToFollow (5)
                    .withStatusCode (&localStatus);

    juce::URL requestUrl (pairWithToken (url));
    std::unique_ptr<juce::InputStream> stream (requestUrl.createInputStream (opts));

    if (statusCode != nullptr)
        *statusCode = localStatus;

    if (stream == nullptr)
    {
        errorOut = "Network error contacting Freesound (" + juce::String (localStatus) + ")";
        return {};
    }

    const auto body = stream->readEntireStreamAsString();

    if (localStatus >= 400)
    {
        errorOut = "HTTP " + juce::String (localStatus) + ": " + body.substring (0, 240);
        return {};
    }

    auto parsed = juce::JSON::parse (body);
    if (parsed.isVoid())
    {
        errorOut = "Failed to parse JSON response";
        return {};
    }

    return parsed;
}

bool FreesoundClient::downloadToFile (juce::URL url, const juce::File& dest, bool needsAuth,
                                      juce::String& errorOut) const
{
    errorOut.clear();
    dest.getParentDirectory().createDirectory();

    juce::String headerBlock;
    const auto authHeader = auth.getAuthorizationHeader();
    if (needsAuth)
    {
        if (authHeader.isEmpty())
        {
            errorOut = "Authentication required to download original files.";
            return false;
        }
        headerBlock << "Authorization: " << authHeader << "\n";
    }
    else if (authHeader.isNotEmpty())
    {
        headerBlock << "Authorization: " << authHeader << "\n";
    }

    int status = 0;
    auto opts = juce::URL::InputStreamOptions (juce::URL::ParameterHandling::inAddress)
                    .withExtraHeaders (headerBlock)
                    .withConnectionTimeoutMs (120000)
                    .withNumRedirectsToFollow (8)
                    .withStatusCode (&status);

    juce::URL requestUrl (needsAuth ? pairWithToken (url) : url.toString (true));
    std::unique_ptr<juce::InputStream> in (requestUrl.createInputStream (opts));

    if (in == nullptr)
    {
        errorOut = "Download failed (network). HTTP " + juce::String (status);
        return false;
    }

    if (status >= 400)
    {
        errorOut = "Download HTTP " + juce::String (status);
        return false;
    }

    dest.deleteFile();
    juce::FileOutputStream out (dest);
    if (! out.openedOk())
    {
        errorOut = "Cannot write to " + dest.getFullPathName();
        return false;
    }

    auto written = out.writeFromInputStream (*in, -1);
    out.flush();

    if (written <= 0 || dest.getSize() <= 0)
    {
        errorOut = "Downloaded file is empty";
        dest.deleteFile();
        return false;
    }

    return true;
}

void FreesoundClient::search (const SearchFilters& filters, SearchCallback cb)
{
    enqueue ("search", [this, filters, cb]
    {
        juce::String error;
        juce::URL url (juce::String (kApiBase) + "/search/text/");
        url = url.withParameter ("query", filters.query.isNotEmpty() ? filters.query : "*")
                 .withParameter ("page", juce::String (filters.page))
                 .withParameter ("page_size", juce::String (filters.pageSize))
                 .withParameter ("fields", defaultSearchFields())
                 .withParameter ("sort", filters.sort);

        const auto filterStr = filters.buildFilterString();
        if (filterStr.isNotEmpty())
            url = url.withParameter ("filter", filterStr);

        int status = 0;
        auto json = fetchJson (url, true, error, &status);
        SearchResult result;

        if (error.isEmpty())
            result = parseSearchResponse (json);

        juce::MessageManager::callAsync ([cb, error, result, ok = error.isEmpty()]
        {
            if (cb)
                cb (ok, result, error);
        });
    });
}

void FreesoundClient::getSound (int soundId, SoundCallback cb)
{
    enqueue ("getSound", [this, soundId, cb]
    {
        juce::String error;
        juce::URL url (juce::String (kApiBase) + "/sounds/" + juce::String (soundId) + "/");
        url = url.withParameter ("fields", defaultSearchFields());

        auto json = fetchJson (url, true, error);
        SoundInfo sound;
        if (error.isEmpty())
            sound = parseSoundObject (json);

        juce::MessageManager::callAsync ([cb, error, sound, ok = error.isEmpty()]
        {
            if (cb)
                cb (ok, sound, error);
        });
    });
}

void FreesoundClient::downloadSound (const SoundInfo& sound, const juce::File& destination,
                                     DownloadCallback cb)
{
    downloadUrl (sound.downloadUrl, destination, std::move (cb), true);
}

void FreesoundClient::downloadUrl (const juce::String& url,
                                   const juce::File& destination,
                                   DownloadCallback cb,
                                   bool needsAuth)
{
    enqueue ("download", [this, url, destination, cb, needsAuth]
    {
        juce::String error;
        bool ok = false;

        if (url.isEmpty())
            error = "Empty download URL";
        else
            ok = downloadToFile (juce::URL (url), destination, needsAuth, error);

        juce::MessageManager::callAsync ([cb, ok, destination, error]
        {
            if (cb)
                cb (ok, destination, error);
        });
    });
}

void FreesoundClient::exchangeOAuthCode (const juce::String& code, SimpleCallback cb)
{
    enqueue ("oauth", [this, code, cb]
    {
        juce::String error;
        bool ok = false;
        juce::String message;

        const auto clientId = auth.getClientId();
        const auto clientSecret = auth.getClientSecret();

        if (clientId.isEmpty() || clientSecret.isEmpty())
        {
            error = "OAuth client id/secret missing in Settings.";
        }
        else
        {
            juce::URL url ("https://freesound.org/apiv2/oauth2/access_token/");
            auto post = url.withPOSTData (
                "client_id=" + urlEncode (clientId)
                + "&client_secret=" + urlEncode (clientSecret)
                + "&grant_type=authorization_code"
                + "&code=" + urlEncode (code));

            int status = 0;
            auto opts = juce::URL::InputStreamOptions (juce::URL::ParameterHandling::inPostData)
                            .withConnectionTimeoutMs (30000)
                            .withExtraHeaders ("Content-Type: application/x-www-form-urlencoded\n"
                                               "Accept: application/json\n")
                            .withStatusCode (&status);

            std::unique_ptr<juce::InputStream> stream (post.createInputStream (opts));
            if (stream == nullptr)
            {
                error = "OAuth token request failed (network)";
            }
            else
            {
                const auto body = stream->readEntireStreamAsString();
                auto json = juce::JSON::parse (body);
                if (auto* o = json.getDynamicObject())
                {
                    const auto access = o->getProperty ("access_token").toString();
                    const auto refresh = o->getProperty ("refresh_token").toString();
                    if (access.isNotEmpty())
                    {
                        auth.setAccessToken (access);
                        if (refresh.isNotEmpty())
                            auth.setRefreshToken (refresh);
                        auth.save();
                        ok = true;
                        message = "OAuth access token stored.";
                    }
                    else
                    {
                        error = "OAuth response missing access_token: " + body.substring (0, 200);
                    }
                }
                else
                {
                    error = "Bad OAuth response (HTTP " + juce::String (status) + ")";
                }
            }
        }

        juce::MessageManager::callAsync ([cb, ok, error, message]
        {
            if (cb)
                cb (ok, ok ? message : error);
        });
    });
}

void FreesoundClient::cancelAll()
{
    pool.removeAllJobs (false, 100);
}

} // namespace freesound
