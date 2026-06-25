#include "PluginProcessor.h"
#include "PluginEditor.h"

using APVTS = juce::AudioProcessorValueTreeState;
using FloatParam  = juce::AudioParameterFloat;
using ChoiceParam = juce::AudioParameterChoice;

static juce::NormalisableRange<float> rng (float lo, float hi, float step = 0.0f, float skewCentre = 0.0f)
{
    juce::NormalisableRange<float> r (lo, hi);
    if (step > 0.0f) r.interval = step;
    if (skewCentre > 0.0f) r.setSkewForCentre (skewCentre);
    return r;
}

PlatonAudioProcessor::PlatonAudioProcessor()
    : juce::AudioProcessor (BusesProperties()
        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)) {}

APVTS::ParameterLayout PlatonAudioProcessor::createLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;
    auto add = [&p](const char* id, const char* nm, juce::NormalisableRange<float> r, float def)
    { p.push_back (std::make_unique<FloatParam>(id, nm, r, def)); };

    add("tune","Tune",rng(-24,24,1),0);          add("fine","Fine",rng(-50,50,1),0);
    add("shape","Shape",rng(0,1,0.01f),0);        add("glide","Glide",rng(0,0.4f,0.001f),0.06f);
    add("uni","Unison",rng(1,7,1),1);             add("detune","Detune",rng(0,50,1),18);

    add("o2lvl","OSC2 Level",rng(0,1,0.01f),0);   add("o2semi","OSC2 Semi",rng(-24,24,1),7);
    add("fm","FM",rng(0,1,0.01f),0);              add("fmenv","FM Env",rng(0,1,0.01f),0);
    add("noise","Noise",rng(0,1,0.01f),0);

    add("penv","Pitch Env",rng(0,36,1),18);       add("ptime","Pitch Time",rng(0.005f,0.4f,0.001f,0.08f),0.06f);

    add("atk","Attack",rng(0.0005f,0.2f,0.0005f,0.04f),0.002f);  add("dec","Decay",rng(0.01f,2,0.01f,0.3f),0.25f);
    add("sus","Sustain",rng(0,1,0.01f),0.9f);     add("rel","Release",rng(0.01f,1.5f,0.01f,0.3f),0.18f);
    add("punch","Punch",rng(0,1,0.01f),0.3f);     add("click","Click",rng(0,1,0.01f),0);

    add("drive","Drive",rng(1,40,0.5f),6);        add("dmix","Dist Mix",rng(0,1,0.01f),0.85f);
    add("bias","Bias",rng(0,1,0.01f),0);          add("crush","Crush",rng(0,1,0.01f),0);
    add("focus","Focus",rng(-1,1,0.01f),0);
    p.push_back (std::make_unique<ChoiceParam>("dist","Dist Type",
        juce::StringArray{"Tanh","Soft Clip","Hard Clip","Foldback","Diode/Tube","Bitcrush"},0));

    add("hpf","Pre HPF",rng(20,1200,5,0.3f),120);
    add("cutoff","Cutoff",rng(60,14000,20,0.3f),2600);  add("reso","Reso",rng(0,1,0.01f),0.1f);
    add("fenvAmt","Filter Env",rng(0,5,0.1f),0);  add("ftime","Filter Time",rng(0.01f,0.6f,0.01f,0.15f),0.12f);
    p.push_back (std::make_unique<ChoiceParam>("fmode","Filter Mode",juce::StringArray{"Low Pass","Band Pass","High Pass"},0));

    add("sub","Sub",rng(0,1.2f,0.01f),0.5f);
    p.push_back (std::make_unique<ChoiceParam>("subOct","Sub Oct",juce::StringArray{"Same","-1 Oct"},0));

    add("lrate","LFO Rate",rng(0.05f,20,0.05f,2.0f),5);  add("ldepth","LFO Depth",rng(0,1,0.01f),0);
    p.push_back (std::make_unique<ChoiceParam>("lfoDest","LFO Dest",juce::StringArray{"Off","Pitch","Filter","Volume","Drive"},0));

    add("chorus","Chorus",rng(0,1,0.01f),0);      add("comp","Glue",rng(0,1,0.01f),0);
    add("width","Width",rng(0,1,0.01f),0);        add("clip","Clip",rng(0,1,0.01f),0);
    add("out","Volume",rng(0,1.2f,0.01f),0.8f);

    return { p.begin(), p.end() };
}

void PlatonAudioProcessor::prepareToPlay (double sampleRate, int)
{
    sr = sampleRate;
    active = false; stage = 0; amp = 0;
    for (auto& x : phases) x = 0; for (auto& x : fbk) x = 0;
    for (int i=0;i<2;++i){ hp[i]=lastIn[i]=ic1[i]=ic2[i]=hold[i]=flp[i]=0; }
    o2phase = subPhase = 0.0f;
    fmEnvCoeff = std::exp (-1.0f / (0.18f * (float) sr));
    chBufL.assign (2048, 0.0f); chBufR.assign (2048, 0.0f); chPos = 0; chPhase = 0;
    compEnv = 0;
}

