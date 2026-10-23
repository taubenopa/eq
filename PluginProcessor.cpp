#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace ceq
{
    static Coeffs::Ptr identityCoeffs()
    {
        return Coeffs::Ptr (new Coeffs (1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f));
    }

    Coeffs::Ptr makeStage (int type, double sr, float freq, float q, float gainDb, int stage)
    {
        freq = juce::jlimit (minFreq, (float) (sr * 0.45), freq);
        q = juce::jmax (0.05f, q);
        const float gain = juce::Decibels::decibelsToGain (gainDb);

        switch (type)
        {
            case Bell:
                return stage == 0 ? Coeffs::makePeakFilter (sr, freq, q, gain) : identityCoeffs();
            case LowShelf:
                return stage == 0 ? Coeffs::makeLowShelf (sr, freq, q, gain) : identityCoeffs();
            case HighShelf:
                return stage == 0 ? Coeffs::makeHighShelf (sr, freq, q, gain) : identityCoeffs();
            case HighPass12:
                return stage == 0 ? Coeffs::makeHighPass (sr, freq, q) : identityCoeffs();
            case HighPass24:
                return Coeffs::makeHighPass (sr, freq, stage == 0 ? 0.5412f : 1.3066f);
            case LowPass12:
                return stage == 0 ? Coeffs::makeLowPass (sr, freq, q) : identityCoeffs();
            case LowPass24:
                return Coeffs::makeLowPass (sr, freq, stage == 0 ? 0.5412f : 1.3066f);
            case Notch:
                return stage == 0 ? Coeffs::makeNotch (sr, freq, q) : identityCoeffs();
            default:
                break;
        }
        return identityCoeffs();
    }
}

namespace
{
    juce::AudioParameterFloatAttributes freqAttr()
    {
        return juce::AudioParameterFloatAttributes()
            .withStringFromValueFunction ([] (float v, int) -> juce::String
                                          {
                                              return v >= 1000.0f ? juce::String (v / 1000.0f, 2) + " kHz"
                                                                  : juce::String (v, 1) + " Hz";
                                          })
            .withValueFromStringFunction ([] (const juce::String& s) -> float
                                          {
                                              auto t = s.trim().toLowerCase();
                                              float v = t.getFloatValue();
                                              if (t.contains ("k"))
                                                  v *= 1000.0f;
                                              return v;
                                          });
    }

    juce::AudioParameterFloatAttributes dbAttr()
    {
        return juce::AudioParameterFloatAttributes()
            .withStringFromValueFunction ([] (float v, int) -> juce::String { return juce::String (v, 1) + " dB"; })
            .withValueFromStringFunction ([] (const juce::String& s) -> float { return s.getFloatValue(); });
    }

    juce::AudioParameterFloatAttributes qAttr()
    {
        return juce::AudioParameterFloatAttributes()
            .withStringFromValueFunction ([] (float v, int) -> juce::String { return juce::String (v, 2); })
            .withValueFromStringFunction ([] (const juce::String& s) -> float { return s.getFloatValue(); });
    }
}

juce::AudioProcessorValueTreeState::ParameterLayout CrazyEQAudioProcessor::createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    const float defFreq[ceq::numBands] = { 40.0f, 110.0f, 250.0f, 600.0f, 1300.0f, 2800.0f, 6000.0f, 12000.0f };

    juce::StringArray types;
    for (int t = 0; t < (int) ceq::numTypes; ++t)
        types.add (ceq::typeName (t));

    juce::NormalisableRange<float> freqRange (ceq::minFreq, ceq::maxFreq, 0.01f);
    freqRange.setSkewForCentre (1000.0f);

    juce::NormalisableRange<float> qRange (0.1f, 18.0f, 0.01f);
    qRange.setSkewForCentre (1.0f);

    for (int b = 0; b < ceq::numBands; ++b)
    {
        const int defType = b == 0 ? (int) ceq::LowShelf : (b == ceq::numBands - 1 ? (int) ceq::HighShelf : (int) ceq::Bell);
        const float defQ = (defType == (int) ceq::Bell) ? 1.0f : 0.71f;
        const juce::String n = "Band " + juce::String (b + 1) + " ";

        layout.add (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { ceq::pid (b, "on"), 1 }, n + "On", true));
        layout.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { ceq::pid (b, "type"), 1 }, n + "Type", types, defType));
        layout.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { ceq::pid (b, "freq"), 1 }, n + "Freq", freqRange, defFreq[b], freqAttr()));
        layout.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { ceq::pid (b, "gain"), 1 }, n + "Gain",
                                                                 juce::NormalisableRange<float> (-24.0f, 24.0f, 0.01f), 0.0f, dbAttr()));
        layout.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { ceq::pid (b, "q"), 1 }, n + "Q", qRange, defQ, qAttr()));
    }

    layout.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "out_gain", 1 }, "Output Gain",
                                                             juce::NormalisableRange<float> (-24.0f, 24.0f, 0.01f), 0.0f, dbAttr()));
    return layout;
}

