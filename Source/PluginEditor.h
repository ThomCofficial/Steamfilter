// Dynamic vector interface: no faceplate pretending to be live controls.
#pragma once
#include <juce_gui_extra/juce_gui_extra.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include <array>
#include <memory>
#include <vector>

class BrassLookAndFeel final : public juce::LookAndFeel_V4 {
public:
    BrassLookAndFeel();
    void drawRotarySlider(juce::Graphics&,int,int,int,int,float,float,float,juce::Slider&) override;
    void drawButtonBackground(juce::Graphics&,juce::Button&,const juce::Colour&,bool,bool) override;
    void drawButtonText(juce::Graphics&,juce::TextButton&,bool,bool) override;
    void drawToggleButton(juce::Graphics&,juce::ToggleButton&,bool,bool) override;
};

class SteamFilterAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                             private juce::Timer {
public:
    explicit SteamFilterAudioProcessorEditor(SteamFilterAudioProcessor&);
    ~SteamFilterAudioProcessorEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;
private:
    struct WaveView final : juce::Component, private juce::Timer {
        SteamFilterAudioProcessor& owner;
        explicit WaveView(SteamFilterAudioProcessor& p):owner(p){startTimerHz(24);}
        void paint(juce::Graphics&) override;
        void timerCallback() override{if(isShowing())repaint();}
    };
    struct ModuleMeter final : juce::Component, public juce::SettableTooltipClient, private juce::Timer {
        SteamFilterAudioProcessor& owner;
        int index;float displayed=0;
        ModuleMeter(SteamFilterAudioProcessor& p,int n):owner(p),index(n){startTimerHz(24);}
        void paint(juce::Graphics&) override;
        void timerCallback() override;
    };
    struct SideMeter final : juce::Component, private juce::Timer {
        SteamFilterAudioProcessor& owner;
        bool isInput;float displayed=0;
        SideMeter(SteamFilterAudioProcessor& p,bool input):owner(p),isInput(input){startTimerHz(24);}
        void paint(juce::Graphics&) override;
        void timerCallback() override;
    };
    struct PressureGauge final : juce::Component, private juce::Timer {
        SteamFilterAudioProcessor& owner;float displayed=0;
        explicit PressureGauge(SteamFilterAudioProcessor& p):owner(p){startTimerHz(12);}
        void paint(juce::Graphics&) override;
        void timerCallback() override;
    };
    struct KnobInfo {juce::Slider* slider=nullptr;juce::Rectangle<float> r;};
    struct ButtonInfo {juce::Button* button=nullptr;juce::Rectangle<float> r;};
    SteamFilterAudioProcessor& processor;
    BrassLookAndFeel theme;
    juce::Image faceplate;
    std::vector<std::unique_ptr<juce::Slider>> knobs;
    std::vector<std::unique_ptr<SteamFilterAudioProcessor::APVTS::SliderAttachment>> knobsBind;
    // All legends and scales are painted on the approved engraved faceplate.
    std::vector<KnobInfo> knobRects;
    std::vector<std::unique_ptr<juce::TextButton>> textButtons;
    std::vector<std::unique_ptr<juce::ToggleButton>> toggleButtons;
    std::vector<std::unique_ptr<SteamFilterAudioProcessor::APVTS::ButtonAttachment>> toggleBind;
    std::vector<ButtonInfo> buttonRects;
    std::array<std::unique_ptr<ModuleMeter>,steam::kModuleCount> moduleMeters;
    std::unique_ptr<WaveView> waveform;
    std::unique_ptr<SideMeter> leftMeter,rightMeter;
    std::unique_ptr<PressureGauge> cpuGauge;
    std::unique_ptr<juce::FileChooser> fileChooser;
    juce::TextButton* analyzeButton=nullptr;
    juce::TextButton* suggestButton=nullptr;
    juce::TextButton* bankAButton=nullptr;
    juce::TextButton* bankBButton=nullptr;
    juce::TextButton* gainMatchButton=nullptr;
    juce::TextButton* deltaButton=nullptr;
    juce::TextButton* bypassButton=nullptr;
    juce::TextButton* sidechainButton=nullptr;
    juce::TextButton* voiceProtectButton=nullptr;
    std::array<juce::TextButton*,3> polishButtons{};
    std::array<std::array<juce::TextButton*,3>,4> modes{};
    std::array<juce::TextButton*,3> factoryButtons{};
    bool shuttingDown=false;
    bool suggestionAvailable=false;
    steam::Recommendation currentSuggestion{};
    juce::String analysisStatus="Play audio, then analyse";
    static constexpr float baseWidth=1672.f,baseHeight=941.f;
    float xScale() const{return float(getWidth())/baseWidth;}
    float yScale() const{return float(getHeight())/baseHeight;}
    juce::Slider* addKnob(const char* id,float x,float y,float size,bool main=false);
    juce::TextButton* addTextButton(const juce::String& text,float x,float y,float w,float h,std::function<void()> fn);
    juce::ToggleButton* addToggle(const char* id,float x,float y,float size);
    void addPanel(int index);
    void drawStaticUI(juce::Graphics&);
    void showPresets();
    void savePreset();
    void loadPreset();
    void timerCallback() override;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SteamFilterAudioProcessorEditor)
};