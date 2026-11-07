#include "../Source/DSP/SteamDSP.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <random>
#include <vector>
using namespace steam;
static constexpr float pi=3.141592653589793f;
void expect(bool b,const char* text){if(!b){std::cerr<<"FAIL: "<<text<<"\n";std::exit(1);}}
struct Tester {
    Engine engine;Settings config;int rate=48000;
    float data[256]{};float* channels[1]={data};
    Tester(){engine.prepare(rate,256,1);config.enabled.fill(false);}
    void run(){engine.process(channels,nullptr,1,0,256,config);for(auto s:data)expect(std::isfinite(s),"non-finite sample");}
};
void moduleSpectra(){
    Tester t;t.config.enabled[steam::Sweep]=true;t.config.rumbleHz=90;
    // A 30Hz stage-rumble tone must be attenuated without affecting the VST3 host.
    float last=0;
    for(int k=0;k<300;++k){for(int i=0;i<256;++i)t.data[i]=.18f*std::sin(2.f*pi*30.f*(k*256+i)/t.rate);
        t.run();last=t.engine.readStats().reductionDb[steam::Sweep];}
    expect(last < -5.f,"SteamSweep 30Hz highpass processing meter");
    std::cout<<"SteamSweep: "<<last<<" dB measured reduction\n";
}
void breathSynthetic(){
    Tester t;t.config.enabled[steam::Breath]=true;t.config.breathReductionDb=16;t.config.breathSensitivity=.65f;
    std::mt19937 rng(3);std::normal_distribution<float> noise(0.f,1.f);float lp=.0f,hp=.0f;
    float minReduction=0;unsigned unionEvents=0;
    for(int k=0;k<250;++k){for(int i=0;i<256;++i){float r=.2f*noise(rng);lp+=.15f*(r-lp);hp+=.01f*(r-hp);t.data[i]=lp-hp;}
        t.run();auto s=t.engine.readStats();if(k>50)minReduction=std::min(minReduction,s.reductionDb[steam::Breath]);unionEvents|=s.events;}
    expect((unionEvents&EventBreath)!=0,"breath classifier identifies synthesized noisy inhale");
    expect(minReduction < -3.f,"SteamBreath attenuates classified inhale");
    std::cout<<"SteamBreath: "<<minReduction<<" dB measured reduction\n";
}
void sibilanceSynthetic(){
    Tester t;t.config.enabled[steam::Sibil]=true;t.config.sibilReductionDb=15;t.config.sibilSensitivity=.85f;
    float minReduction=0;unsigned unionEvents=0;
    for(int k=0;k<250;++k){for(int i=0;i<256;++i)t.data[i]=.12f*std::sin(2.f*pi*7200.f*(k*256+i)/t.rate);
        t.run();const auto s=t.engine.readStats();minReduction=std::min(minReduction,s.reductionDb[steam::Sibil]);unionEvents|=s.events;}
    expect((unionEvents&EventSibil)!=0,"sibilance classifier identifies sustained high-frequency tone");
    expect(minReduction < -2.f,"SteamSibil attenuates the high band");
    std::cout<<"SteamSibil: "<<minReduction<<" dB measured reduction\n";
}
void dynamics(){
    for(auto id:{steam::Rider,steam::Lift}){
        Tester t;t.config.enabled[id]=true;
        if(id==steam::Rider){t.config.riderAmount=1;t.config.riderTargetDb=-19;t.config.riderRangeDb=10;}
        else {t.config.liftStrength=1;t.config.liftThresholdDb=-25;t.config.liftMaxDb=12;}
        for(int k=0;k<250;++k){for(int i=0;i<256;++i)t.data[i]=.012f*std::sin(2.f*pi*190.f*(k*256+i)/t.rate);t.run();}
        auto s=t.engine.readStats();expect(s.reductionDb[id]>1.f,"positive gain meter for upward processing");
        std::cout<<(id==steam::Rider?"SteamRider":"WhisperLift")<<": +"<<s.reductionDb[id]<<" dB measured added gain\n";
    }
}
void polish(){
    Tester t;t.config.enabled[steam::Polish]=true;t.config.polishAmount=1;
    for(int mode=0;mode<3;++mode){t.config.polishMode=mode;float activity=0;
       for(int k=0;k<200;++k){for(int i=0;i<256;++i)t.data[i]=.07f*std::sin(2.f*pi*8000.f*(k*256+i)/t.rate);t.run();}
       activity=t.engine.readStats().reductionDb[steam::Polish];
       expect(std::isfinite(activity),"SteamPolish measured activity is finite");
       std::cout<<"SteamPolish mode "<<mode<<": "<<activity<<" dB measured alteration\n";
    }
}
void guardTransient(){
    Tester t;t.config.enabled[steam::Guard]=true;t.config.plosiveReductionDb=18;t.config.plosiveSensitivity=.85;
    float mostReduction=0;unsigned unionEvents=0;
    for(int k=0;k<120;++k){for(int i=0;i<256;++i){const float time=(k*256+i)/48000.f;
        const bool burst=time>.10f && time<.28f;
        t.data[i]=burst?.7f*std::sin(2.f*pi*95.f*time):0.f;
      }
      t.run();auto s=t.engine.readStats();mostReduction=std::min(mostReduction,s.reductionDb[steam::Guard]);unionEvents|=s.events;
    }
    expect((unionEvents&EventPlosive)!=0,"SteamGuard detects transient low burst");
    expect(mostReduction<-.5f,"SteamGuard attenuates plosive stage");
    std::cout<<"SteamGuard: "<<mostReduction<<" dB measured reduction\n";
}
void mouthClickLookahead(){
    // A brief high-frequency mouth click is surrounded by an otherwise quiet voiced tone.
    auto clickPeak=[](bool enabled) {
        Tester t;t.config.enabled[steam::Sweep]=enabled;t.config.sweepAmount=1.f;
        t.config.clickStrength=1.f;t.config.rumbleHz=20.f;t.config.noiseReductionDb=0.f;
        double outputMax=0;
        for(int b=0;b<180;++b){for(int j=0;j<256;++j){
            const int k=b*256+j;
            const float time=float(k)/48000.f;
            float v=.005f*std::sin(2.f*pi*200.f*time);
            if(k>=12000 && k<12120)
                v+=.4f*std::sin(2.f*pi*8500.f*time)*std::sin(pi*float(k-12000)/120.f);
            t.data[j]=v;
        }
        t.run();for(float v:t.data)outputMax=std::max(outputMax,double(std::abs(v)));
        }
        return outputMax;
    };
    const double dry=clickPeak(false),processed=clickPeak(true);
    std::cout<<"SteamSweep click test: peak "<<dry<<" → "<<processed<<"\n";
    expect(processed<dry*.6,"lookahead click attenuation significantly lowers isolated spike");
}
void lookaheadLatency(){
    Tester t;t.config.bypass=true;
    expect(t.engine.latencySamples()==960,"constant maximum lookahead exactly 20ms at 48k");
    float max=0;int where=-1;
    for(int b=0;b<8;++b){
        std::fill(std::begin(t.data),std::end(t.data),0.f);
        if(b==0)t.data[0]=.45f;
        t.run();
        for(int i=0;i<256;++i)if(std::abs(t.data[i])>max){max=std::abs(t.data[i]);where=b*256+i;}
    }
    expect(where==960 && std::abs(max-.45f)<.003f,"host-compensated bypass preserves delayed impulse");
}
void silentAnalysisRefusal(){
    Tester t;t.engine.requestAnalysis();
    for(int k=0;k<1900;++k){std::fill(std::begin(t.data),std::end(t.data),0.f);t.run();}
    Recommendation r;expect(t.engine.takeRecommendation(r),"silent capture completes");
    expect(r.confidence<.12f,"silent capture is rejected as untrustworthy");
}
void allBypassedDelta(){
    Tester t;t.config.enabled.fill(false);t.config.delta=true;
    // Settling 8ms delta crossfade requires a few initial blocks.
    for(int k=0;k<80;++k){std::fill(std::begin(t.data),std::end(t.data),.05f);t.run();}
    for(auto x:t.data)expect(std::abs(x)<.005f,"delta monitor emits near-zero when no stage is active");
}

