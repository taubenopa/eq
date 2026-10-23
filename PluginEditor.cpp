#include "PluginEditor.h"
#include <algorithm>
#include <cmath>

using namespace juce;

namespace
{
    constexpr float kPi    = MathConstants<float>::pi;
    constexpr float kTwoPi = MathConstants<float>::twoPi;

    const float kRanges[] = { 6.0f, 12.0f, 18.0f, 24.0f };

    Colour hsv (float h, float s, float v, float a = 1.0f)
    {
        h -= std::floor (h);
        return Colour::fromHSV (h, s, v, a);
    }

    String noteName (int midi)
    {
        static const char* n[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
        return String (n[((midi % 12) + 12) % 12]) + String (midi / 12 - 1);
    }

    ColourGradient rainbowGradient (Rectangle<float> r, float time, float alpha)
    {
        ColourGradient gr (hsv (time * 0.08f, 0.85f, 1.0f, alpha), r.getX(), 0.0f,
                           hsv (time * 0.08f + 0.9f, 0.85f, 1.0f, alpha), r.getRight(), 0.0f, false);
        for (int k = 1; k < 6; ++k)
            gr.addColour ((double) k / 6.0, hsv (time * 0.08f + 0.15f * (float) k, 0.85f, 1.0f, alpha));
        return gr;
    }
}

//==============================================================================
NeonLookAndFeel::NeonLookAndFeel()
{
    setColour (Slider::textBoxTextColourId, Colours::white.withAlpha (0.9f));
    setColour (Slider::textBoxOutlineColourId, Colours::transparentBlack);
    setColour (Slider::textBoxBackgroundColourId, Colours::transparentBlack);
    setColour (ComboBox::backgroundColourId, Colour (0xff12152a));
    setColour (ComboBox::textColourId, Colours::white);
    setColour (ComboBox::outlineColourId, Colour (0xff3a4070));
    setColour (ComboBox::arrowColourId, Colours::white);
    setColour (PopupMenu::backgroundColourId, Colour (0xff10132a));
    setColour (PopupMenu::textColourId, Colours::white);
    setColour (PopupMenu::highlightedBackgroundColourId, Colour (0xff3a4aa0));
    setColour (TextButton::textColourOffId, Colours::white.withAlpha (0.85f));
    setColour (TextButton::textColourOnId, Colours::white);
    setColour (TextButton::buttonColourId, Colour (0xff6670a8));
    setColour (TextButton::buttonOnColourId, Colour (0xff00e5ff));
}

void NeonLookAndFeel::drawRotarySlider (Graphics& g, int x, int y, int w, int h, float pos,
                                        float startAngle, float endAngle, Slider& slider)
{
    auto bounds = Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (4.0f);
    const float radius = jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const auto c = bounds.getCentre();
    const auto col = slider.findColour (Slider::rotarySliderFillColourId);
    const float angle = startAngle + pos * (endAngle - startAngle);
    const float arcR = radius - 3.0f;

    Path track;
    track.addCentredArc (c.x, c.y, arcR, arcR, 0.0f, startAngle, endAngle, true);
    g.setColour (Colours::white.withAlpha (0.09f));
    g.strokePath (track, PathStrokeType (5.0f, PathStrokeType::curved, PathStrokeType::rounded));

    Path val;
    val.addCentredArc (c.x, c.y, arcR, arcR, 0.0f, startAngle, angle, true);
    g.setColour (col.withAlpha (0.10f));
    g.strokePath (val, PathStrokeType (13.0f, PathStrokeType::curved, PathStrokeType::rounded));
    g.setColour (col.withAlpha (0.22f));
    g.strokePath (val, PathStrokeType (9.0f, PathStrokeType::curved, PathStrokeType::rounded));
    g.setColour (col);
    g.strokePath (val, PathStrokeType (4.0f, PathStrokeType::curved, PathStrokeType::rounded));

    const float r2 = radius - 10.0f;
    g.setGradientFill (ColourGradient (Colour (0xff2c3150), c.x, c.y - r2, Colour (0xff0a0c18), c.x, c.y + r2, false));
    g.fillEllipse (c.x - r2, c.y - r2, r2 * 2.0f, r2 * 2.0f);
    g.setColour (Colours::white.withAlpha (0.12f));
    g.drawEllipse (c.x - r2, c.y - r2, r2 * 2.0f, r2 * 2.0f, 1.0f);

    const float d0 = r2 * 0.30f, d1 = r2 * 0.92f;
    g.setColour (col.brighter (0.4f));
    g.drawLine (c.x + std::sin (angle) * d0, c.y - std::cos (angle) * d0,
                c.x + std::sin (angle) * d1, c.y - std::cos (angle) * d1, 2.5f);
}

void NeonLookAndFeel::drawButtonBackground (Graphics& g, Button& b, const Colour&, bool hl, bool down)
{
    auto r = b.getLocalBounds().toFloat().reduced (0.5f);
    const bool on = b.getToggleState();
    auto base = on ? b.findColour (TextButton::buttonOnColourId) : b.findColour (TextButton::buttonColourId);
    if (hl)   base = base.brighter (0.2f);
    if (down) base = base.brighter (0.3f);

    g.setColour (base.withAlpha (on ? 0.38f : 0.16f));
    g.fillRoundedRectangle (r, 6.0f);
    g.setColour (base.withAlpha (on ? 1.0f : 0.5f));
    g.drawRoundedRectangle (r, 6.0f, on ? 1.7f : 1.0f);
}

//==============================================================================
CrazyEQAudioProcessorEditor::CrazyEQAudioProcessorEditor (CrazyEQAudioProcessor& p)
    : AudioProcessorEditor (&p),
      proc (p),
      fft (fftOrder),
      window ((size_t) fftSize, dsp::WindowingFunction<float>::hann, false)
{
    setLookAndFeel (&laf);

    fftData.assign ((size_t) fftSize * 2, 0.0f);
    specPost.assign ((size_t) numCols, -120.0f);
    specPre.assign ((size_t) numCols, -120.0f);
    specPeak.assign ((size_t) numCols, -120.0f);
    specLoF.resize ((size_t) numCols);
    specHiF.resize ((size_t) numCols);
    specCenF.resize ((size_t) numCols);

    const float invN = 1.0f / (float) (numCols - 1);
    for (int i = 0; i < numCols; ++i)
    {
        const float t0 = jlimit (0.0f, 1.0f, ((float) i - 0.5f) * invN);
        const float t1 = jlimit (0.0f, 1.0f, ((float) i + 0.5f) * invN);
        const float tc = (float) i * invN;
        specLoF[(size_t) i]  = 20.0f * std::pow (1000.0f, t0);
        specHiF[(size_t) i]  = 20.0f * std::pow (1000.0f, t1);
        specCenF[(size_t) i] = 20.0f * std::pow (1000.0f, tc);
    }

    curveFreq.resize ((size_t) numCurve);
    totalCurve.assign ((size_t) numCurve, 0.0f);
    selCurve.assign ((size_t) numCurve, 0.0f);
    for (int i = 0; i < numCurve; ++i)
        curveFreq[(size_t) i] = 20.0f * std::pow (1000.0f, (float) i / (float) (numCurve - 1));

    for (int i = 0; i < 90; ++i)
        stars.push_back ({ rng.nextFloat(), rng.nextFloat(), rng.nextFloat() * kTwoPi });

    // Header-Buttons
    auto setupToggle = [this] (TextButton& b, Colour c, bool on)
    {
        b.setClickingTogglesState (true);
        b.setToggleState (on, dontSendNotification);
        b.setColour (TextButton::buttonColourId, c);
        b.setColour (TextButton::buttonOnColourId, c);
        addAndMakeVisible (b);
    };
    setupToggle (analyzerButton, Colour (0xff00e5ff), true);
    setupToggle (preButton, Colour (0xffb388ff), true);
    setupToggle (crazyButton, Colour (0xffff3dcb), true);

    rangeButton.setColour (TextButton::buttonColourId, Colour (0xffffd740));
    rangeButton.onClick = [this]
    {
        rangeIdx = (rangeIdx + 1) % 4;
        gainRange = kRanges[rangeIdx];
        rangeButton.setButtonText ("RANGE " + String ((int) gainRange) + " dB");
    };
    addAndMakeVisible (rangeButton);

    resetAllButton.setColour (TextButton::buttonColourId, Colour (0xffff5252));
    resetAllButton.onClick = [this]
    {
        for (auto* prm : proc.getParameters())
        {
            prm->beginChangeGesture();
            prm->setValueNotifyingHost (prm->getDefaultValue());
            prm->endChangeGesture();
        }
    };
    addAndMakeVisible (resetAllButton);

    // Band-Auswahl
    for (int i = 0; i < ceq::numBands; ++i)
    {
        auto& bt = bandButtons[i];
        bt.setButtonText (String (i + 1));
        bt.setClickingTogglesState (true);
        bt.setRadioGroupId (1001);
        bt.setColour (TextButton::buttonColourId, bandColour (i));
        bt.setColour (TextButton::buttonOnColourId, bandColour (i));
        bt.onClick = [this, i] { if (bandButtons[i].getToggleState()) select (i); };
        addAndMakeVisible (bt);
    }

    enableButton.setClickingTogglesState (true);
    addAndMakeVisible (enableButton);

    for (int t = 0; t < (int) ceq::numTypes; ++t)
        typeBox.addItem (ceq::typeName (t), t + 1);
    addAndMakeVisible (typeBox);

    auto setupKnob = [this] (Slider& s)
    {
        s.setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle (Slider::TextBoxBelow, false, 100, 18);
        s.setRotaryParameters (kPi * 1.2f, kPi * 2.8f, true);
        addAndMakeVisible (s);
    };
    setupKnob (freqSlider);
    setupKnob (gainSlider);
    setupKnob (qSlider);
    setupKnob (outSlider);
    outSlider.setColour (Slider::rotarySliderFillColourId, Colour (0xff00e5ff));
    outAtt = std::make_unique<SliderAttachment> (proc.apvts, "out_gain", outSlider);

    select (0);

    setResizable (true, true);
    setResizeLimits (900, 580, 2400, 1400);
    setSize (1100, 700);

    startTimerHz (45);
}

CrazyEQAudioProcessorEditor::~CrazyEQAudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

Colour CrazyEQAudioProcessorEditor::bandColour (int b)
{
    return hsv ((float) b / (float) ceq::numBands * 0.88f, 0.72f, 1.0f);
}

void CrazyEQAudioProcessorEditor::select (int b)
{
    selected = jlimit (0, ceq::numBands - 1, b);
    auto& apvts = proc.apvts;

    freqAtt.reset();
    gainAtt.reset();
    qAtt.reset();
    typeAtt.reset();
    enAtt.reset();

    freqAtt = std::make_unique<SliderAttachment> (apvts, ceq::pid (selected, "freq"), freqSlider);
    gainAtt = std::make_unique<SliderAttachment> (apvts, ceq::pid (selected, "gain"), gainSlider);
    qAtt    = std::make_unique<SliderAttachment> (apvts, ceq::pid (selected, "q"), qSlider);
    typeAtt = std::make_unique<ComboBoxAttachment> (apvts, ceq::pid (selected, "type"), typeBox);
    enAtt   = std::make_unique<ButtonAttachment> (apvts, ceq::pid (selected, "on"), enableButton);

    const auto col = bandColour (selected);
    for (auto* s : { &freqSlider, &gainSlider, &qSlider })
        s->setColour (Slider::rotarySliderFillColourId, col);
    enableButton.setColour (TextButton::buttonOnColourId, col);
    enableButton.setColour (TextButton::buttonColourId, col);

    bandButtons[selected].setToggleState (true, dontSendNotification);
    repaint();
}

CrazyEQAudioProcessorEditor::Band CrazyEQAudioProcessorEditor::readBand (int b) const
{
    auto get = [this] (const String& id, float fallback) -> float
    {
        if (auto* v = proc.apvts.getRawParameterValue (id))
            return v->load();
        return fallback;
    };

    Band s;
    s.en   = get (ceq::pid (b, "on"), 1.0f) > 0.5f;
    s.type = (int) get (ceq::pid (b, "type"), 0.0f);
    s.f    = get (ceq::pid (b, "freq"), 1000.0f);
    s.g    = get (ceq::pid (b, "gain"), 0.0f);
    s.q    = get (ceq::pid (b, "q"), 1.0f);
    return s;
}

//==============================================================================
// Koordinaten
float CrazyEQAudioProcessorEditor::freqToX (float f) const
{
    return plot.getX() + plot.getWidth() * (std::log (jmax (f, 1.0f) / 20.0f) / std::log (1000.0f));
}

float CrazyEQAudioProcessorEditor::xToFreq (float x) const
{
    const float t = jlimit (0.0f, 1.0f, (x - plot.getX()) / plot.getWidth());
    return 20.0f * std::pow (1000.0f, t);
}

float CrazyEQAudioProcessorEditor::gainToY (float db) const
{
    return jmap (jlimit (-gainRange, gainRange, db), gainRange, -gainRange, plot.getY(), plot.getBottom());
}

float CrazyEQAudioProcessorEditor::yToGain (float y) const
{
    return jmap (jlimit (plot.getY(), plot.getBottom(), y), plot.getY(), plot.getBottom(), gainRange, -gainRange);
}

float CrazyEQAudioProcessorEditor::specToY (float db) const
{
    return jmap (jlimit (-90.0f, -10.0f, db), -90.0f, -10.0f, plot.getBottom(), plot.getY());
}

Point<float> CrazyEQAudioProcessorEditor::nodePos (int b) const
{
    const auto& s = bands[(size_t) b];
    return { freqToX (s.f), ceq::typeHasGain (s.type) ? gainToY (s.g) : gainToY (0.0f) };
}

int CrazyEQAudioProcessorEditor::hitTest (Point<float> p) const
{
    int best = -1;
    float bestD = 18.0f * 18.0f;
    for (int b = 0; b < ceq::numBands; ++b)
    {
        const auto n = nodePos (b);
        const float dx = n.x - p.x, dy = n.y - p.y;
        const float d = dx * dx + dy * dy;
        if (d < bestD)
        {
            bestD = d;
            best = b;
        }
    }
    return best;
}

Path CrazyEQAudioProcessorEditor::curvePath (const std::vector<float>& v) const
{
    Path p;
    for (int i = 0; i < numCurve; ++i)
    {
        const float x = plot.getX() + plot.getWidth() * (float) i / (float) (numCurve - 1);
        const float y = gainToY (v[(size_t) i]);
        if (i == 0) p.startNewSubPath (x, y);
        else        p.lineTo (x, y);
    }
    return p;
}

//==============================================================================
// Logik pro Frame
float CrazyEQAudioProcessorEditor::bandDb (int b, float f) const
{
    if (! bands[(size_t) b].en)
        return 0.0f;

    double m = 1.0;
    const double sr = proc.getCurrentSampleRate();
    for (int s = 0; s < 2; ++s)
        m *= coeffs[(size_t) b][(size_t) s]->getMagnitudeForFrequency ((double) f, sr);
    return Decibels::gainToDecibels ((float) m, -100.0f);
}

void CrazyEQAudioProcessorEditor::updateBands()
{
    const double sr = proc.getCurrentSampleRate();
    for (int b = 0; b < ceq::numBands; ++b)
    {
        auto& s = bands[(size_t) b];
        s = readBand (b);
        for (int st = 0; st < 2; ++st)
            coeffs[(size_t) b][(size_t) st] = ceq::makeStage (s.type, sr, s.f, s.q, s.g, st);
    }

    for (int i = 0; i < numCurve; ++i)
    {
        const float f = curveFreq[(size_t) i];
        float total = 0.0f;
        for (int b = 0; b < ceq::numBands; ++b)
            total += bandDb (b, f);
        totalCurve[(size_t) i] = total;
        selCurve[(size_t) i] = bandDb (selected, f);
    }
}

void CrazyEQAudioProcessorEditor::updateSpectrum (const SampleRing& ring, std::vector<float>& out)
{
    ring.copyLatest (fftData.data(), fftSize);
    window.multiplyWithWindowingTable (fftData.data(), (size_t) fftSize);
    std::fill (fftData.begin() + fftSize, fftData.end(), 0.0f);
    fft.performFrequencyOnlyForwardTransform (fftData.data());

    const float binHz = (float) (proc.getCurrentSampleRate() / (double) fftSize);
    const int maxBin = fftSize / 2 - 1;

    for (int i = 0; i < numCols; ++i)
    {
        const float bLo = specLoF[(size_t) i] / binHz;
        const float bHi = specHiF[(size_t) i] / binHz;
        float mag = 0.0f;

        if (bHi - bLo < 1.0f)
        {
            const float bc = 0.5f * (bLo + bHi);
            const int i0 = jmin ((int) bc, maxBin);
            const int i1 = jmin (i0 + 1, maxBin);
            const float fr = bc - (float) i0;
            mag = fftData[(size_t) i0] * (1.0f - fr) + fftData[(size_t) i1] * fr;
        }
        else
        {
            const int a = jmin ((int) bLo, maxBin);
            const int e = jmin ((int) bHi, maxBin);
            for (int k = a; k <= e; ++k)
                mag = jmax (mag, fftData[(size_t) k]);
        }

        // +3 dB/Oktave Tilt, damit die Anzeige wie bei Pro-Analyzern aussieht
        const float db = Decibels::gainToDecibels (mag * 4.0f / (float) fftSize, -120.0f)
                         + 3.0f * std::log2 (specCenF[(size_t) i] / 1000.0f);
        auto& s = out[(size_t) i];
        s += (db - s) * (db > s ? 0.55f : 0.13f);
    }
}

void CrazyEQAudioProcessorEditor::updateMeters()
{
    const float pk[2] = { proc.peakL.exchange (0.0f), proc.peakR.exchange (0.0f) };

    for (int c = 0; c < 2; ++c)
    {
        meterLin[c] = jmax (pk[c], meterLin[c] * 0.88f);
        meterDb[c] = Decibels::gainToDecibels (meterLin[c], -100.0f);

        const float pdb = Decibels::gainToDecibels (pk[c], -100.0f);
        if (pdb > holdDb[c])
            holdDb[c] = pdb;
        else
            holdDb[c] = jmax (-100.0f, holdDb[c] - 0.3f);

        if (pk[c] >= 1.0f)
            clipHold = 90;
    }

    if (clipHold > 0)
        --clipHold;

    const float level = jmax (meterDb[0], meterDb[1]);
    const float target = jlimit (0.0f, 1.0f, (level + 45.0f) / 40.0f);
    energy += (target - energy) * (target > energy ? 0.5f : 0.1f);
}

void CrazyEQAudioProcessorEditor::spawnBurst (float x, float y, float hue, int n)
{
    for (int i = 0; i < n && particles.size() < 800; ++i)
    {
        const float a = rng.nextFloat() * kTwoPi;
        const float sp = 0.5f + rng.nextFloat() * 2.5f;
        particles.push_back ({ x, y, std::cos (a) * sp, std::sin (a) * sp, 1.0f,
                               0.8f + rng.nextFloat() * 1.2f, hue + rng.nextFloat() * 0.1f,
                               1.5f + rng.nextFloat() * 2.5f });
    }
}

void CrazyEQAudioProcessorEditor::spawnRipple (float x, float y, float hue)
{
    ripples.push_back ({ x, y, 0.0f, hue });
}

void CrazyEQAudioProcessorEditor::updateParticles (float dt)
{
    for (auto& p : particles)
    {
        p.x += p.vx;
        p.y += p.vy;
        p.vx *= 0.99f;
        p.life -= p.decay * dt;
    }

    particles.erase (std::remove_if (particles.begin(), particles.end(),
                                     [] (const Particle& p) { return p.life <= 0.0f; }),
                     particles.end());

    for (auto& r : ripples)
        r.age += dt;

    ripples.erase (std::remove_if (ripples.begin(), ripples.end(),
                                   [] (const Ripple& r) { return r.age > 1.2f; }),
                   ripples.end());

    const int n = (int) (energy * 7.0f * crazy) + (rng.nextFloat() < 0.25f * crazy ? 1 : 0);
    const bool useSpec = analyzerButton.getToggleState();

    for (int i = 0; i < n && particles.size() < 800; ++i)
    {
        const int col = rng.nextInt (numCols);
        const float t = (float) col / (float) (numCols - 1);
        const float x = plot.getX() + plot.getWidth() * t;
        const int ci = jlimit (0, numCurve - 1, (int) (t * (float) (numCurve - 1)));
        const float y = useSpec ? specToY (specPost[(size_t) col]) : gainToY (totalCurve[(size_t) ci]);

        particles.push_back ({ x, y, (rng.nextFloat() - 0.5f) * 0.8f, -(0.6f + rng.nextFloat() * 2.4f), 1.0f,
                               0.6f + rng.nextFloat() * 0.9f, t * 0.9f + time * 0.08f,
                               1.2f + rng.nextFloat() * 2.2f });
    }
}

void CrazyEQAudioProcessorEditor::timerCallback()
{
    time += 1.0f / 45.0f;
    crazy = crazyButton.getToggleState() ? 1.0f : 0.25f;

    updateBands();
    updateMeters();

    if (analyzerButton.getToggleState())
    {
        updateSpectrum (proc.postRing, specPost);

        if (preButton.getToggleState())
            updateSpectrum (proc.preRing, specPre);
        else
            for (auto& v : specPre)
                v += (-120.0f - v) * 0.2f;

        for (size_t i = 0; i < specPeak.size(); ++i)
            specPeak[i] = jmax (specPeak[i] - 0.5f, specPost[i]);
    }

    // Shockwave bei Aktivieren/Deaktivieren eines Bands
    for (int b = 0; b < ceq::numBands; ++b)
    {
        if (bands[(size_t) b].en != prevEn[b])
        {
            prevEn[b] = bands[(size_t) b].en;
            if (plot.getWidth() > 10.0f)
            {
                const auto pos = nodePos (b);
                spawnRipple (pos.x, pos.y, (float) b / (float) ceq::numBands * 0.88f);
                spawnBurst (pos.x, pos.y, (float) b / (float) ceq::numBands * 0.88f, 24);
            }
        }
        bandButtons[b].setAlpha (bands[(size_t) b].en ? 1.0f : 0.45f);
    }

    gainSlider.setAlpha (ceq::typeHasGain (bands[(size_t) selected].type) ? 1.0f : 0.35f);

    updateParticles (1.0f / 45.0f);
    repaint();
}

//==============================================================================
// Parameter-Helfer
void CrazyEQAudioProcessorEditor::setParam (const String& id, float v)
{
    if (auto* p = proc.apvts.getParameter (id))
        p->setValueNotifyingHost (p->convertTo0to1 (v));
}

void CrazyEQAudioProcessorEditor::setParamGesture (const String& id, float v)
{
    if (auto* p = proc.apvts.getParameter (id))
    {
        p->beginChangeGesture();
        p->setValueNotifyingHost (p->convertTo0to1 (v));
        p->endChangeGesture();
    }
}

void CrazyEQAudioProcessorEditor::resetParam (const String& id)
{
    if (auto* p = proc.apvts.getParameter (id))
    {
        p->beginChangeGesture();
        p->setValueNotifyingHost (p->getDefaultValue());
        p->endChangeGesture();
    }
}

void CrazyEQAudioProcessorEditor::beginGestures (int b)
{
    for (auto* k : { "freq", "gain", "q" })
        if (auto* p = proc.apvts.getParameter (ceq::pid (b, k)))
            p->beginChangeGesture();
}

void CrazyEQAudioProcessorEditor::endGestures (int b)
{
    for (auto* k : { "freq", "gain", "q" })
        if (auto* p = proc.apvts.getParameter (ceq::pid (b, k)))
            p->endChangeGesture();
}

void CrazyEQAudioProcessorEditor::showBandMenu (int b)
{
    PopupMenu m;
    const auto st = readBand (b);

    for (int t = 0; t < (int) ceq::numTypes; ++t)
        m.addItem (100 + t, ceq::typeName (t), true, st.type == t);

    m.addSeparator();
    m.addItem (1, st.en ? "Disable band" : "Enable band");
    m.addItem (2, "Reset band");

    SafePointer<CrazyEQAudioProcessorEditor> safe (this);
    m.showMenuAsync (PopupMenu::Options(), [safe, b] (int result)
    {
        if (safe == nullptr || result == 0)
            return;

        auto& ed = *safe;
        if (result >= 100)
        {
            ed.setParamGesture (ceq::pid (b, "type"), (float) (result - 100));
        }
        else if (result == 1)
        {
            ed.setParamGesture (ceq::pid (b, "on"), ed.readBand (b).en ? 0.0f : 1.0f);
        }
        else if (result == 2)
        {
            for (auto* id : { "on", "type", "freq", "gain", "q" })
                ed.resetParam (ceq::pid (b, id));
        }
    });
}

//==============================================================================
// Maus
void CrazyEQAudioProcessorEditor::mouseDown (const MouseEvent& e)
{
    if (! plot.contains (e.position))
        return;

    const int hit = hitTest (e.position);
    if (hit < 0)
        return;

    select (hit);

    if (e.mods.isPopupMenu())
    {
        showBandMenu (hit);
        return;
    }

    dragBand = hit;
    dragLastY = e.position.y;
    beginGestures (hit);

    const auto pos = nodePos (hit);
    spawnBurst (pos.x, pos.y, (float) hit / (float) ceq::numBands * 0.88f, 14);
}

void CrazyEQAudioProcessorEditor::mouseDrag (const MouseEvent& e)
{
    mousePos = e.position;
    if (dragBand < 0)
        return;

    const auto st = readBand (dragBand);
    const float x = jlimit (plot.getX(), plot.getRight(), e.position.x);
    setParam (ceq::pid (dragBand, "freq"), xToFreq (x));

    if (ceq::typeHasGain (st.type))
    {
        setParam (ceq::pid (dragBand, "gain"), yToGain (e.position.y));
    }
    else
    {
        // Filter ohne Gain: vertikales Ziehen veraendert die Guete (Q)
        const float dy = e.position.y - dragLastY;
        dragLastY = e.position.y;
        setParam (ceq::pid (dragBand, "q"), jlimit (0.1f, 18.0f, st.q * std::exp (-dy * 0.01f)));
    }

    const auto pos = nodePos (dragBand);
    if (rng.nextFloat() < 0.6f)
        spawnBurst (pos.x, pos.y, (float) dragBand / (float) ceq::numBands * 0.88f, 2);
}

void CrazyEQAudioProcessorEditor::mouseUp (const MouseEvent&)
{
    if (dragBand >= 0)
    {
        endGestures (dragBand);
        dragBand = -1;
    }
}

void CrazyEQAudioProcessorEditor::mouseMove (const MouseEvent& e)
{
    mousePos = e.position;
    mouseInPlot = plot.contains (e.position);
    hoverBand = mouseInPlot ? hitTest (e.position) : -1;
    setMouseCursor (hoverBand >= 0 ? MouseCursor::DraggingHandCursor : MouseCursor::NormalCursor);
}

void CrazyEQAudioProcessorEditor::mouseExit (const MouseEvent&)
{
    mouseInPlot = false;
    hoverBand = -1;
}

void CrazyEQAudioProcessorEditor::mouseDoubleClick (const MouseEvent& e)
{
    if (! plot.contains (e.position))
        return;

    const int hit = hitTest (e.position);
    if (hit >= 0)
    {
        setParamGesture (ceq::pid (hit, "on"), bands[(size_t) hit].en ? 0.0f : 1.0f);
        select (hit);
        return;
    }

    // Doppelklick ins Leere: erstes freies Band dorthin setzen
    for (int b = 0; b < ceq::numBands; ++b)
    {
        if (! bands[(size_t) b].en)
        {
            setParamGesture (ceq::pid (b, "type"), (float) ceq::Bell);
            setParamGesture (ceq::pid (b, "q"), 1.0f);
            setParamGesture (ceq::pid (b, "freq"), xToFreq (e.position.x));
            setParamGesture (ceq::pid (b, "gain"), yToGain (e.position.y));
            setParamGesture (ceq::pid (b, "on"), 1.0f);
            select (b);
            return;
        }
    }
}

void CrazyEQAudioProcessorEditor::mouseWheelMove (const MouseEvent& e, const MouseWheelDetails& w)
{
    if (! plot.contains (e.position))
        return;

    int b = hitTest (e.position);
    if (b >= 0)
        select (b);
    else
        b = selected;

    const auto st = readBand (b);
    setParamGesture (ceq::pid (b, "q"), jlimit (0.1f, 18.0f, st.q * std::exp (w.deltaY * 2.0f)));
}

//==============================================================================
// Layout
void CrazyEQAudioProcessorEditor::resized()
{
    auto r = getLocalBounds().reduced (14);
    auto header = r.removeFromTop (46);
    headerArea = header.toFloat();

    int x = header.getRight();
    const int gap = 6;
    auto place = [&] (Component& c, int w)
    {
        x -= w;
        c.setBounds (x, header.getY() + 10, w, 26);
        x -= gap;
    };
    place (resetAllButton, 104);
    place (rangeButton, 100);
    place (crazyButton, 80);
    place (preButton, 80);
    place (analyzerButton, 96);

    r.removeFromTop (6);
    auto bottom = r.removeFromBottom (140);
    bottomArea = bottom.toFloat();
    r.removeFromBottom (8);

    meterArea = r.removeFromRight (74).toFloat();
    r.removeFromRight (8);

    graphArea = r.toFloat();
    plot = graphArea.withTrimmedBottom (22.0f);
    pianoArea = Rectangle<float> (graphArea.getX(), plot.getBottom() + 4.0f, graphArea.getWidth(), 16.0f);

    auto inner = bottom.reduced (10, 8);
    auto row = inner.removeFromTop (26);
    int bx = row.getX();
    for (int i = 0; i < ceq::numBands; ++i)
    {
        bandButtons[i].setBounds (bx, row.getY() + 1, 40, 24);
        bx += 44;
    }

    inner.removeFromTop (6);
    auto left = inner.removeFromLeft (176);
    typeBox.setBounds (left.getX(), left.getY() + 16, 164, 28);
    enableButton.setBounds (left.getX(), left.getY() + 52, 164, 28);

    auto outArea = inner.removeFromRight (120);
    freqSlider.setBounds (inner.removeFromLeft (120).withTrimmedTop (16));
    gainSlider.setBounds (inner.removeFromLeft (120).withTrimmedTop (16));
    qSlider.setBounds (inner.removeFromLeft (120).withTrimmedTop (16));
    outSlider.setBounds (outArea.withTrimmedTop (16));
}

//==============================================================================
// Zeichnen
void CrazyEQAudioProcessorEditor::paint (Graphics& g)
{
    paintBackground (g);
    paintHeader (g);

    g.setColour (Colour (0xff04050c).withAlpha (0.66f));
    g.fillRoundedRectangle (graphArea, 10.0f);

    g.saveState();
    g.reduceClipRegion (graphArea.toNearestInt());

    paintGrid (g);
    paintPiano (g);

    g.saveState();
    g.reduceClipRegion (plot.toNearestInt());
    paintSelectedBand (g);
    paintSpectrum (g);
    paintParticles (g);
    paintCurve (g);
    paintScanner (g);
    paintNodes (g);
    paintRipples (g);
    paintHover (g);
    g.restoreState();

    g.restoreState();

    g.setGradientFill (ColourGradient (hsv (time * 0.1f, 0.8f, 1.0f, 0.75f), graphArea.getX(), graphArea.getY(),
                                       hsv (time * 0.1f + 0.5f, 0.8f, 1.0f, 0.75f), graphArea.getRight(), graphArea.getBottom(), false));
    g.drawRoundedRectangle (graphArea.reduced (0.5f), 10.0f, 1.5f);

    paintMeters (g);
    paintPanel (g);
}

void CrazyEQAudioProcessorEditor::paintBackground (Graphics& g)
{
    const auto b = getLocalBounds().toFloat();

    g.setGradientFill (ColourGradient (Colour (0xff05060d), 0.0f, 0.0f, Colour (0xff0e1226), 0.0f, b.getHeight(), false));
    g.fillRect (b);

    for (int k = 0; k < 3; ++k)
    {
        const float ph = (float) k * 2.1f;
        const float cx = b.getWidth() * (0.5f + 0.45f * std::sin (time * 0.31f + ph));
        const float cy = b.getHeight() * (0.5f + 0.45f * std::cos (time * 0.23f + ph * 1.3f));
        const float rad = jmax (b.getWidth(), b.getHeight()) * 0.55f;
        const float a = (0.07f + 0.16f * energy) * crazy;
        const auto c = hsv (time * 0.02f + (float) k * 0.28f, 0.8f, 1.0f, a);

        g.setGradientFill (ColourGradient (c, cx, cy, c.withAlpha (0.0f), cx + rad, cy, true));
        g.fillRect (b);
    }

    for (const auto& s : stars)
    {
        const float tw = 0.5f + 0.5f * std::sin (time * 2.0f + s.phase);
        g.setColour (Colours::white.withAlpha ((0.05f + 0.35f * tw) * crazy));
        g.fillRect (s.x * b.getWidth(), s.y * b.getHeight(), 1.6f, 1.6f);
    }
}

void CrazyEQAudioProcessorEditor::paintHeader (Graphics& g)
{
    const auto h = headerArea;

    const int nb = 7;
    const float bw = 5.0f, gap = 3.0f, maxH = 30.0f;
    for (int i = 0; i < nb; ++i)
    {
        float v = 0.25f + 0.75f * (0.5f + 0.5f * std::sin (time * (2.0f + (float) i * 0.7f) + (float) i));
        v = jlimit (0.1f, 1.0f, v * (0.45f + energy));
        const float hh = maxH * v;
        g.setColour (hsv (time * 0.1f + (float) i * 0.08f, 0.8f, 1.0f));
        g.fillRoundedRectangle (h.getX() + (float) i * (bw + gap), h.getCentreY() + maxH * 0.5f - hh, bw, hh, 2.0f);
    }

    const float tx = h.getX() + 70.0f;
    const Rectangle<float> t (tx, h.getY() + 2.0f, 360.0f, 28.0f);
    g.setFont (Font (26.0f, Font::bold));
    g.setGradientFill (ColourGradient (hsv (time * 0.15f, 0.7f, 1.0f), t.getX(), 0.0f,
                                       hsv (time * 0.15f + 0.5f, 0.7f, 1.0f), t.getRight(), 0.0f, false));
    g.drawText ("CRAZY VISUAL EQ", t, Justification::centredLeft);

    g.setFont (Font (11.0f));
    g.setColour (Colours::white.withAlpha (0.45f));
    g.drawText ("8-BAND PARAMETRIC  |  LIVE SPECTRUM  |  VST3",
                Rectangle<float> (tx, h.getY() + 30.0f, 420.0f, 14.0f), Justification::centredLeft);
}

void CrazyEQAudioProcessorEditor::paintGrid (Graphics& g)
{
    const float a = 0.05f + 0.08f * energy * crazy;

    for (int decade = 1; decade <= 4; ++decade)
    {
        const float base = std::pow (10.0f, (float) decade);
        for (int m = 1; m <= 9; ++m)
        {
            const float f = (float) m * base;
            if (f < 20.0f || f > 20000.0f)
                continue;

            const float x = freqToX (f);
            g.setColour (Colours::white.withAlpha (m == 1 ? a * 2.2f : a));
            g.drawLine (x, plot.getY(), x, plot.getBottom(), 1.0f);
        }
    }

    const float step = gainRange <= 6.0f ? 2.0f : (gainRange <= 12.0f ? 3.0f : 6.0f);
    g.setFont (Font (10.0f));
    for (float v = -gainRange; v <= gainRange + 0.001f; v += step)
    {
        const float y = gainToY (v);
        const bool zero = std::abs (v) < 0.001f;
        g.setColour (Colours::white.withAlpha (zero ? 0.28f : a));
        g.drawLine (plot.getX(), y, plot.getRight(), y, zero ? 1.4f : 1.0f);

        g.setColour (Colours::white.withAlpha (0.4f));
        g.drawText ((v > 0.0f ? "+" : "") + String ((int) v), Rectangle<float> (plot.getX() + 3.0f, y - 12.0f, 30.0f, 11.0f),
                    Justification::centredLeft);
    }

    static const float labelF[] = { 20, 50, 100, 200, 500, 1000, 2000, 5000, 10000, 20000 };
    static const char* labelT[] = { "20", "50", "100", "200", "500", "1k", "2k", "5k", "10k", "20k" };
    g.setColour (Colours::white.withAlpha (0.5f));
    for (int i = 0; i < 10; ++i)
    {
        const float x = jlimit (plot.getX() + 14.0f, plot.getRight() - 14.0f, freqToX (labelF[i]));
        g.drawText (labelT[i], Rectangle<float> (x - 16.0f, plot.getBottom() - 13.0f, 32.0f, 12.0f), Justification::centred);
    }
}

void CrazyEQAudioProcessorEditor::paintPiano (Graphics& g)
{
    const auto pa = pianoArea;
    const int selMidi = (int) std::round (69.0f + 12.0f * std::log2 (bands[(size_t) selected].f / 440.0f));
    int hovMidi = -1;
    if (mouseInPlot)
        hovMidi = (int) std::round (69.0f + 12.0f * std::log2 (xToFreq (mousePos.x) / 440.0f));

    const float semi = std::pow (2.0f, 1.0f / 24.0f);
    g.setFont (Font (8.0f));

    for (int m = 12; m <= 131; ++m)
    {
        const float f = 440.0f * std::pow (2.0f, ((float) m - 69.0f) / 12.0f);
        if (f * semi < 20.0f || f / semi > 20000.0f)
            continue;

        const float x0 = jmax (pa.getX(), freqToX (f / semi));
        const float x1 = jmin (pa.getRight(), freqToX (f * semi));
        if (x1 <= x0)
            continue;

        const int pc = m % 12;
        const bool black = (pc == 1 || pc == 3 || pc == 6 || pc == 8 || pc == 10);
        const Rectangle<float> k (x0, pa.getY(), x1 - x0, pa.getHeight() * (black ? 0.62f : 1.0f));

        Colour c = black ? Colour (0xff1b1e33) : Colour (0xffcfd6ff).withAlpha (0.20f);
        if (m == selMidi)      c = bandColour (selected);
        else if (m == hovMidi) c = Colours::white.withAlpha (0.8f);

        g.setColour (c);
        g.fillRect (k.reduced (0.4f, 0.0f));

        if (pc == 0 && (x1 - x0) > 1.0f)
        {
            g.setColour (Colours::white.withAlpha (0.6f));
            g.drawText (noteName (m), Rectangle<float> (k.getX() + 1.0f, pa.getY() + 3.0f, 22.0f, pa.getHeight() - 3.0f),
                        Justification::bottomLeft, false);
        }
    }
}

void CrazyEQAudioProcessorEditor::paintSelectedBand (Graphics& g)
{
    const auto& sb = bands[(size_t) selected];
    if (! sb.en)
        return;

    const auto col = bandColour (selected);
    const float zeroY = gainToY (0.0f);

    Path line = curvePath (selCurve);
    Path area = line;
    area.lineTo (plot.getRight(), zeroY);
    area.lineTo (plot.getX(), zeroY);
    area.closeSubPath();

    g.setColour (col.withAlpha (0.20f));
    g.fillPath (area);
    g.setColour (col.withAlpha (0.9f));
    g.strokePath (line, PathStrokeType (1.6f));

    if (sb.type == (int) ceq::Bell || sb.type == (int) ceq::Notch)
    {
        const float k = 1.0f / (2.0f * sb.q);
        const float r = std::sqrt (1.0f + k * k);
        const float x0 = freqToX (sb.f * (r - k));
        const float x1 = freqToX (sb.f * (r + k));

        const auto region = Rectangle<float> (x0, plot.getY(), x1 - x0, plot.getHeight()).getIntersection (plot);
        g.setColour (col.withAlpha (0.07f + 0.05f * std::sin (time * 4.0f)));
        g.fillRect (region);
        g.setColour (col.withAlpha (0.5f));
        g.drawLine (x0, plot.getY(), x0, plot.getBottom(), 1.0f);
        g.drawLine (x1, plot.getY(), x1, plot.getBottom(), 1.0f);
    }
}

void CrazyEQAudioProcessorEditor::paintSpectrum (Graphics& g)
{
    if (! analyzerButton.getToggleState())
        return;

    auto makePath = [this] (const std::vector<float>& v, bool close)
    {
        Path p;
        for (int i = 0; i < numCols; ++i)
        {
            const float x = plot.getX() + plot.getWidth() * (float) i / (float) (numCols - 1);
            const float y = specToY (v[(size_t) i]);
            if (i == 0) p.startNewSubPath (x, y);
            else        p.lineTo (x, y);
        }
        if (close)
        {
            p.lineTo (plot.getRight(), plot.getBottom());
            p.lineTo (plot.getX(), plot.getBottom());
            p.closeSubPath();
        }
        return p;
    };

    // Gefuellte Flaeche mit Regenbogen + vertikalem Fade
    const Path fill = makePath (specPost, true);
    g.saveState();
    g.reduceClipRegion (fill, AffineTransform());
    g.setGradientFill (rainbowGradient (plot, time, 0.75f));
    g.fillRect (plot);
    g.setGradientFill (ColourGradient (Colours::transparentBlack, 0.0f, plot.getY(),
                                       Colour (0xff05060d).withAlpha (0.92f), 0.0f, plot.getBottom(), false));
    g.fillRect (plot);
    g.restoreState();

    if (preButton.getToggleState())
    {
        g.setColour (Colours::white.withAlpha (0.28f));
        g.strokePath (makePath (specPre, false), PathStrokeType (1.0f));
    }

    g.setColour (Colours::white.withAlpha (0.35f));
    g.strokePath (makePath (specPeak, false), PathStrokeType (1.0f));

    g.setGradientFill (rainbowGradient (plot, time, 1.0f));
    g.strokePath (makePath (specPost, false), PathStrokeType (1.6f));
}

void CrazyEQAudioProcessorEditor::paintParticles (Graphics& g)
{
    for (const auto& p : particles)
    {
        const float a = std::pow (jlimit (0.0f, 1.0f, p.life), 1.5f);
        g.setColour (hsv (p.hue, 0.6f, 1.0f, a * 0.18f));
        g.fillEllipse (p.x - p.size * 1.8f, p.y - p.size * 1.8f, p.size * 3.6f, p.size * 3.6f);
        g.setColour (hsv (p.hue, 0.4f, 1.0f, a * 0.9f));
        g.fillEllipse (p.x - p.size * 0.5f, p.y - p.size * 0.5f, p.size, p.size);
    }
}

void CrazyEQAudioProcessorEditor::paintCurve (Graphics& g)
{
    const float zeroY = gainToY (0.0f);
    const Path line = curvePath (totalCurve);

    Path area = line;
    area.lineTo (plot.getRight(), zeroY);
    area.lineTo (plot.getX(), zeroY);
    area.closeSubPath();

    g.setGradientFill (rainbowGradient (plot, time, 0.20f));
    g.fillPath (area);

    const float boost = 0.6f + 0.8f * energy;
    for (float w : { 18.0f, 11.0f, 6.0f })
    {
        g.setGradientFill (rainbowGradient (plot, time, (0.03f + 0.10f * (1.0f - w / 18.0f)) * boost));
        g.strokePath (line, PathStrokeType (w, PathStrokeType::curved, PathStrokeType::rounded));
    }

    g.setColour (Colours::white.withAlpha (0.95f));
    g.strokePath (line, PathStrokeType (2.4f, PathStrokeType::curved, PathStrokeType::rounded));
}

void CrazyEQAudioProcessorEditor::paintScanner (Graphics& g)
{
    const float a = 0.35f * crazy;
    const float sx = plot.getX() + std::fmod (time * 0.11f, 1.0f) * plot.getWidth();
    const int idx = jlimit (0, numCurve - 1, (int) ((sx - plot.getX()) / plot.getWidth() * (float) (numCurve - 1)));
    const float sy = gainToY (totalCurve[(size_t) idx]);
    const auto col = hsv (time * 0.1f + 0.3f, 0.7f, 1.0f);

    g.setGradientFill (ColourGradient (col.withAlpha (0.0f), sx - 70.0f, 0.0f, col.withAlpha (a), sx, 0.0f, false));
    g.fillRect (sx - 70.0f, plot.getY(), 70.0f, plot.getHeight());

    g.setColour (col.withAlpha (a + 0.2f));
    g.drawLine (sx, plot.getY(), sx, plot.getBottom(), 1.5f);

    const float pulse = 5.0f + 2.5f * std::sin (time * 8.0f);
    g.setColour (col.withAlpha (0.25f * crazy));
    g.fillEllipse (sx - pulse * 2.0f, sy - pulse * 2.0f, pulse * 4.0f, pulse * 4.0f);
    g.setColour (Colours::white.withAlpha (0.4f + 0.6f * crazy));
    g.fillEllipse (sx - 3.0f, sy - 3.0f, 6.0f, 6.0f);
}

void CrazyEQAudioProcessorEditor::paintNodes (Graphics& g)
{
    for (int b = 0; b < ceq::numBands; ++b)
    {
        const auto& st = bands[(size_t) b];
        const auto pos = nodePos (b);
        const auto col = bandColour (b);
        const bool isSel = (b == selected);
        const bool isHov = (b == hoverBand);

        if (! st.en)
        {
            g.setColour (col.withAlpha (0.35f));
            g.drawEllipse (pos.x - 8.0f, pos.y - 8.0f, 16.0f, 16.0f, 1.5f);
            g.setFont (Font (10.0f, Font::bold));
            g.setColour (col.withAlpha (0.55f));
            g.drawText (String (b + 1), Rectangle<float> (pos.x - 8.0f, pos.y - 8.0f, 16.0f, 16.0f), Justification::centred);
            continue;
        }

        const float rad = (isHov || isSel) ? 11.0f : 9.5f;

        for (float gr : { 28.0f, 21.0f, 15.0f })
        {
            g.setColour (col.withAlpha (0.05f + (isSel ? 0.04f : 0.0f)));
            g.fillEllipse (pos.x - gr, pos.y - gr, gr * 2.0f, gr * 2.0f);
        }

        const float rp = std::fmod (time * 0.6f + (float) b * 0.125f, 1.0f);
        const float rr = 10.0f + rp * 26.0f;
        g.setColour (col.withAlpha ((1.0f - rp) * 0.4f * crazy));
        g.drawEllipse (pos.x - rr, pos.y - rr, rr * 2.0f, rr * 2.0f, 1.5f);

        g.setColour (col);
        g.fillEllipse (pos.x - rad, pos.y - rad, rad * 2.0f, rad * 2.0f);
        g.setColour (Colours::white.withAlpha (0.85f));
        g.drawEllipse (pos.x - rad, pos.y - rad, rad * 2.0f, rad * 2.0f, 1.5f);

        g.setFont (Font (11.0f, Font::bold));
        g.setColour (Colour (0xff05060d));
        g.drawText (String (b + 1), Rectangle<float> (pos.x - rad, pos.y - rad, rad * 2.0f, rad * 2.0f), Justification::centred);

        if (isSel)
        {
            Path arcs;
            const float rot = time * 2.5f;
            arcs.addCentredArc (pos.x, pos.y, 17.0f, 17.0f, 0.0f, rot, rot + 1.6f, true);
            arcs.addCentredArc (pos.x, pos.y, 17.0f, 17.0f, 0.0f, rot + kPi, rot + kPi + 1.6f, true);
            g.setColour (Colours::white.withAlpha (0.9f));
            g.strokePath (arcs, PathStrokeType (2.0f));
        }
    }
}

void CrazyEQAudioProcessorEditor::paintRipples (Graphics& g)
{
    for (const auto& r : ripples)
    {
        const float radius = r.age * 130.0f;
        const float a = (1.0f - r.age / 1.2f) * 0.6f;
        g.setColour (hsv (r.hue, 0.7f, 1.0f, a));
        g.drawEllipse (r.x - radius, r.y - radius, radius * 2.0f, radius * 2.0f, 2.5f);
        g.setColour (hsv (r.hue, 0.5f, 1.0f, a * 0.4f));
        g.drawEllipse (r.x - radius * 0.7f, r.y - radius * 0.7f, radius * 1.4f, radius * 1.4f, 1.5f);
    }
}

void CrazyEQAudioProcessorEditor::paintHover (Graphics& g)
{
    if (! mouseInPlot)
        return;

    const float mx = mousePos.x, my = mousePos.y;
    g.setColour (Colours::white.withAlpha (0.22f));
    g.drawLine (mx, plot.getY(), mx, plot.getBottom(), 1.0f);
    g.drawLine (plot.getX(), my, plot.getRight(), my, 1.0f);

    const float f = xToFreq (mx);
    const float midiF = 69.0f + 12.0f * std::log2 (f / 440.0f);
    const int midi = (int) std::round (midiF);
    const int cents = (int) std::round ((midiF - (float) midi) * 100.0f);
    const int idx = jlimit (0, numCurve - 1, (int) ((mx - plot.getX()) / plot.getWidth() * (float) (numCurve - 1)));

    const String fs = f >= 1000.0f ? String (f / 1000.0f, 2) + " kHz" : String (f, 1) + " Hz";
    const String text = fs + "   " + noteName (midi) + " " + (cents >= 0 ? "+" : "") + String (cents) + "c   "
                        + String (totalCurve[(size_t) idx], 1) + " dB";

    const Font font (12.0f, Font::bold);
    g.setFont (font);
    const float tw = font.getStringWidthFloat (text) + 16.0f;

    float bx = mx + 14.0f;
    if (bx + tw > plot.getRight())
        bx = mx - 14.0f - tw;
    const float by = jlimit (plot.getY() + 4.0f, plot.getBottom() - 26.0f, my - 30.0f);

    const Rectangle<float> pill (bx, by, tw, 22.0f);
    g.setColour (Colour (0xff0a0c18).withAlpha (0.88f));
    g.fillRoundedRectangle (pill, 6.0f);
    g.setColour (Colours::white.withAlpha (0.35f));
    g.drawRoundedRectangle (pill, 6.0f, 1.0f);
    g.setColour (Colours::white);
    g.drawText (text, pill, Justification::centred, false);
}

void CrazyEQAudioProcessorEditor::paintMeters (Graphics& g)
{
    const auto a = meterArea;
    g.setColour (Colours::white.withAlpha (0.04f));
    g.fillRoundedRectangle (a, 10.0f);
    g.setColour (Colours::white.withAlpha (0.10f));
    g.drawRoundedRectangle (a.reduced (0.5f), 10.0f, 1.0f);

    g.setFont (Font (11.0f, Font::bold));
    g.setColour (Colours::white.withAlpha (0.55f));
    g.drawText ("LEVEL", a.withHeight (20.0f), Justification::centred);

    const auto inner = a.reduced (12.0f, 0.0f).withTrimmedTop (24.0f).withTrimmedBottom (26.0f);
    const float bw = (inner.getWidth() - 8.0f) * 0.5f;
    const int nSeg = 36;
    const float segH = inner.getHeight() / (float) nSeg;

    for (int c = 0; c < 2; ++c)
    {
        const float x = inner.getX() + (float) c * (bw + 8.0f);

        for (int k = 0; k < nSeg; ++k)
        {
            const float segDb = -60.0f + 66.0f * ((float) k + 0.5f) / (float) nSeg;
            const float y = inner.getBottom() - (float) (k + 1) * segH;
            const bool lit = meterDb[c] >= segDb;

            const Colour col = segDb > 0.0f ? Colour (0xffff2d55)
                             : segDb > -9.0f ? Colour (0xffffd400)
                             : Colour (0xff1affb0).interpolatedWith (Colour (0xff00c8ff), (segDb + 60.0f) / 51.0f);

            g.setColour (lit ? col : col.withAlpha (0.08f));
            g.fillRect (x, y + 1.0f, bw, segH - 1.5f);
        }

        const float hy = inner.getBottom() - jlimit (0.0f, 1.0f, (holdDb[c] + 60.0f) / 66.0f) * inner.getHeight();
        g.setColour (Colours::white.withAlpha (0.9f));
        g.fillRect (x, hy - 1.0f, bw, 2.0f);
    }

    const float pk = jmax (holdDb[0], holdDb[1]);
    g.setColour (clipHold > 0 ? Colour (0xffff2d55) : Colours::white.withAlpha (0.8f));
    g.drawText (pk <= -99.0f ? String ("-inf") : String (pk, 1) + " dB",
                a.withTrimmedTop (a.getHeight() - 24.0f), Justification::centred);
}

void CrazyEQAudioProcessorEditor::paintPanel (Graphics& g)
{
    g.setColour (Colours::white.withAlpha (0.04f));
    g.fillRoundedRectangle (bottomArea, 10.0f);
    g.setColour (Colours::white.withAlpha (0.10f));
    g.drawRoundedRectangle (bottomArea.reduced (0.5f), 10.0f, 1.0f);

    g.setFont (Font (11.0f, Font::bold));
    g.setColour (Colours::white.withAlpha (0.55f));

    auto labelAbove = [&] (Component& c, const char* text)
    {
        g.drawText (text, Rectangle<int> (c.getX(), c.getY() - 14, c.getWidth(), 14), Justification::centred);
    };
    labelAbove (freqSlider, "FREQUENCY");
    labelAbove (gainSlider, "GAIN");
    labelAbove (qSlider, "Q / WIDTH");
    labelAbove (outSlider, "OUTPUT");
    g.drawText ("FILTER TYPE", Rectangle<int> (typeBox.getX(), typeBox.getY() - 14, typeBox.getWidth(), 14), Justification::centredLeft);

    const auto& st = bands[(size_t) selected];
    g.setColour (bandColour (selected));
    g.setFont (Font (14.0f, Font::bold));
    g.drawText ("BAND " + String (selected + 1) + "  -  " + ceq::typeName (st.type) + (st.en ? "" : "  (off)"),
                Rectangle<int> (bandButtons[ceq::numBands - 1].getRight() + 14, bandButtons[0].getY(), 360, 24),
                Justification::centredLeft);

    g.setFont (Font (10.0f));
    g.setColour (Colours::white.withAlpha (0.30f));
    g.drawText ("Drag: Freq/Gain  |  Wheel: Q  |  Double-click: add/remove  |  Right-click: type menu",
                Rectangle<int> (getLocalBounds().getRight() - 14 - 520, bandButtons[0].getY(), 520, 24),
                Justification::centredRight);
}