CrazyEQAudioProcessor::CrazyEQAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "CrazyEQState", createLayout())
{
    for (int b = 0; b < ceq::numBands; ++b)
    {
        freqP[b]    = apvts.getRawParameterValue (ceq::pid (b, "freq"));
        gainP[b]    = apvts.getRawParameterValue (ceq::pid (b, "gain"));
        qP[b]       = apvts.getRawParameterValue (ceq::pid (b, "q"));
        typeP[b]    = apvts.getRawParameterValue (ceq::pid (b, "type"));
        enabledP[b] = apvts.getRawParameterValue (ceq::pid (b, "on"));
    }
    outP = apvts.getRawParameterValue ("out_gain");
}

void CrazyEQAudioProcessor::prepareToPlay (double newSampleRate, int)
{
    sampleRate = newSampleRate;

    for (int b = 0; b < ceq::numBands; ++b)
    {
        cache[b] = Cache();
        wasEnabled[b] = false;
        for (int c = 0; c < 2; ++c)
            for (int s = 0; s < 2; ++s)
                filt[b][c][s].reset();
    }

    lastOutGain = juce::Decibels::decibelsToGain (outP->load());
}

bool CrazyEQAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::stereo() && out != juce::AudioChannelSet::mono())
        return false;
    return out == layouts.getMainInputChannelSet();
}

void CrazyEQAudioProcessor::updateFilters()
{
    for (int b = 0; b < ceq::numBands; ++b)
    {
        const float f = freqP[b]->load();
        const float g = gainP[b]->load();
        const float q = qP[b]->load();
        const int type = (int) typeP[b]->load();

        auto& c = cache[b];
        if (c.f == f && c.g == g && c.q == q && c.type == type)
            continue;

        c.f = f;
        c.g = g;
        c.q = q;
        c.type = type;

        for (int s = 0; s < 2; ++s)
        {
            auto coeffs = ceq::makeStage (type, sampleRate, f, q, g, s);
            for (int ch = 0; ch < 2; ++ch)
                filt[b][ch][s].coefficients = coeffs;
        }

        stage2[b] = (type == (int) ceq::HighPass24 || type == (int) ceq::LowPass24);
    }
}

static void pushToRing (SampleRing& ring, const juce::AudioBuffer<float>& buffer, int numCh, int n)
{
    if (numCh <= 0)
        return;

    const float inv = 1.0f / (float) numCh;
    for (int i = 0; i < n; ++i)
    {
        float m = 0.0f;
        for (int c = 0; c < numCh; ++c)
            m += buffer.getReadPointer (c)[i];
        ring.push (m * inv);
    }
}

void CrazyEQAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int n = buffer.getNumSamples();
    const int numCh = juce::jmin (buffer.getNumChannels(), 2);

    for (int c = numCh; c < buffer.getNumChannels(); ++c)
        buffer.clear (c, 0, n);

    updateFilters();
    pushToRing (preRing, buffer, numCh, n);

    for (int b = 0; b < ceq::numBands; ++b)
    {
        const bool en = enabledP[b]->load() > 0.5f;
        if (! en)
        {
            wasEnabled[b] = false;
            continue;
        }

        if (! wasEnabled[b])
        {
            for (int c = 0; c < 2; ++c)
                for (int s = 0; s < 2; ++s)
                    filt[b][c][s].reset();
            wasEnabled[b] = true;
        }

        const bool two = stage2[b];
        for (int c = 0; c < numCh; ++c)
        {
            auto* d = buffer.getWritePointer (c);
            auto& f0 = filt[b][c][0];
            auto& f1 = filt[b][c][1];

            if (two)
                for (int i = 0; i < n; ++i)
                    d[i] = f1.processSample (f0.processSample (d[i]));
            else
                for (int i = 0; i < n; ++i)
                    d[i] = f0.processSample (d[i]);
        }
    }

    const float g = juce::Decibels::decibelsToGain (outP->load());
    buffer.applyGainRamp (0, n, lastOutGain, g);
    lastOutGain = g;

    pushToRing (postRing, buffer, numCh, n);

    if (numCh > 0)
        peakL.store (juce::jmax (peakL.load(), buffer.getMagnitude (0, 0, n)));
    if (numCh > 1)
        peakR.store (juce::jmax (peakR.load(), buffer.getMagnitude (1, 0, n)));
    else if (numCh == 1)
        peakR.store (peakL.load());
}

juce::AudioProcessorEditor* CrazyEQAudioProcessor::createEditor()
{
    return new CrazyEQAudioProcessorEditor (*this);
}

void CrazyEQAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void CrazyEQAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));
    if (xml != nullptr && xml->hasTagName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new CrazyEQAudioProcessor();
}
