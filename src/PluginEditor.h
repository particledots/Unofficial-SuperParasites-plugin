#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"

// Simple flat look for rotary knobs.
class SuperParasitesLookAndFeel : public juce::LookAndFeel_V4
{
public:
    SuperParasitesLookAndFeel();
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos,
                           float startAngle, float endAngle, juce::Slider&) override;
    void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool highlighted, bool down) override;
};

class SuperParasitesEditor : public juce::AudioProcessorEditor
{
public:
    explicit SuperParasitesEditor (SuperParasitesAudioProcessor&);
    ~SuperParasitesEditor() override;
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    using SliderAtt = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAtt = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using ComboAtt  = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    struct Knob
    {
        juce::Slider slider;
        juce::Label label;
        std::unique_ptr<SliderAtt> att;
    };

    struct Menu
    {
        juce::ComboBox box;
        juce::Label label;
        std::unique_ptr<ComboAtt> att;
    };

    void addKnob (Knob&, const juce::String& paramId, const juce::String& name);
    void addMenu (Menu&, const juce::String& paramId, const juce::String& name, const juce::StringArray& items);

    SuperParasitesAudioProcessor& proc;
    SuperParasitesLookAndFeel laf;

    Knob position, size, pitch, density, texture;
    Knob dryWet, spread, feedback, reverb, outputLevel;
    Menu mode, quality, pitchSnap;
    juce::ToggleButton limiter { "LIMITER" };
    juce::ToggleButton freeze { "FREEZE" }, reverse { "REVERSE" }, gate { "GATE" }, pitchMidi { "PITCH FOLLOWS MIDI" };
    juce::TextButton capture { "CAPTURE" };
    juce::Label hint;
    std::unique_ptr<ButtonAtt> limiterAtt, freezeAtt, reverseAtt, gateAtt, pitchMidiAtt;
    std::unique_ptr<juce::ParameterAttachment> modeWatch;
    std::unique_ptr<juce::ParameterAttachment> snapReadoutAtt;   // keeps the Pitch readout in step with the Pitch Snap menu
    void updateHint (int mode);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SuperParasitesEditor)
};
