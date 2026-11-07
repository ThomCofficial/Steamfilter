#include "PluginEditor.h"
#include <BinaryData.h>
#include <cmath>

namespace {
const juce::Colour brass(0xffb47e33),gold(0xffffbf57),ivory(0xffeee3cc),dark(0xff171b20),
    brown(0xff493526),ink(0xff2d251c),steel(0xff777d7d),amber(0xffff9e24),green(0xff69cb86);
constexpr float pi=3.14159265358979f;
void letters(juce::Graphics& g, juce::String value,float x,float y,float w,float h,float size,juce::Colour c,bool bold=false) {
    g.setColour(c);
    g.setFont(juce::Font("Georgia",size,bold?juce::Font::bold:juce::Font::plain));
    g.drawFittedText(value,juce::Rectangle<int>(int(x),int(y),int(w),int(h)),juce::Justification::centred,1);
}
void metal(juce::Graphics& g,juce::Rectangle<float> b,bool pale=true) {
    juce::ColourGradient fill(pale?juce::Colour(0xfff2e7d2):juce::Colour(0xff33383e),b.getX(),b.getY(),
           pale?juce::Colour(0xffcbb99a):juce::Colour(0xff191d21),b.getRight(),b.getBottom(),false);
    fill.addColour(.45,pale?juce::Colour(0xffe8ddc7):juce::Colour(0xff42444b));
    g.setGradientFill(fill);g.fillRoundedRectangle(b,6.f);
    g.setColour(brass);g.drawRoundedRectangle(b.reduced(1),6.f,2.5f);
    g.setColour(juce::Colour(0xff3d2920));g.drawRoundedRectangle(b.reduced(4),5.f,.9f);
    for(const auto point:{juce::Point<float>(b.getX()+10,b.getY()+10),juce::Point<float>(b.getRight()-10,b.getY()+10),
      juce::Point<float>(b.getX()+10,b.getBottom()-10),juce::Point<float>(b.getRight()-10,b.getBottom()-10)}) {
        g.setColour(juce::Colour(0xffc0954a));g.fillEllipse(point.x-3,point.y-3,6,6);
        g.setColour(ink);g.drawLine(point.x-2,point.y+1,point.x+2,point.y-1,1.f);
    }
}
void plate(juce::Graphics& g,float x,float y,float w,float h,juce::String text) {
    const auto r=juce::Rectangle<float>(x,y,w,h);
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xff51545a),x,y,juce::Colour(0xff101418),x,y+h,false));
    g.fillRoundedRectangle(r,6);
    g.setColour(brass);g.drawRoundedRectangle(r,6,2);
    letters(g,text,x+4,y+2,w-8,h-4,h*.60f,ivory,true);
}
void wallpaper(juce::Graphics& g,juce::Rectangle<float> r) {
    juce::Graphics::ScopedSaveState save(g);
    g.reduceClipRegion(r.toNearestInt());
    g.setColour(juce::Colour(0x18a27028));
    for(float x=r.getX()+14;x<r.getRight();x+=29.f)
        for(float y=r.getY()+12;y<r.getBottom();y+=24.f){
            juce::Path p;p.startNewSubPath(x,y+6);p.quadraticTo(x+9,y-4,x+11,y+7);
            p.quadraticTo(x+6,y+13,x+1,y+10);g.strokePath(p,juce::PathStrokeType(.8f));
        }
}
}

