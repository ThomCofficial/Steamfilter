#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <chrono>

namespace {
using P=juce::RangedAudioParameter;
using APVTS=juce::AudioProcessorValueTreeState;
void pushFloat(std::vector<std::unique_ptr<P>>& v,const char* id,const char* label,float min,float max,float def,float step=0.f) {
    v.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{id,1},label,
        juce::NormalisableRange<float>{min,max,step},def));
}
void pushBool(std::vector<std::unique_ptr<P>>& v,const char* id,const char* label,bool def) {
    v.push_back(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{id,1},label,def));
}
}

SteamFilterAudioProcessor::SteamFilterAudioProcessor()
 : juce::AudioProcessor(juce::AudioProcessor::BusesProperties()
    .withInput("Input",juce::AudioChannelSet::stereo(),true)
    .withInput("Sidechain",juce::AudioChannelSet::stereo(),false)
    .withOutput("Output",juce::AudioChannelSet::stereo(),true)),
   state(*this,nullptr,"SteamFilterParameters",createParameterLayout()) {}

SteamFilterAudioProcessor::APVTS::ParameterLayout SteamFilterAudioProcessor::createParameterLayout() {
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;
    pushFloat(p,"input","Input (dB)",-24,24,0,.01f);
    pushFloat(p,"output","Output (dB)",-24,24,0,.01f);
    pushFloat(p,"globalMix","Global parallel mix",0,1,1.f,.005f);
    pushFloat(p,"maxGain","Combined maximum makeup gain (dB)",0,12,12.f,.1f);
    pushFloat(p,"detectorSensitivity","Overall detector sensitivity",0,1,.5f,.01f);
    pushFloat(p,"lookahead","Transient detector lookahead (ms)",0,20,4.f,.1f);
    pushFloat(p,"smoothing","Global detector smoothing",0,1,.5f,.01f);
    pushBool(p,"bypass","Global bypass",false);
    pushBool(p,"delta","Monitor removed signal",false);
    pushBool(p,"gainMatch","Automatic level match",false);
    pushBool(p,"sweepOn","SteamSweep enabled",true);
    pushBool(p,"breathOn","SteamBreath enabled",true);
    pushBool(p,"guardOn","SteamGuard enabled",true);
    pushBool(p,"sibilOn","SteamSibil enabled",true);
    pushBool(p,"riderOn","SteamRider enabled",true);
    pushBool(p,"liftOn","WhisperLift enabled",true);
    pushBool(p,"polishOn","SteamPolish enabled",true);
    pushFloat(p,"sweepAmount","SteamSweep amount",0,1,.55f);
    pushFloat(p,"click","Mouth click reduction",0,1,.2f);
    pushFloat(p,"rumble","Rumble highpass (Hz)",20,120,65,1);
    pushFloat(p,"noise","Noise attenuation (dB)",0,18,6,.1f);
    pushFloat(p,"breathReduction","SteamBreath Amount (dB)",0,36,12,.1f);
    pushFloat(p,"breathSensitivity","Breath sensitivity",0,1,.54f);
    pushFloat(p,"breathLength","Breath duration detection",0,1,.5f);
    pushFloat(p,"breathPreserve","Breath naturalness",0,1,.3f);
    pushFloat(p,"guardReduction","Plosive attenuation (dB)",0,24,11,.1f);
    pushFloat(p,"guardSensitivity","Plosive sensitivity",0,1,.5f);
    pushFloat(p,"guardFrequency","Plosive frequency (Hz)",100,300,200,1);
    pushFloat(p,"guardHold","Plosive hold (ms)",10,150,45,1);
    pushFloat(p,"sibilReduction","De-ess reduction (dB)",0,20,8,.1f);
    pushFloat(p,"sibilSensitivity","De-ess sensitivity",0,1,.55f);
    pushFloat(p,"sibilFrequency","De-ess frequency (Hz)",3000,12000,6500,10);
    pushFloat(p,"sibilWidth","De-ess width",0,1,.5f);
    pushFloat(p,"riderAmount","SteamRider amount",0,1,.7f);
    pushFloat(p,"riderTarget","SteamRider target (dB)",-36,-12,-23,.1f);
    pushFloat(p,"riderRange","SteamRider gain limit (dB)",0,12,5,.1f);
    pushFloat(p,"riderSpeed","SteamRider speed",0,1,.5f);
    pushFloat(p,"sidechain","SteamRider sidechain influence",0,1,0);
    pushFloat(p,"liftAmount","WhisperLift strength",0,1,.5f);
    pushFloat(p,"liftThreshold","WhisperLift threshold (dB)",-60,-12,-37,.1f);
    pushFloat(p,"liftRange","WhisperLift maximum gain (dB)",0,12,7,.1f);
    pushFloat(p,"liftAttack","WhisperLift attack (ms)",15,250,80,1);
    pushBool(p,"voiceProtect","Protect against noise lift",true);
    p.push_back(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"polishMode",1},"SteamPolish character",
        juce::StringArray{"Warm","Presence","Air"},0));
    pushFloat(p,"polishAmount","SteamPolish amount",0,1,.34f);
    return APVTS::ParameterLayout(p.begin(),p.end());
}

