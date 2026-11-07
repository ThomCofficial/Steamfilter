#include "../Source/DSP/SteamDSP.h"
#include <iostream>
#include <vector>
#include <random>
#include <cassert>
#include <chrono>
#include <cstring>
using namespace steam;
static constexpr double pi=3.14159265358979323846;

void failure(const char* name){ std::cerr<<"FAIL: "<<name<<'\n';std::exit(1); }
void check(bool condition,const char* name){if(!condition)failure(name);}
struct Rig {
 Engine e;Settings p; std::vector<float> l,r;float* a[2];
 Rig(int block=1024,double sr=48000):l(block),r(block),a{l.data(),r.data()} {e.prepare(sr,block,2);}
 void fill(float v) {std::fill(l.begin(),l.end(),v);std::fill(r.begin(),r.end(),v);}
 void go() {e.process(a,nullptr,2,0,int(l.size()),p);
     for(float v:l)if(!std::isfinite(v) || std::abs(v)>2.f)failure("finite output and safety ceiling");
     for(float v:r)if(!std::isfinite(v) || std::abs(v)>2.f)failure("finite stereo output");}
};
void testSilence(){Rig rig;rig.fill(0);for(int i=0;i<150;++i)rig.go();for(float s:rig.l)check(s==0,"silence preservation");}
void testAutomation(){Rig rig(127);std::mt19937 rng(42);std::uniform_real_distribution<float> dist(-1.f,1.f);
 for(int j=0;j<2500;++j){for(int i=0;i<127;++i){rig.l[i]=dist(rng)*.1f;rig.r[i]=dist(rng)*.1f;}
   rig.p.inputDb=dist(rng)*20;rig.p.outputDb=dist(rng)*20;rig.p.rumbleHz=45.f+80.f*(dist(rng)+1.f)*.5f;
   rig.p.breathReductionDb=16.f*(dist(rng)+1.f)*.5f;rig.p.sibilSensitivity=(dist(rng)+1.f)*.5f;
   rig.p.riderTargetDb=-28.f+10.f*(dist(rng)+1.f)*.5f;rig.p.polishMode=j%3;rig.p.polishAmount=float(j%100)/100.f;
   rig.p.bypass=j%13==0;rig.p.enabled[j%7]=j%2==0;rig.go();
 }}
void testLoudness(){Rig rig(4096); for(int i=0;i<4096;++i)rig.l[i]=rig.r[i]=.01f*std::sin(float(2*pi*170*i/48000));
 rig.p.enabled.fill(false);rig.p.enabled[Lift]=true;rig.p.liftThresholdDb=-30;rig.p.liftStrength=1;rig.p.liftMaxDb=12;
 double original=0,raised=0;for(float s:rig.l)original+=s*s;
 for(int i=0;i<80;++i){for(int j=0;j<4096;++j)rig.l[j]=rig.r[j]=.01f*std::sin(float(2*pi*170*j/48000));rig.go();}
 for(float s:rig.l)raised+=s*s;
 check(raised>original*1.03,"quiet speech upward lift");
}
void testAnalysis(){Rig rig(256);rig.e.requestAnalysis();for(int n=0;n<2100;++n){for(int j=0;j<256;++j){float s=.05f*std::sin(float(2*pi*195*(n*256+j)/48000));rig.l[j]=rig.r[j]=s;}rig.go();}
 Recommendation r;check(rig.e.takeRecommendation(r),"analysis completes after ten seconds");
 check(!rig.e.takeRecommendation(r),"recommendation consumed only once");
 check(r.confidence>0,"analysis confidence");
}
void testWave(){Rig rig(480);rig.fill(.1f);rig.go();auto index=rig.e.waveWrite.load();check(index>0,"waveform populated");
 float peak,db;unsigned e;Engine::unpackWave(rig.e.waveform[(index-1)%Engine::waveSize].load(),peak,db,e);check(peak>0,"meter peak contains processed audio");
}
void testSampleRates(){for(auto sr:{8000.,11025.,22050.,44100.,48000.,88200.,96000.,192000.,384000.}){
  Rig rig(509,sr); for(int k=0;k<140;++k){for(int j=0;j<509;++j)rig.l[j]=rig.r[j]=.7f*std::sin(float(2*pi*(sr*.38)*(j+k*509)/sr));rig.go();}}
}
void testIdle(){Rig rig;rig.fill(.1f);rig.p.bypass=true;for(int n=0;n<100;++n){rig.fill(.1f);rig.go();}check(std::abs(rig.l.back()-.1f)<1.e-5,"bypass becomes transparent");}
void testAutomationDiscontinuity(){
    Rig rig(64);rig.p.enabled.fill(true);
    float previous=0,maxJump=0;
    for(int block=0;block<2000;++block){
        rig.p.polishMode=block%3;
        rig.p.polishAmount=(block%2)?1.f:0.f;
        rig.p.bypass=block%7==0;
        rig.p.enabled[block%7]=block%3!=0;
        rig.p.inputDb=(block%2)?6.f:-6.f;
        rig.p.outputDb=(block%2)?-6.f:6.f;
        rig.p.liftStrength=(block%2)?1.f:0.f;
        rig.p.riderTargetDb=(block%2)?-16.f:-31.f;
        rig.fill(.03f);rig.go();
        for(float v:rig.l){maxJump=std::max(maxJump,std::abs(v-previous));previous=v;}
    }
    check(maxJump<.045f,"rapid module and gain automation avoids damaging discontinuities");
}
void testEfficiency(){Rig rig(512);rig.fill(.05f);auto start=std::chrono::steady_clock::now();for(int i=0;i<3000;++i)rig.go();
  auto ms=std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()-start).count();
  std::cout<<"Processing ~32 sec stereo at 48k took "<<ms<<"ms (non-gating diagnostic)\n";}
int main(){testSilence();testAutomation();testLoudness();testAnalysis();testWave();testSampleRates();testIdle();testAutomationDiscontinuity();testEfficiency();std::cout<<"All SteamDSP tests passed\n";}
