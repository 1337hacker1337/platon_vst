#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

using APVTS = juce::AudioProcessorValueTreeState;

class PlatonLNF : public juce::LookAndFeel_V4
{
public:
    PlatonLNF();
    void drawRotarySlider (juce::Graphics&, int, int, int, int, float, float, float, juce::Slider&) override;
};

class PlatonAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit PlatonAudioProcessorEditor (PlatonAudioProcessor&);
    ~PlatonAudioProcessorEditor() override;
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    struct Spec { juce::String id, name; };
    PlatonAudioProcessor& proc;
    PlatonLNF lnf;

    juce::OwnedArray<juce::Slider> sliders;
    juce::OwnedArray<juce::Label>  labels;
    juce::OwnedArray<APVTS::SliderAttachment> sAtt;

    juce::OwnedArray<juce::ComboBox> combos;
    juce::OwnedArray<juce::Label>    comboLabels;
    juce::OwnedArray<APVTS::ComboBoxAttachment> cAtt;

    std::vector<Spec> specs;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PlatonAudioProcessorEditor)
};
