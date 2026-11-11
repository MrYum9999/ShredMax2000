#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <atomic>
#include "CompressorEngine.h"

//==============================================================================
// ShredMaxx2000: a character compressor. Fixed internal threshold, Input drives
// into compression and saturation, Output is a Class-A makeup amp.
// The engine runs at 2x oversampling so the fast VCA and the harmonic circuits
// stay clean of aliasing.
//==============================================================================
class ShredMaxxProcessor : public juce::AudioProcessor
{
public:
    ShredMaxxProcessor();
    ~ShredMaxxProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override                     { return true; }

    const juce::String getName() const override         { return JucePlugin_Name; }
    bool acceptsMidi() const override                   { return false; }
    bool producesMidi() const override                  { return false; }
    bool isMidiEffect() const override                  { return false; }
    double getTailLengthSeconds() const override        { return 0.0; }

    int getNumPrograms() override                       { return 1; }
    int getCurrentProgram() override                    { return 0; }
    void setCurrentProgram (int) override               {}
    const juce::String getProgramName (int) override    { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    static const juce::StringArray& ratioNames();
    juce::AudioProcessorValueTreeState apvts;

    // Read by the editor for the meters and lights
    std::atomic<float> meterGr    { 0.0f };   // dB of gain reduction
    std::atomic<float> meterThd   { 0.0f };   // fraction
    std::atomic<float> meterOpto  { 0.0f };   // 0..1
    std::atomic<float> meterIn    { 0.0f };   // peak, linear
    std::atomic<float> meterOut   { 0.0f };

private:
    void processChunk (float* L, float* R, int n);

    std::unique_ptr<juce::dsp::Oversampling<float>> oversampler;
    sm::Engine engine;
    juce::AudioBuffer<float> work, dryDelay;
    int latency = 0, dryWrite = 0, maxBlock = 512;

    juce::SmoothedValue<float> sInput, sMix;
    float thdSmooth = 0.0f;

    std::atomic<float>* pInput    = nullptr;
    std::atomic<float>* pAttack   = nullptr;
    std::atomic<float>* pRelease  = nullptr;
    std::atomic<float>* pOutput   = nullptr;
    std::atomic<float>* pHeadroom = nullptr;
    std::atomic<float>* pMix      = nullptr;
    std::atomic<float>* pRatio    = nullptr;
    std::atomic<float>* pScHpf    = nullptr;
    std::atomic<float>* pBand     = nullptr;
    std::atomic<float>* pLink     = nullptr;
    std::atomic<float>* pAudioHpf = nullptr;
    std::atomic<float>* pDist2    = nullptr;
    std::atomic<float>* pDist3    = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ShredMaxxProcessor)
};
