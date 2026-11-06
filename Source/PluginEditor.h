#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

// Knob estilo baquelite serrilhado
class RdpKnobLookAndFeel : public juce::LookAndFeel_V4
{
public:
    void drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h,
                           float pos, float startAngle, float endAngle, juce::Slider&) override;
};

class RdpAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit RdpAudioProcessorEditor (RdpAudioProcessor&);
    ~RdpAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void setupKnob (juce::Slider& s, double doubleClickValue);

    RdpAudioProcessor& audioProcessor;
    RdpKnobLookAndFeel lookAndFeel;
    juce::Slider volumeKnob, effectKnob, noiseKnob;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> volumeAtt, effectAtt, noiseAtt;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RdpAudioProcessorEditor)
};
