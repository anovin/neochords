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

// One chord in a recorded take (times in quarter-note beats from the first chord).
struct CapturedChord
{
    juce::String name, roman;
    std::vector<int> notes;
    int velocity = 100;
    double start = 0.0;
    double end = -1.0;            // -1 while the key is still held
    int inputNote = -1;
};

class NoviChordsProcessor : public juce::AudioProcessor, private juce::Timer
{
public:
    NoviChordsProcessor();
    ~NoviChordsProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "NoviChords"; }
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
    void setKeyAndScale (int key, neo::Scale scale);

    // Click-to-audition from the UI (thread-safe)
    void auditionSlot (int slot, bool on);

    // What is sounding now (written by audio thread, read by UI)
    std::atomic<int> lastSlot { -1 };
    std::atomic<int> lastNoteCount { 0 };
    std::array<std::atomic<int>, 16> lastNotes {};
    std::atomic<int> playCounter { 0 };

    // Chord recorder (message thread) ----------------------------------------
    void startRecording();            // clears the previous take and arms
    void stopRecording();
    void clearRecording();
    bool isRecording() const          { return recording.load(); }
    std::vector<CapturedChord> getTake() const;
    double getTakeBpm() const         { return takeBpm; }
    int getTakeVersion() const        { return takeVersion.load(); }

    static const juce::StringArray& quantizeNames();
    // Writes the take as a standard MIDI file; returns the file (or {} if the take is empty).
    juce::File writeTakeToMidiFile (const juce::File& file, int quantizeIndex) const;
    juce::File writeTakeToTempMidi (int quantizeIndex) const;
    int quantizeIndex = 2;            // editor setting, kept with the plugin state
    int mapView = 0;                  // harmony map: 0 = circle, 1 = grid, 2 = flow

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    void timerCallback() override;

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

    struct CaptureEvent
    {
        bool on = true;
        int inputNote = 0;
        int slot = 0;
        int velocity = 0;
        neo::Settings settings;
        int count = 0;
        std::array<int, 16> notes {};
        double seconds = 0.0;
    };

    void noteOn (int inputNote, int velocity, int channel, juce::int64 time, juce::MidiBuffer& out, int pos);
    void noteOff (int inputNote, juce::MidiBuffer& out, int pos, juce::int64 time);
    void allOff (juce::MidiBuffer& out, int pos);
    void pushCapture (const CaptureEvent&);

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

    // recorder
    std::atomic<bool> recording { false };
    std::atomic<double> hostBpm { 120.0 };
    juce::AbstractFifo captureFifo { 512 };
    std::array<CaptureEvent, 512> captureBuf {};
    mutable juce::CriticalSection takeLock;
    std::vector<CapturedChord> take;
    double takeStartSeconds = -1.0;
    double takeBpm = 120.0;
    std::atomic<int> takeVersion { 0 };
    neo::ChordEngine nameEngine;      // message-thread copy for chord names

    std::atomic<float>* pKey = nullptr;
    std::atomic<float>* pScale = nullptr;
    std::atomic<float>* pStyle = nullptr;
    std::atomic<float>* pComplexity = nullptr;
    std::atomic<float>* pVoicing = nullptr;
    std::atomic<float>* pVoiceLead = nullptr;
    std::atomic<float>* pOctave = nullptr;
    std::atomic<float>* pStrum = nullptr;
    std::atomic<float>* pHumanize = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NoviChordsProcessor)
};
