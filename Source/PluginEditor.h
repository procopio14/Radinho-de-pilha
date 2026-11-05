#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

// Knob de baquelite
class RadioKnobLookAndFeel : public juce::LookAndFeel_V4
{
public:
    void drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h,
                           float pos, float startAngle, float endAngle, juce::Slider&) override;
};

class RadinhoAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit RadinhoAudioProcessorEditor (RadinhoAudioProcessor&);
    ~RadinhoAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    RadinhoAudioProcessor& processor;
    RadioKnobLookAndFeel lookAndFeel;
    juce::Slider knob;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RadinhoAudioProcessorEditor)
};
