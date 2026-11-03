cmake_minimum_required(VERSION 3.22)

project(FreesoundBrowser VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# ------------------------------------------------------------------------------
# Options
# ------------------------------------------------------------------------------
option(FREESOUND_BUILD_AAX "Build AAX format (requires AAX SDK)" ON)
option(FREESOUND_BUILD_VST3 "Build VST3 for development/testing" ON)
option(FREESOUND_BUILD_STANDALONE "Build standalone app for UI development" ON)

# Path to the Avid AAX SDK (set via -DAAX_SDK_PATH=... or env)
if(NOT DEFINED AAX_SDK_PATH)
    if(DEFINED ENV{AAX_SDK_PATH})
        set(AAX_SDK_PATH "$ENV{AAX_SDK_PATH}")
    else()
        set(AAX_SDK_PATH "" CACHE PATH "Path to the Avid AAX SDK")
    endif()
endif()

# ------------------------------------------------------------------------------
# JUCE via FetchContent
# ------------------------------------------------------------------------------
include(FetchContent)

FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.4
    GIT_SHALLOW    TRUE
)

FetchContent_MakeAvailable(JUCE)

# ------------------------------------------------------------------------------
# Plugin target
# ------------------------------------------------------------------------------
set(FREESOUND_FORMATS "")

if(FREESOUND_BUILD_STANDALONE)
    list(APPEND FREESOUND_FORMATS Standalone)
endif()

if(FREESOUND_BUILD_VST3)
    list(APPEND FREESOUND_FORMATS VST3)
endif()

if(FREESOUND_BUILD_AAX)
    if(AAX_SDK_PATH AND EXISTS "${AAX_SDK_PATH}")
        juce_set_aax_sdk_path("${AAX_SDK_PATH}")
        list(APPEND FREESOUND_FORMATS AAX)
        message(STATUS "AAX SDK found at: ${AAX_SDK_PATH}")
    else()
        message(WARNING
            "FREESOUND_BUILD_AAX is ON but AAX_SDK_PATH is not set or invalid. "
            "AAX format will be skipped. Set -DAAX_SDK_PATH=/path/to/aax-sdk")
    endif()
endif()

if(FREESOUND_FORMATS STREQUAL "")
    message(FATAL_ERROR "No plugin formats enabled. Enable Standalone, VST3, and/or AAX.")
endif()

juce_add_plugin(FreesoundBrowser
    COMPANY_NAME               "FreesoundTools"
    COMPANY_WEBSITE            "https://freesound.org"
    COMPANY_EMAIL              "support@example.com"
    BUNDLE_ID                  com.freesoundtools.browser
    PLUGIN_MANUFACTURER_CODE   FrSd
    PLUGIN_CODE                FsBr
    FORMATS                    ${FREESOUND_FORMATS}
    PRODUCT_NAME               "Freesound Browser"
    PLUGIN_NAME                "Freesound Browser"
    DESCRIPTION                "In-DAW Freesound.org SFX browser, preview, and importer"
    IS_SYNTH                   FALSE
    NEEDS_MIDI_INPUT           FALSE
    NEEDS_MIDI_OUTPUT          FALSE
    IS_MIDI_EFFECT             FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD    FALSE
    ICON_BIG                   "${CMAKE_CURRENT_SOURCE_DIR}/Assets/icon_big.png"
    ICON_SMALL                 "${CMAKE_CURRENT_SOURCE_DIR}/Assets/icon_small.png"
    VST3_CATEGORIES            "Fx|Tools"
    AAX_CATEGORY               AAX_ePlugInCategory_None
    AU_MAIN_TYPE               kAudioUnitType_Effect
)

juce_generate_juce_header(FreesoundBrowser)

target_sources(FreesoundBrowser
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginProcessor.h
        Source/PluginEditor.cpp
        Source/PluginEditor.h

        Source/Freesound/FreesoundModels.h
        Source/Freesound/AuthManager.cpp
        Source/Freesound/AuthManager.h
        Source/Freesound/FreesoundClient.cpp
        Source/Freesound/FreesoundClient.h

        Source/Audio/AudioConverter.cpp
        Source/Audio/AudioConverter.h
        Source/Audio/PreviewPlayer.cpp
        Source/Audio/PreviewPlayer.h
        Source/Audio/WaveformDisplay.cpp
        Source/Audio/WaveformDisplay.h

        Source/UI/LookAndFeel.cpp
        Source/UI/LookAndFeel.h
        Source/UI/SearchBar.cpp
        Source/UI/SearchBar.h
        Source/UI/FilterPanel.cpp
        Source/UI/FilterPanel.h
        Source/UI/ResultsTable.cpp
        Source/UI/ResultsTable.h
        Source/UI/SettingsPanel.cpp
        Source/UI/SettingsPanel.h
        Source/UI/PreviewComponent.cpp
        Source/UI/PreviewComponent.h

        Source/Import/SessionImporter.cpp
        Source/Import/SessionImporter.h
        Source/Import/PtslClient.cpp
        Source/Import/PtslClient.h
)

target_compile_definitions(FreesoundBrowser
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=1
        JUCE_USE_FLAC=1
        JUCE_USE_OGGVORBIS=1
        JUCE_USE_MP3AUDIOFORMAT=1
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0
        JUCE_REPORT_APP_USAGE=0
)

target_link_libraries(FreesoundBrowser
    PRIVATE
        juce::juce_audio_utils
        juce::juce_audio_formats
        juce::juce_dsp
        juce::juce_cryptography
        juce::juce_gui_extra
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

# On Windows, link WinHTTP / networking already covered by juce_core with CURL
if(WIN32)
    target_compile_definitions(FreesoundBrowser PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/Source" PREFIX "Source" FILES
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h
    Source/Freesound/FreesoundModels.h
    Source/Freesound/AuthManager.cpp
    Source/Freesound/AuthManager.h
    Source/Freesound/FreesoundClient.cpp
    Source/Freesound/FreesoundClient.h
    Source/Audio/AudioConverter.cpp
    Source/Audio/AudioConverter.h
    Source/Audio/PreviewPlayer.cpp
    Source/Audio/PreviewPlayer.h
    Source/Audio/WaveformDisplay.cpp
    Source/Audio/WaveformDisplay.h
    Source/UI/LookAndFeel.cpp
    Source/UI/LookAndFeel.h
    Source/UI/SearchBar.cpp
    Source/UI/SearchBar.h
    Source/UI/FilterPanel.cpp
    Source/UI/FilterPanel.h
    Source/UI/ResultsTable.cpp
    Source/UI/ResultsTable.h
    Source/UI/SettingsPanel.cpp
    Source/UI/SettingsPanel.h
    Source/UI/PreviewComponent.cpp
    Source/UI/PreviewComponent.h
    Source/Import/SessionImporter.cpp
    Source/Import/SessionImporter.h
)

message(STATUS "Freesound Browser formats: ${FREESOUND_FORMATS}")
