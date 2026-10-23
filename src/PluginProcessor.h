#pragma once
#include <atomic>
#include <juce_audio_processors/juce_audio_processors.h>
#include "SuperParasitesEngine.h"
#include "OutputStage.h"

class SuperParasitesAudioProcessor : public juce::AudioProcessor
{
public:
    SuperParasitesAudioProcessor();
    ~SuperParasitesAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    using AudioProcessor::processBlock;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 8.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    // The module's CAPTURE button / input: called by the editor's Capture button (MIDI note-ons also fire it).
    void requestCapture() { captureRequested.store (true); }

    // Waveform display: the editor reads the picture the audio thread keeps (see WaveformPicture.h); the audio side only keeps the
    // picture up to date while at least one editor has the display open.
    const WaveformPicture& waveform() const { return engine.waveform(); }
    void waveformViewerOpened() { engine.waveformViewerOpened(); }
    void waveformViewerClosed() { engine.waveformViewerClosed(); }

    juce::AudioProcessorValueTreeState apvts;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    SuperParasitesEngine engine;
    OutputStage::Stage outputStage;
    std::vector<float> scratchL, scratchR, inL, inR;
    std::atomic<bool> captureRequested { false };
    int lastMidiNote = 60;
    int heldNotes = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SuperParasitesAudioProcessor)
};
