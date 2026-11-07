cmake_minimum_required(VERSION 3.22)
project(RenanSantosCrusher VERSION 1.0.0)

# O JUCE é baixado automaticamente na hora de compilar
include(FetchContent)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.4
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(RenanSantosCrusher
    COMPANY_NAME "Meme Audio"
    PLUGIN_MANUFACTURER_CODE Memo
    PLUGIN_CODE Rsc1
    FORMATS VST3 Standalone
    PRODUCT_NAME "Renan Santos Crusher"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE)

target_sources(RenanSantosCrusher PRIVATE Source/Plugin.cpp)

target_compile_definitions(RenanSantosCrusher PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(RenanSantosCrusher PRIVATE
    juce::juce_audio_utils
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags)
