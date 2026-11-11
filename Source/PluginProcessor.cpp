#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
namespace
{
    inline float safetyCeiling (float x)
    {
        constexpr float knee = 0.9f, ceil = 0.999f;
        const float a = std::abs (x);
        if (a <= knee) return x;
        const float o = knee + (ceil - knee) * std::tanh ((a - knee) / (ceil - knee));
        return x > 0.0f ? o : -o;
    }
}

const juce::StringArray& ShredMaxxProcessor::ratioNames()
{
    static const juce::StringArray names { "1:1", "2:1", "3:1", "4:1", "6:1", "10:1", "20:1", "KABOOM" };
    return names;
}

ShredMaxxProcessor::ShredMaxxProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createLayout())
{
    pInput    = apvts.getRawParameterValue ("input");
    pAttack   = apvts.getRawParameterValue ("attack");
    pRelease  = apvts.getRawParameterValue ("release");
    pOutput   = apvts.getRawParameterValue ("output");
    pHeadroom = apvts.getRawParameterValue ("headroom");
    pMix      = apvts.getRawParameterValue ("mix");
    pRatio    = apvts.getRawParameterValue ("ratio");
    pScHpf    = apvts.getRawParameterValue ("schpf");
    pBand     = apvts.getRawParameterValue ("bandbeater");
    pLink     = apvts.getRawParameterValue ("link");
    pAudioHpf = apvts.getRawParameterValue ("audiohpf");
    pDist2    = apvts.getRawParameterValue ("dist2");
    pDist3    = apvts.getRawParameterValue ("dist3");
}

juce::AudioProcessorValueTreeState::ParameterLayout ShredMaxxProcessor::createLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout layout;

    auto dB = [] (int decimals)
    {
        return AudioParameterFloatAttributes().withStringFromValueFunction (
            [decimals] (float v, int) { return (v > 0.0f ? "+" : "") + String (v, decimals) + " dB"; });
    };
    auto timeAttr = AudioParameterFloatAttributes().withStringFromValueFunction ([] (float ms, int)
    {
        if (ms < 1.0f)     return String (roundToInt (ms * 1000.0f)) + " us";
        if (ms < 10.0f)    return String (ms, 1) + " ms";
        if (ms < 1000.0f)  return String (roundToInt (ms)) + " ms";
        return String (ms / 1000.0f, 2) + " s";
    });

    auto logRange = [] (float lo, float hi)
    {
        NormalisableRange<float> r (lo, hi,
            [] (float a, float b, float t) { return a * std::pow (b / a, t); },
            [] (float a, float b, float v) { return std::log (v / a) / std::log (b / a); });
        return r;
    };

    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "input", 1 }, "Input",
                    NormalisableRange<float> (-12.0f, 36.0f, 0.1f), 0.0f, dB (1)));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "attack", 1 }, "Attack",
                    logRange (0.05f, 30.0f), 3.0f, timeAttr));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "release", 1 }, "Release",
                    logRange (50.0f, 3500.0f), 300.0f, timeAttr));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "output", 1 }, "Output",
                    NormalisableRange<float> (-24.0f, 24.0f, 0.1f), 0.0f, dB (1)));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "headroom", 1 }, "Headroom",
                    NormalisableRange<float> (4.0f, 28.0f, 0.1f), 18.0f, dB (1)));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "mix", 1 }, "Mix",
                    NormalisableRange<float> (0.0f, 100.0f, 0.1f), 100.0f,
                    AudioParameterFloatAttributes().withStringFromValueFunction (
                        [] (float v, int) { return String (roundToInt (v)) + "% wet"; })));
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { "ratio", 1 }, "Ratio", ratioNames(), 3));

    layout.add (std::make_unique<AudioParameterBool> (ParameterID { "schpf", 1 },      "Eardrum HPF 100 Hz",  false));
    layout.add (std::make_unique<AudioParameterBool> (ParameterID { "bandbeater", 1 }, "Eardrum BandBeater",  false));
    layout.add (std::make_unique<AudioParameterBool> (ParameterID { "link", 1 },       "Eardrum Stereo Link", true));
    layout.add (std::make_unique<AudioParameterBool> (ParameterID { "audiohpf", 1 },   "Audio HPF 80 Hz",     false));
    layout.add (std::make_unique<AudioParameterBool> (ParameterID { "dist2", 1 },      "Audio Dist Deux",     false));
    layout.add (std::make_unique<AudioParameterBool> (ParameterID { "dist3", 1 },      "Audio Dist Trois",    false));

    return layout;
}

//==============================================================================
bool ShredMaxxProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto in  = layouts.getMainInputChannelSet();
    const auto out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::stereo() && out != juce::AudioChannelSet::mono()) return false;
    return in == out;
}

void ShredMaxxProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    maxBlock = juce::jmax (32, samplesPerBlock);

    oversampler = std::make_unique<juce::dsp::Oversampling<float>> (
        2, 1, juce::dsp::Oversampling<float>::filterHalfBandFIREquiripple, true, true);
    oversampler->initProcessing ((size_t) maxBlock);
    oversampler->reset();

    engine.prepare ((float) (sampleRate * 2.0));

    latency = (int) std::round (oversampler->getLatencyInSamples());
    setLatencySamples (latency);

    work.setSize (2, maxBlock);
    dryDelay.setSize (2, latency + 1);
    dryDelay.clear();
    dryWrite = 0;

    sInput.reset (sampleRate, 0.03);
    sMix.reset (sampleRate, 0.03);
    sInput.setCurrentAndTargetValue (sm::dbToGain (pInput->load()));
    sMix.setCurrentAndTargetValue (pMix->load() * 0.01f);
    thdSmooth = 0.0f;
}

//==============================================================================
void ShredMaxxProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    const int numCh = juce::jmin (buffer.getNumChannels(), getTotalNumOutputChannels());
    if (n == 0 || numCh == 0 || oversampler == nullptr) return;

    float* L = buffer.getWritePointer (0);
    float* R = numCh > 1 ? buffer.getWritePointer (1) : nullptr;

    for (int start = 0; start < n; start += maxBlock)
    {
        const int len = juce::jmin (maxBlock, n - start);
        float* r = R != nullptr ? R + start : nullptr;
        processChunk (L + start, r, len);
    }
}

void ShredMaxxProcessor::processChunk (float* L, float* R, int n)
{
    const bool stereo = R != nullptr;

    sInput.setTargetValue (sm::dbToGain (pInput->load()));
    sMix.setTargetValue (pMix->load() * 0.01f);

    sm::Settings s;
    s.attackMs   = pAttack->load();
    s.releaseMs  = pRelease->load();
    s.outputDb   = pOutput->load();
    s.headroomDb = pHeadroom->load();
    s.mode       = juce::jlimit (0, sm::kNumModes - 1, (int) std::round (pRatio->load()));
    s.scHpf      = pScHpf->load()    > 0.5f;
    s.bandBeater = pBand->load()     > 0.5f;
    s.link       = pLink->load()     > 0.5f;
    s.audioHpf   = pAudioHpf->load() > 0.5f;
    s.dist2      = pDist2->load()    > 0.5f;
    s.dist3      = pDist3->load()    > 0.5f;

    // Copy in, apply Input gain, keep the untouched signal for the dry path
    float* wL = work.getWritePointer (0);
    float* wR = work.getWritePointer (1);
    float* dL = dryDelay.getWritePointer (0);
    float* dR = dryDelay.getWritePointer (1);
    const int dSize = dryDelay.getNumSamples();
    float inPeak = 0.0f;

    for (int i = 0; i < n; ++i)
    {
        const float xl = L[i], xr = stereo ? R[i] : L[i];
        inPeak = juce::jmax (inPeak, std::abs (xl), std::abs (xr));
        const float g = sInput.getNextValue();
        wL[i] = xl * g;
        wR[i] = xr * g;
    }

    // Oversampled compressor
    juce::dsp::AudioBlock<float> block (work.getArrayOfWritePointers(), 2, (size_t) n);
    auto up = oversampler->processSamplesUp (block);
    const auto meters = engine.process (up.getChannelPointer (0), up.getChannelPointer (1),
                                        (int) up.getNumSamples(), s);
    oversampler->processSamplesDown (block);

    // Dry/Wet mix (dry delayed to line up with the oversampler) and safety
    float outPeak = 0.0f;
    bool bad = false;
    for (int i = 0; i < n; ++i)
    {
        const float xl = L[i], xr = stereo ? R[i] : L[i];
        dL[dryWrite] = xl; dR[dryWrite] = xr;
        const int rd = (dryWrite + 1) % dSize;           // oldest sample = exactly 'latency' behind
        const float dryL = dL[rd], dryR = dR[rd];
        dryWrite = rd;

        const float m = sMix.getNextValue();
        float oL = dryL + m * (wL[i] - dryL);
        float oR = dryR + m * (wR[i] - dryR);

        if (! std::isfinite (oL) || ! std::isfinite (oR)) { bad = true; oL = oR = 0.0f; }
        oL = safetyCeiling (oL);    // transparent below -0.9 dBFS, never above 0 dBFS
        oR = safetyCeiling (oR);
        outPeak = juce::jmax (outPeak, std::abs (oL), std::abs (oR));

        if (stereo) { L[i] = oL; R[i] = oR; }
        else        { L[i] = 0.5f * (oL + oR); }
    }

    if (bad) { engine.reset(); oversampler->reset(); }

    // Meters
    meterGr.store (meters.maxGrDb);
    const float blockSec = (float) n / (float) getSampleRate();
    const float k = 1.0f - std::exp (-blockSec / 0.08f);
    thdSmooth += (meters.thd - thdSmooth) * k;
    meterThd.store (thdSmooth);
    meterOpto.store (meters.optoMemory);
    meterIn.store (inPeak);
    meterOut.store (outPeak);
}

//==============================================================================
juce::AudioProcessorEditor* ShredMaxxProcessor::createEditor()
{
    return new ShredMaxxEditor (*this);
}

void ShredMaxxProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void ShredMaxxProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ShredMaxxProcessor();
}