bool PlatonAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    auto o = layouts.getMainOutputChannelSet();
    return o == juce::AudioChannelSet::mono() || o == juce::AudioChannelSet::stereo();
}

void PlatonAudioProcessor::noteOn (int n)
{
    curNote = n;
    target = (float) juce::MidiMessage::getMidiNoteInHertz (n);
    const float glide = apvts.getRawParameterValue("glide")->load();
    if (! active || glide < 0.001f) freq = target;
    active = true; stage = 1; pe = 1; fenv = 1; fmEnv = 1; punchEnv = 1; clickEnv = 1;
    const float t  = juce::jmax (0.001f, apvts.getRawParameterValue("ptime")->load());
    peCoeff   = std::exp (-1.0f / (t * (float) sr / 4.0f));
    const float ft = juce::jmax (0.001f, apvts.getRawParameterValue("ftime")->load());
    fenvCoeff = std::exp (-1.0f / (ft * (float) sr / 4.0f));
    for (int j=0;j<7;++j) phases[j] = (float) j / 7.0f;
}

void PlatonAudioProcessor::noteOff (int n) { if (active && n == curNote) stage = 4; }

float PlatonAudioProcessor::wave (float p, float shape) const
{
    const float sine = std::sin (juce::MathConstants<float>::twoPi * p);
    const float tri  = 1.0f - 4.0f * std::abs (p - 0.5f);
    const float saw  = 2.0f * p - 1.0f;
    const float sq   = p < 0.5f ? 1.0f : -1.0f;
    if (shape < 0.333f) { float t = shape/0.333f;        return sine*(1-t)+tri*t; }
    if (shape < 0.666f) { float t = (shape-0.333f)/0.333f; return tri*(1-t)+saw*t; }
    float t = (shape-0.666f)/0.334f; return saw*(1-t)+sq*t;
}

float PlatonAudioProcessor::distort (float x, int type, float bits) const
{
    switch (type)
    {
        case 0: return std::tanh (x);
        case 1: return x / (1.0f + std::abs (x));
        case 2: return juce::jlimit (-1.0f, 1.0f, x);
        case 3: { float y=x; while (y>1) y=2-y; while (y<-1) y=-2-y; return y; }
        case 4: return x >= 0 ? std::tanh (1.3f*x) : std::tanh (0.7f*x);
        case 5: { float lv = std::pow (2.0f, bits); return std::round (juce::jlimit(-1.0f,1.0f,x)*lv)/lv; }
    }
    return x;
}

void PlatonAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();

    auto gp = [this](const char* id){ return apvts.getRawParameterValue(id)->load(); };
    const float p_tune=gp("tune"),p_fine=gp("fine"),p_shape=gp("shape"),p_glide=gp("glide");
    const int   p_uni=(int)gp("uni"); const float p_detune=gp("detune");
    const float p_o2lvl=gp("o2lvl"),p_o2semi=gp("o2semi"),p_fm=gp("fm"),p_fmenv=gp("fmenv"),p_noise=gp("noise");
    const float p_penv=gp("penv");
    const float p_atk=juce::jmax(0.0005f,gp("atk")),p_dec=juce::jmax(0.001f,gp("dec")),p_sus=gp("sus"),p_rel=juce::jmax(0.001f,gp("rel"));
    const float p_punch=gp("punch"),p_click=gp("click");
    const float p_drive=gp("drive"),p_dmix=gp("dmix"),p_bias=gp("bias"),p_crush=gp("crush"),p_focus=gp("focus");
    const int   p_dist=(int)gp("dist");
    const float p_hpf=gp("hpf"),p_cutoff=gp("cutoff"),p_reso=gp("reso"),p_fenvAmt=gp("fenvAmt");
    const int   p_fmode=(int)gp("fmode");
    const float p_sub=gp("sub"); const int p_subOct=(int)gp("subOct");
    const float p_lrate=gp("lrate"),p_ldepth=gp("ldepth"); const int p_lfoDest=(int)gp("lfoDest");
    const float p_chorus=gp("chorus"),p_comp=gp("comp"),p_width=gp("width"),p_clip=gp("clip"),p_out=gp("out");

    const int   uni = juce::jlimit (1,7,p_uni);
    const float tuneMul = std::pow (2.0f,(p_tune+p_fine/100.0f)/12.0f);
    const float glideC = p_glide<0.001f?0.0f:std::exp(-1.0f/(p_glide*(float)sr));
    const float aC=1-std::exp(-1.0f/(p_atk*(float)sr)), dC=1-std::exp(-1.0f/(p_dec*(float)sr)), rC=1-std::exp(-1.0f/(p_rel*(float)sr));
    const float hpC=std::exp(-2.0f*juce::MathConstants<float>::pi*juce::jmin(0.45f,p_hpf/(float)sr));
    const float focusLpC=1-std::exp(-2.0f*juce::MathConstants<float>::pi*juce::jmin(0.45f,800.0f/(float)sr));
    const float kf=1.0f/(0.6f+p_reso*12.0f);
    const int   crushStep=1+(int)std::round(p_crush*30.0f);
    const float punchC=std::exp(-1.0f/(0.03f*(float)sr)), clickC=std::exp(-1.0f/(0.008f*(float)sr));
    const float norm=1.0f/std::sqrt((float)uni);
    const float twoPi=juce::MathConstants<float>::twoPi;
    auto& rnd = juce::Random::getSystemRandom();

    auto* outL = buffer.getWritePointer(0);
    auto* outR = buffer.getNumChannels()>1 ? buffer.getWritePointer(1) : nullptr;
    const int N = buffer.getNumSamples();
    auto it = midi.begin();

    for (int i=0;i<N;++i)
    {
        while (it != midi.end() && (*it).samplePosition <= i)
        {
            auto m=(*it).getMessage();
            if (m.isNoteOn()) noteOn(m.getNoteNumber());
            else if (m.isNoteOff()) noteOff(m.getNoteNumber());
            ++it;
        }

        float sL=0,sR=0;
        if (active)
        {
            if (glideC>0 && true) freq = target + (freq-target)*glideC; else freq = target;
            pe *= peCoeff; fmEnv *= fmEnvCoeff;

            lfoPhase += p_lrate/(float)sr; if (lfoPhase>=1) lfoPhase-=1;
            const float lfo=std::sin(twoPi*lfoPhase);
            float pitchLfo=0,filtLfo=0,volLfo=1,driveLfo=1;
            if      (p_lfoDest==1) pitchLfo=lfo*p_ldepth*7;
            else if (p_lfoDest==2) filtLfo=lfo*p_ldepth*4;
            else if (p_lfoDest==3) volLfo=1-p_ldepth*0.5f*(0.5f-0.5f*lfo);
            else if (p_lfoDest==4) driveLfo=1+lfo*p_ldepth*0.8f;

            const float pMul=std::pow(2.0f,(p_penv*pe+pitchLfo)/12.0f);
            const float baseF=freq*tuneMul*pMul;

            const float fmMod=p_fm*(1.0f+p_fmenv*3.0f*fmEnv);
            float o2val=0;
            if (p_o2lvl>0 || fmMod>0)
            {
                const float o2f=baseF*std::pow(2.0f,p_o2semi/12.0f);
                o2phase+=o2f/(float)sr; if (o2phase>=1) o2phase-=1;
                o2val=wave(o2phase,p_shape);
            }
            float mL=0,mR=0;
            for (int j=0;j<uni;++j)
            {
                const float det = uni>1 ? ((float)j/(uni-1)-0.5f)*2.0f*p_detune : 0.0f;
                const float fv = baseF*std::pow(2.0f,det/1200.0f);
                float ph=phases[j]; ph+=fv/(float)sr; if (ph>=1) ph-=1; phases[j]=ph;
                float pm=ph+fmMod*0.8f*o2val+fmMod*0.5f*fbk[j]; pm-=std::floor(pm);
                const float w=wave(pm,p_shape); fbk[j]=w;
                float gl=0.707f,gr=0.707f;
                if (uni>1){ float pan=((float)j/(uni-1)-0.5f)*2.0f*p_width; float a=(pan+1)*0.25f*juce::MathConstants<float>::pi; gl=std::cos(a); gr=std::sin(a); }
                mL+=w*gl; mR+=w*gr;
            }
            mL*=norm; mR*=norm;
            mL+=o2val*p_o2lvl; mR+=o2val*p_o2lvl;
            if (p_noise>0){ float nz=(rnd.nextFloat()*2-1)*p_noise*0.6f; mL+=nz; mR+=nz; }
            if (clickEnv>0.0001f){ float nz=(rnd.nextFloat()*2-1)*clickEnv*p_click; mL+=nz; mR+=nz; clickEnv*=clickC; }

            float subF=freq*tuneMul; if (p_subOct==1) subF*=0.5f;
            subPhase+=subF/(float)sr; if (subPhase>=1) subPhase-=1;
            const float sub=std::sin(twoPi*subPhase)*p_sub;

            bool doSample=false;
            if (crushStep>1){ if (ccount<=0){doSample=true;ccount=crushStep;} ccount--; }

            fenv*=fenvCoeff;
            const float cut=juce::jlimit(20.0f,(float)sr*0.45f,p_cutoff*std::pow(2.0f,p_fenvAmt*fenv+filtLfo));
            const float g=std::tan(juce::MathConstants<float>::pi*cut/(float)sr);
            const float a1=1.0f/(1.0f+g*(g+kf)),a2=g*a1,a3=g*a2;
            const float driveEff=p_drive*driveLfo;
            const float bits=juce::jmax(2.0f,12.0f-driveEff*0.25f);

            const float in[2]={mL,mR}; float out[2]={0,0};
            for (int ch=0;ch<2;++ch)
            {
                float x=in[ch];
                flp[ch]+=focusLpC*(x-flp[ch]);
                const float high=x-flp[ch];
                x=x+p_focus*high*1.5f;
                hp[ch]=hpC*(hp[ch]+x-lastIn[ch]); lastIn[ch]=x;
                float driven=hp[ch]*driveEff, wet;
                if (p_bias>0){ float b=p_bias*0.7f; wet=distort(driven+b,p_dist,bits)-distort(b,p_dist,bits); }
                else wet=distort(driven,p_dist,bits);
                float chx=(1-p_dmix)*x+p_dmix*wet;
                if (crushStep>1){ if (doSample) hold[ch]=chx; chx=hold[ch]; }
                const float v3=chx-ic2[ch];
                const float v1=a1*ic1[ch]+a2*v3;
                const float v2=ic2[ch]+a2*ic1[ch]+a3*v3;
                ic1[ch]=2*v1-ic1[ch]; ic2[ch]=2*v2-ic2[ch];
                float filt; if (p_fmode==0) filt=v2; else if (p_fmode==1) filt=v1; else filt=chx-kf*v1-v2;
                out[ch]=filt;
            }

            if (p_chorus>0.001f)
            {
                const float baseS=0.012f*(float)sr, depthS=0.004f*(float)sr*p_chorus;
                chPhase+=0.7f/(float)sr; if (chPhase>=1) chPhase-=1;
                const float s2=twoPi*chPhase, mA=0.5f+0.5f*std::sin(s2), mB=0.5f+0.5f*std::sin(s2+juce::MathConstants<float>::pi);
                chBufL[chPos]=out[0]; chBufR[chPos]=out[1];
                float rL=chPos-(baseS+depthS*mA); while (rL<0) rL+=2048;
                float rR=chPos-(baseS+depthS*mB); while (rR<0) rR+=2048;
                int iL=(int)std::floor(rL); float fL=rL-iL; float wL=chBufL[iL%2048]*(1-fL)+chBufL[(iL+1)%2048]*fL;
                int iR=(int)std::floor(rR); float fR=rR-iR; float wR=chBufR[iR%2048]*(1-fR)+chBufR[(iR+1)%2048]*fR;
                out[0]=(1-0.5f*p_chorus)*out[0]+p_chorus*wL;
                out[1]=(1-0.5f*p_chorus)*out[1]+p_chorus*wR;
                chPos=(chPos+1)%2048;
            }

            if      (stage==1){ amp+=aC*(1-amp); if (amp>=0.999f){amp=1;stage=2;} }
            else if (stage==2){ amp+=dC*(p_sus-amp); if (std::abs(amp-p_sus)<0.002f){amp=p_sus;stage=3;} }
            else if (stage==3){ amp=p_sus; }
            else if (stage==4){ amp+=rC*(0-amp); if (amp<0.0008f){amp=0;active=false;stage=0;} }
            if (punchEnv>0.0001f) punchEnv*=punchC;
            const float ampG=amp*(1+p_punch*punchEnv*2)*volLfo;
            sL=(out[0]+sub)*ampG; sR=(out[1]+sub)*ampG;
        }

        if (p_comp>0.001f)
        {
            const float rect=juce::jmax(std::abs(sL),std::abs(sR));
            const float ca=rect>compEnv?0.02f:0.0008f; compEnv+=ca*(rect-compEnv);
            const float thr=1-p_comp*0.85f; float gg=1;
            if (compEnv>thr && compEnv>0.0001f){ float over=compEnv-thr; gg=(thr+over/4)/compEnv; }
            const float makeup=1+p_comp*1.8f; sL*=gg*makeup; sR*=gg*makeup;
        }
        const float og=p_out*1.4f*(1+p_clip*3);
        sL=std::tanh(sL*og); sR=std::tanh(sR*og);
        outL[i]=sL; if (outR) outR[i]=sR;
    }
}

void PlatonAudioProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    if (auto xml = apvts.copyState().createXml()) copyXmlToBinary (*xml, dest);
}
void PlatonAudioProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessorEditor* PlatonAudioProcessor::createEditor() { return new PlatonAudioProcessorEditor (*this); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new PlatonAudioProcessor(); }
