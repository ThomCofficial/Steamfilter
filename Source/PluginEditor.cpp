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
    const float d=float(std::min(w,h))*.92f,cx=x+w*.5f,cy=y+h*.5f,r=d*.5f;
    g.setColour(juce::Colours::black.withAlpha(.5f));g.fillEllipse(cx-r+3,cy-r+5,d,d);
    g.setColour(brass.darker(.25f));g.fillEllipse(cx-r-2,cy-r-2,d+4,d+4);
    g.setColour(gold);g.drawEllipse(cx-r,cy-r,d,d,1.7f);
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xff77787a),cx-r,cy-r,
                     juce::Colour(0xff202125),cx+r,cy+r,true));
    g.fillEllipse(cx-r+5,cy-r+5,d-10,d-10);
    for(int i=0;i<16;++i){
        const float t=start+(end-start)*(float(i)/15.f);
        const float x1=cx+std::cos(t)*(r-8),y1=cy+std::sin(t)*(r-8);
        const float x2=cx+std::cos(t)*(r-3),y2=cy+std::sin(t)*(r-3);
        g.setColour(i%2==0?ivory:brass);g.drawLine(x1,y1,x2,y2,1);
    }
    const float angle=start+pos*(end-start);
    const float dx=std::cos(angle),dy=std::sin(angle);
    g.setColour(juce::Colours::black);g.drawLine(cx+2,cy+2,cx+dx*(r-12)+2,cy+dy*(r-12)+2,5);
    g.setColour(gold);g.drawLine(cx,cy,cx+dx*(r-12),cy+dy*(r-12),2.5f);
    g.setColour(ivory);g.fillEllipse(cx-3,cy-3,6,6);
}
void BrassLookAndFeel::drawButtonBackground(juce::Graphics& g,juce::Button& b,const juce::Colour&,bool over,bool down) {
    auto r=b.getLocalBounds().toFloat().reduced(1);
    const bool selected=b.getToggleState();
    const auto c=selected?juce::Colour(0xffc97514):(down?juce::Colour(0xff745330):juce::Colour(0xff22262a));
    g.setGradientFill(juce::ColourGradient(c.brighter(over?.18f:0.f),0,0,c.darker(.35f),0,r.getHeight(),false));
    g.fillRoundedRectangle(r,4);
    g.setColour(selected?gold:brass);g.drawRoundedRectangle(r,4,1.6f);
    if(selected){g.setColour(amber.withAlpha(.26f));g.drawRoundedRectangle(r.reduced(2),4,2);}
}
void BrassLookAndFeel::drawButtonText(juce::Graphics& g,juce::TextButton& b,bool,bool) {
    g.setColour(b.isEnabled()?ivory:steel);
    g.setFont(juce::Font("Georgia",std::min(17.f,float(b.getHeight())*.49f),juce::Font::bold));
    g.drawFittedText(b.getButtonText(),b.getLocalBounds().reduced(4,2),juce::Justification::centred,1);
}
void BrassLookAndFeel::drawToggleButton(juce::Graphics& g,juce::ToggleButton& b,bool,bool) {
    auto r=b.getLocalBounds().toFloat().reduced(1);
    g.setColour(brass);g.fillEllipse(r);
    g.setColour(b.getToggleState()?amber:dark);g.fillEllipse(r.reduced(3));
    g.setColour(b.getToggleState()?ivory:steel);
    juce::Path path;const auto cx=r.getCentreX(),cy=r.getCentreY();
    path.addCentredArc(cx,cy,r.getWidth()*.22f,r.getHeight()*.22f,0.f,.4f,5.9f,true);
    g.strokePath(path,juce::PathStrokeType(2.f));
    g.drawLine(cx,cy-r.getHeight()*.28f,cx,cy,2.f);
}

