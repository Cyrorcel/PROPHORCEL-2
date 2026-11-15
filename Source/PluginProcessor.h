#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>

#include "Engine.h"

class ProphorcelProcessor : public juce::AudioProcessor
{
public:
    ProphorcelProcessor();
    ~ProphorcelProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Prophorcel"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 6.0; }

    int getNumPrograms() override;
    int getCurrentProgram() override { return currentProgram; }
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    juce::AudioProcessorValueTreeState apvts;
    juce::MidiKeyboardState keyboardState;
    proph::Engine engine;

    // lus et écrits par l'interface
    std::atomic<float> guiBend { 0.f }, shownBend { 0.f };
    std::atomic<int> octShift { 0 };

private:
    void readParams (proph::Params& p) const;
    void handleMidi (const juce::MidiMessage& m);

    std::atomic<float>* raw (const char* id) const { return apvts.getRawParameterValue (id); }

    int currentProgram = 0;
    juce::MidiBuffer keyMidi, merged;
    int keyTrans[128];
    float lastGuiBend = 0.f;
};