BrassLookAndFeel::BrassLookAndFeel(){
    setColour(juce::Slider::textBoxTextColourId,ivory);
    setColour(juce::Slider::textBoxBackgroundColourId,dark);
    setColour(juce::Slider::textBoxOutlineColourId,brass);
}
void BrassLookAndFeel::drawRotarySlider(juce::Graphics& g,int x,int y,int w,int h,float pos,float start,float end,juce::Slider&) {
    // One *moving* indicator and a procedurally illuminated dial. Do not
    // overlay a static pointer image on another, otherwise knobs appear doubled.
    const float cx=float(x)+float(w)*.5f,cy=float(y)+float(h)*.5f;
    const float d=float(std::min(w,h))*.83f;
    const float left=cx-d*.5f,top=cy-d*.5f;
    auto outer=juce::Rectangle<float>(left,top,d,d);
    g.setColour(juce::Colour(0xff241e18));g.fillEllipse(outer.expanded(2.0f));
    g.setColour(juce::Colour(0xfff0b64d));g.drawEllipse(outer,3.f);
    auto inner=outer.reduced(d*.085f);
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xff7c7770),
        inner.getX()+inner.getWidth()*.13f,inner.getY()+inner.getHeight()*.10f,
        juce::Colour(0xff181b20),inner.getRight(),inner.getBottom(),false));
    g.fillEllipse(inner);
    g.setColour(juce::Colour(0xffd6a45c));g.drawEllipse(inner,1.4f);
    auto centre=inner.reduced(d*.13f);
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xff494b4b),
        centre.getX(),centre.getY(),juce::Colour(0xff17191c),
        centre.getRight(),centre.getBottom(),false));
    g.fillEllipse(centre);
    const float angle=start+pos*(end-start);
    const float radius=d*.28f;
    const float vx=std::sin(angle),vy=-std::cos(angle);
    g.setColour(juce::Colours::black.withAlpha(.8f));
    g.drawLine(cx+1.5f,cy+2.f,cx+vx*radius+1.5f,cy+vy*radius+2.f,d*.085f);
    g.setColour(juce::Colour(0xffffa43d));
    g.drawLine(cx,cy,cx+vx*radius,cy+vy*radius,d*.064f);
    g.setColour(juce::Colour(0xffffedac));
    g.drawLine(cx,cy,cx+vx*radius*.95f,cy+vy*radius*.95f,d*.019f);
    g.setColour(juce::Colour(0xffedc277));g.fillEllipse(cx-2.5f,cy-2.5f,5.f,5.f);
}

void BrassLookAndFeel::drawButtonBackground(juce::Graphics& g,juce::Button& b,const juce::Colour&,bool over,bool down) {
    // The engraved buttons are already part of the approved artwork. Only interactive
    // illumination is painted here, never cheap replacement rectangles or text.
    const auto r=b.getLocalBounds().toFloat().reduced(3.f);
    if(b.getToggleState() || down){
        g.setColour(juce::Colour(0x88ff9d20));g.fillRoundedRectangle(r,5.f);
        g.setColour(juce::Colour(0xffffc061));g.drawRoundedRectangle(r,5.f,1.8f);
    }else if(over){g.setColour(juce::Colour(0x50ffd98e));g.fillRoundedRectangle(r,4.f);}
}
void BrassLookAndFeel::drawButtonText(juce::Graphics&,juce::TextButton&,bool,bool) {
    // Names and numeric scales live on the faceplate, never on top of movable buttons.
}
void BrassLookAndFeel::drawToggleButton(juce::Graphics& g,juce::ToggleButton& b,bool over,bool) {
    const auto r=b.getLocalBounds().toFloat().reduced(4);
    const auto col=b.getToggleState()?juce::Colour(0xffffa31d):juce::Colour(0xff322624);
    g.setColour(col.withAlpha(over?.95f:.85f));g.fillEllipse(r);
    g.setColour(b.getToggleState()?juce::Colour(0xffffd683):juce::Colour(0xffa1773a));g.drawEllipse(r,2.f);
    if(b.getToggleState()){
        g.setColour(juce::Colour(0xffffffbf));
        g.drawLine(r.getCentreX(),r.getY()+r.getHeight()*.25f,r.getCentreX(),r.getY()+r.getHeight()*.47f,2.5f);
        juce::Path powerArc;
        powerArc.addCentredArc(r.getCentreX(),r.getCentreY()+r.getHeight()*.08f,r.getWidth()*.23f,r.getHeight()*.23f,0.f,2.4f,6.9f,true);
        g.strokePath(powerArc,juce::PathStrokeType(2.5f));
    }
}

