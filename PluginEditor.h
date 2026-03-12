#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class TrackedDistortionAudioProcessorEditor  : public juce::AudioProcessorEditor, public juce::Timer
{
public:
    TrackedDistortionAudioProcessorEditor (TrackedDistortionAudioProcessor&);
    ~TrackedDistortionAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;

private:
    TrackedDistortionAudioProcessor& audioProcessor;

    juce::Slider driveSlider, freqSlider, depthSlider, outSlider, floorSlider, ceilingSlider;
    juce::ComboBox modeBox, bandBox; // Added bandBox
    
    // Added infoLabel for the signal flow visualizer
    juce::Label driveLabel, freqLabel, depthLabel, outLabel, modeLabel, floorLabel, ceilingLabel, bandLabel, meterLabel, infoLabel;
    
    double currentMeterValue = 0.0;
    juce::ProgressBar trackingMeter { currentMeterValue };

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboBoxAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    std::unique_ptr<SliderAttachment> driveAttach, freqAttach, depthAttach, outAttach, floorAttach, ceilingAttach;
    std::unique_ptr<ComboBoxAttachment> modeAttach, bandAttach; // Added bandAttach

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TrackedDistortionAudioProcessorEditor)
};