void globalBlendIsTrueDry() {
    Tester t;t.config.enabled.fill(true);t.config.globalMix=0.f;
    t.config.inputDb=12.f;t.config.outputDb=-5.f;t.config.polishAmount=1.f;
    float maximumError=0.f;
    for(int block=0;block<160;++block){
        for(int i=0;i<256;++i)t.data[i]=.06f*std::sin(2.f*pi*197.f*(block*256+i)/48000.f);
        t.run();
        if(block>80)for(int i=0;i<256;++i){
            const int source=block*256+i-t.engine.latencySamples();
            const float expected=.06f*std::sin(2.f*pi*197.f*source/48000.f);
            maximumError=std::max(maximumError,std::abs(t.data[i]-expected));
        }
    }
    expect(maximumError<.002f,"global 0% blend must pass the delayed original unchanged");
}
void makeupCapAndAnalysis() {
    Tester t;t.config.enabled.fill(false);t.config.enabled[steam::Rider]=true;
    t.config.enabled[steam::Lift]=true;t.config.riderAmount=1.f;
    t.config.riderTargetDb=-15.f;t.config.riderRangeDb=12.f;
    t.config.liftStrength=1.f;t.config.liftThresholdDb=-15.f;
    t.config.liftMaxDb=12.f;t.config.maxGainDb=3.f;
    float peak=0;
    for(int block=0;block<340;++block){
        for(int i=0;i<256;++i)t.data[i]=.016f*std::sin(2.f*pi*195.f*(block*256+i)/48000.f);
        t.run();if(block>220)for(auto x:t.data)peak=std::max(peak,std::abs(x));
    }
    expect(peak<.016f*dbToGain(3.f)*1.07f,"rider plus lift must honour global makeup cap");
    t.engine.requestAnalysis();
    for(int k=0;k<810;++k){for(int i=0;i<256;++i)t.data[i]=.12f*std::sin(2.f*pi*200.f*(k*256+i)/48000.f);t.run();}
    Recommendation result;expect(t.engine.takeRecommendation(result),"4-second analysis produced a suggestion");
    expect(result.confidence>.12f,"analysis accepts voiced test tone");
}
void lookaheadAndSmoothingAutomation() {
    Tester t;t.config.enabled.fill(true);t.config.inputDb=0;t.config.outputDb=0;
    float prev=0, largestJump=0;
    for(int block=0;block<1400;++block){
        t.config.lookaheadMs=(block%3==0)?0.f:(block%3==1?2.f:4.f);
        t.config.smoothing=(block%7)/6.f;
        t.config.detectorSensitivity=(block%9)/8.f;
        for(int i=0;i<256;++i)t.data[i]=.035f*std::sin(2.f*pi*310.f*(block*256+i)/48000.f);
        t.run();
        for(float x:t.data){largestJump=std::max(largestJump,std::abs(x-prev));prev=x;}
    }
    expect(largestJump<.2f,"lookahead and smoothing changes remain bounded");
}

