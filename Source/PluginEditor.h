#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <array>
#include "PluginProcessor.h"

//==============================================================================
class AmpLookAndFeel : public juce::LookAndFeel_V4
{
public:
    static juce::Font font (float height, const juce::StringArray& families, const juce::String& style = "Bold");
    static juce::Font stencil (float height);   // panel lettering
    static juce::Font marker (float height);    // handwriting on gaffer tape

    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h,
                           float pos, float startAngle, float endAngle, juce::Slider&) override;
    void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool highlighted, bool down) override;
};

//==============================================================================
class ShredMaxxEditor : public juce::AudioProcessorEditor,
                        private juce::Timer
{
public:
    explicit ShredMaxxEditor (ShredMaxxProcessor&);
    ~ShredMaxxEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    struct Knob
    {
        juce::Slider slider;
        juce::String label;
        juce::Point<float> centre;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
        juce::String lastText;
    };

    struct Switch
    {
        juce::ToggleButton button;
        juce::String title, subtitle;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> attachment;
    };

    void setupKnob (Knob&, const juce::String& paramId, const juce::String& label, float centreX, bool big = false);
    void setupSwitch (Switch&, const juce::String& paramId, const juce::String& title, const juce::String& subtitle);
    void paintBackground();
    void drawVu (juce::Graphics&);
    void drawLamps (juce::Graphics&);
    void drawTape (juce::Graphics&, Knob&, int index);
    void timerCallback() override;

    juce::Rectangle<float> tapeRect (const Knob&) const;
    juce::Point<float> ratioLed (int i) const;

    ShredMaxxProcessor& processor;
    AmpLookAndFeel lnf;

    std::array<Knob, 6> knobs;      // input, attack, release, output, headroom, mix
    Knob ratio;
    std::array<Switch, 3> eardrum;  // hpf, bandbeater, link
    std::array<Switch, 3> audio;    // hpf, dist2, dist3

    juce::Image background;
    juce::Rectangle<float> vuFace, lampArea;
    float needle = 0.0f, opto = 0.0f;
    double thd1Hold = 0.0, thd3Hold = 0.0;
    bool thd1 = false, thd3 = false;
    int lastRatio = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ShredMaxxEditor)
};
