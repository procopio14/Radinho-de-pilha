#include "PluginEditor.h"

using juce::Colour;
namespace Colours = juce::Colours;
using juce::Rectangle;

namespace
{
    constexpr float kBodyX = 40.f, kBodyY = 150.f, kBodyW = 480.f, kBodyH = 250.f;
    constexpr float kRightColX = kBodyX + 226.f;   // coluna direita (dial + knobs)
    constexpr int   kKnobSize = 78;
}

//==============================================================================
void RdpKnobLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h,
                                           float pos, float startAngle, float endAngle, juce::Slider&)
{
    auto bounds = Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (2.f);
    const float r  = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const auto  c  = bounds.getCentre();
    const float kr = r * 0.72f;
    const float angle = startAngle + pos * (endAngle - startAngle);

    // marcacoes ao redor
    g.setColour (Colour (0xffeedfb5));
    for (int i = 0; i <= 10; ++i)
    {
        const float a = startAngle + (endAngle - startAngle) * (float) i / 10.f;
        const float s = std::sin (a), co = -std::cos (a);
        g.drawLine (c.x + s * r * 0.86f, c.y + co * r * 0.86f,
                    c.x + s * r * 1.00f, c.y + co * r * 1.00f, i % 5 == 0 ? 2.0f : 1.1f);
    }

    // sombra
    g.setColour (Colours::black.withAlpha (0.40f));
    g.fillEllipse (c.x - kr + 1.5f, c.y - kr + 3.5f, kr * 2.f, kr * 2.f);

    // corpo
    juce::ColourGradient grad (Colour (0xff626262), c.x - kr * 0.5f, c.y - kr * 0.6f,
                               Colour (0xff101010), c.x + kr * 0.6f, c.y + kr * 0.8f, true);
    g.setGradientFill (grad);
    g.fillEllipse (c.x - kr, c.y - kr, kr * 2.f, kr * 2.f);

    // serrilhado da borda
    g.setColour (Colours::white.withAlpha (0.10f));
    for (int i = 0; i < 36; ++i)
    {
        const float a = juce::MathConstants<float>::twoPi * (float) i / 36.f;
        const float s = std::sin (a), co = -std::cos (a);
        g.drawLine (c.x + s * kr * 0.90f, c.y + co * kr * 0.90f,
                    c.x + s * kr * 0.99f, c.y + co * kr * 0.99f, 1.0f);
    }

    // aro metalico
    g.setColour (Colour (0xffc9b88c));
    g.drawEllipse (c.x - kr, c.y - kr, kr * 2.f, kr * 2.f, 1.6f);

    // tampa central
    const float cr = kr * 0.62f;
    juce::ColourGradient cap (Colour (0xff3a3a3a), c.x - cr * 0.4f, c.y - cr * 0.6f,
                              Colour (0xff0c0c0c), c.x + cr * 0.5f, c.y + cr * 0.7f, true);
    g.setGradientFill (cap);
    g.fillEllipse (c.x - cr, c.y - cr, cr * 2.f, cr * 2.f);

    // ponteiro
    const float s = std::sin (angle), co = -std::cos (angle);
    g.setColour (Colour (0xffeedfb5));
    g.drawLine (c.x + s * kr * 0.18f, c.y + co * kr * 0.18f,
                c.x + s * kr * 0.92f, c.y + co * kr * 0.92f, 3.2f);
}

//==============================================================================
void RdpAudioProcessorEditor::setupKnob (juce::Slider& s, double doubleClickValue)
{
    s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    s.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    s.setRotaryParameters (juce::MathConstants<float>::pi * 1.2f,
                           juce::MathConstants<float>::pi * 2.8f, true);
    s.setLookAndFeel (&lookAndFeel);
    s.setDoubleClickReturnValue (true, doubleClickValue);
    s.setPopupDisplayEnabled (true, true, this);
    s.onValueChange = [this] { repaint(); };
    addAndMakeVisible (s);
}