SteamFilterAudioProcessorEditor::SteamFilterAudioProcessorEditor(SteamFilterAudioProcessor& p)
 : juce::AudioProcessorEditor(&p),processor(p) {
    setLookAndFeel(&theme);
    faceplate=juce::ImageCache::getFromMemory(BinaryData::ApprovedFaceplate_jpg,BinaryData::ApprovedFaceplate_jpgSize);
    // Functional animated modules: every knob is a live APVTS attachment.
    addKnob("input",40,226,102,true);addKnob("output",1526,226,102,true);
    constexpr const char* onKeys[]={"sweepOn","breathOn","guardOn","sibilOn","riderOn","liftOn","polishOn"};
    for(int i=0;i<steam::kModuleCount;++i){
        constexpr float powerX[]={146,348,548,749,949,1147,1343};
        addToggle(onKeys[i],powerX[i],354,40);
        addPanel(i);
        moduleMeters[i]=std::make_unique<ModuleMeter>(processor,i);
        addAndMakeVisible(*moduleMeters[i]);
    }
    waveform=std::make_unique<WaveView>(processor);addAndMakeVisible(*waveform);
    leftMeter=std::make_unique<SideMeter>(processor,true);addAndMakeVisible(*leftMeter);
    rightMeter=std::make_unique<SideMeter>(processor,false);addAndMakeVisible(*rightMeter);
    cpuGauge=std::make_unique<PressureGauge>(processor);addAndMakeVisible(*cpuGauge);
    analyzeButton=addTextButton("ANALYSE",1255,179,197,51,[this]{processor.beginAnalyze();suggestionAvailable=false;
        if(suggestButton)suggestButton->setEnabled(false);
        analysisStatus="Listening to playback...";});
    suggestButton=addTextButton("Suggestions",1248,279,203,33,[this]{
        if(!suggestionAvailable) return;
        juce::String msg="Apply these analysis suggestions?\n\nBreath reduction: "+juce::String(currentSuggestion.breathReductionDb,1)+" dB\n"+
          "Plosive reduction: "+juce::String(currentSuggestion.plosiveReductionDb,1)+" dB\n"+
          "De-esser reduction: "+juce::String(currentSuggestion.sibilReductionDb,1)+" dB\n"+
          "Rider target: "+juce::String(currentSuggestion.riderTargetDb,1)+" dB";
        auto safe=juce::Component::SafePointer<SteamFilterAudioProcessorEditor>(this);
        juce::AlertWindow::showOkCancelBox(juce::AlertWindow::QuestionIcon,"SteamFilter analysis",msg,"Apply","Cancel",this,
           juce::ModalCallbackFunction::create([safe](int result){
              if(safe!=nullptr && result!=0 && safe->suggestionAvailable){safe->processor.applyRecommendation(safe->currentSuggestion);
                  safe->analysisStatus="Suggestions applied";safe->suggestionAvailable=false;
                  if(safe->suggestButton)safe->suggestButton->setEnabled(false);}
           }));
    });
    suggestButton->setEnabled(false);
    addTextButton("CAPTURE",1255,243,197,33,[this]{processor.beginAnalyze();suggestionAvailable=false;
        if(suggestButton)suggestButton->setEnabled(false);analysisStatus="Capturing playback...";});
    addTextButton("PRESETS",1179,112,151,33,[this]{showPresets();});
    addTextButton("SAVE",1538,112,41,33,[this]{savePreset();});
    addTextButton("LOAD",1347,112,187,33,[this]{loadPreset();});
    bankAButton=addTextButton("A",890,827,61,43,[this]{processor.switchAB(0);});
    bankBButton=addTextButton("B",954,827,58,43,[this]{processor.switchAB(1);});
    gainMatchButton=addTextButton("GAIN MATCH",1011,827,58,43,[this]{
        if(auto* v=processor.state.getRawParameterValue("gainMatch"))processor.setParameterValue("gainMatch",v->load()>.5f?0.f:1.f);
    });
    bypassButton=addTextButton("BYPASS",1466,744,160,58,[this]{processor.setParameterValue("bypass",processor.state.getRawParameterValue("bypass")->load()>.5f?0.f:1.f);});
    deltaButton=addTextButton("DELTA",1466,816,160,47,[this]{
        if(auto* param=processor.state.getParameter("delta")) processor.setParameterValue("delta",param->getValue()>.5f?0:1);
    });
    addKnob("globalMix",425,808,54);addKnob("maxGain",345,808,54);
    addKnob("detectorSensitivity",557,808,54);addKnob("lookahead",650,808,54);
    addKnob("smoothing",757,808,54);
    sidechainButton=addTextButton("SIDECHAIN",953,631,160,35,[this]{auto* p=processor.state.getRawParameterValue("sidechain");
       if(p)processor.setParameterValue("sidechain",p->load()>.5f?0.f:1.f);});
    voiceProtectButton=addTextButton("VOICE PROTECT",1150,631,162,35,[this]{auto* p=processor.state.getRawParameterValue("voiceProtect");
       if(p)processor.setParameterValue("voiceProtect",p->load()>.5f?0.f:1.f);});
    for(int k=0;k<3;++k){
        const char* names[]={"WARM","PRESENCE","AIR"};
        polishButtons[k]=addTextButton(names[k],1344+k*60.f,573,55,41,[this,k]{processor.setParameterValue("polishMode",float(k));});
    }
    addKnob("input",254,808,54);
    startTimerHz(20);
    setResizable(true,true);setResizeLimits(1254,706,2006,1129);
    if(auto* constrainer=getConstrainer())constrainer->setFixedAspectRatio(double(baseWidth)/baseHeight);
    setSize(1672,941);
}
SteamFilterAudioProcessorEditor::~SteamFilterAudioProcessorEditor(){
    stopTimer();
    // Detach listeners before labels and dynamic controls are destroyed.
    for (auto& knob : knobs) knob->onValueChange = nullptr;
    for (auto& button : textButtons) button->onClick = nullptr;
    knobsBind.clear();
    toggleBind.clear();
    // Stop any pending native dialog before releasing its callbacks.
    fileChooser.reset();
    setLookAndFeel(nullptr);
}

