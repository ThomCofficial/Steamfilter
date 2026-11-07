// Exercises actual JUCE editor lifecycle to catch insert removal regressions.
// This test intentionally runs on a Windows host runner, not the dependency-free DSP.
#include "../Source/PluginProcessor.h"
#include <juce_gui_extra/juce_gui_extra.h>
#include <cmath>
#include <memory>
#include <iostream>

int main() {
    juce::ScopedJuceInitialiser_GUI gui;
    juce::AudioBuffer<float> audio(2,256);
    juce::MidiBuffer midi;
    for(int attempt=0; attempt<40; ++attempt) {
        auto plugin=std::make_unique<SteamFilterAudioProcessor>();
        plugin->prepareToPlay(48000.0,256);
        {
            std::unique_ptr<juce::AudioProcessorEditor> ui(plugin->createEditor());
            if(!ui || ui->getWidth()<=0 || ui->getHeight()<=0) return 10;
            ui->setSize(1505,847);
            if(attempt==0) {
                // A real JUCE render captured from the actual live editor, not
                // an illustration or a mockup. Keeps the visual state auditable.
                const auto image=ui->createComponentSnapshot(ui->getLocalBounds());
                if(!image.isValid()) return 20;
                juce::PNGImageFormat png;
                juce::FileOutputStream out(juce::File::getCurrentWorkingDirectory()
                    .getChildFile("SteamFilter-UI-PREVIEW.png"));
                if(!out.openedOk() || !png.writeImageToStream(image,out))return 21;
            }
            // Exercise parameter callbacks and UI teardown around active audio.
            for(int b=0;b<14;++b) {
                for(int c=0;c<2;++c) {
                    float* out=audio.getWritePointer(c);
                    for(int i=0;i<256;++i)
                        out[i]=.06f*std::sin(float((b*256+i)*.025));
                }
                if(b%3==0)plugin->setParameterValue("breathReduction",float(2+b));
                if(b%5==0)plugin->setParameterValue("polishAmount",.5f);
                plugin->processBlock(audio,midi);
            }
            // Like Cubase removing an insert with its GUI still open.
            ui.reset();
        }
        plugin->releaseResources();
        plugin.reset();
    }
    std::cout<<"PASS: 40 create/process/editor close/plugin destroy cycles\n";
    return 0;
}
