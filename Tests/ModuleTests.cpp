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
    expect(processed<dry*.6,"4ms-lookahead click attenuation significantly lowers isolated spike");
}
void lookaheadLatency(){
    Tester t;t.config.bypass=true;
    expect(t.engine.latencySamples()==192,"lookahead exactly 4ms at 48k");
    for(int b=0;b<4;++b){std::fill(std::begin(t.data),std::end(t.data),0.f);if(b==0)t.data[0]=.45f;
       t.run();if(b==0)expect(std::abs(t.data[192]-.45f)<.003,"host-compensated bypass preserves delayed impulse");
    }
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
int main(){moduleSpectra();breathSynthetic();sibilanceSynthetic();dynamics();polish();guardTransient();mouthClickLookahead();lookaheadLatency();silentAnalysisRefusal();allBypassedDelta();std::cout<<"All module behavior tests passed\n";}