juce::Slider* SteamFilterAudioProcessorEditor::addKnob(const char* id,float x,float y,float size,bool main) {
    auto slider=std::make_unique<juce::Slider>();auto* ptr=slider.get();
    ptr->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    ptr->setRotaryParameters(-2.4f,2.4f,true);
    ptr->setTextBoxStyle(juce::Slider::NoTextBox,false,0,0);
    ptr->setPopupDisplayEnabled(true,false,this);
    if(auto* param=processor.state.getParameter(id))ptr->setTooltip(param->getName(128));
    addAndMakeVisible(*ptr);
    knobsBind.push_back(std::make_unique<SteamFilterAudioProcessor::APVTS::SliderAttachment>(processor.state,id,*ptr));
    knobRects.push_back({ptr,{x,y,size,size}});
    knobs.push_back(std::move(slider));return ptr;
}
juce::TextButton* SteamFilterAudioProcessorEditor::addTextButton(const juce::String& text,float x,float y,float w,float h,std::function<void()> fn) {
    auto b=std::make_unique<juce::TextButton>(text);auto* ptr=b.get();
    ptr->onClick=std::move(fn);addAndMakeVisible(*ptr);
    buttonRects.push_back({ptr,{x,y,w,h}});textButtons.push_back(std::move(b));return ptr;
}
juce::ToggleButton* SteamFilterAudioProcessorEditor::addToggle(const char* id,float x,float y,float size) {
    auto b=std::make_unique<juce::ToggleButton>();auto* ptr=b.get();addAndMakeVisible(*ptr);
    toggleBind.push_back(std::make_unique<SteamFilterAudioProcessor::APVTS::ButtonAttachment>(processor.state,id,*ptr));
    buttonRects.push_back({ptr,{x,y,size,size}});toggleButtons.push_back(std::move(b));return ptr;
}
void SteamFilterAudioProcessorEditor::addPanel(int index) {
    const char* large[]={"sweepAmount","breathReduction","guardReduction","sibilReduction","riderAmount","liftAmount","polishAmount"};
    constexpr int mainX[]={190,389,592,793,989,1190,1385};
    addKnob(large[index],float(mainX[index]),423,102,true);
    constexpr const char* small[6][3]={
        {"click","rumble","noise"},{"breathSensitivity","breathLength","breathPreserve"},
        {"guardFrequency","guardHold","guardSensitivity"},{"sibilFrequency","sibilWidth","sibilSensitivity"},
        {"riderTarget","riderRange","riderSpeed"},{"liftThreshold","liftRange","liftAttack"}};
    constexpr int positions[6][3]={{147,209,272},{349,410,474},{552,617,682},
                                    {748,811,870},{948,1010,1072},{1146,1210,1272}};
    if(index<6 && index!=1)for(int j=0;j<3;++j)addKnob(small[index][j],float(positions[index][j]),546,54);
}
void SteamFilterAudioProcessorEditor::drawStaticUI(juce::Graphics& g) {
    if(faceplate.isValid())
        g.drawImage(faceplate,0.f,0.f,baseWidth,baseHeight,0,0,faceplate.getWidth(),faceplate.getHeight());
    else g.fillAll(juce::Colour(0xff282427));

    // The approved artwork contains baked-in obsolete labels and buttons.
    // These four areas are now clean Victorian wallpaper, not dead controls.
    auto cover=[&](juce::Rectangle<float> r) {
        metal(g,r); wallpaper(g,r.reduced(7.f));
    };
    cover({12.f,393.f,131.f,229.f}); // removed VOCAL/SPEECH/CUSTOM
    cover({149.f,621.f,184.f,57.f}); // removed SteamSweep presets
    cover({348.f,621.f,190.f,57.f}); // removed SteamBreath presets
    cover({551.f,621.f,184.f,57.f}); // removed SteamGuard presets
    cover({747.f,621.f,184.f,57.f}); // removed SteamSibil presets
    cover({347.f,546.f,190.f,73.f}); // SteamBreath only needs its AMOUNT knob
    letters(g,"AUTOMATIC BREATH DETECTION",352.f,566.f,180.f,24.f,12.f,ink,true);
    cover({1163.f,785.f,286.f,122.f}); // removed LIMITS & SAFETY

}
void SteamFilterAudioProcessorEditor::paint(juce::Graphics& g) {
    juce::Graphics::ScopedSaveState guard(g);
    g.addTransform(juce::AffineTransform::scale(xScale(),yScale()));
    drawStaticUI(g);
    if(analysisStatus.isNotEmpty() && (analysisStatus.startsWith("Listening") ||
        analysisStatus.startsWith("Capturing") || analysisStatus.startsWith("Ready") ||
        analysisStatus.startsWith("No usable") || analysisStatus.startsWith("Suggestions"))) {
        g.setColour(juce::Colour(0xff251e1a));g.fillRoundedRectangle(1252.f,315.f,200.f,18.f,3.f);
        letters(g,analysisStatus,1254,315,195,17,11,juce::Colour(0xfff7d995),true);
    }
}
void SteamFilterAudioProcessorEditor::resized() {
    const auto sx=xScale(),sy=yScale();
    for(auto& c:knobRects){const auto r=c.r;
        c.slider->setBounds(int(r.getX()*sx),int(r.getY()*sy),int(r.getWidth()*sx),int(r.getHeight()*sy));}
    for(auto& c:buttonRects){const auto r=c.r;
        c.button->setBounds(int(r.getX()*sx),int(r.getY()*sy),int(r.getWidth()*sx),int(r.getHeight()*sy));}
    waveform->setBounds(int(203*sx),int(172*sy),int(1022*sx),int(161*sy));
    leftMeter->setBounds(int(47*sx),int(357*sy),int(82*sx),int(10*sy));
    rightMeter->setBounds(int(1537*sx),int(393*sy),int(83*sx),int(290*sy));
    cpuGauge->setBounds(int(10*sx),int(719*sy),int(169*sx),int(176*sy));
    constexpr float metersX[]={307,510,708,910,1110,1305,1502};
    for(int k=0;k<steam::kModuleCount;++k)
        moduleMeters[k]->setBounds(int(metersX[k]*sx),int(438*sy),int(17*sx),int(64*sy));
}