bool SteamFilterAudioProcessor::isBusesLayoutSupported(const BusesLayout& l) const {
    const auto in=l.getMainInputChannelSet(),out=l.getMainOutputChannelSet();
    if(in!=out || (in!=juce::AudioChannelSet::mono() && in!=juce::AudioChannelSet::stereo()))return false;
    if(l.inputBuses.size()>1) {
        const auto sc=l.inputBuses[1];
        if(!sc.isDisabled() && sc!=juce::AudioChannelSet::mono() && sc!=juce::AudioChannelSet::stereo()) return false;
    }
    return true;
}
void SteamFilterAudioProcessor::prepareToPlay(double sampleRate,int block) {
    engine.prepare(sampleRate,block,getMainBusNumInputChannels());
    setLatencySamples(engine.latencySamples());
    dspLoad.store(0,std::memory_order_relaxed);
}
float SteamFilterAudioProcessor::value(const char* parameter) const {
    const auto* v=state.getRawParameterValue(parameter);
    return v?v->load(std::memory_order_relaxed):0;
}

void SteamFilterAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer,juce::MidiBuffer&) {
    juce::ScopedNoDenormals noDenormals;
    const auto began=std::chrono::steady_clock::now();
    auto main=getBusBuffer(buffer,true,0);
    const int channels=juce::jmin(main.getNumChannels(),2),frames=main.getNumSamples();
    if(channels==0 || frames==0)return;
    float* audio[2]={main.getWritePointer(0), channels>1?main.getWritePointer(1):nullptr};
    const float* sc[2]={nullptr,nullptr};int scChannels=0;
    if(getBusCount(true)>1 && getBus(true,1)->isEnabled()) {
        auto aux=getBusBuffer(buffer,true,1);
        scChannels=juce::jmin(2,aux.getNumChannels());
        if(scChannels>0){sc[0]=aux.getReadPointer(0);if(scChannels>1)sc[1]=aux.getReadPointer(1);}
    }
    steam::Settings p;
    p.inputDb=value("input");p.outputDb=value("output");p.bypass=value("bypass")>.5f;
    p.globalMix=value("globalMix");p.maxGainDb=value("maxGain");
    p.detectorSensitivity=value("detectorSensitivity");p.lookaheadMs=value("lookahead");
    p.smoothing=value("smoothing");
    p.delta=value("delta")>.5f;p.gainMatch=value("gainMatch")>.5f;
    constexpr const char* switches[]={"sweepOn","breathOn","guardOn","sibilOn","riderOn","liftOn","polishOn"};
    for(int k=0;k<steam::kModuleCount;++k)p.enabled[k]=value(switches[k])>.5f;
    p.sweepAmount=value("sweepAmount");p.clickStrength=value("click");p.rumbleHz=value("rumble");p.noiseReductionDb=value("noise");
    p.breathReductionDb=value("breathReduction");p.breathSensitivity=value("breathSensitivity");
    p.breathLength=value("breathLength");p.breathPreserve=value("breathPreserve");
    p.plosiveReductionDb=value("guardReduction");p.plosiveSensitivity=value("guardSensitivity");
    p.plosiveFrequency=value("guardFrequency");p.plosiveHoldMs=value("guardHold");
    p.sibilReductionDb=value("sibilReduction");p.sibilSensitivity=value("sibilSensitivity");
    p.sibilFrequency=value("sibilFrequency");p.sibilWidth=value("sibilWidth");
    p.riderAmount=value("riderAmount");p.riderTargetDb=value("riderTarget");p.riderRangeDb=value("riderRange");
    p.riderSpeed=value("riderSpeed");p.riderSidechain=value("sidechain");
    p.liftStrength=value("liftAmount");p.liftThresholdDb=value("liftThreshold");p.liftMaxDb=value("liftRange");
    p.liftAttackMs=value("liftAttack");p.voiceProtect=value("voiceProtect")>.5f;
    p.polishMode=int(value("polishMode"));p.polishAmount=value("polishAmount");
    engine.process(audio,scChannels?sc:nullptr,channels,scChannels,frames,p);
    const auto ended=std::chrono::steady_clock::now();
    const auto used=std::chrono::duration<double>(ended-began).count();
    const auto available=double(frames)/getSampleRate();
    const float load=juce::jlimit(0.f,1.f,float(used/juce::jmax(1.e-8,available)));
    dspLoad.store(.92f*dspLoad.load(std::memory_order_relaxed)+.08f*load,std::memory_order_relaxed);
}

