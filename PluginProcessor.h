#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <atomic>

namespace ceq
{
    constexpr int numBands = 8;
    constexpr float minFreq = 20.0f;
    constexpr float maxFreq = 20000.0f;

    enum BandType
    {
        Bell = 0,
        LowShelf,
        HighShelf,
        HighPass12,
        HighPass24,
        LowPass12,
        LowPass24,
        Notch,
        numTypes
    };

    inline const char* typeName (int t)
    {
        static const char* names[] = { "Bell", "Low Shelf", "High Shelf",
                                       "High-Pass 12", "High-Pass 24",
                                       "Low-Pass 12", "Low-Pass 24", "Notch" };
        return names[juce::jlimit (0, (int) numTypes - 1, t)];
    }

    inline bool typeHasGain (int t)
    {
        return t == Bell || t == LowShelf || t == HighShelf;
    }

    // Parameter-IDs: b1_freq, b1_gain, b1_q, b1_type, b1_on ...
    inline juce::String pid (int band, const juce::String& what)
    {
        return "b" + juce::String (band + 1) + "_" + what;
    }

    using Coeffs = juce::dsp::IIR::Coefficients<float>;

    // Liefert die Koeffizienten einer Biquad-Stufe (stage 0 oder 1).
    // 24-dB-Filter nutzen beide Stufen (Butterworth), alle anderen nur Stufe 0.
    Coeffs::Ptr makeStage (int type, double sampleRate, float freq, float q, float gainDb, int stage);
}

// Sehr einfacher Ringpuffer fuer den Spektrum-Analyzer (Audio-Thread schreibt, GUI liest)
class SampleRing
{
public:
    static constexpr int size = 1 << 15;

    SampleRing()
    {
        for (auto& s : buf)
            s.store (0.0f, std::memory_order_relaxed);
    }

    void push (float x) noexcept
    {
        const auto w = pos.load (std::memory_order_relaxed);
        buf[w & (size - 1)].store (x, std::memory_order_relaxed);
        pos.store (w + 1, std::memory_order_release);
    }

    void copyLatest (float* dest, int n) const noexcept
    {
        const auto w = pos.load (std::memory_order_acquire);
        for (int i = 0; i < n; ++i)
            dest[i] = buf[(w - (unsigned int) n + (unsigned int) i) & (size - 1)].load (std::memory_order_relaxed);
    }

private:
    std::array<std::atomic<float>, size> buf;
    std::atomic<unsigned int> pos { 0 };
};

class CrazyEQAudioProcessor : public juce::AudioProcessor
{
public:
    CrazyEQAudioProcessor();
    ~CrazyEQAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Crazy Visual EQ"; }
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

    double getCurrentSampleRate() const { return sampleRate > 0.0 ? sampleRate : 44100.0; }

    juce::AudioProcessorValueTreeState apvts;

    SampleRing preRing, postRing;
    std::atomic<float> peakL { 0.0f }, peakR { 0.0f };

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    void updateFilters();

    double sampleRate = 44100.0;

    std::atomic<float>* freqP[ceq::numBands] {};
    std::atomic<float>* gainP[ceq::numBands] {};
    std::atomic<float>* qP[ceq::numBands] {};
    std::atomic<float>* typeP[ceq::numBands] {};
    std::atomic<float>* enabledP[ceq::numBands] {};
    std::atomic<float>* outP = nullptr;

    struct Cache
    {
        float f = -1.0f, g = -999.0f, q = -1.0f;
        int type = -1;
    };

    Cache cache[ceq::numBands];
    bool wasEnabled[ceq::numBands] {};
    bool stage2[ceq::numBands] {};

    juce::dsp::IIR::Filter<float> filt[ceq::numBands][2][2];

    float lastOutGain = 1.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CrazyEQAudioProcessor)
};
