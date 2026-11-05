#include "PluginEditor.h"

using juce::Colour;

//==============================================================================
void RadioKnobLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h,
                                             float pos, float startAngle, float endAngle, juce::Slider&)
{
    auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (4.f);
    const float r  = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const auto  c  = bounds.getCentre();
    const float kr = r * 0.70f;
    const float angle = startAngle + pos * (endAngle - startAngle);

    // marcacoes ao redor
    g.setColour (Colour (0xffe6d3a8));
    for (int i = 0; i <= 10; ++i)
    {
        const float a = startAngle + (endAngle - startAngle) * (float) i / 10.f;
        const float s = std::sin (a), co = -std::cos (a);
        g.drawLine (c.x + s * r * 0.84f, c.y + co * r * 0.84f,
                    c.x + s * r * 0.98f, c.y + co * r * 0.98f, i % 5 == 0 ? 2.2f : 1.2f);
    }

    // sombra
    g.setColour (juce::Colours::black.withAlpha (0.35f));
    g.fillEllipse (c.x - kr + 2.f, c.y - kr + 4.f, kr * 2.f, kr * 2.f);

    // corpo do knob
    juce::ColourGradient grad (Colour (0xff5a5a5a), c.x - kr * 0.5f, c.y - kr * 0.6f,
                               Colour (0xff121212), c.x + kr * 0.6f, c.y + kr * 0.8f, true);
    g.setGradientFill (grad);
    g.fillEllipse (c.x - kr, c.y - kr, kr * 2.f, kr * 2.f);

    g.setColour (Colour (0xffc9b88c));
    g.drawEllipse (c.x - kr, c.y - kr, kr * 2.f, kr * 2.f, 2.f);

    // ponteiro
    const float s = std::sin (angle), co = -std::cos (angle);
    g.setColour (Colour (0xffe6d3a8));
    g.drawLine (c.x + s * kr * 0.30f, c.y + co * kr * 0.30f,
                c.x + s * kr * 0.85f, c.y + co * kr * 0.85f, 3.5f);
}

//==============================================================================
RadinhoAudioProcessorEditor::RadinhoAudioProcessorEditor (RadinhoAudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    setSize (520, 440);

    knob.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    knob.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    knob.setRotaryParameters (juce::MathConstants<float>::pi * 1.2f,
                              juce::MathConstants<float>::pi * 2.8f, true);
    knob.setLookAndFeel (&lookAndFeel);
    knob.setDoubleClickReturnValue (true, 0.0);
    knob.onValueChange = [this] { repaint(); };
    addAndMakeVisible (knob);

    attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        processor.apvts, "radio", knob);
}

RadinhoAudioProcessorEditor::~RadinhoAudioProcessorEditor()
{
    knob.setLookAndFeel (nullptr);
}

void RadinhoAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::white);

    const float v = (float) knob.getValue();
    const juce::Rectangle<float> body (80.f, 130.f, 360.f, 240.f);

    // sombra no chao
    g.setColour (juce::Colours::black.withAlpha (0.10f));
    g.fillEllipse (body.getX() + 10.f, body.getBottom() - 2.f, body.getWidth() - 20.f, 24.f);

    // antena
    g.setColour (Colour (0xff9a9a9a));
    g.drawLine (body.getRight() - 60.f, body.getY() + 4.f, 470.f, 40.f, 3.f);
    g.fillEllipse (464.f, 34.f, 12.f, 12.f);

    // pes
    g.setColour (Colour (0xff3a1f0e));
    g.fillRoundedRectangle (body.getX() + 30.f,  body.getBottom() - 4.f, 50.f, 14.f, 4.f);
    g.fillRoundedRectangle (body.getRight() - 80.f, body.getBottom() - 4.f, 50.f, 14.f, 4.f);

    // corpo
    juce::ColourGradient wood (Colour (0xffb5683a), 0.f, body.getY(),
                               Colour (0xff6b3720), 0.f, body.getBottom(), false);
    g.setGradientFill (wood);
    g.fillRoundedRectangle (body, 28.f);
    g.setColour (Colour (0xff3d1e0e));
    g.drawRoundedRectangle (body.reduced (1.f), 28.f, 3.f);

    auto hlArea = body.reduced (10.f);
    g.setColour (juce::Colours::white.withAlpha (0.12f));
    g.fillRoundedRectangle (hlArea.removeFromTop (14.f), 8.f);

    // grade do alto-falante
    const juce::Rectangle<float> grille (body.getX() + 20.f, body.getY() + 25.f, 170.f, 190.f);
    g.setColour (Colour (0xffe6d3a8));
    g.fillRoundedRectangle (grille, 14.f);
    g.setColour (Colour (0xff3d1e0e));
    g.drawRoundedRectangle (grille, 14.f, 2.f);
    g.setColour (Colour (0xff5a3a22).withAlpha (0.85f));
    for (float yy = grille.getY() + 18.f; yy < grille.getBottom() - 12.f; yy += 13.f)
        g.fillRoundedRectangle (grille.getX() + 16.f, yy, grille.getWidth() - 32.f, 5.f, 2.5f);

    // visor do dial (acende conforme o efeito aumenta)
    const juce::Rectangle<float> dial (body.getX() + 205.f, body.getY() + 25.f, 135.f, 70.f);
    g.setColour (Colour (0xffefe3bd).interpolatedWith (Colour (0xffffc94d), v * 0.9f));
    g.fillRoundedRectangle (dial, 8.f);
    g.setColour (Colour (0xff3d1e0e));
    g.drawRoundedRectangle (dial, 8.f, 2.5f);

    g.setColour (Colour (0xff5a3a22));
    g.setFont (juce::Font (juce::FontOptions (11.f, juce::Font::bold)));
    g.drawText ("RADINHO", juce::Rectangle<int> ((int) dial.getX(), (int) dial.getY() + 4, (int) dial.getWidth(), 14),
                juce::Justification::centred);

    const auto inner = dial.withTrimmedTop (22.f).reduced (12.f, 8.f);
    for (int i = 0; i <= 20; ++i)
    {
        const float tx = inner.getX() + inner.getWidth() * (float) i / 20.f;
        const float len = (i % 5 == 0) ? 14.f : 8.f;
        g.drawLine (tx, inner.getBottom(), tx, inner.getBottom() - len, 1.2f);
    }
    const float nx = inner.getX() + inner.getWidth() * v;
    g.setColour (Colour (0xffd0301f));
    g.drawLine (nx, inner.getY(), nx, inner.getBottom(), 2.f);

    // rotulo do knob
    g.setColour (Colour (0xffe6d3a8));
    g.setFont (juce::Font (juce::FontOptions (12.f, juce::Font::bold)));
    g.drawText ("RADIO", juce::Rectangle<int> (302, 346, 100, 16), juce::Justification::centred);
}

void RadinhoAudioProcessorEditor::resized()
{
    knob.setBounds (302, 240, 100, 100);
}