void breathVsVoicedAndRelease() {
    Tester t; t.config.enabled[steam::Breath]=true;
    t.config.breathReductionDb=30.f;
    t.config.breathSensitivity=.65f;
    // A harmonic lead vowel must not be confused with breath.
    float worstVoiced=0.f;
    for(int b=0;b<210;++b) {
        for(int i=0;i<256;++i)
            t.data[i]=.07f*std::sin(2.f*pi*180.f*(b*256+i)/48000.f);
        t.run();
        if(b>90) worstVoiced=std::min(worstVoiced,t.engine.readStats().reductionDb[steam::Breath]);
    }
    expect(worstVoiced>-.8f,"voiced harmonic vowel is preserved by SteamBreath");
    // Realistic broadband air must be identified at quieter recording levels.
    std::mt19937 rng(827);std::normal_distribution<float> noise(0,1);
    float peakReduction=0.f;bool heardBreath=false;
    for(int b=0;b<160;++b){
        for(int i=0;i<256;++i)t.data[i]=noise(rng)*.012f;
        t.run();
        const auto stats=t.engine.readStats();
        if(b>40) {
            peakReduction=std::min(peakReduction,stats.reductionDb[steam::Breath]);
            heardBreath |= (stats.events&steam::EventBreath)!=0;
        }
    }
    expect(heardBreath,"broadband breath triggers detector despite high-frequency content");
    expect(peakReduction < -5.f,"single breath amount knob produces meaningful reduction");
    // On returning to pitched speech, attenuation must release naturally.
    for(int b=0;b<220;++b){
        for(int i=0;i<256;++i)t.data[i]=.06f*std::sin(2.f*pi*220.f*(b*256+i)/48000.f);
        t.run();
    }
    expect(t.engine.readStats().reductionDb[steam::Breath]>-.8f,
        "SteamBreath returns to unity on the next voiced phrase");
    std::cout<<"Breath versus voice: preserved vowel, "<<peakReduction<<" dB air reduction, clean release\n";
}

int main(){breathVsVoicedAndRelease();globalBlendIsTrueDry();makeupCapAndAnalysis();lookaheadAndSmoothingAutomation();moduleSpectra();breathSynthetic();sibilanceSynthetic();dynamics();polish();guardTransient();mouthClickLookahead();lookaheadLatency();silentAnalysisRefusal();allBypassedDelta();std::cout<<"All module behavior tests passed\n";}