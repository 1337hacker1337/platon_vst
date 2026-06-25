#include "PluginEditor.h"

PlatonLNF::PlatonLNF()
{
    setColour (juce::Slider::textBoxTextColourId, juce::Colour(0xffff2e88));
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::ComboBox::backgroundColourId, juce::Colour(0xff000000));
    setColour (juce::ComboBox::textColourId, juce::Colour(0xffadff2f));
    setColour (juce::ComboBox::outlineColourId, juce::Colour(0xff3a2a4a));
    setColour (juce::PopupMenu::backgroundColourId, juce::Colour(0xff16101e));
    setColour (juce::PopupMenu::textColourId, juce::Colour(0xffece8e1));
    setColour (juce::PopupMenu::highlightedBackgroundColourId, juce::Colour(0xffff2e88));
}
void PlatonLNF::drawRotarySlider (juce::Graphics& g,int x,int y,int w,int h,float pos,float a0,float a1,juce::Slider&)
{
    auto b=juce::Rectangle<float>(x,y,w,h).reduced(7.0f);
    auto cx=b.getCentreX(),cy=b.getCentreY(),r=juce::jmin(b.getWidth(),b.getHeight())*0.5f;
    auto ang=a0+pos*(a1-a0);
    juce::ColourGradient grad(juce::Colour(0xff4a2a60),cx-r,cy-r,juce::Colour(0xff140e1c),cx+r,cy+r,true);
    g.setGradientFill(grad); g.fillEllipse(cx-r,cy-r,r*2,r*2);
    g.setColour(juce::Colour(0xff5a3a72)); g.drawEllipse(cx-r,cy-r,r*2,r*2,1.5f);
    juce::Path arc; arc.addCentredArc(cx,cy,r-2,r-2,0,a0,ang,true);
    g.setColour(juce::Colour(0xffadff2f)); g.strokePath(arc,juce::PathStrokeType(2.5f));
    juce::Path pt; pt.addRoundedRectangle(-1.5f,-r+4,3.0f,r*0.55f,1.5f);
    g.setColour(juce::Colour(0xffadff2f)); g.fillPath(pt,juce::AffineTransform::rotation(ang).translated(cx,cy));
}

PlatonAudioProcessorEditor::PlatonAudioProcessorEditor (PlatonAudioProcessor& p)
    : juce::AudioProcessorEditor(&p), proc(p)
{
    setLookAndFeel(&lnf);
    specs={
        {"tune","TUNE"},{"fine","FINE"},{"shape","SHAPE"},{"glide","GLIDE"},{"uni","UNISON"},{"detune","DETUNE"},
        {"o2lvl","OSC2"},{"o2semi","SEMI"},{"fm","FM"},{"fmenv","FM ENV"},{"noise","NOISE"},
        {"penv","P.ENV"},{"ptime","P.TIME"},
        {"atk","ATTACK"},{"dec","DECAY"},{"sus","SUSTAIN"},{"rel","RELEASE"},{"punch","PUNCH"},{"click","CLICK"},
        {"drive","DRIVE"},{"dmix","MIX"},{"bias","BIAS"},{"crush","CRUSH"},{"focus","FOCUS"},
        {"hpf","PRE HPF"},
        {"cutoff","CUTOFF"},{"reso","RESO"},{"fenvAmt","F.ENV"},{"ftime","F.TIME"},
        {"sub","SUB"},{"lrate","LFO RT"},{"ldepth","LFO DP"},
        {"chorus","CHORUS"},{"comp","GLUE"},
        {"width","WIDTH"},{"clip","CLIP"},{"out","VOLUME"}
    };
    for (auto& s:specs)
    {
        auto* sl=new juce::Slider(juce::Slider::RotaryHorizontalVerticalDrag,juce::Slider::TextBoxBelow);
        sl->setTextBoxStyle(juce::Slider::TextBoxBelow,false,58,15);
        addAndMakeVisible(sl); sliders.add(sl);
        sAtt.add(new APVTS::SliderAttachment(proc.apvts,s.id,*sl));
        auto* lb=new juce::Label({},s.name); lb->setJustificationType(juce::Justification::centred);
        lb->setColour(juce::Label::textColourId,juce::Colour(0xffece8e1)); lb->setFont(juce::Font(10.0f,juce::Font::bold));
        addAndMakeVisible(lb); labels.add(lb);
    }
    auto addCombo=[&](const juce::String& id,const juce::String& nm)
    {
        auto* cb=new juce::ComboBox(); addAndMakeVisible(cb); combos.add(cb);
        cAtt.add(new APVTS::ComboBoxAttachment(proc.apvts,id,*cb));
        auto* lb=new juce::Label({},nm); lb->setColour(juce::Label::textColourId,juce::Colour(0xffadff2f));
        lb->setFont(juce::Font(10.0f,juce::Font::bold)); lb->setJustificationType(juce::Justification::centredLeft);
        addAndMakeVisible(lb); comboLabels.add(lb);
    };
    addCombo("dist","DIST"); addCombo("fmode","FILTER"); addCombo("lfoDest","LFO"); addCombo("subOct","SUB OCT");
    setSize(770,610);
}
PlatonAudioProcessorEditor::~PlatonAudioProcessorEditor(){ setLookAndFeel(nullptr); }

void PlatonAudioProcessorEditor::paint (juce::Graphics& g)
{
    juce::ColourGradient bg(juce::Colour(0xff16101e),0,0,juce::Colour(0xff0b0a0c),0,(float)getHeight(),false);
    g.setGradientFill(bg); g.fillAll();
    g.setColour(juce::Colour(0xffadff2f)); g.setFont(juce::Font(34.0f,juce::Font::bold|juce::Font::italic));
    g.drawText("PLATON ///",18,10,400,40,juce::Justification::left);
    g.setColour(juce::Colour(0xffff2e88)); g.setFont(juce::Font(11.0f,juce::Font::bold));
    g.drawText("RAGE 808 BASS  -  v1",20,46,400,16,juce::Justification::left);
}

void PlatonAudioProcessorEditor::resized()
{
    auto area=getLocalBounds().reduced(14); area.removeFromTop(56);
    auto comboRow=area.removeFromTop(28);
    int cw=comboRow.getWidth()/4;
    for (int i=0;i<combos.size();++i){
        auto cell=comboRow.removeFromLeft(cw).reduced(3,0);
        comboLabels[i]->setBounds(cell.removeFromLeft(54));
        combos[i]->setBounds(cell);
    }
    area.removeFromTop(8);
    const int cols=7, rows=6;
    const int gw=area.getWidth()/cols, gh=area.getHeight()/rows;
    for (int i=0;i<sliders.size();++i){
        int c=i%cols,r=i/cols;
        juce::Rectangle<int> cell(area.getX()+c*gw,area.getY()+r*gh,gw,gh);
        labels[i]->setBounds(cell.removeFromTop(14));
        sliders[i]->setBounds(cell.reduced(3));
    }
}