void SteamFilterAudioProcessorEditor::timerCallback() {
    steam::Recommendation suggestion;
    if(processor.getRecommendation(suggestion)){
        currentSuggestion=suggestion;
        suggestionAvailable=suggestion.confidence>=.12f;
        suggestButton->setEnabled(suggestionAvailable);
        analysisStatus=suggestionAvailable?"Ready — review suggestions":"No usable vocal detected";repaint();
    }
    const float pr=processor.engine.analysisProgress();
    if(pr>0.f && pr<1.f && !suggestionAvailable) {
        const auto newStatus=juce::String("Listening ")+juce::String(juce::roundToInt(pr*100.f))+"%";
        if(analysisStatus!=newStatus){analysisStatus=newStatus;repaint();}
    }
    if(bankAButton && bankBButton){bankAButton->setToggleState(processor.currentAB()==0,juce::dontSendNotification);
        bankBButton->setToggleState(processor.currentAB()==1,juce::dontSendNotification);}
    const auto reflect=[this](juce::TextButton* button,const char* param){
        if(button)if(auto* raw=processor.state.getRawParameterValue(param))
            button->setToggleState(raw->load(std::memory_order_relaxed)>.5f,juce::dontSendNotification);
    };
    reflect(gainMatchButton,"gainMatch");reflect(deltaButton,"delta");reflect(bypassButton,"bypass");
    reflect(sidechainButton,"sidechain");reflect(voiceProtectButton,"voiceProtect");
    if(auto* v=processor.state.getRawParameterValue("polishMode")){
        const int m=juce::jlimit(0,2,int(v->load()));
        for(int i=0;i<3;++i)if(polishButtons[i])polishButtons[i]->setToggleState(i==m,juce::dontSendNotification);
    }
}

