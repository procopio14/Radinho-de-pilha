#include "PluginProcessor.h"
#include "PluginEditor.h"

static juce::AudioProcessorValueTreeState::ParameterLayout createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "radio", 1 }, "Radio",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.5f));
    return layout;
}

RadinhoAudioProcessor::RadinhoAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", createLayout())
{
    radioParam = apvts.getRawParameterValue ("radio");
}

bool RadinhoAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;
    return out == layouts.getMainInputChannelSet();
}

void RadinhoAudioProcessor::prepareToPlay (double sampleRate, int)
{
    sr = sampleRate;
    amountSmoothed.reset (sr, 0.05);
    amountSmoothed.setCurrentAndTargetValue (radioParam->load());

    hp1.reset(); hp2.reset(); lp1.reset(); lp2.reset(); peak.reset();

    const double twoPi = juce::MathConstants<double>::twoPi;
    lpCoef = (float) (1.0 - std::exp (-twoPi * 4000.0 / sr));   // hiss ate ~4 kHz
    hpCoef = (float) (1.0 - std::exp (-twoPi * 300.0 / sr));    // tira grave do ruido
    crackleDecay = (float) std::pow (0.97, 44100.0 / sr);
    lpState = hpState = crackle = 0.f;

    updateFilters (amountSmoothed.getCurrentValue());
    coefCounter = 0;
}

// ============ AQUI ESTA O "SOM DE RADINHO" - mexa nestes numeros ============
void RadinhoAudioProcessor::updateFilters (float a)
{
    const double hpFreq = 20.0    * std::pow (20.0,  (double) a);   // 20 Hz  -> 400 Hz
    const double lpFreq = 20000.0 * std::pow (0.175, (double) a);   // 20 kHz -> 3.5 kHz

    hp1.setHighPass (sr, hpFreq, 0.7071);
    hp2.setHighPass (sr, hpFreq, 0.7071);
    lp1.setLowPass  (sr, lpFreq, 0.7071);
    lp2.setLowPass  (sr, lpFreq, 0.7071);
    peak.setPeak    (sr, 1500.0, 1.1, 7.0 * a);                     // "nasal" de caixinha

    drive   = 1.0f + 5.0f * a;                                      // saturacao
    satNorm = 1.0f / std::tanh (drive);
    outGain = 1.0f + 0.5f * a;                                      // compensa perda de volume
}

float RadinhoAudioProcessor::radioNoise (float a)
{
    const float white = rng.nextFloat() * 2.f - 1.f;
    lpState += lpCoef * (white - lpState);
    hpState += hpCoef * (lpState - hpState);
    const float hiss = lpState - hpState;

    // estalinhos aleatorios (~6 por segundo no maximo)
    if (rng.nextFloat() < (6.f * a) / (float) sr)
        crackle = (rng.nextFloat() * 2.f - 1.f) * 0.6f;
    const float c = crackle;
    crackle *= crackleDecay;
    if (std::abs (crackle) < 1.0e-6f) crackle = 0.f;

    const float a2 = a * a;
    return hiss * 0.10f * a2 + c * 0.15f * a2;
}

void RadinhoAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int numCh = juce::jmin (buffer.getNumChannels(), 2);
    const int n = buffer.getNumSamples();
    if (numCh == 0) return;

    amountSmoothed.setTargetValue (radioParam->load());
    float* data[2] = { buffer.getWritePointer (0), numCh > 1 ? buffer.getWritePointer (1) : nullptr };

    for (int i = 0; i < n; ++i)
    {
        const float a = amountSmoothed.getNextValue();

        if (coefCounter == 0) { updateFilters (a); coefCounter = 32; }
        --coefCounter;

        const float monoMix = juce::jmin (1.f, a * 2.f);   // radio antigo e mono
        const float wet     = juce::jmin (1.f, a * 5.f);   // em 0 = bypass perfeito

        float in[2] = { data[0][i], numCh > 1 ? data[1][i] : 0.f };
        const float mono = numCh > 1 ? 0.5f * (in[0] + in[1]) : in[0];
        const float noise = radioNoise (a);

        for (int ch = 0; ch < numCh; ++ch)
        {
            float x = in[ch] + (mono - in[ch]) * monoMix;

            float y = hp1.process (ch, x);
            y = hp2.process (ch, y);
            y = lp1.process (ch, y);
            y = lp2.process (ch, y);
            y = peak.process (ch, y);

            y = std::tanh (drive * y) * satNorm * outGain;

            data[ch][i] = in[ch] * (1.f - wet) + y * wet + noise;
        }
    }
}

void RadinhoAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void RadinhoAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessorEditor* RadinhoAudioProcessor::createEditor()
{
    return new RadinhoAudioProcessorEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new RadinhoAudioProcessor();
}
