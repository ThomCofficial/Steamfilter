#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include <iostream>
#include <cmath>
#include <memory>

// Exercise the actual VST3 wrapper, host instance, audio, GUI and binary unload.
// The old 40-cycle test only constructed the raw AudioProcessor, whereas Cubase
// also destroys a JUCE VST3 wrapper and host-managed plugin instance.
int main(int argc, char* argv[]) {
    juce::ScopedJuceInitialiser_GUI gui;
    if(argc<2)return 2;
    const juce::File vst3(juce::String::fromUTF8(argv[1]));
    if(!vst3.exists()) {std::cerr<<"VST3 bundle missing\n";return 3;}
    juce::AudioPluginFormatManager manager;
    manager.addDefaultFormats();
    juce::VST3PluginFormat format;
    juce::OwnedArray<juce::PluginDescription> descriptions;
    format.findAllTypesForFile(descriptions,vst3.getFullPathName());
    if(descriptions.isEmpty()){std::cerr<<"VST3 scan found no plugin\n";return 4;}
    juce::MidiBuffer midi;
    for(int iteration=0;iteration<12;++iteration){
        juce::String error;
        auto instance=manager.createPluginInstance(*descriptions[0],48000.0,256,error);
        if(!instance){std::cerr<<"VST3 instance failed: "<<error<<"\n";return 5;}
        instance->setPlayConfigDetails(2,2,48000.0,256);
        instance->prepareToPlay(48000.0,256);
        for(int cycle=0;cycle<3;++cycle){
            std::unique_ptr<juce::AudioProcessorEditor> editor(instance->createEditorIfNeeded());
            if(!editor){std::cerr<<"VST3 editor unavailable\n";return 6;}
            editor->setSize(1505,847);
            juce::AudioBuffer<float> audio(2,256);
            for(int b=0;b<15;++b){
                for(int c=0;c<2;++c){
                    auto* samples=audio.getWritePointer(c);
                    for(int n=0;n<256;++n)
                        samples[n]=.03f*std::sin((b*256+n)*.029f);
                }
                instance->processBlock(audio,midi);
                for(int c=0;c<2;++c)
                    for(int n=0;n<256;++n)
                        if(!std::isfinite(audio.getSample(c,n)))return 7;
            }
            editor.reset();
        }
        instance->releaseResources();
        instance.reset(); // unload VST3 instance and wrapper after GUI teardown
    }
    std::cout<<"PASS: 12 hosted VST3 instances, 36 editor open-close cycles\n";
    return 0;
}
