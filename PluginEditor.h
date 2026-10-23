#pragma once

#include "PluginProcessor.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <array>
#include <vector>

// Neon-Look fuer Knobs und Buttons
class NeonLookAndFeel : public juce::LookAndFeel_V4
{
public:
    NeonLookAndFeel();

    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height,
                           float sliderPosProportional, float rotaryStartAngle,
                           float rotaryEndAngle, juce::Slider&) override;

    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour& backgroundColour,
                               bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;
};

class CrazyEQAudioProcessorEditor : public juce::AudioProcessorEditor,
                                    private juce::Timer
{
public:
    explicit CrazyEQAudioProcessorEditor (CrazyEQAudioProcessor&);
    ~CrazyEQAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;

private:
    using SliderAttachment   = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboBoxAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using ButtonAttachment   = juce::AudioProcessorValueTreeState::ButtonAttachment;

    static constexpr int fftOrder = 13;
    static constexpr int fftSize  = 1 << fftOrder;
    static constexpr int numCols  = 512;   // Spektrum-Spalten (log-Frequenzachse)
    static constexpr int numCurve = 360;   // Punkte der EQ-Kurve

    struct Band     { bool en = true; int type = 0; float f = 1000.0f, g = 0.0f, q = 1.0f; };
    struct Particle { float x, y, vx, vy, life, decay, hue, size; };
    struct Ripple   { float x, y, age, hue; };
    struct Star     { float x, y, phase; };

    // --- Logik
    void timerCallback() override;
    void select (int band);
    Band readBand (int b) const;
    void updateBands();
    float bandDb (int b, float freq) const;
    void updateSpectrum (const SampleRing& ring, std::vector<float>& out);
    void updateMeters();
    void updateParticles (float dt);
    void spawnBurst (float x, float y, float hue, int n);
    void spawnRipple (float x, float y, float hue);

    // --- Koordinaten
    float freqToX (float f) const;
    float xToFreq (float x) const;
    float gainToY (float db) const;
    float yToGain (float y) const;
    float specToY (float db) const;
    juce::Point<float> nodePos (int b) const;
    int hitTest (juce::Point<float> p) const;
    juce::Path curvePath (const std::vector<float>& dbValues) const;

    // --- Parameter-Helfer
    void setParam (const juce::String& id, float plainValue);
    void setParamGesture (const juce::String& id, float plainValue);
    void resetParam (const juce::String& id);
    void beginGestures (int b);
    void endGestures (int b);
    void showBandMenu (int b);

    static juce::Colour bandColour (int b);

    // --- Zeichnen
    void paintBackground (juce::Graphics&);
    void paintHeader (juce::Graphics&);
    void paintGrid (juce::Graphics&);
    void paintPiano (juce::Graphics&);
    void paintSelectedBand (juce::Graphics&);
    void paintSpectrum (juce::Graphics&);
    void paintParticles (juce::Graphics&);
    void paintCurve (juce::Graphics&);
    void paintScanner (juce::Graphics&);
    void paintNodes (juce::Graphics&);
    void paintRipples (juce::Graphics&);
    void paintHover (juce::Graphics&);
    void paintMeters (juce::Graphics&);
    void paintPanel (juce::Graphics&);

    // --- Daten
    CrazyEQAudioProcessor& proc;
    NeonLookAndFeel laf;

    juce::dsp::FFT fft;
    juce::dsp::WindowingFunction<float> window;

    std::vector<float> fftData, specPost, specPre, specPeak;
    std::vector<float> specLoF, specHiF, specCenF;
    std::vector<float> curveFreq, totalCurve, selCurve;

    std::array<Band, ceq::numBands> bands;
    std::array<std::array<ceq::Coeffs::Ptr, 2>, ceq::numBands> coeffs;
    bool prevEn[ceq::numBands] = {};

    std::vector<Particle> particles;
    std::vector<Ripple> ripples;
    std::vector<Star> stars;
    juce::Random rng;

    float time = 0.0f, energy = 0.0f, crazy = 1.0f, gainRange = 18.0f;
    int rangeIdx = 2;

    float meterLin[2] = { 0.0f, 0.0f };
    float meterDb[2]  = { -100.0f, -100.0f };
    float holdDb[2]   = { -100.0f, -100.0f };
    int clipHold = 0;

    int selected = 0, dragBand = -1, hoverBand = -1;
    float dragLastY = 0.0f;
    juce::Point<float> mousePos;
    bool mouseInPlot = false;

    juce::Rectangle<float> graphArea, plot, pianoArea, meterArea, bottomArea, headerArea;

    // --- Komponenten
    juce::TextButton analyzerButton { "ANALYZER" }, preButton { "PRE-EQ" }, crazyButton { "CRAZY" },
                     rangeButton { "RANGE 18 dB" }, resetAllButton { "RESET ALL" };
    juce::TextButton bandButtons[ceq::numBands];
    juce::TextButton enableButton { "BAND ON" };
    juce::ComboBox typeBox;
    juce::Slider freqSlider, gainSlider, qSlider, outSlider;

    // Attachments muessen NACH den Komponenten deklariert sein (Zerstoerungsreihenfolge)
    std::unique_ptr<SliderAttachment> freqAtt, gainAtt, qAtt, outAtt;
    std::unique_ptr<ComboBoxAttachment> typeAtt;
    std::unique_ptr<ButtonAttachment> enAtt;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CrazyEQAudioProcessorEditor)
};
