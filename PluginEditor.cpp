#include "PluginProcessor.h"
#include "PluginEditor.h"

TrackedDistortionAudioProcessorEditor::TrackedDistortionAudioProcessorEditor (TrackedDistortionAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    auto setupKnob = [this](juce::Slider& slider, juce::Label& label, const juce::String& text) {
        label.setText(text, juce::dontSendNotification);
        label.setJustificationType(juce::Justification::centred);
        addAndMakeVisible(label);
        slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
        addAndMakeVisible(slider);
    };

    // Row 1
    setupKnob(driveSlider, driveLabel, "Drive");
    driveAttach = std::make_unique<SliderAttachment>(audioProcessor.apvts, "DRIVE", driveSlider);

    setupKnob(freqSlider, freqLabel, "Split Freq");
    freqAttach = std::make_unique<SliderAttachment>(audioProcessor.apvts, "XOVER", freqSlider);

    setupKnob(depthSlider, depthLabel, "Key Depth");
    depthAttach = std::make_unique<SliderAttachment>(audioProcessor.apvts, "TRACKDEPTH", depthSlider);

    setupKnob(outSlider, outLabel, "Output");
    outAttach = std::make_unique<SliderAttachment>(audioProcessor.apvts, "OUTGAIN", outSlider);

    // Row 2
    setupKnob(floorSlider, floorLabel, "Note Floor");
    floorAttach = std::make_unique<SliderAttachment>(audioProcessor.apvts, "NOTEFLOOR", floorSlider);

    setupKnob(ceilingSlider, ceilingLabel, "Note Ceiling");
    ceilingAttach = std::make_unique<SliderAttachment>(audioProcessor.apvts, "NOTECEILING", ceilingSlider);

    // Mode Selector
    modeLabel.setText("Mode", juce::dontSendNotification);
    modeLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(modeLabel);
    modeBox.addItemList({ "Multiband Only", "Keytrack Only", "Combined" }, 1);
    addAndMakeVisible(modeBox);
    modeAttach = std::make_unique<ComboBoxAttachment>(audioProcessor.apvts, "MODE", modeBox);

    // Band Selector
    bandLabel.setText("Target Band", juce::dontSendNotification);
    bandLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(bandLabel);
    bandBox.addItemList({ "Distort Highs", "Distort Lows" }, 1);
    addAndMakeVisible(bandBox);
    bandAttach = std::make_unique<ComboBoxAttachment>(audioProcessor.apvts, "BANDSELECT", bandBox);

    // Dynamic Info Label
    infoLabel.setJustificationType(juce::Justification::centred);
    infoLabel.setFont(juce::Font(14.0f, juce::Font::bold));
    infoLabel.setColour(juce::Label::textColourId, juce::Colours::orange);
    addAndMakeVisible(infoLabel);

    // Helper to update the text and states dynamically
    auto updateUI = [this]() {
        int mode = modeBox.getSelectedId() - 1; 
        bool distortHighs = (bandBox.getSelectedId() == 1);
        
        freqSlider.setEnabled(mode != 1);  
        bandBox.setEnabled(mode != 1); // Crossover target doesn't matter if crossover is off
        depthSlider.setEnabled(mode != 0); 
        floorSlider.setEnabled(mode != 0);
        ceilingSlider.setEnabled(mode != 0);

        juce::String target = distortHighs ? "Highs" : "Lows";
        juce::String clean = distortHighs ? "Lows" : "Highs";

        if (mode == 0) {
            infoLabel.setText("Flow: Split Freq -> Distort " + target + " (Keep " + clean + " Clean)", juce::dontSendNotification);
        } else if (mode == 1) {
            infoLabel.setText("Flow: MIDI Note Intensity -> Distort Full Signal", juce::dontSendNotification);
        } else if (mode == 2) {
            infoLabel.setText("Flow: Split Freq + MIDI Intensity -> Dynamic Distort " + target + " (Keep " + clean + " Clean)", juce::dontSendNotification);
        }
    };

    modeBox.onChange = updateUI;
    bandBox.onChange = updateUI;
    updateUI(); // Call once to set initial state

    // Meter Setup
    meterLabel.setText("Tracking Intensity", juce::dontSendNotification);
    meterLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(meterLabel);
    addAndMakeVisible(trackingMeter);

    setSize (500, 420);
    startTimerHz(30); 
}

TrackedDistortionAudioProcessorEditor::~TrackedDistortionAudioProcessorEditor() {
    stopTimer();
}

void TrackedDistortionAudioProcessorEditor::timerCallback()
{
    currentMeterValue = static_cast<double>(audioProcessor.getCurrentTrackRatio());
}

void TrackedDistortionAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
}

void TrackedDistortionAudioProcessorEditor::resized()
{
    int knobSize = 80;
    int labelHeight = 20;
    int spacing = 20;
    
    // ROW 1
    int currentX = 40;
    int currentY = 20; 

    driveLabel.setBounds(currentX, currentY, knobSize, labelHeight);
    driveSlider.setBounds(currentX, currentY + labelHeight, knobSize, knobSize);
    currentX += knobSize + spacing;

    freqLabel.setBounds(currentX, currentY, knobSize, labelHeight);
    freqSlider.setBounds(currentX, currentY + labelHeight, knobSize, knobSize);
    currentX += knobSize + spacing;

    depthLabel.setBounds(currentX, currentY, knobSize, labelHeight);
    depthSlider.setBounds(currentX, currentY + labelHeight, knobSize, knobSize);
    currentX += knobSize + spacing;

    outLabel.setBounds(currentX, currentY, knobSize, labelHeight);
    outSlider.setBounds(currentX, currentY + labelHeight, knobSize, knobSize);

    // ROW 2 (Split with Knobs and Dropdowns)
    currentX = 40;
    currentY = 150;

    floorLabel.setBounds(currentX, currentY, knobSize, labelHeight);
    floorSlider.setBounds(currentX, currentY + labelHeight, knobSize, knobSize);
    currentX += knobSize + spacing;

    ceilingLabel.setBounds(currentX, currentY, knobSize, labelHeight);
    ceilingSlider.setBounds(currentX, currentY + labelHeight, knobSize, knobSize);
    currentX += knobSize + spacing + 10; // Extra space before dropdowns

    // Stack the dropdowns vertically
    modeLabel.setBounds(currentX, currentY, 140, labelHeight);
    modeBox.setBounds(currentX, currentY + labelHeight, 140, 30);
    
    bandLabel.setBounds(currentX, currentY + 50, 140, labelHeight);
    bandBox.setBounds(currentX, currentY + 50 + labelHeight, 140, 30);

    // ROW 3: Signal Flow & Meter
    currentX = 40;
    currentY = 300;

    infoLabel.setBounds(currentX, currentY, 420, labelHeight);
    
    meterLabel.setBounds(currentX, currentY + 30, 420, labelHeight);
    trackingMeter.setBounds(currentX, currentY + 50, 420, 20);
}