SteamFilterAudioProcessorEditor::SteamFilterAudioProcessorEditor(SteamFilterAudioProcessor& p)
 : juce::AudioProcessorEditor(&p),processor(p) {
    setLookAndFeel(&theme);
    header=juce::ImageCache::getFromMemory(BinaryData::MaritimeHeader_jpg,BinaryData::MaritimeHeader_jpgSize);
    // Functional animated modules: every knob is a live APVTS attachment.
    addKnob("input",38,223,96,true);addKnob("output",1536,223,96,true);
    constexpr const char* onKeys[]={"sweepOn","breathOn","guardOn","sibilOn","riderOn","liftOn","polishOn"};
    for(int i=0;i<steam::kModuleCount;++i){
        addToggle(onKeys[i],174.f+i*184.f,356,33);
        addPanel(i);
        moduleMeters[i]=std::make_unique<ModuleMeter>(processor,i);
        addAndMakeVisible(*moduleMeters[i]);
    }
    waveform=std::make_unique<WaveView>(processor);addAndMakeVisible(*waveform);
    leftMeter=std::make_unique<SideMeter>(processor,true);addAndMakeVisible(*leftMeter);
    rightMeter=std::make_unique<SideMeter>(processor,false);addAndMakeVisible(*rightMeter);
    cpuGauge=std::make_unique<PressureGauge>(processor);addAndMakeVisible(*cpuGauge);
    analyzeButton=addTextButton("ANALYSE",1299,189,158,51,[this]{processor.beginAnalyze();suggestionAvailable=false;
        if(suggestButton)suggestButton->setEnabled(false);
        analysisStatus="Listening to playback...";});
    suggestButton=addTextButton("Suggestions",1299,278,158,30,[this]{
        if(!suggestionAvailable) return;
        juce::String msg="Apply these analysis suggestions?\n\nBreath reduction: "+juce::String(currentSuggestion.breathReductionDb,1)+" dB\n"+
          "Plosive reduction: "+juce::String(currentSuggestion.plosiveReductionDb,1)+" dB\n"+
          "De-esser reduction: "+juce::String(currentSuggestion.sibilReductionDb,1)+" dB\n"+
          "Rider target: "+juce::String(currentSuggestion.riderTargetDb,1)+" dB";
        auto safe = juce::Component::SafePointer<SteamFilterAudioProcessorEditor>(this);
        juce::AlertWindow::showOkCancelBox(juce::AlertWindow::QuestionIcon,"SteamFilter analysis",msg,"Apply","Cancel",this,
           juce::ModalCallbackFunction::create([safe](int result){
              if(safe!=nullptr && result!=0 && safe->suggestionAvailable){safe->processor.applyRecommendation(safe->currentSuggestion);
                  safe->analysisStatus="Suggestions applied";safe->suggestionAvailable=false;
                  if(safe->suggestButton)safe->suggestButton->setEnabled(false);}
           }));
    });
    suggestButton->setEnabled(false);
    addTextButton("PRESETS",1206,110,112,33,[this]{showPresets();});
    addTextButton("SAVE",1380,110,67,33,[this]{savePreset();});
    addTextButton("LOAD",1451,110,66,33,[this]{loadPreset();});
    addTextButton("VOCAL",32,454,115,37,[this]{processor.selectFactoryPreset(1);});
    addTextButton("SPEECH",32,502,115,37,[this]{processor.selectFactoryPreset(0);});
    addTextButton("CUSTOM",32,550,115,37,[this]{analysisStatus="Custom: adjust parameters freely";repaint();});
    bankAButton=addTextButton("A",907,814,57,44,[this]{processor.switchAB(0);});
    bankBButton=addTextButton("B",974,814,57,44,[this]{processor.switchAB(1);});
    gainMatchButton=addTextButton("GAIN MATCH",1040,814,112,44,[this]{
        if(auto* v=processor.state.getRawParameterValue("gainMatch"))processor.setParameterValue("gainMatch",v->load()>.5f?0.f:1.f);
    });
    bypassButton=addTextButton("BYPASS",1486,783,150,49,[this]{processor.setParameterValue("bypass",processor.state.getRawParameterValue("bypass")->load()>.5f?0.f:1.f);});
    deltaButton=addTextButton("DELTA",1486,850,150,43,[this]{
        if(auto* param=processor.state.getParameter("delta")) processor.setParameterValue("delta",param->getValue()>.5f?0:1);
    });
    addKnob("riderRange",1218,807,60);addKnob("liftRange",1330,807,60);
    sidechainButton=addTextButton("SIDECHAIN",909,638,164,33,[this]{auto* p=processor.state.getRawParameterValue("sidechain");
       if(p)processor.setParameterValue("sidechain",p->load()>.5f?0.f:1.f);});
    voiceProtectButton=addTextButton("VOICE PROTECT",1094,638,164,33,[this]{auto* p=processor.state.getRawParameterValue("voiceProtect");
       if(p)processor.setParameterValue("voiceProtect",p->load()>.5f?0.f:1.f);});
    for(int k=0;k<4;++k){
       constexpr const char* presetNames[4][3]={{"GENTLE","BALANCED","DEEP"},
       {"NATURAL","GENTLE","CLEAN"},{"BROAD","FOCUS","DYNAMIC"},{"SPLIT","WIDE","DYNAMIC"}};
       for(int j=0;j<3;++j) addTextButton(presetNames[k][j],173+k*184.f+j*53.f,638,50,32,[this,k,j]{
          if(k==0){processor.setParameterValue("sweepAmount",j==0?.2f:j==1?.55f:.95f);}
          if(k==1){processor.setParameterValue("breathPreserve",j==0?.9f:j==1?.5f:.15f);
                   processor.setParameterValue("breathReduction",j==0?5.f:j==1?9.f:15.f);}
          if(k==2){processor.setParameterValue("guardReduction",j==0?5.f:j==1?11.f:16.f);
                   processor.setParameterValue("guardHold",j==0?24.f:j==1?45.f:70.f);}
          if(k==3){processor.setParameterValue("sibilWidth",j==0?0.f:j==1?1.f:.5f);}
       });
    }
    for(int k=0;k<3;++k){
        const char* names[]={"WARM","PRESENCE","AIR"};
        polishButtons[k]=addTextButton(names[k],1275+k*53.f,638,50,32,[this,k]{processor.setParameterValue("polishMode",float(k));});
    }
    startTimerHz(12);
    setResizable(true,true);setResizeLimits(1254,706,2006,1129);setSize(1672,941);
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
    ptr->setTooltip(processor.state.getParameter(id)?processor.state.getParameter(id)->getName(128):id);
    addAndMakeVisible(*ptr);
    auto attachment=std::make_unique<SteamFilterAudioProcessor::APVTS::SliderAttachment>(processor.state,id,*ptr);
    knobsBind.push_back(std::move(attachment));
    {
        auto lab=std::make_unique<juce::Label>();auto* disp=lab.get();
        disp->setJustificationType(juce::Justification::centred);
        disp->setColour(juce::Label::textColourId,ink);disp->setFont(juce::Font("Georgia",main?13.f:10.f,juce::Font::bold));
        addAndMakeVisible(*disp);valueLabels.push_back(std::move(lab));
        ptr->onValueChange=[ptr,disp]{disp->setText(juce::String(ptr->getValue(),1),juce::dontSendNotification);};
        ptr->onValueChange();
        knobRects.push_back({ptr,disp,{x,y,size,size}});
    }
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
    const float x=164+184.f*index;
    const char* large[]={"sweepAmount","breathReduction","guardReduction","sibilReduction","riderAmount","liftAmount","polishAmount"};
    addKnob(large[index],x+31,433,103,true);
    if(index==0){addKnob("click",x+7,553,44);addKnob("rumble",x+61,553,44);addKnob("noise",x+115,553,44);}
    if(index==1){addKnob("breathSensitivity",x+7,553,44);addKnob("breathLength",x+61,553,44);addKnob("breathPreserve",x+115,553,44);}
    if(index==2){addKnob("guardFrequency",x+7,553,44);addKnob("guardHold",x+61,553,44);addKnob("guardSensitivity",x+115,553,44);}
    if(index==3){addKnob("sibilFrequency",x+7,553,44);addKnob("sibilWidth",x+61,553,44);addKnob("sibilSensitivity",x+115,553,44);}
    if(index==4){addKnob("riderTarget",x+7,553,44);addKnob("riderRange",x+61,553,44);addKnob("riderSpeed",x+115,553,44);}
    if(index==5){addKnob("liftThreshold",x+7,553,44);addKnob("liftRange",x+61,553,44);addKnob("liftAttack",x+115,553,44);}
    // SteamPolish mode buttons are placed lower, no duplicate knobs.
}
void SteamFilterAudioProcessorEditor::drawFrame(juce::Graphics& g,juce::Rectangle<float> r,const juce::String& title,bool titled) {
    metal(g,r,true);wallpaper(g,r.reduced(7));
    if(titled)plate(g,r.getX()+8,r.getY()+7,r.getWidth()-16,29,title);
}
void SteamFilterAudioProcessorEditor::drawStaticUI(juce::Graphics& g) {
    auto frame=juce::Rectangle<float>(0,0,baseWidth,baseHeight);
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xff5e4c3b),0,0,dark,0,baseHeight,false));
    g.fillRect(frame);
    if(header.isValid())g.drawImage(header,0,0,1672,151,0,0,header.getWidth(),header.getHeight());
    g.setColour(gold);g.drawRect(frame.reduced(2),4.f);
    g.setColour(brass);g.drawRect(frame.reduced(9),2.f);
    drawFrame(g,{12,161,145,529},"INPUT");
    drawFrame(g,{1518,161,143,529},"OUTPUT");
    drawFrame(g,{167,159,1118,178},"",false);
    drawFrame(g,{1292,162,181,174},"ANALYSE");
    plate(g,38,175,95,33,"INPUT");plate(g,1533,175,106,33,"OUTPUT");
    letters(g,"dB",55,329,54,19,15,ink,true);letters(g,"dB",1547,329,54,19,15,ink,true);
    plate(g,28,396,112,35,"MODE");
    for(int k=0;k<steam::kModuleCount;++k){
        const float x=164+184.f*k;
        const juce::String names[]={"SteamSweep","SteamBreath","SteamGuard","SteamSibil","SteamRider","WhisperLift","SteamPolish"};
        const juce::String subtitles[]={"MOUTH CLICKS & NOISE","BREATH CONTROL","PLOSIVE CONTROL","DE-ESSER","VOCAL LEVEL RIDER","UPWARD COMPRESSION","VOCAL EQ & CHARACTER"};
        drawFrame(g,{x,344,176,340},"",false);
        plate(g,x+44,355,123,33,names[k]);
        letters(g,subtitles[k],x+5,390,166,27,12,ink,true);
        letters(g,(k==4||k==6)?"AMOUNT":(k==5?"LIFT":"REDUCTION"),x+28,532,107,22,12,ink,true);
        if(k<6){
            constexpr const char* labels[6][3]={
                {"CLICK","RUMBLE","NOISE"},{"SENS","LENGTH","PRESERVE"},
                {"FREQ","HOLD","SENS"},{"FREQ","WIDTH","SENS"},
                {"TARGET","RANGE","SPEED"},{"THRESH","RANGE","ATTACK"}};
            for(int j=0;j<3;++j)letters(g,labels[k][j],x+5+j*54,597,54,16,10,ink,true);
        }
        letters(g,k==6?"EQ":"GR",x+139,542,27,16,10,ink,true);
    }
    plate(g,230,703,1200,31,"OPTIMIZED PROCESSING CHAIN   •   PLOSIVES  ›  CLICKS  ›  BREATH  ›  DE-ESS  ›  RIDE  ›  LIFT  ›  POLISH");
    drawFrame(g,{180,747,335,150},"GLOBAL & SAFETY");
    drawFrame(g,{522,747,358,150},"DETECTION & ANALYSIS");
    drawFrame(g,{888,747,279,150},"A / B PRESETS");
    drawFrame(g,{1176,747,285,150},"GAIN LIMITS");
    letters(g,"RIDER MAX",1234,876,101,18,12,ink,true);
    letters(g,"LIFT MAX",1333,876,101,18,12,ink,true);
    letters(g,"Captures 10s of playback; review before applying",1296,315,178,18,10,ink);
    letters(g,"Stereo-linked detectors  •  zero heap allocation in audio",539,792,326,23,12,ink);
    letters(g,"CPU meter shows processing time, not DAW CPU",185,865,311,18,10,ink);
    plate(g,1480,734,167,33,"MASTER");
}
void SteamFilterAudioProcessorEditor::paint(juce::Graphics& g) {
    juce::Graphics::ScopedSaveState s(g);
    g.addTransform(juce::AffineTransform::scale(xScale(),yScale()));
    drawStaticUI(g);
    letters(g,analysisStatus,1298,247,167,27,11,ink,true);
}
void SteamFilterAudioProcessorEditor::resized() {
    const auto sx=xScale(),sy=yScale();
    for(auto& c:knobRects){
        auto r=c.r;
        c.slider->setBounds(juce::Rectangle<int>(int(r.getX()*sx),int(r.getY()*sy),int(r.getWidth()*sx),int(r.getHeight()*sy)));
        if(c.display){const float y=r.getY()<300?r.getBottom()+2:(r.getY()>545 && r.getY()<590?611.f:r.getY()-23);
            c.display->setBounds(int(r.getX()*sx),int(y*sy),int(r.getWidth()*sx),int(20*sy));}
    }
    for(auto& c:buttonRects){auto r=c.r;c.button->setBounds(int(r.getX()*sx),int(r.getY()*sy),int(r.getWidth()*sx),int(r.getHeight()*sy));}
    waveform->setBounds(int(194*sx),int(179*sy),int(1081*sx),int(147*sy));
    leftMeter->setBounds(int(38*sx),int(680*sy),int(98*sx),int(11*sy));
    rightMeter->setBounds(int(1540*sx),int(391*sy),int(96*sx),int(288*sy));
    cpuGauge->setBounds(int(14*sx),int(741*sy),int(155*sx),int(160*sy));
    for(int k=0;k<steam::kModuleCount;++k)
        moduleMeters[k]->setBounds(int((164+184.f*k+151)*sx),int(444*sy),int(15*sx),int(96*sy));
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
    g.setColour(ivory.withAlpha(.8f));g.setFont(juce::Font("Georgia",11.f,juce::Font::plain));
    g.drawText("BREATH yellow  PLOSIVE red  CLICK violet  SIBILANCE blue  RIDER green  WHISPER orange",8,5,getWidth()-16,17,juce::Justification::centredLeft);
}
void SteamFilterAudioProcessorEditor::ModuleMeter::timerCallback() {
    const auto data=owner.engine.readStats();
    const float target=data.reductionDb[index];
    displayed+=(target-displayed)*.28f;
    if(isShowing())repaint();
}
void SteamFilterAudioProcessorEditor::ModuleMeter::paint(juce::Graphics& g) {
    const auto b=getLocalBounds().toFloat();
    g.setColour(dark);g.fillRoundedRectangle(b,3);
    g.setColour(brass);g.drawRoundedRectangle(b.reduced(.5f),3,1);
    // Attenuation: amber. Added gain: mint. SteamPolish: actual activity.
    const auto magnitude=juce::jlimit(0.f,1.f,std::abs(displayed)/18.f);
    const auto h=(b.getHeight()-6)*magnitude;
    g.setColour(displayed>=0?green:amber);
    g.fillRoundedRectangle({3.f,b.getHeight()-3-h,b.getWidth()-6,h},2);
}
void SteamFilterAudioProcessorEditor::SideMeter::timerCallback() {
    auto s=owner.engine.readStats();
    float target=isInput?s.inputPeak:s.outputPeak;
    displayed=juce::jmax(target,displayed*.82f);
    if(isShowing())repaint();
}
void SteamFilterAudioProcessorEditor::SideMeter::paint(juce::Graphics& g) {
    auto b=getLocalBounds().toFloat();g.setColour(dark);g.fillRoundedRectangle(b,4);
    const float db=steam::gainToDb(displayed);
    const float fraction=juce::jlimit(0.f,1.f,(db+60.f)/60.f);
    const auto light=juce::Colour(0xfffaa02b);
    if(b.getHeight()>b.getWidth()) {
        const float h=(b.getHeight()-7)*fraction;
        g.setGradientFill(juce::ColourGradient(green,0,b.getBottom(),light,0,b.getY(),false));
        g.fillRoundedRectangle({8.f,b.getBottom()-h-3,b.getWidth()-16,h},4);
    } else {
        g.setColour(light);g.fillRoundedRectangle({3.f,3.f,(b.getWidth()-6)*fraction,b.getHeight()-6},3);
    }
    g.setColour(brass);g.drawRoundedRectangle(b.reduced(1),4,2.f);
}
void SteamFilterAudioProcessorEditor::PressureGauge::timerCallback(){
    displayed+=(owner.dspLoad.load(std::memory_order_relaxed)-displayed)*.2f;
    if(isShowing())repaint();
}
void SteamFilterAudioProcessorEditor::PressureGauge::paint(juce::Graphics& g) {
    auto b=getLocalBounds().toFloat().reduced(5);
    g.setColour(juce::Colour(0xffa47a38));g.fillEllipse(b);
    g.setColour(ivory);g.fillEllipse(b.reduced(6));
    g.setColour(ink);g.drawEllipse(b.reduced(6),2);
    for(int k=0;k<=10;++k){
        const float angle=pi*.75f+float(k)/10.f*pi*1.5f;
        const auto center=b.getCentre();const float r=b.getWidth()*.36f;
        g.setColour(k>7?juce::Colours::darkred:ink);
        g.drawLine(center.x+std::cos(angle)*r*.78f,center.y+std::sin(angle)*r*.78f,
                   center.x+std::cos(angle)*r,center.y+std::sin(angle)*r,1.5f);
    }
    const auto center=b.getCentre();const float angle=pi*.75f+displayed*pi*1.5f;
    g.setColour(juce::Colours::darkred);
    g.drawLine(center.x,center.y,center.x+std::cos(angle)*b.getWidth()*.34f,
               center.y+std::sin(angle)*b.getWidth()*.34f,3.f);
    letters(g,"DSP LOAD",b.getX()+20,b.getY()+b.getHeight()*.67f,b.getWidth()-40,21,13,ink,true);
    letters(g,juce::String(displayed*100.f,1)+"%",b.getX()+20,b.getY()+b.getHeight()*.78f,b.getWidth()-40,21,12,ink,true);
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
