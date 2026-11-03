#pragma once

#include <JuceHeader.h>

namespace freesound
{

/**
    Stores Freesound API credentials in the user's application settings.

    Freesound supports:
      - Client credentials / API token (simplest for plugins):  Authorization: Token <api_key>
      - OAuth2 authorization code flow for user-scoped downloads of original files

    This manager persists the API token (and optional OAuth client id/secret + access token)
    via juce::PropertiesFile under the "FreesoundBrowser" application name.
*/
class AuthManager
{
public:
    AuthManager();
    ~AuthManager() = default;

    //--------------------------------------------------------------------------
    void setApiToken (const juce::String& token);
    juce::String getApiToken() const;

    void setClientId (const juce::String& id);
    juce::String getClientId() const;

    void setClientSecret (const juce::String& secret);
    juce::String getClientSecret() const;

    void setAccessToken (const juce::String& token);
    juce::String getAccessToken() const;

    void setRefreshToken (const juce::String& token);
    juce::String getRefreshToken() const;

    bool hasApiToken() const { return getApiToken().isNotEmpty(); }
    bool hasAccessToken() const { return getAccessToken().isNotEmpty(); }

    /** Prefer OAuth access token when present; otherwise fall back to API token. */
    juce::String getBearerOrTokenHeaderValue() const;

    /** "Token <key>" or "Bearer <oauth>" for Authorization header. */
    juce::String getAuthorizationHeader() const;

    void save();
    void reload();
    void clearSecrets();

    juce::File getSettingsFile() const;

private:
    std::unique_ptr<juce::PropertiesFile> props;
    juce::PropertiesFile::Options makeOptions() const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AuthManager)
};

} // namespace freesound
