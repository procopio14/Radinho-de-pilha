#pragma once
#include <JuceHeader.h>

class RadinhoAudioProcessor : public juce::AudioProcessor
{
public:
    RadinhoAudioProcessor();
    ~RadinhoAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Radinho"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;

private:
    // Biquad simples (RBJ cookbook), estado separado por canal
    struct Biquad
    {
        double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0;
        double z1[2] { 0, 0 }, z2[2] { 0, 0 };

        void reset() { z1[0] = z1[1] = z2[0] = z2[1] = 0; }

        void set (double nb0, double nb1, double nb2, double a0, double na1, double na2)
        {
            b0 = nb0 / a0; b1 = nb1 / a0; b2 = nb2 / a0;
            a1 = na1 / a0; a2 = na2 / a0;
        }

        void setHighPass (double sr, double f, double q)
        {
            f = juce::jlimit (5.0, sr * 0.45, f);
            const double w = juce::MathConstants<double>::twoPi * f / sr;
            const double cw = std::cos (w), al = std::sin (w) / (2.0 * q);
            set ((1 + cw) / 2, -(1 + cw), (1 + cw) / 2, 1 + al, -2 * cw, 1 - al);
        }

        void setLowPass (double sr, double f, double q)
        {
            f = juce::jlimit (5.0, sr * 0.45, f);
            const double w = juce::MathConstants<double>::twoPi * f / sr;
            const double cw = std::cos (w), al = std::sin (w) / (2.0 * q);
            set ((1 - cw) / 2, 1 - cw, (1 - cw) / 2, 1 + al, -2 * cw, 1 - al);
        }

        void setPeak (double sr, double f, double q, double gainDb)
        {
            f = juce::jlimit (5.0, sr * 0.45, f);
            const double A = std::pow (10.0, gainDb / 40.0);
            const double w = juce::MathConstants<double>::twoPi * f / sr;
            const double cw = std::cos (w), al = std::sin (w) / (2.0 * q);
            set (1 + al * A, -2 * cw, 1 - al * A, 1 + al / A, -2 * cw, 1 - al / A);
        }

        inline float process (int ch, float x)
        {
            const double in = (double) x;
            const double y = b0 * in + z1[ch];
            z1[ch] = b1 * in - a1 * y + z2[ch];
            z2[ch] = b2 * in - a2 * y;
            return (float) y;
        }
    };

    void updateFilters (float amount);
    float radioNoise (float amount);

    Biquad hp1, hp2, lp1, lp2, peak;
    juce::SmoothedValue<float> amountSmoothed;
    std::atomic<float>* radioParam = nullptr;

    juce::Random rng;
    double sr = 44100.0;
    float lpCoef = 0.f, hpCoef = 0.f, crackleDecay = 0.95f;
    float lpState = 0.f, hpState = 0.f, crackle = 0.f;
    float drive = 1.f, satNorm = 1.f, outGain = 1.f;
    int coefCounter = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RadinhoAudioProcessor)
};
