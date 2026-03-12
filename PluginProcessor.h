#pragma once
#include <JuceHeader.h>
#include <array>
#include <atomic> // Mandatory for the UI bridge

class TrackedDistortionAudioProcessor  : public juce::AudioProcessor
{
public:
    TrackedDistortionAudioProcessor();
    ~TrackedDistortionAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;
    const juce::String getName() const override;
    
    bool acceptsMidi() const override { return true; } 
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int index) override {}
    const juce::String getProgramName (int index) override { return "Default"; }
    void changeProgramName (int index, const juce::String& newName) override {}
    void getStateInformation (juce::MemoryBlock& destData) override {}
    void setStateInformation (const void* data, int sizeInBytes) override {}

    juce::AudioProcessorValueTreeState apvts;

    // --- The method the compiler was missing ---
    float getCurrentTrackRatio() const { return currentTrackRatio.load(); }

private:
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    std::array<juce::dsp::LinkwitzRileyFilter<float>, 2> lowPassFilters;
    std::array<juce::dsp::LinkwitzRileyFilter<float>, 2> highPassFilters;

    juce::BigInteger activeNotes; 
    float lastNoteRatio = 1.0f; 

    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothedDrive;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothedNoteRatio;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> smoothedFreq;

    // --- The variable the compiler was missing ---
    std::atomic<float> currentTrackRatio { 0.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TrackedDistortionAudioProcessor)
};