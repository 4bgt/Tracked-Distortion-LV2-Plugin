#include "PluginProcessor.h"
#include "PluginEditor.h"

TrackedDistortionAudioProcessor::TrackedDistortionAudioProcessor()
    : AudioProcessor (BusesProperties().withInput("Input", juce::AudioChannelSet::stereo(), true)
                                       .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "Parameters", createParameterLayout())
{}

TrackedDistortionAudioProcessor::~TrackedDistortionAudioProcessor() {}
const juce::String TrackedDistortionAudioProcessor::getName() const { return JucePlugin_Name; }
bool TrackedDistortionAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const { return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo(); }

void TrackedDistortionAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = samplesPerBlock;
    spec.numChannels = 1; 

    for (int ch = 0; ch < 2; ++ch) {
        lowPassFilters[ch].prepare(spec);
        lowPassFilters[ch].setType(juce::dsp::LinkwitzRileyFilterType::lowpass);
        highPassFilters[ch].prepare(spec);
        highPassFilters[ch].setType(juce::dsp::LinkwitzRileyFilterType::highpass);
    }

    smoothedNoteRatio.reset(sampleRate, 0.05); 
    smoothedDrive.reset(sampleRate, 0.02);
    smoothedFreq.reset(sampleRate, 0.02);
    activeNotes.clear();
}

void TrackedDistortionAudioProcessor::releaseResources() {}

void TrackedDistortionAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    float floorNote = apvts.getRawParameterValue("NOTEFLOOR")->load();
    float ceilNote = apvts.getRawParameterValue("NOTECEILING")->load();
    if (ceilNote <= floorNote) ceilNote = floorNote + 1.0f; 

    for (const auto metadata : midiMessages) {
        auto msg = metadata.getMessage();
        if (msg.isNoteOn()) activeNotes.setBit(msg.getNoteNumber());
        else if (msg.isNoteOff()) activeNotes.clearBit(msg.getNoteNumber());
        else if (msg.isAllNotesOff()) activeNotes.clear();
    }

    int highestNote = activeNotes.getHighestBit();
    if (highestNote >= 0) {
        float ratio = (highestNote - floorNote) / (ceilNote - floorNote);
        lastNoteRatio = juce::jlimit(0.0f, 1.0f, ratio);
    }
    smoothedNoteRatio.setTargetValue(lastNoteRatio);
    currentTrackRatio.store(smoothedNoteRatio.getCurrentValue());

    smoothedDrive.setTargetValue(apvts.getRawParameterValue("DRIVE")->load());
    smoothedFreq.setTargetValue(apvts.getRawParameterValue("XOVER")->load());
    float trackDepth = apvts.getRawParameterValue("TRACKDEPTH")->load();
    float outGain = apvts.getRawParameterValue("OUTGAIN")->load();
    
    int mode = (int)apvts.getRawParameterValue("MODE")->load(); 
    // NEW: Find out which band we are targeting
    int targetBand = (int)apvts.getRawParameterValue("BANDSELECT")->load(); // 0 = Highs, 1 = Lows

    for (int sample = 0; sample < buffer.getNumSamples(); ++sample) {
        
        float currentFreq = smoothedFreq.getNextValue();
        float currentDrive = smoothedDrive.getNextValue();
        float currentRatio = smoothedNoteRatio.getNextValue();

        float noteReduction = 1.0f - currentRatio; 
        float trackMultiplier = 1.0f - (noteReduction * trackDepth);
        float effectiveDrive = 1.0f + ((currentDrive - 1.0f) * trackMultiplier);

        for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
            if (ch > 1) continue; 

            lowPassFilters[ch].setCutoffFrequency(currentFreq);
            highPassFilters[ch].setCutoffFrequency(currentFreq);

            float input = buffer.getReadPointer(ch)[sample];
            
            float lowBand = lowPassFilters[ch].processSample(0, input);
            float highBand = highPassFilters[ch].processSample(0, input);

            // Dynamically assign which band gets distorted and which stays clean!
            float cleanBand = (targetBand == 0) ? lowBand : highBand;
            float distBand  = (targetBand == 0) ? highBand : lowBand;

            float outSample = 0.0f;
            if (mode == 0)      outSample = cleanBand + std::tanh(distBand * currentDrive);
            else if (mode == 1) outSample = std::tanh(input * effectiveDrive);
            else if (mode == 2) outSample = cleanBand + std::tanh(distBand * effectiveDrive);

            buffer.getWritePointer(ch)[sample] = outSample * outGain;
        }
    }
}

juce::AudioProcessorValueTreeState::ParameterLayout TrackedDistortionAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"DRIVE", 1}, "Drive", juce::NormalisableRange<float>(1.0f, 10.0f, 0.01f), 1.0f));
    
    juce::NormalisableRange<float> freqRange(100.0f, 5000.0f, 1.0f);
    freqRange.setSkewForCentre(500.0f);
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"XOVER", 1}, "Split Freq", freqRange, 500.0f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"TRACKDEPTH", 1}, "Key Depth", juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 1.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"OUTGAIN", 1}, "Output", juce::NormalisableRange<float>(0.0f, 2.0f, 0.01f), 1.0f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"NOTEFLOOR", 1}, "Note Floor", juce::NormalisableRange<float>(0.0f, 127.0f, 1.0f), 36.0f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"NOTECEILING", 1}, "Note Ceiling", juce::NormalisableRange<float>(0.0f, 127.0f, 1.0f), 84.0f));

    juce::StringArray modeOptions = { "Multiband Only", "Keytrack Only", "Combined" };
    layout.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"MODE", 1}, "Mode", modeOptions, 2));

    // NEW: Band Target selector
    juce::StringArray bandOptions = { "Distort Highs", "Distort Lows" };
    layout.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"BANDSELECT", 1}, "Target Band", bandOptions, 0));

    return layout;
}

bool TrackedDistortionAudioProcessor::hasEditor() const { return true; }
juce::AudioProcessorEditor* TrackedDistortionAudioProcessor::createEditor() { return new TrackedDistortionAudioProcessorEditor (*this); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new TrackedDistortionAudioProcessor(); }