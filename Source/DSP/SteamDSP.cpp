#include "SteamDSP.h"
#include <limits>

namespace steam {
namespace {
constexpr float pi=3.14159265358979323846f;
float smooth(float prev, float target, float seconds, float fs) {
    const float a = 1.f/(1.f+std::max(1.f,seconds*fs));
    return prev+(target-prev)*a;
}
float poleAlpha(float frequency,float fs) {
    return 1.f-std::exp(-2.f*pi*std::min(frequency,fs*.44f)/fs);
}
float onepole(float old,float x,float alpha) { return old+(x-old)*alpha; }
float amp(float x) { return std::abs(x); }
}
void Biquad::highpass(float sampleRate,float fc) {
    fc=clamp(fc,10.f,sampleRate*.43f);
    const float w=2.f*pi*fc/sampleRate, cosw=std::cos(w), sn=std::sin(w);
    const float alpha=sn/(2.f*.70710678f);
    const float inv=1.f/(1.f+alpha);
    const float B0=(1.f+cosw)*.5f*inv, B1=-(1.f+cosw)*inv;
    setCoefficients(B0,B1,B0,-2.f*cosw*inv,(1.f-alpha)*inv,sampleRate);
}
void Biquad::shelf(float sr,float frequency,float gainDb,bool high) {
    frequency=clamp(frequency,35.f,sr*.43f);
    const float A=std::pow(10.f,gainDb/40.f), w=2.f*pi*frequency/sr;
    const float cs=std::cos(w),sn=std::sin(w), alpha=sn*std::sqrt(2.f)*.5f;
    const float root=2.f*std::sqrt(A)*alpha;
    float B0,B1,B2,A0,A1,A2;
    if(high) {
        B0=A*((A+1)+(A-1)*cs+root);
        B1=-2*A*((A-1)+(A+1)*cs);
        B2=A*((A+1)+(A-1)*cs-root);
        A0=(A+1)-(A-1)*cs+root;
        A1=2*((A-1)-(A+1)*cs);
        A2=(A+1)-(A-1)*cs-root;
    } else {
        B0=A*((A+1)-(A-1)*cs+root);
        B1=2*A*((A-1)-(A+1)*cs);
        B2=A*((A+1)-(A-1)*cs-root);
        A0=(A+1)+(A-1)*cs+root;
        A1=-2*((A-1)+(A+1)*cs);
        A2=(A+1)+(A-1)*cs-root;
    }
    const float inv=1.f/A0;
    setCoefficients(B0*inv,B1*inv,B2*inv,A1*inv,A2*inv,sr);
}

void Engine::prepare(double sampleRate,int maxBlock,int channels) {
    fs=clamp(sampleRate,8000.,384000.);
    maximumBlock=std::max(1,maxBlock);
    activeChannels=clamp(channels,1,2);
    lookaheadSamples=clamp(int(fs*.004),1,delayCapacity-1);
    reset();
    constexpr float frequencies[]={150.f,450.f,1600.f,4500.f,9000.f};
    for(int i=0;i<5;++i)detectorAlpha[i]=poleAlpha(frequencies[i],float(fs));
    popAlpha=poleAlpha(200.f,float(fs));
    polishLowAlpha=poleAlpha(350.f,float(fs));
}
void Engine::reset() {
    for(auto& c:ch) c=Channel{};
    for(auto& a:delay)a.fill(0);
    delayWrite=0;
    detectorLp.fill(0);eFast.fill(0);eSlow.fill(0);
    moduleMix.fill(1.f);moduleMixInitialized=false;
    rmsEnv=slowRms=sideEnv=0; breathGain=sibilGain=plosiveGain=clickGain=noiseGain=1;
    riderGainDb=liftGainDb=0; riderGainLinear=liftGainLinear=riderTargetLinear=liftTargetLinear=1; bypassMix=1;inputGain=outputGain=1; modCounter=0;
    deltaMix=0;gainMatchGain=matchTarget=1;matchInputEnv=matchOutputEnv=0;
    detectCounter=breathCount=popHold=clickHold=0;events=0;lastVocalActive=false;
    wavePeak=waveDb=0;waveEvents=0;waveCounter=0;waveWrite.store(0);
    for(auto& v:waveform) v.store(0,std::memory_order_relaxed);
    for(auto& v:moduleMeter) v.store(0,std::memory_order_relaxed);
    inputPeak.store(0); outputPeak.store(0);currentRmsDb.store(-100);lastEvents.store(0);
    lastBreathTarget=lastSibilTarget=lastPlosiveTarget=lastClickTarget=noiseTarget=1;
    lastRiderTarget=lastLiftTarget=0;
    fcRumble=fcSibil=-1;lastPopFrequency=-1;lastPolishAmount=-1;lastPolishMode=-1;
    analysisRequest.store(false);ready.store(false);progress.store(0);capture={};
}

void Engine::updateFilters(const Settings& s) {
    deessAlpha=poleAlpha(s.sibilFrequency,float(fs));
    if(lastPopFrequency!=s.plosiveFrequency) {
        popAlpha=poleAlpha(s.plosiveFrequency,float(fs));
        lastPopFrequency=s.plosiveFrequency;
    }
    polishHighAlpha=poleAlpha((s.polishMode==1)?1000.f:(s.polishMode==2?4300.f:2300.f),float(fs));
    if(fcRumble!=s.rumbleHz) {
        for(auto& c:ch) c.rumble.highpass(float(fs),s.rumbleHz);
        fcRumble=s.rumbleHz;
    }
    // Coefficient update on discrete preset change or amount automation; gain is capped to 2.4 dB.
    // Modulation changes in a block are bounded by the host's automation rate.
    if(lastPolishMode!=s.polishMode || std::abs(lastPolishAmount-s.polishAmount)>.001f) {
        const int mode=clamp(s.polishMode,0,2);
        const float lo[]={250,300,350},hi[]={12000,16000,20000};
        const float amount=clamp(s.polishAmount,0.f,1.f);
        for(auto& c:ch) {
            c.lowShelf.shelf(float(fs),lo[mode],2.2f*amount,false);
            c.highShelf.shelf(float(fs),hi[mode],2.4f*amount,true);
        }
        lastPolishMode=s.polishMode;lastPolishAmount=s.polishAmount;
    }
}

void Engine::updateDetectors(const Settings& s) {
    const float lo=eFast[0], body=eFast[1]+eFast[2], presence=eFast[3], hiss=eFast[4];
    const float broadband=lo+body+presence+hiss+1.e-7f;
    const float db=gainToDb(rmsEnv);
    // Sibilance: relatively strong sustained HF above ~4.5k; suppress short clicks.
    const bool hissStrong=(hiss>(body+lo)*(.65f-.28f*s.sibilSensitivity)) &&
                          (hiss>presence*1.0f) && hiss>.0007f && db>-58.f;
    // Plosives: transient low-frequency energy dominates the vocal bands.
    const bool bassBurst=(lo > std::max(0.003f,eSlow[0])* (2.8f-1.5f*s.plosiveSensitivity)) &&
                          lo>(body+presence)*(.7f-.3f*s.plosiveSensitivity) && db>-52.f;
    // Broadband click candidate: exceptionally brief high-band transient;
    // the safety cap prevents aggressive mutation of ordinary consonants.
    const bool click=(hiss>std::max(.0005f,eSlow[4])*(8.f-3.f*s.clickStrength)) &&
                     hiss>.004f && db>-60.f;
    // Breath classification requires sustained, low-mid-poor noise, excludes sibilants/plosives.
    const bool noiseLike=(hiss+presence) > (body+lo)*(.78f-.33f*s.breathSensitivity) &&
                         !hissStrong && !bassBurst && db>-56.f;
    breathCount=noiseLike?std::min(breathCount+1,12):std::max(0,breathCount-(s.breathLength>.5f?1:2));
    if(bassBurst) popHold=int(fs*clamp(s.plosiveHoldMs,10.f,150.f)*.001f/64.f)+1;
    else popHold=std::max(0,popHold-1);
    if(click) clickHold=std::max(1,int(fs*.012f/64.f));
    else clickHold=std::max(0,clickHold-1);
    events=0;
    if(breathCount>=(s.breathLength>.75f?5:7)) events|=EventBreath;
    if(popHold>0) events|=EventPlosive;
    if(hissStrong && !click) events|=EventSibil;
    if(clickHold>0) events|=EventClick;

    lastBreathTarget = (events&EventBreath)? dbToGain(-s.breathReductionDb*(1.f-.6f*s.breathPreserve)):1.f;
    lastSibilTarget  = (events&EventSibil)? dbToGain(-s.sibilReductionDb):1.f;
    lastPlosiveTarget=(events&EventPlosive)?dbToGain(-s.plosiveReductionDb):1.f;
    lastClickTarget=(events&EventClick)?(1.f-.75f*s.clickStrength*s.sweepAmount):1.f;
    noiseTarget=(db < -54.f && !(events&EventBreath))?dbToGain(-s.noiseReductionDb*s.sweepAmount):1.f;

    // Low-frequency energy and spectral activity help distinguish speech from silence;
    // no lift or riding below -55 dBFS, where boosting noise is unsafe.
    const bool vocalActive = db>-55.f && (body+presence+hiss)>.00045f;
    const bool meaningfulVocal=vocalActive && (events&(EventBreath|EventSibil|EventPlosive|EventClick))==0;
    lastVocalActive=meaningfulVocal;
    const float drive=(s.riderTargetDb-db);
    lastRiderTarget=meaningfulVocal?clamp(drive*.45f*s.riderAmount,-s.riderRangeDb,s.riderRangeDb):0.f;
    const float margin=s.liftThresholdDb-db;
    lastLiftTarget=(meaningfulVocal || (!s.voiceProtect && db>-70.f)) && margin>0 ?
        clamp(margin*.50f*s.liftStrength,0.f,s.liftMaxDb):0.f;
    if(std::abs(lastRiderTarget)>.25f) events|=EventRider;
    if(lastLiftTarget>.25f) events|=EventWhisper;
    currentRmsDb.store(db,std::memory_order_relaxed);

    if(capture.samples>0) {
        ++capture.detected;
        capture.active+=(db>-48.f && db < 0.f)?1:0;
        capture.breaths+=(events&EventBreath)?1:0;
        capture.pops+=(events&EventPlosive)?1:0;
        capture.sibilants+=(events&EventSibil)?1:0;
        capture.clicks+=(events&EventClick)?1:0;
        capture.rmsDbSum+=db;
        capture.brightSum+=double(hiss/(broadband));
    }
}

uint64_t Engine::packWave(float peak,float riderDb,unsigned event,float whisperDb) {
    const uint64_t p=uint64_t(clamp(peak,0.f,1.f)*65535.f);
    const uint64_t rider=uint64_t(clamp(riderDb+32.f,0.f,64.f)*1023.f);
    const uint64_t whisper=uint64_t(clamp(whisperDb+32.f,0.f,64.f)*1023.f);
    return p | (rider<<16) | (uint64_t(event&255u)<<32) | (whisper<<40);
}
void Engine::unpackWave(uint64_t packed,float& peak,float& riderDb,unsigned& event,float& whisperDb) {
    peak=float(packed&65535u)/65535.f;
    riderDb=float((packed>>16)&65535u)/1023.f-32.f;
    event=unsigned((packed>>32)&255u);
    whisperDb=float((packed>>40)&65535u)/1023.f-32.f;
}

void Engine::requestAnalysis(){
    ready.store(false,std::memory_order_release);
    analysisRequest.store(true,std::memory_order_release);
}
void Engine::completeAnalysis() {
    const float n=float(std::max(1,capture.detected));
    const float b=float(capture.breaths)/n,p=float(capture.pops)/n,si=float(capture.sibilants)/n;
    const float rms=float(capture.rmsDbSum/n), bright=float(capture.brightSum/n);
    const float values[10]={
        clamp(.45f+.30f*b,.35f,.8f),clamp(6.f+10.f*b,4.f,14.f),
        clamp(.45f+.5f*p,.35f,.8f),clamp(7.f+12.f*p,6.f,16.f),
        clamp(.42f+.55f*si,.35f,.8f), bright>.23f?7400.f:5800.f,
        clamp(5.f+12.f*si,4.f,12.f),clamp(rms-1.f,-30.f,-17.f),
        clamp(rms-13.f,-47.f,-32.f),clamp(float(capture.active)/std::max(1.f,n)*1.5f,0.f,1.f)
    };
    for(int i=0;i<10;++i) resultFields[i].store(values[i],std::memory_order_relaxed);
    ready.store(true,std::memory_order_release);
    capture.samples=0;
    progress.store(1.f,std::memory_order_relaxed);
}
bool Engine::takeRecommendation(Recommendation& r) {
    if(!ready.exchange(false,std::memory_order_acq_rel)) return false;
    float v[10];for(int i=0;i<10;++i)v[i]=resultFields[i].load(std::memory_order_relaxed);
    r.breathSensitivity=v[0];r.breathReductionDb=v[1];r.plosiveSensitivity=v[2];r.plosiveReductionDb=v[3];
    r.sibilSensitivity=v[4];r.sibilFrequency=v[5];r.sibilReductionDb=v[6];r.riderTargetDb=v[7];
    r.liftThresholdDb=v[8];r.confidence=v[9];
    return true;
}

void Engine::process(float* const* audio,const float* const* sidechain,int channels,int sideChannels,int frames,const Settings& s) {
    if(frames<=0 || audio==nullptr || channels<1) return;
    channels=clamp(channels,1,2);
    if(analysisRequest.exchange(false,std::memory_order_acq_rel)) {
        capture={};capture.samples=1;progress.store(0,std::memory_order_relaxed);
    }
    updateFilters(s);
    if(!moduleMixInitialized){
        for(int k=0;k<kModuleCount;++k)moduleMix[k]=s.enabled[k]?1.f:0.f;
        moduleMixInitialized=true;
    }
    const float sr=float(fs),inTarget=dbToGain(s.inputDb),outTarget=dbToGain(s.outputDb);
    float inMax=0,outMax=0;
    std::array<double,kModuleCount> beforeEnergy{},afterEnergy{};
    auto accumulate=[&](int module,float before,float after){
        beforeEnergy[module]+=double(before)*before;
        afterEnergy[module]+=double(after)*after;
    };

    for(int i=0;i<frames;++i) {
        float detectorInput=0,peakIn=0;
        for(int c=0;c<channels;++c) {
            float sample=audio[c][i];
            if(!std::isfinite(sample)) sample=0;
            detectorInput+=sample*inTarget;
            peakIn=std::max(peakIn,amp(sample));
        }
        detectorInput/=float(channels);
        inMax=std::max(inMax,peakIn);
        const float mono=detectorInput;
        // Multiband feature extraction, 5 one-poles, lightweight and deterministic.
        for(int k=0;k<5;++k) detectorLp[k]=onepole(detectorLp[k],mono,detectorAlpha[k]);
        const float f[5]={detectorLp[0],detectorLp[1]-detectorLp[0],detectorLp[2]-detectorLp[1],
                          detectorLp[3]-detectorLp[2],mono-detectorLp[3]};
        for(int k=0;k<5;++k) {
            eFast[k]=smooth(eFast[k],amp(f[k]),.009f,sr);
            eSlow[k]=smooth(eSlow[k],amp(f[k]),.064f,sr);
        }
        rmsEnv=smooth(rmsEnv,amp(mono),.035f,sr);
        slowRms=smooth(slowRms,amp(mono),.15f,sr);
        if((++detectCounter&63)==0) updateDetectors(s);

        if(sidechain && sideChannels>0 && sidechain[0]) {
            const float sc=sidechain[0][i];
            sideEnv=smooth(sideEnv,amp(std::isfinite(sc)?sc:0.f),.070f,sr);
        } else sideEnv=smooth(sideEnv,0,.12f,sr);
        float riderTarget=lastRiderTarget;
        if(lastVocalActive && sidechain && sideChannels>0 && s.riderSidechain>0)
            riderTarget+=s.riderSidechain*clamp(gainToDb(sideEnv)-(-23.f),-4.f,4.f);
        riderTarget=clamp(riderTarget,-s.riderRangeDb,s.riderRangeDb);
        if((++modCounter & 31)==0) {
            riderTargetLinear=dbToGain(riderTarget);
            liftTargetLinear=dbToGain(lastLiftTarget);
        }
        for(int k=0;k<kModuleCount;++k)
            moduleMix[k]=smooth(moduleMix[k],s.enabled[k]?1.f:0.f,.006f,sr);
        // Conservative continuous gain smoothing; avoids CPU-spiky discontinuities.
        breathGain=smooth(breathGain,lastBreathTarget,.015f,sr);
        plosiveGain=smooth(plosiveGain,lastPlosiveTarget,.006f,sr);
        clickGain=smooth(clickGain,lastClickTarget,.001f,sr);
        sibilGain=smooth(sibilGain,lastSibilTarget,.007f,sr);
        noiseGain=smooth(noiseGain,noiseTarget,.045f,sr);
        riderGainLinear=smooth(riderGainLinear,riderTargetLinear,.08f+.24f*(1.f-s.riderSpeed),sr);
        liftGainLinear=smooth(liftGainLinear,liftTargetLinear,s.liftAttackMs*.001f,sr);
        bypassMix=smooth(bypassMix,s.bypass?0.f:1.f,.008f,sr);
        deltaMix=smooth(deltaMix,s.delta?1.f:0.f,.008f,sr);
        gainMatchGain=smooth(gainMatchGain,s.gainMatch?matchTarget:1.f,.6f,sr);
        inputGain=smooth(inputGain,inTarget,.006f,sr);
        outputGain=smooth(outputGain,outTarget,.006f,sr);
        const float effectiveRider=1.f+(riderGainLinear-1.f)*moduleMix[Rider];
        const float effectiveLift=1.f+(liftGainLinear-1.f)*moduleMix[Lift];
        const float stageBreath=breathGain,stageSibil=sibilGain;
        const int readDelay=(delayWrite+delayCapacity-lookaheadSamples)%delayCapacity;
        for(int c=0;c<channels;++c) {
            const float current=std::isfinite(audio[c][i])?audio[c][i]:0.f;
            delay[c][delayWrite]=current;
            const float original=delay[c][readDelay];
            float x=original*inputGain;
            // Plosive reduction precedes HPF. Subtract only the detected low-frequency burst.
            auto& state=ch[c];
            float before=x;
            state.popLow=onepole(state.popLow,x,popAlpha);
            x-=state.popLow*(1.f-plosiveGain)*moduleMix[Guard];
            accumulate(Guard,before,x);
            before=x;
            const float highpassed=state.rumble.tick(x); // state remains warm while bypassed
            const float swept=highpassed*clickGain*noiseGain;
            x+=(swept-x)*moduleMix[Sweep];
            accumulate(Sweep,before,x);
            before=x;
            x*=1.f+(stageBreath-1.f)*moduleMix[Breath];
            accumulate(Breath,before,x);
            before=x;
            state.deessLow=onepole(state.deessLow,x,deessAlpha);
            {
                const float wide=clamp(s.sibilWidth,0.f,1.f)*.25f;
                const float deessed=(state.deessLow+(x-state.deessLow)*stageSibil)*(1.f-wide)+x*stageSibil*wide;
                x+=(deessed-x)*moduleMix[Sibil];
            }
            accumulate(Sibil,before,x);
            before=x;
            x*=effectiveRider;
            accumulate(Rider,before,x);
            before=x;
            x*=effectiveLift;
            accumulate(Lift,before,x);
            before=x;
            if(s.polishAmount>0.f) {
                x=state.lowShelf.tick(x);
                x=state.highShelf.tick(x);
                state.polishLow=onepole(state.polishLow,x,polishLowAlpha);
                state.polishHigh=onepole(state.polishHigh,x,polishHighAlpha);
                const float band=state.polishHigh-state.polishLow;
                const float colourDrive[]={.025f,.020f,.009f};
                const float drive=colourDrive[clamp(s.polishMode,0,2)]*s.polishAmount;
                x+=(band/(1.f+amp(band)*.6f)-band)*drive;
                x=before+(x-before)*moduleMix[Polish];
            }
            accumulate(Polish,before,x);
            x*=outputGain;
            if(c==0) {
                matchInputEnv=smooth(matchInputEnv,amp(original),.35f,sr);
                matchOutputEnv=smooth(matchOutputEnv,amp(x),.35f,sr);
                if((modCounter&127)==0 && matchInputEnv>.0001f && matchOutputEnv>.0001f)
                    matchTarget=dbToGain(clamp(gainToDb(matchInputEnv/matchOutputEnv),-6.f,6.f));
            }
            x*=gainMatchGain;
            // Ultimate ceiling: avoids catastrophic output from abusive automation;
            // benign below ~+0 dBFS, gently limited above that.
            if(x>1.f) x=1.f+.15f*(x-1.f)/(1.f+amp(x-1.f));
            else if(x< -1.f) x=-1.f+.15f*(x+1.f)/(1.f+amp(x+1.f));
            if(!std::isfinite(x)) x=0;
            const float wet=original+(x-original)*bypassMix;
            const float delta=original-x;
            const float output=wet+(delta-wet)*deltaMix*bypassMix;
            audio[c][i]=output;
            outMax=std::max(outMax,amp(output));
        }
        delayWrite=(delayWrite+1)%delayCapacity;
        wavePeak=std::max(wavePeak,amp(mono));waveEvents|=events;
        if(++waveCounter>=std::max(1,int(sr/100.f))) {
            const auto index=waveWrite.load(std::memory_order_relaxed)%waveSize;
            waveform[index].store(packWave(wavePeak,gainToDb(effectiveRider),waveEvents,
                                           gainToDb(effectiveLift)),std::memory_order_release);
            waveWrite.store(waveWrite.load(std::memory_order_relaxed)+1,std::memory_order_release);
            waveCounter=0;wavePeak=waveDb=0;waveEvents=0;
        }
        if(capture.samples>0) {
            ++capture.samples;
            if(capture.samples>=int(fs*10.0)) completeAnalysis();
            else progress.store(float(capture.samples)/(sr*10.f),std::memory_order_relaxed);
        }
    }
    // Signed meters: negative = actual attenuation, positive = upward gain.
    for(int k=0;k<kModuleCount;++k) {
        const float actualDb=beforeEnergy[k]>1.e-12?float(10.0*std::log10(
            (afterEnergy[k]+1.e-12)/(beforeEnergy[k]+1.e-12))):0.f;
        moduleMeter[k].store(clamp(actualDb,-18.f,18.f),std::memory_order_relaxed);
    }
    inputPeak.store(inMax,std::memory_order_relaxed);
    outputPeak.store(outMax,std::memory_order_relaxed);
    lastEvents.store(events,std::memory_order_relaxed);
}
Stats Engine::readStats() const {
    Stats s;
    s.inputPeak=inputPeak.load(std::memory_order_relaxed);
    s.outputPeak=outputPeak.load(std::memory_order_relaxed);
    s.detectorRmsDb=currentRmsDb.load(std::memory_order_relaxed);
    s.analysisProgress=progress.load(std::memory_order_relaxed);
    s.events=lastEvents.load(std::memory_order_relaxed);
    for(int i=0;i<kModuleCount;++i)s.reductionDb[i]=moduleMeter[i].load(std::memory_order_relaxed);
    return s;
}
} // namespace steam