void SteamFilterAudioProcessorEditor::WaveView::paint(juce::Graphics& g) {
    auto b=getLocalBounds().toFloat();
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xff0c141e),0,0,juce::Colour(0xff22211e),0,b.getHeight(),false));
    g.fillRoundedRectangle(b,3.f);
    g.setColour(brass);g.drawRoundedRectangle(b.reduced(1),3.f,1.4f);
    const float middle=b.getCentreY();
    g.setColour(juce::Colour(0x44b3ad83));
    for(int j=1;j<=5;++j)g.drawHorizontalLine(int(b.getHeight()*j/6),0,b.getWidth());
    const uint32_t end=owner.engine.waveWrite.load(std::memory_order_acquire);
    const float dx=b.getWidth()/float(steam::Engine::waveSize);
    juce::Path rider,lift;rider.preallocateSpace(steam::Engine::waveSize*3);lift.preallocateSpace(steam::Engine::waveSize*3);
    for(int i=0;i<steam::Engine::waveSize;++i){
        const auto index=(end+i)%steam::Engine::waveSize;
        const auto packed=owner.engine.waveform[index].load(std::memory_order_acquire);
        float peak,riderDb,liftDb;unsigned events;
        steam::Engine::unpackWave(packed,peak,riderDb,events,liftDb);
        const float x=float(i)*dx;
        const float height=std::min(peak,1.f)*middle*.85f;
        g.setColour(juce::Colour(0xffe9c995).withAlpha(.35f+peak*.6f));
        g.drawVerticalLine(juce::roundToInt(x),middle-height,middle+height);
        if(events!=0){
            const juce::Colour mark= (events&steam::EventPlosive)?juce::Colour(0xffd4453d):
                ((events&steam::EventClick)?juce::Colour(0xffbb65d2):
                ((events&steam::EventSibil)?juce::Colour(0xff4bb7df):amber));
            g.setColour(mark.withAlpha(.23f));g.fillRect(x,0.f,dx+1.f,b.getHeight());
        }
        const float y=b.getHeight()*.75f-juce::jlimit(-13.f,13.f,riderDb)*2.f;
        const float ly=b.getHeight()*.89f-juce::jlimit(-13.f,13.f,liftDb)*2.f;
        if(i==0){rider.startNewSubPath(x,y);lift.startNewSubPath(x,ly);}
        else {rider.lineTo(x,y);lift.lineTo(x,ly);}
    }
    g.setColour(green);g.strokePath(rider,juce::PathStrokeType(1.6f));
    g.setColour(amber);g.strokePath(lift,juce::PathStrokeType(1.4f));
    // ASCII-only typography and individually painted colour swatches.
    // Font substitution in Cubase must not produce mojibake squares.
    struct Legend{const char* name;juce::Colour colour;float step;};
    const Legend legend[]={
        {"BREATH",juce::Colour(0xfff6bd32),110.f},
        {"PLOSIVE",juce::Colour(0xffd4453d),113.f},
        {"CLICK",juce::Colour(0xffbb65d2),95.f},
        {"SIBILANCE",juce::Colour(0xff4bb7df),122.f},
        {"RIDER",juce::Colour(0xff69cb86),93.f},
        {"WHISPER",juce::Colour(0xffff9e24),105.f}
    };
    float xx=10.f;
    for(const auto& item:legend){
        g.setColour(item.colour);g.fillRoundedRectangle(xx,9.f,8.f,8.f,1.f);
        g.setColour(ivory);g.setFont(juce::Font("Georgia",11.f,juce::Font::plain));
        g.drawText(item.name,int(xx+12),4,int(item.step-12),20,juce::Justification::centredLeft);
        xx+=item.step;
    }
}
void SteamFilterAudioProcessorEditor::ModuleMeter::timerCallback() {
    const auto data=owner.engine.readStats();
    const float target=data.reductionDb[index];
    displayed+=(target-displayed)*.28f;
    setTooltip(juce::String(displayed,1)+" dB actual "+
        (displayed>=0.f?"gain":"reduction")+" (not input level)");
    if(isShowing())repaint();
}
void SteamFilterAudioProcessorEditor::ModuleMeter::paint(juce::Graphics& g) {
    auto b=getLocalBounds().toFloat();
    g.setColour(juce::Colour(0xff15171a));g.fillRoundedRectangle(b,2.f);
    const float mag=juce::jlimit(0.f,1.f,std::abs(displayed)/12.f);
    for(int i=0;i<8;++i) {
        const float y=b.getHeight()-5.f-float(i)*7.1f;
        const bool lit=mag>(float(i)+.45f)/8.f;
        const auto col=displayed<0?juce::Colour(0xff9fe96a):juce::Colour(0xffffae31);
        g.setColour(lit?col:juce::Colour(0xff34412b));
        g.fillRoundedRectangle(3.f,y,b.getWidth()-6.f,4.4f,1.f);
    }
    g.setColour(juce::Colour(0xffd29b45));g.drawRoundedRectangle(b.reduced(.6f),2.f,1.f);
}
void SteamFilterAudioProcessorEditor::SideMeter::timerCallback() {
    auto s=owner.engine.readStats();
    float target=isInput?s.inputPeak:s.outputPeak;
    displayed=juce::jmax(target,displayed*.82f);
    if(isShowing())repaint();
}
void SteamFilterAudioProcessorEditor::SideMeter::paint(juce::Graphics& g) {
    const auto b=getLocalBounds().toFloat();
    const float db=steam::gainToDb(displayed);
    const float fraction=juce::jlimit(0.f,1.f,(db+60.f)/60.f);
    if(!isInput){
        // Two antique glowing glass columns. The lower light level is not baked in.
        for(int col=0;col<2;++col){
            auto strip=juce::Rectangle<float>(18.f+col*36.f,9.f,19.f,b.getHeight()-18.f);
            g.setColour(juce::Colour(0xf31b1314));g.fillRoundedRectangle(strip,8.f);
            const float segHeight=(strip.getHeight()-8.f)/24.f;
            for(int k=0;k<24;++k){
                const float y=strip.getBottom()-5.f-(k+1)*segHeight;
                auto c=k>20?juce::Colour(0xffed5746):k>15?juce::Colour(0xffffb63e):juce::Colour(0xffc77d26);
                if((k+.5f)/24.f>fraction)c=c.withMultipliedBrightness(.14f);
                g.setColour(c);g.fillRoundedRectangle(strip.getX()+3,y,strip.getWidth()-6,segHeight-2.f,1.4f);
            }
            g.setColour(juce::Colour(0xffcf9439));g.drawRoundedRectangle(strip,8.f,1.4f);
        }
    }else{
        g.setColour(juce::Colour(0xff251d19));g.fillRoundedRectangle(b,4.f);
        g.setColour(juce::Colour(0xfff4a733));
        g.fillRoundedRectangle({2.f,2.f,(b.getWidth()-4.f)*fraction,b.getHeight()-4.f},2.f);
    }
}
void SteamFilterAudioProcessorEditor::PressureGauge::timerCallback(){
    displayed+=(owner.dspLoad.load(std::memory_order_relaxed)-displayed)*.2f;
    if(isShowing())repaint();
}
void SteamFilterAudioProcessorEditor::PressureGauge::paint(juce::Graphics& g) {
    static const auto img=juce::ImageCache::getFromMemory(BinaryData::GaugeFace_png,BinaryData::GaugeFace_pngSize);
    if(img.isValid())g.drawImageWithin(img,0,0,getWidth(),getHeight(),juce::RectanglePlacement::centred);
    const auto b=getLocalBounds().toFloat();
    const auto center=juce::Point<float>(b.getWidth()*.5f,b.getHeight()*.46f);
    const float angle=2.65f+juce::jlimit(0.f,1.f,displayed)*4.3f;
    const auto length=b.getWidth()*.29f;
    g.setColour(juce::Colour(0x80251910));
    g.drawLine(center.x+1.4f,center.y+1.4f,center.x+std::cos(angle)*length+1.4f,
        center.y+std::sin(angle)*length+1.4f,4.f);
    g.setColour(juce::Colour(0xff9f2317));
    g.drawLine(center.x,center.y,center.x+std::cos(angle)*length,
        center.y+std::sin(angle)*length,2.5f);
    g.setColour(juce::Colour(0xffd5a25a));g.fillEllipse(center.x-3,center.y-3,6,6);
}
void SteamFilterAudioProcessorEditor::showPresets(){
    juce::PopupMenu m;m.addItem(1,"Transparent Vocal");m.addItem(2,"Vocal Clean & Polish");m.addItem(3,"Whisper Forward");
    m.addSeparator();m.addItem(4,"Save preset...");m.addItem(5,"Load preset...");
    juce::Component::SafePointer<SteamFilterAudioProcessorEditor> safe(this);
    m.showMenuAsync(juce::PopupMenu::Options(),[safe](int result){
        if(safe==nullptr)return;
        if(result>=1 && result<=3)safe->processor.selectFactoryPreset(result-1);
        else if(result==4)safe->savePreset();else if(result==5)safe->loadPreset();
    });
}
void SteamFilterAudioProcessorEditor::savePreset(){
    fileChooser=std::make_unique<juce::FileChooser>("Save SteamFilter preset",juce::File{},"*.steamfilter");
    fileChooser->launchAsync(juce::FileBrowserComponent::saveMode|juce::FileBrowserComponent::canSelectFiles,
        [safe=juce::Component::SafePointer<SteamFilterAudioProcessorEditor>(this)](const juce::FileChooser& f){
            if (safe == nullptr) return;
            auto file=f.getResult();if(file==juce::File{})return;
            if(file.getFileExtension()!=".steamfilter")file=file.withFileExtension(".steamfilter");
            auto xml=safe->processor.state.copyState().createXml();
            if(xml && file.replaceWithText(xml->toString())) {safe->analysisStatus="Preset saved";safe->repaint();}
        });
}
void SteamFilterAudioProcessorEditor::loadPreset(){
    fileChooser=std::make_unique<juce::FileChooser>("Load SteamFilter preset",juce::File{},"*.steamfilter");
    fileChooser->launchAsync(juce::FileBrowserComponent::openMode|juce::FileBrowserComponent::canSelectFiles,
        [safe=juce::Component::SafePointer<SteamFilterAudioProcessorEditor>(this)](const juce::FileChooser& f){
            if (safe == nullptr) return;
            auto file=f.getResult();if(file==juce::File{})return;
            auto xml=juce::XmlDocument::parse(file);
            if(xml){auto tree=juce::ValueTree::fromXml(*xml);
                if(tree.isValid() && tree.hasType(safe->processor.state.state.getType())){
                    safe->processor.state.replaceState(tree);safe->analysisStatus="Preset loaded";safe->repaint();}
            }
        });
}