juce::AudioProcessorEditor* SteamFilterAudioProcessor::createEditor(){return new SteamFilterAudioProcessorEditor(*this);}
void SteamFilterAudioProcessor::getStateInformation(juce::MemoryBlock& out) {
    // Preserve both A/B banks and the active bank across Cubase session reloads.
    juce::ValueTree root("SteamFilterSession");
    root.setProperty("activeBank",bankIndex,nullptr);
    root.addChild(state.copyState(),-1,nullptr);
    const auto saveBank=[&](const char* name,const juce::ValueTree& data){
        auto wrapper=juce::ValueTree(name);
        if(data.isValid())wrapper.addChild(data.createCopy(),-1,nullptr);
        root.addChild(wrapper,-1,nullptr);
    };
    saveBank("BankA",bankIndex==0?state.copyState():bankA);
    saveBank("BankB",bankIndex==1?state.copyState():bankB);
    auto xml=root.createXml();
    if(xml)copyXmlToBinary(*xml,out);
}
void SteamFilterAudioProcessor::setStateInformation(const void* bytes,int size) {
    if(auto xml=getXmlFromBinary(bytes,size)) {
        auto root=juce::ValueTree::fromXml(*xml);
        if(!root.isValid())return;
        if(root.hasType(state.state.getType())) {
            state.replaceState(root);bankA={};bankB={};bankIndex=0;return;
        }
        if(!root.hasType("SteamFilterSession"))return;
        const auto parameters=root.getChildWithName(state.state.getType());
        if(parameters.isValid())state.replaceState(parameters);
        const auto loadBank=[&](const char* name)->juce::ValueTree{
            const auto wrapper=root.getChildWithName(name);
            if(wrapper.isValid() && wrapper.getNumChildren()>0)
                return wrapper.getChild(0).createCopy();
            return {};
        };
        bankA=loadBank("BankA");bankB=loadBank("BankB");
        bankIndex=juce::jlimit(0,1,int(root.getProperty("activeBank",0)));
    }
}
void SteamFilterAudioProcessor::setParameterValue(const juce::String& id,float newValue) {
    if(auto* p=state.getParameter(id)) {
        p->beginChangeGesture();
        p->setValueNotifyingHost(p->convertTo0to1(newValue));
        p->endChangeGesture();
    }
}
void SteamFilterAudioProcessor::applyRecommendation(const steam::Recommendation& r) {
    setParameterValue("breathSensitivity",r.breathSensitivity);
    setParameterValue("breathReduction",r.breathReductionDb);
    setParameterValue("guardSensitivity",r.plosiveSensitivity);
    setParameterValue("guardReduction",r.plosiveReductionDb);
    setParameterValue("sibilSensitivity",r.sibilSensitivity);
    setParameterValue("sibilFrequency",r.sibilFrequency);
    setParameterValue("sibilReduction",r.sibilReductionDb);
    setParameterValue("riderTarget",r.riderTargetDb);
    setParameterValue("liftThreshold",r.liftThresholdDb);
}
void SteamFilterAudioProcessor::switchAB(int target) {
    target=target==0?0:1;
    if(target==bankIndex)return;
    if(bankIndex==0)bankA=state.copyState(); else bankB=state.copyState();
    auto& next=target==0?bankA:bankB;
    if(next.isValid())state.replaceState(next);
    bankIndex=target;
}
void SteamFilterAudioProcessor::selectFactoryPreset(int choice) {
    // Subtle presets — always allow the user to refine every parameter.
    if(choice==0) { // Transparent
        setParameterValue("sweepAmount",.25f);setParameterValue("breathReduction",5);
        setParameterValue("sibilReduction",4);setParameterValue("riderAmount",.4f);
        setParameterValue("liftAmount",.2f);setParameterValue("polishAmount",.15f);
    } else if(choice==1) { // Clean & Polish
        setParameterValue("sweepAmount",.55f);setParameterValue("breathReduction",9);
        setParameterValue("sibilReduction",8);setParameterValue("riderAmount",.7f);
        setParameterValue("liftAmount",.5f);setParameterValue("polishAmount",.34f);
    } else if(choice==2) { // Whisper Forward
        setParameterValue("sweepAmount",.3f);setParameterValue("breathReduction",5);
        setParameterValue("sibilReduction",5);setParameterValue("riderAmount",.6f);
        setParameterValue("liftAmount",.8f);setParameterValue("liftRange",9);
        setParameterValue("polishAmount",.28f);
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter(){return new SteamFilterAudioProcessor();}