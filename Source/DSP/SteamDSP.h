// SteamFilter — ThomC vocal refinement engine
// Copyright (c) 2026 ThomC. All rights reserved.
// Dependency-free audio engine. No heap allocations, locks or I/O in process().
#pragma once
#include <array>
#include <atomic>
#include <cstdint>
#include <cmath>
#include <algorithm>

namespace steam {
constexpr int kModuleCount = 7;
enum Module { Sweep, Breath, Guard, Sibil, Rider, Lift, Polish };

template<class T> constexpr T clamp(T a, T lo, T hi) { return std::min(hi, std::max(lo, a)); }
inline float dbToGain(float db) { return std::pow(10.f, db / 20.f); }
inline float gainToDb(float gain) { return 20.f * std::log10(std::max(1.e-10f, gain)); }

struct Settings {
    std::array<bool, kModuleCount> enabled{{true,true,true,true,true,true,true}};
    bool bypass = false, delta = false, gainMatch = false;
    float inputDb = 0, outputDb = 0;
    float sweepAmount = .55f, rumbleHz = 65, clickStrength = .20f, noiseReductionDb = 6;
    float breathSensitivity = .54f, breathReductionDb = 9, breathLength = .5f, breathPreserve = .3f;
    float plosiveSensitivity = .50f, plosiveReductionDb = 11, plosiveHoldMs = 45, plosiveFrequency = 200;
    float sibilSensitivity = .55f, sibilFrequency = 6500, sibilReductionDb = 8, sibilWidth = .5f;
    float riderAmount = .7f, riderTargetDb = -23, riderRangeDb = 5, riderSpeed = .5f, riderSidechain = 0;
    float liftThresholdDb = -37, liftMaxDb = 7, liftStrength = .5f, liftAttackMs = 80;
    bool voiceProtect = true;
    int polishMode = 0;
    float polishAmount = .34f;
};

struct Stats {
    float inputPeak = 0, outputPeak = 0;
    std::array<float,kModuleCount> reductionDb{}; // negative=attenuation, positive=boost
    float detectorRmsDb = -100, analysisProgress = 0;
    unsigned events = 0;
};

// Each event bit represents detector activity, not an incoming level.
enum Event : unsigned { EventBreath=1, EventPlosive=2, EventSibil=4, EventClick=8, EventRider=16, EventWhisper=32 };

struct Recommendation {
    float breathSensitivity = .5f, breathReductionDb = 8;
    float plosiveSensitivity = .5f, plosiveReductionDb = 10;
    float sibilSensitivity = .5f, sibilFrequency = 6000, sibilReductionDb = 7;
    float riderTargetDb = -23, liftThresholdDb = -38, liftMaxDb = 6;
    float noiseReductionDb = 5;
    float confidence = 0;
};

struct Biquad {
    float b0=1,b1=0,b2=0,a1=0,a2=0,x1=0,x2=0,y1=0,y2=0;
    float db0=0,db1=0,db2=0,da1=0,da2=0;
    int interpolationLeft=0;bool initialized=false;
    void reset() { x1=x2=y1=y2=0; }
    void setCoefficients(float B0,float B1,float B2,float A1,float A2,float sampleRate) {
        if(!initialized) {b0=B0;b1=B1;b2=B2;a1=A1;a2=A2;initialized=true;interpolationLeft=0;return;}
        interpolationLeft=std::max(1,int(sampleRate*.025f)); // 25ms morph: safe automation
        const float inv=1.f/float(interpolationLeft);
        db0=(B0-b0)*inv;db1=(B1-b1)*inv;db2=(B2-b2)*inv;
        da1=(A1-a1)*inv;da2=(A2-a2)*inv;
    }
    float tick(float in) {
        if(interpolationLeft>0) {
            b0+=db0;b1+=db1;b2+=db2;a1+=da1;a2+=da2;--interpolationLeft;
        }
        const float out = b0*in+b1*x1+b2*x2-a1*y1-a2*y2;
        x2=x1; x1=in; y2=y1; y1=std::isfinite(out)?out:0.f;
        return y1;
    }
    void highpass(float sampleRate,float fc);
    void shelf(float sampleRate,float fc,float gainDb,bool high);
};

struct Channel {
    Biquad rumble, lowShelf, highShelf;
    float popLow=0, deessLow=0, polishLow=0, polishHigh=0;
};

class Engine {
public:
    void prepare(double sampleRate, int maxBlock, int channels);
    int latencySamples() const {return lookaheadSamples;}
    void reset();
    // sidechain can be null. channels 1 or 2; planar audio, processed in-place.
    void process(float* const* audio, const float* const* sidechain, int channels, int sideChannels, int frames, const Settings& settings);
    Stats readStats() const;
    // Each packed datum: 16-bit peak, 16-bit processed gain, 8-bit event mask.
    static constexpr int waveSize=512;
    std::array<std::atomic<uint64_t>, waveSize> waveform{};
    std::atomic<uint32_t> waveWrite{0};
    void requestAnalysis(); // next audio block begins a ten-second streaming capture
    bool takeRecommendation(Recommendation& out);
    float analysisProgress() const { return progress.load(std::memory_order_relaxed); }
    static uint64_t packWave(float peak, float riderDb, unsigned events, float whisperDb=0);
    static void unpackWave(uint64_t data,float& peak,float& riderDb,unsigned& events,float& whisperDb);
    static void unpackWave(uint64_t data,float& peak,float& riderDb,unsigned& events) {
        float ignored;unpackWave(data,peak,riderDb,events,ignored);
    }
private:
    double fs=44100;
    int activeChannels=2, maximumBlock=512;
    std::array<Channel,2> ch;
    // Fixed 4ms lookahead buffers for pre-emptive click/plosive attenuation.
    static constexpr int delayCapacity=4096;
    std::array<std::array<float,delayCapacity>,2> delay{};
    int delayWrite=0, lookaheadSamples=176;
    // Detector one-pole filters + their 15ms and 65ms feature envelopes.
    std::array<float,5> detectorLp{};
    std::array<float,5> eFast{}, eSlow{};
    float rmsEnv=0, slowRms=0, sideEnv=0;
    std::array<float,kModuleCount> moduleMix{};
    bool moduleMixInitialized=false;
    float breathGain=1, sibilGain=1, plosiveGain=1, clickGain=1;
    float noiseGain=1, riderGainDb=0, liftGainDb=0, bypassMix=1;
    float inputGain=1, outputGain=1, riderTargetLinear=1, liftTargetLinear=1;
    float deltaMix=0, gainMatchGain=1, matchTarget=1, matchInputEnv=0, matchOutputEnv=0;
    float riderGainLinear=1, liftGainLinear=1;
    std::array<float,5> detectorAlpha{};
    float popAlpha=0, deessAlpha=0, polishLowAlpha=0, polishHighAlpha=0;
    int modCounter=0;
    int detectCounter=0, breathCount=0, popHold=0, clickHold=0;
    unsigned events=0, analysisEvents=0;bool lastVocalActive=false;
    float wavePeak=0, waveDb=0;
    unsigned waveEvents=0;
    int waveCounter=0;
    std::array<std::atomic<float>,kModuleCount> moduleMeter{};
    std::atomic<float> inputPeak{0}, outputPeak{0}, currentRmsDb{-100};
    std::atomic<unsigned> lastEvents{0};
    std::atomic<bool> analysisRequest{false}, ready{false};
    std::atomic<float> progress{0};
    std::atomic<float> resultFields[10];
    struct Capture { int samples=0, detected=0, active=0, breaths=0,pops=0,sibilants=0,clicks=0; double rmsDbSum=0, brightSum=0; } capture;
    float lastBreathTarget=1, lastSibilTarget=1, lastPlosiveTarget=1,lastClickTarget=1;
    float noiseTarget=1, lastRiderTarget=0,lastLiftTarget=0;
    float fcRumble=0, fcSibil=0, lastPolishAmount=-1, lastPopFrequency=-1;
    int lastPolishMode=-1;
    void updateFilters(const Settings& s);
    void updateDetectors(const Settings& s);
    void completeAnalysis();
};
} // namespace steam
