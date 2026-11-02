#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <array>
#include <atomic>
#include "ChordEngine.h"

namespace ParamID
{
    inline constexpr const char* key        = "key";
    inline constexpr const char* scale      = "scale";
    inline constexpr const char* style      = "style";
    inline constexpr const char* complexity = "complexity";
    inline constexpr const char* voicing    = "voicing";
    inline constexpr const char* voiceLead  = "voiceLead";
    inline constexpr const char* octave     = "octave";
    inline constexpr const char* strum      = "strum";
    inline constexpr const char* humanize   = "humanize";
}

class NeoChordsProcessor : public juce::AudioProcessor
{
public:
    NeoChordsProcessor();
    ~NeoChordsProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "NeoChords"; }
    bool acceptsMidi() const override  { return true; }
    bool producesMidi() const override { return true; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return "Default"; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    // Shared with the editor --------------------------------------------------
    juce::AudioProcessorValueTreeState apvts;
    neo::Settings currentSettings() const;

    // Click-to-audition from the UI (thread-safe)
    void auditionSlot (int slot, bool on);

    // What is sounding now (written by audio thread, read by UI)
    std::atomic<int> lastSlot { -1 };
    std::atomic<int> lastNoteCount { 0 };
    std::array<std::atomic<int>, 16> lastNotes {};
    std::atomic<int> playCounter { 0 };

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    struct Held
    {
        bool active = false;
        int channel = 1;
        int count = 0;
        std::array<int, 16> notes {};
    };

    struct Pending
    {
        juce::int64 time;
        juce::MidiMessage msg;
        int sourceNote;
    };

    void noteOn (int inputNote, int velocity, int channel, juce::int64 time, juce::MidiBuffer& out, int pos);
    void noteOff (int inputNote, juce::MidiBuffer& out, int pos);
    void allOff (juce::MidiBuffer& out, int pos);

    neo::ChordEngine engine;
    std::array<Held, 128> held {};
    std::array<std::array<int, 128>, 17> refCount {};
    std::vector<Pending> pending;
    std::vector<int> lastUpper;
    int lastCentre = 60;
    juce::int64 sampleClock = 0;
    double sr = 44100.0;
    juce::Random rng;

    juce::AbstractFifo auditionFifo { 64 };
    std::array<int, 64> auditionBuf {};
    juce::SpinLock auditionWriteLock;
    std::array<bool, 12> auditionHeld {};

    std::atomic<float>* pKey = nullptr;
    std::atomic<float>* pScale = nullptr;
    std::atomic<float>* pStyle = nullptr;
    std::atomic<float>* pComplexity = nullptr;
    std::atomic<float>* pVoicing = nullptr;
    std::atomic<float>* pVoiceLead = nullptr;
    std::atomic<float>* pOctave = nullptr;
    std::atomic<float>* pStrum = nullptr;
    std::atomic<float>* pHumanize = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NeoChordsProcessor)
};
