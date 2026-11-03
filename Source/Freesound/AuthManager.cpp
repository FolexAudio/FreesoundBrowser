#include "AuthManager.h"

namespace freesound
{

AuthManager::AuthManager()
{
    props = std::make_unique<juce::PropertiesFile> (makeOptions());
}

juce::PropertiesFile::Options AuthManager::makeOptions() const
{
    juce::PropertiesFile::Options o;
    o.applicationName     = "FreesoundBrowser";
    o.filenameSuffix      = "settings";
    o.osxLibrarySubFolder = "Application Support";
    o.folderName          = "FreesoundTools";
    o.commonToAllUsers    = false;
    o.ignoreCaseOfKeyNames = true;
    o.doNotSave           = false;
    o.millisecondsBeforeSaving = 200;
    o.storageFormat       = juce::PropertiesFile::storeAsXML;
    return o;
}

void AuthManager::setApiToken (const juce::String& token)
{
    props->setValue ("apiToken", token.trim());
}

juce::String AuthManager::getApiToken() const
{
    return props->getValue ("apiToken");
}

void AuthManager::setClientId (const juce::String& id)
{
    props->setValue ("clientId", id.trim());
}

juce::String AuthManager::getClientId() const
{
    return props->getValue ("clientId");
}

void AuthManager::setClientSecret (const juce::String& secret)
{
    props->setValue ("clientSecret", secret.trim());
}

juce::String AuthManager::getClientSecret() const
{
    return props->getValue ("clientSecret");
}

void AuthManager::setAccessToken (const juce::String& token)
{
    props->setValue ("accessToken", token.trim());
}

juce::String AuthManager::getAccessToken() const
{
    return props->getValue ("accessToken");
}

void AuthManager::setRefreshToken (const juce::String& token)
{
    props->setValue ("refreshToken", token.trim());
}

juce::String AuthManager::getRefreshToken() const
{
    return props->getValue ("refreshToken");
}

juce::String AuthManager::getBearerOrTokenHeaderValue() const
{
    if (hasAccessToken())
        return getAccessToken();
    return getApiToken();
}

juce::String AuthManager::getAuthorizationHeader() const
{
    if (hasAccessToken())
        return "Bearer " + getAccessToken();

    if (hasApiToken())
        return "Token " + getApiToken();

    return {};
}

void AuthManager::save()
{
    props->saveIfNeeded();
}

void AuthManager::reload()
{
    props = std::make_unique<juce::PropertiesFile> (makeOptions());
}

void AuthManager::clearSecrets()
{
    props->removeValue ("apiToken");
    props->removeValue ("clientSecret");
    props->removeValue ("accessToken");
    props->removeValue ("refreshToken");
    save();
}

juce::File AuthManager::getSettingsFile() const
{
    return props->getFile();
}

} // namespace freesound
