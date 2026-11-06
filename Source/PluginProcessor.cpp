#include "PluginProcessor.h"
#include "PluginEditor.h"

static juce::String percentText (float v, int) { return juce::String (juce::roundToInt (v * 100.f)) + " %"; }
static juce::String dbText      (float v, int) { return juce::String (v, 1) + " dB"; }

static juce::AudioProcessorValueTreeState::ParameterLayout createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "volume", 1 }, "Volume",
        juce::NormalisableRange<float> (-24.0f, 12.0f, 0.1f), 0.0f,
        juce::AudioParameterFloatAttributes().withStringFromValueFunction (dbText)));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "effect", 1 }, "Efeito",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.5f,
        juce::AudioParameterFloatAttributes().withStringFromValueFunction (percentText)));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "noise", 1 }, "Ruido",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.0f,
        juce::AudioParameterFloatAttributes().withStringFromValueFunction (percentText)));

    return layout;
}

RdpAudioProcessor::RdpAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", createLayout())
{
    effectParam = apvts.getRawParameterValue ("effect");
    noiseParam  = apvts.getRawParameterValue ("noise");
    volumeParam = apvts.getRawParameterValue ("volume");
}

bool RdpAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;
    return out == layouts.getMainInputChannelSet();
}

void RdpAudioProcessor::prepareToPlay (double sampleRate, int)
{
    sr = sampleRate;

    effectSm.reset (sr, 0.05);
    noiseSm.reset  (sr, 0.05);
    volSm.reset    (sr, 0.03);
    effectSm.setCurrentAndTargetValue (effectParam->load());
    noiseSm.setCurrentAndTargetValue  (noiseParam->load());
    volSm.setCurrentAndTargetValue    (juce::Decibels::decibelsToGain (volumeParam->load()));

    hp1.reset(); hp2.reset(); lp1.reset(); lp2.reset(); peak.reset();

    const double twoPi = juce::MathConstants<double>::twoPi;
    lpCoef = (float) (1.0 - std::exp (-twoPi * 3200.0 / sr));   // ruido: teto ~3.2 kHz
    hpCoef = (float) (1.0 - std::exp (-twoPi * 250.0 / sr));    // ruido: sem grave
    crackleDecay = (float) std::pow (0.97, 44100.0 / sr);
    lpState = hpState = crackle = 0.f;

    driftCur = driftTarget = 1.f;
    driftCoef = (float) (1.0 - std::exp (-1.0 / (sr * 0.08)));  // oscilacao lenta do ruido
    driftCounter = 0;

    updateFilters (effectSm.getCurrentValue());
    coefCounter = 0;
}

// ============ SOM DE RADINHO (knob EFEITO) - mexa nestes numeros ============
void RdpAudioProcessor::updateFilters (float a)
{
    const double hpFreq = 20.0    * std::pow (20.0,  (double) a);   // 20 Hz  -> 400 Hz
    const double lpFreq = 20000.0 * std::pow (0.175, (double) a);   // 20 kHz -> 3.5 kHz

    hp1.setHighPass (sr, hpFreq, 0.7071);
    hp2.setHighPass (sr, hpFreq, 0.7071);
    lp1.setLowPass  (sr, lpFreq, 0.7071);
    lp2.setLowPass  (sr, lpFreq, 0.7071);
    peak.setPeak    (sr, 1500.0, 1.1, 6.0 * a);                     // "nasal" de caixinha

    drive = 1.0f + 3.0f * a;                                        // saturacao suave
}

// ============ RUIDO / DESSINTONIA (knob RUIDO) ============
float RdpAudioProcessor::radioNoise (float n)
{
    if (n < 0.0001f) return 0.f;

    // intensidade do ruido "flutua" devagar, como estacao mal sintonizada
    if (--driftCounter <= 0)
    {
        driftCounter = (int) (sr * (0.12 + 0.25 * rng.nextFloat()));
        driftTarget  = 0.35f + 0.9f * rng.nextFloat();
    }
    driftCur += driftCoef * (driftTarget - driftCur);

    // chiado limitado a faixa de radio (250 Hz - 3.2 kHz), nada de "ar" nos agudos
    const float white = rng.nextFloat() * 2.f - 1.f;
    lpState += lpCoef * (white - lpState);
    hpState += hpCoef * (lpState - hpState);
    const float hiss = lpState - hpState;

    // estalos raros (ate ~5 por segundo)
    if (rng.nextFloat() < (5.f * n) / (float) sr)
        crackle = (rng.nextFloat() * 2.f - 1.f) * 0.6f;
    const float c = crackle;
    crackle *= crackleDecay;
    if (std::abs (crackle) < 1.0e-6f) crackle = 0.f;

    const float level = n * std::sqrt (n);                          // curva suave: comeca baixinho
    return (hiss * 0.10f + c * 0.12f) * level * driftCur;
}

void RdpAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int numCh = juce::jmin (buffer.getNumChannels(), 2);
    const int n = buffer.getNumSamples();
    if (numCh == 0) return;

    effectSm.setTargetValue (effectParam->load());
    noiseSm.setTargetValue  (noiseParam->load());
    volSm.setTargetValue    (juce::Decibels::decibelsToGain (volumeParam->load()));

    float* data[2] = { buffer.getWritePointer (0), numCh > 1 ? buffer.getWritePointer (1) : nullptr };

    for (int i = 0; i < n; ++i)
    {
        const float a   = effectSm.getNextValue();
        const float nz  = noiseSm.getNextValue();
        const float vol = volSm.getNextValue();

        if (coefCounter == 0) { updateFilters (a); coefCounter = 32; }
        --coefCounter;

        const float monoMix = juce::jmin (1.f, a * 2.f);   // radio antigo e mono
        const float wet     = juce::jmin (1.f, a * 5.f);   // efeito em 0 = bypass perfeito

        float in[2] = { data[0][i], numCh > 1 ? data[1][i] : 0.f };
        const float mono  = numCh > 1 ? 0.5f * (in[0] + in[1]) : in[0];
        const float noise = radioNoise (nz);

        for (int ch = 0; ch < numCh; ++ch)
        {
            float x = in[ch] + (mono - in[ch]) * monoMix;

            float y = hp1.process (ch, x);
            y = hp2.process (ch, y);
            y = lp1.process (ch, y);
            y = lp2.process (ch, y);
            y = peak.process (ch, y);

            // saturacao com ganho unitario p/ sinal baixo (nao "infla" o volume)
            y = std::tanh (drive * y) / drive;

            data[ch][i] = (in[ch] * (1.f - wet) + y * wet + noise) * vol;
        }
    }
}

void RdpAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void RdpAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessorEditor* RdpAudioProcessor::createEditor()
{
    return new RdpAudioProcessorEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new RdpAudioProcessor();
}
