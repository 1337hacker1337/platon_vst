#pragma once
#include <JuceHeader.h>

//==============================================================================
//  PLATON V1 — rage / rest-in-bass 808 synthesizer
//  Mono voice + glide, 808 pitch knock, unison, OSC2 + FM (env + feedback),
//  noise, pre-dist focus tilt + HPF, 6 distortions (+bias/crush),
//  resonant SVF with env, clean mono sub, chorus, glue comp, width, clip.
//==============================================================================
class PlatonAudioProcessor : public juce::AudioProcessor
{
public:
    PlatonAudioProcessor();
    ~PlatonAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Platon V1"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 3.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    juce::AudioProcessorValueTreeState apvts { *this, nullptr, "PARAMS", createLayout() };

private:
    void  noteOn (int midiNote);
    void  noteOff (int midiNote);
    float wave (float ph, float shape) const;
    float distort (float x, int type, float bits) const;

    double sr = 44100.0;

    // mono voice state
    bool   active = false;
    int    curNote = -1;
    float  phases[7] = {}, fbk[7] = {};
    float  o2phase = 0.0f, subPhase = 0.0f;
    float  freq = 55.0f, target = 55.0f;
    float  pe = 0.0f, peCoeff = 0.999f;
    float  fenv = 0.0f, fenvCoeff = 0.999f;
    float  fmEnv = 0.0f, fmEnvCoeff = 0.999f;
    float  amp = 0.0f;
    int    stage = 0;
    float  hp[2] = {}, lastIn[2] = {}, ic1[2] = {}, ic2[2] = {}, hold[2] = {}, flp[2] = {};
    int    ccount = 0;
    float  lfoPhase = 0.0f, punchEnv = 0.0f, clickEnv = 0.0f;
    std::vector<float> chBufL, chBufR;
    int    chPos = 0; float chPhase = 0.0f;
    float  compEnv = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PlatonAudioProcessor)
};