RdpAudioProcessorEditor::RdpAudioProcessorEditor (RdpAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setSize (560, 460);

    setupKnob (volumeKnob, 0.0);   // 0 dB
    setupKnob (effectKnob, 0.5);
    setupKnob (noiseKnob,  0.0);

    volumeAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (audioProcessor.apvts, "volume", volumeKnob);
    effectAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (audioProcessor.apvts, "effect", effectKnob);
    noiseAtt  = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (audioProcessor.apvts, "noise",  noiseKnob);
}

RdpAudioProcessorEditor::~RdpAudioProcessorEditor()
{
    volumeKnob.setLookAndFeel (nullptr);
    effectKnob.setLookAndFeel (nullptr);
    noiseKnob.setLookAndFeel (nullptr);
}

void RdpAudioProcessorEditor::resized()
{
    const int y = (int) (kBodyY + 96.f);
    volumeKnob.setBounds ((int) kRightColX + 0 * kKnobSize, y, kKnobSize, kKnobSize);
    effectKnob.setBounds ((int) kRightColX + 1 * kKnobSize, y, kKnobSize, kKnobSize);
    noiseKnob.setBounds  ((int) kRightColX + 2 * kKnobSize, y, kKnobSize, kKnobSize);
}

void RdpAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (Colours::white);

    const float effectV = (float) effectKnob.getValue();   // 0..1
    const float noiseV  = (float) noiseKnob.getValue();    // 0..1
    const Rectangle<float> body (kBodyX, kBodyY, kBodyW, kBodyH);

    auto chrome = [] (Rectangle<float> r)
    {
        return juce::ColourGradient (Colour (0xfff4f4f4), 0.f, r.getY(),
                                     Colour (0xff8a8a8a), 0.f, r.getBottom(), false);
    };

    // ---- sombra no chao (em camadas, bem suave) ----
    for (int i = 0; i < 4; ++i)
    {
        g.setColour (Colours::black.withAlpha (0.035f));
        g.fillEllipse (body.getX() + 8.f - (float) i * 5.f, body.getBottom() - 6.f - (float) i * 1.5f,
                       body.getWidth() - 16.f + (float) i * 10.f, 26.f + (float) i * 4.f);
    }

    // ---- pes de borracha ----
    g.setColour (Colour (0xff1e1009));
    g.fillRoundedRectangle (body.getX() + 40.f, body.getBottom() - 4.f, 64.f, 12.f, 4.f);
    g.fillRoundedRectangle (body.getRight() - 104.f, body.getBottom() - 4.f, 64.f, 12.f, 4.f);

    // ---- alca de transporte ----
    const Rectangle<float> hr (body.getX() + 80.f, body.getY() - 52.f, body.getWidth() - 160.f, 80.f);
    g.setColour (Colour (0xff25160d));
    g.drawRoundedRectangle (hr, 30.f, 12.f);
    g.setColour (Colours::white.withAlpha (0.14f));
    g.drawRoundedRectangle (hr.translated (0.f, -3.f), 30.f, 2.f);
    g.setColour (Colour (0xff1b0f08));
    g.fillRoundedRectangle (hr.getX() - 14.f, body.getY() - 8.f, 32.f, 16.f, 4.f);
    g.fillRoundedRectangle (hr.getRight() - 18.f, body.getY() - 8.f, 32.f, 16.f, 4.f);

    // ---- corpo ----
    juce::ColourGradient wood (Colour (0xffb86a3c), 0.f, body.getY(),
                               Colour (0xff69351f), 0.f, body.getBottom(), false);
    g.setGradientFill (wood);
    g.fillRoundedRectangle (body, 14.f);
    {
        juce::Graphics::ScopedSaveState ss (g);
        juce::Path clip;
        clip.addRoundedRectangle (body, 14.f);
        g.reduceClipRegion (clip);

        // veios finos de madeira
        for (float yy = body.getY(); yy < body.getBottom(); yy += 3.f)
        {
            const float al = 0.02f + 0.035f * (float) (((int) yy * 37) % 7) / 7.f;
            g.setColour (Colours::black.withAlpha (al));
            g.drawHorizontalLine ((int) yy, body.getX(), body.getRight());
        }
        // brilho lateral
        juce::ColourGradient sheen (Colours::white.withAlpha (0.10f), body.getX(), 0.f,
                                    Colours::black.withAlpha (0.14f), body.getRight(), 0.f, false);
        g.setGradientFill (sheen);
        g.fillRect (body);
    }
    g.setColour (Colour (0xff2e1609));
    g.drawRoundedRectangle (body.reduced (1.2f), 14.f, 2.6f);
    g.setColour (Colours::white.withAlpha (0.14f));
    g.drawRoundedRectangle (body.reduced (4.f), 11.f, 1.2f);

    // ---- parafusos ----
    auto screw = [&] (float cx, float cy)
    {
        g.setColour (Colours::black.withAlpha (0.35f));
        g.fillEllipse (cx - 4.5f, cy - 3.5f, 9.f, 9.f);
        juce::ColourGradient sg (Colour (0xffe8e8e8), cx - 3.f, cy - 3.f, Colour (0xff7a7a7a), cx + 3.f, cy + 4.f, false);
        g.setGradientFill (sg);
        g.fillEllipse (cx - 4.5f, cy - 4.5f, 9.f, 9.f);
        g.setColour (Colour (0xff3a3a3a));
        g.drawLine (cx - 3.f, cy + 2.f, cx + 3.f, cy - 2.f, 1.3f);
    };
    screw (body.getX() + 12.f,  body.getY() + 12.f);
    screw (body.getRight() - 12.f, body.getY() + 12.f);
    screw (body.getX() + 12.f,  body.getBottom() - 12.f);
    screw (body.getRight() - 12.f, body.getBottom() - 12.f);

    // ---- grade do alto-falante (chapa perfurada) ----
    const Rectangle<float> grille (body.getX() + 20.f, body.getY() + 22.f, 190.f, 206.f);
    juce::ColourGradient plate (Colour (0xffeadfbd), grille.getX(), grille.getY(),
                                Colour (0xffc9b88c), grille.getRight(), grille.getBottom(), false);
    g.setGradientFill (plate);
    g.fillRoundedRectangle (grille, 8.f);
    {
        juce::Graphics::ScopedSaveState ss (g);
        juce::Path clip;
        clip.addRoundedRectangle (grille.reduced (3.f), 6.f);
        g.reduceClipRegion (clip);

        int row = 0;
        for (float yy = grille.getY() + 12.f; yy < grille.getBottom() - 6.f; yy += 10.f, ++row)
            for (float xx = grille.getX() + 10.f + ((row % 2) ? 6.f : 0.f); xx < grille.getRight() - 6.f; xx += 12.f)
            {
                g.setColour (Colour (0xff2b190d).withAlpha (0.88f));
                g.fillEllipse (xx - 2.5f, yy - 2.5f, 5.f, 5.f);
            }
    }
    g.setColour (Colour (0xff2e1609));
    g.drawRoundedRectangle (grille, 8.f, 2.f);

    // ---- divisoria cromada ----
    {
        const Rectangle<float> bar (body.getX() + 215.f, body.getY() + 22.f, 5.f, 206.f);
        juce::ColourGradient bg (Colour (0xfff0f0f0), bar.getX(), 0.f, Colour (0xff7e7e7e), bar.getRight(), 0.f, false);
        g.setGradientFill (bg);
        g.fillRoundedRectangle (bar, 2.f);
    }

    // ---- visor do dial (acende com o EFEITO; ponteiro anda com o RUIDO) ----
    const Rectangle<float> dial (kRightColX, body.getY() + 22.f, 234.f, 62.f);
    const auto bezel = dial.expanded (4.f);
    g.setGradientFill (chrome (bezel));
    g.fillRoundedRectangle (bezel, 8.f);
    g.setColour (Colour (0xff2e1609));
    g.drawRoundedRectangle (bezel, 8.f, 1.5f);

    g.setColour (Colour (0xffe9dcb0).interpolatedWith (Colour (0xffffc94d), effectV * 0.9f));
    g.fillRoundedRectangle (dial, 5.f);

    const auto inner = dial.reduced (10.f, 6.f);
    g.setColour (Colour (0xff4a2c18));
    g.setFont (juce::Font (juce::FontOptions (10.f, juce::Font::bold)));
    const char* nums[5] = { "55", "70", "90", "120", "160" };
    for (int i = 0; i < 5; ++i)
    {
        Rectangle<int> cell ((int) (inner.getX() + inner.getWidth() * (float) i / 5.f), (int) inner.getY(),
                             (int) (inner.getWidth() / 5.f), 12);
        g.drawText (nums[i], cell, juce::Justification::centred);
    }
    for (int i = 0; i <= 30; ++i)
    {
        const float tx  = inner.getX() + inner.getWidth() * (float) i / 30.f;
        const float len = (i % 5 == 0) ? 12.f : 6.f;
        g.drawLine (tx, inner.getBottom(), tx, inner.getBottom() - len, 1.1f);
    }
    const float nx = inner.getX() + inner.getWidth() * noiseV;
    g.setColour (Colours::black.withAlpha (0.20f));
    g.drawLine (nx + 1.5f, inner.getY() + 14.f, nx + 1.5f, inner.getBottom() + 3.f, 2.f);
    g.setColour (Colour (0xffd0301f));
    g.drawLine (nx, inner.getY() + 13.f, nx, inner.getBottom() + 2.f, 2.f);

    // reflexo do vidro
    juce::ColourGradient glass (Colours::white.withAlpha (0.30f), 0.f, dial.getY(),
                                Colours::white.withAlpha (0.02f), 0.f, dial.getCentreY(), false);
    g.setGradientFill (glass);
    g.fillRoundedRectangle (dial.withHeight (dial.getHeight() * 0.5f), 5.f);

    // ---- rotulos dos knobs (gravados) ----
    const juce::String labels[3] = { "VOLUME", "EFEITO", juce::String::fromUTF8 ("RU\xc3\x8d" "DO") };
    g.setFont (juce::Font (juce::FontOptions (11.f, juce::Font::bold)));
    for (int i = 0; i < 3; ++i)
    {
        Rectangle<int> r ((int) kRightColX + i * kKnobSize, (int) (body.getY() + 175.f), kKnobSize, 14);
        g.setColour (Colour (0xff2e1609));
        g.drawText (labels[i], r.translated (1, 1), juce::Justification::centred);
        g.setColour (Colour (0xffeedfb5));
        g.drawText (labels[i], r, juce::Justification::centred);
    }

    // ---- plaquinha da marca ----
    const Rectangle<float> badge (kRightColX, body.getY() + 198.f, 66.f, 30.f);
    g.setGradientFill (chrome (badge));
    g.fillRoundedRectangle (badge, 4.f);
    g.setColour (Colour (0xff2e1609));
    g.drawRoundedRectangle (badge, 4.f, 1.2f);
    g.setFont (juce::Font (juce::FontOptions (19.f, juce::Font::bold)));
    g.drawText ("RDP", badge.toNearestInt(), juce::Justification::centred);

    g.setFont (juce::Font (juce::FontOptions (11.f, juce::Font::bold)));
    const Rectangle<int> sub ((int) kRightColX + 78, (int) (body.getY() + 198.f), 156, 30);
    g.setColour (Colour (0xff2e1609));
    g.drawText ("RADINHO DE PILHA", sub.translated (1, 1), juce::Justification::centredLeft);
    g.setColour (Colour (0xffeedfb5));
    g.drawText ("RADINHO DE PILHA", sub, juce::Justification::centredLeft);
}
