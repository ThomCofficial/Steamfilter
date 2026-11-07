// ThomC SteamFilter VST3 processor — JUCE host bridge
#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "DSP/SteamDSP.h"
#include <array>
#include <atomic>

class SteamFilterAudioProcessor final : public juce::AudioProcessor {
public:
    SteamFilterAudioProcessor();
    ~SteamFilterAudioProcessor() override = default;
    const juce::String getName() const override { return "SteamFilter"; }
    void prepareToPlay(double sampleRate,int samplesPerBlock) override;
    void releaseResources() override {}
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    bool isBusesLayoutSupported(const BusesLayout&) const override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override {return true;}
    bool acceptsMidi() const override{return false;}
    bool producesMidi() const override{return false;}
    bool isMidiEffect() const override{return false;}
    double getTailLengthSeconds() const override{return 0.08;}
    int getNumPrograms() override{return 1;}
    int getCurrentProgram() override{return 0;}
    void setCurrentProgram(int) override{}
    const juce::String getProgramName(int) override{return "Default";}
    void changeProgramName(int,const juce::String&) override{}
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*,int) override;
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    using APVTS=juce::AudioProcessorValueTreeState;
    APVTS state;
    steam::Engine engine;
    std::atomic<float> dspLoad{0};
    void beginAnalyze(){engine.requestAnalysis();}
    bool getRecommendation(steam::Recommendation& r){return engine.takeRecommendation(r);}
    void applyRecommendation(const steam::Recommendation& r);
    void setParameterValue(const juce::String& id,float newValue);
    void selectFactoryPreset(int choice);
    void switchAB(int target);
    int currentAB() const{return bankIndex;}
private:
    float value(const char* parameter) const;
    juce::ValueTree bankA,bankB;
    int bankIndex=0;
    juce::SpinLock presetLock; // UI-thread only for preset management; never used in processBlock
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SteamFilterAudioProcessor)
};
