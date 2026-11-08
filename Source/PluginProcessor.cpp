#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace neo;

static juce::StringArray namesOf (int count, const char* (*fn) (int))
{
    juce::StringArray a;
    for (int i = 0; i < count; ++i) a.add (juce::String::fromUTF8 (fn (i)));
    return a;
}

juce::AudioProcessorValueTreeState::ParameterLayout NoviChordsProcessor::createLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout l;

    StringArray keys { "C", "C#/Db", "D", "D#/Eb", "E", "F", "F#/Gb", "G", "G#/Ab", "A", "A#/Bb", "B" };

    // Parameter version 2: new scale list and style list.
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { ParamID::key, 2 }, "Key", keys, 0));
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { ParamID::scale, 2 }, "Scale",
            namesOf ((int) Scale::Count, [] (int i) { return scaleName ((Scale) i); }), 0));
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { ParamID::style, 2 }, "Style",
            namesOf ((int) Style::Count, [] (int i) { return styleName ((Style) i); }), (int) Style::NeoSoul));
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { ParamID::complexity, 2 }, "Complexity",
            namesOf ((int) Complexity::Count, [] (int i) { return complexityName ((Complexity) i); }), (int) Complexity::Lush));
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { ParamID::voicing, 2 }, "Voicing",
            namesOf ((int) Voicing::Count, [] (int i) { return voicingName ((Voicing) i); }), 0));
    // Off by default so a key always plays the exact same voicing.
    l.add (std::make_unique<AudioParameterBool> (ParameterID { ParamID::voiceLead, 2 }, "Smooth Voice Leading", false));
    l.add (std::make_unique<AudioParameterInt> (ParameterID { ParamID::octave, 2 }, "Octave", -2, 2, 0));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { ParamID::strum, 2 }, "Strum",
            NormalisableRange<float> (0.0f, 120.0f, 1.0f), 0.0f, AudioParameterFloatAttributes().withLabel ("ms")));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { ParamID::humanize, 2 }, "Humanize",
            NormalisableRange<float> (0.0f, 40.0f, 1.0f), 0.0f, AudioParameterFloatAttributes().withLabel ("%")));
    return l;
}

NoviChordsProcessor::NoviChordsProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "NoviChords", createLayout())
{
    pKey        = apvts.getRawParameterValue (ParamID::key);
    pScale      = apvts.getRawParameterValue (ParamID::scale);
    pStyle      = apvts.getRawParameterValue (ParamID::style);
    pComplexity = apvts.getRawParameterValue (ParamID::complexity);
    pVoicing    = apvts.getRawParameterValue (ParamID::voicing);
    pVoiceLead  = apvts.getRawParameterValue (ParamID::voiceLead);
    pOctave     = apvts.getRawParameterValue (ParamID::octave);
    pStrum      = apvts.getRawParameterValue (ParamID::strum);
    pHumanize   = apvts.getRawParameterValue (ParamID::humanize);

    for (auto& n : lastNotes) n.store (0);
    pending.reserve (256);
    startTimerHz (30);
}

NoviChordsProcessor::~NoviChordsProcessor()
{
    stopTimer();
}

Settings NoviChordsProcessor::currentSettings() const
{
    Settings s;
    s.key        = juce::roundToInt (pKey->load());
    s.scale      = (Scale) juce::roundToInt (pScale->load());
    s.style      = (Style) juce::roundToInt (pStyle->load());
    s.complexity = (Complexity) juce::roundToInt (pComplexity->load());
    s.voicing    = (Voicing) juce::roundToInt (pVoicing->load());
    return s;
}

void NoviChordsProcessor::setKeyAndScale (int key, Scale scale)
{
    auto set = [this] (const char* id, int index) {
        auto* p = apvts.getParameter (id);
        p->beginChangeGesture();
        p->setValueNotifyingHost (p->convertTo0to1 ((float) index));
        p->endChangeGesture();
    };
    set (ParamID::key, ((key % 12) + 12) % 12);
    set (ParamID::scale, (int) scale);
}

bool NoviChordsProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    return (out == juce::AudioChannelSet::stereo() || out == juce::AudioChannelSet::mono())
        && layouts.getMainInputChannelSet().isDisabled();
}

void NoviChordsProcessor::prepareToPlay (double sampleRate, int)
{
    sr = sampleRate;
    pending.clear();
    for (auto& h : held) h = Held {};
    for (auto& ch : refCount) ch.fill (0);
    lastUpper.clear();
    seqActive = false;
    seqNote = -1;
    seqRow.store (-1);
    seqStep.store (-1);
}

//==============================================================================
// Progression player
void NoviChordsProcessor::playProgression (int rowId, const std::vector<int>& slots, double beatsPerChord, bool loop)
{
    {
        const juce::SpinLock::ScopedLockType sl (seqLock);
        seqReqCount = juce::jmin ((int) slots.size(), (int) seqReqSlots.size());
        for (int i = 0; i < seqReqCount; ++i) seqReqSlots[(size_t) i] = ((slots[(size_t) i] % 12) + 12) % 12;
        seqReqBeats = juce::jmax (0.5, beatsPerChord);
        seqReqLoop = loop;
    }
    seqRow.store (rowId);
    seqStep.store (-1);
    seqStartReq.store (true);
}

void NoviChordsProcessor::stopProgression()
{
    seqStopReq.store (true);
}

void NoviChordsProcessor::runSequencer (juce::MidiBuffer& out, int numSamples)
{
    if (seqStopReq.exchange (false))
    {
        if (seqNote >= 0) noteOff (seqNote, out, 0, sampleClock);
        seqNote = -1;
        seqActive = false;
        seqRow.store (-1);
        seqStep.store (-1);
    }

    if (seqStartReq.exchange (false))
    {
        if (seqNote >= 0) noteOff (seqNote, out, 0, sampleClock);
        seqNote = -1;
        {
            const juce::SpinLock::ScopedLockType sl (seqLock);
            seqSlots = seqReqSlots;
            seqCount = seqReqCount;
            seqBeats = seqReqBeats;
            seqLoopNow = seqReqLoop;
        }
        seqActive = seqCount > 0;
        seqIndex = -1;
        seqNext = sampleClock;
    }

    const juce::int64 blockEnd = sampleClock + numSamples;
    while (seqActive && seqNext < blockEnd)
    {
        const int pos = (int) juce::jlimit<juce::int64> (0, numSamples - 1, seqNext - sampleClock);
        if (seqNote >= 0) { noteOff (seqNote, out, pos, seqNext); seqNote = -1; }

        if (++seqIndex >= seqCount)
        {
            if (seqLoopNow) seqIndex = 0;
            else
            {
                seqActive = false;
                seqRow.store (-1);
                seqStep.store (-1);
                break;
            }
        }

        seqNote = 60 + seqSlots[(size_t) seqIndex];     // home octave, so the Octave control applies
        noteOn (seqNote, 100, 1, seqNext, out, pos);
        seqStep.store (seqIndex);
        seqNext += (juce::int64) std::llround (seqBeats * 60.0 / hostBpm.load() * sr);
    }
}

void NoviChordsProcessor::auditionSlot (int slot, bool on)
{
    const juce::SpinLock::ScopedLockType lock (auditionWriteLock);
    auto scope = auditionFifo.write (1);
    if (scope.blockSize1 > 0) auditionBuf[(size_t) scope.startIndex1] = on ? slot + 1 : -(slot + 1);
    else if (scope.blockSize2 > 0) auditionBuf[(size_t) scope.startIndex2] = on ? slot + 1 : -(slot + 1);
}

//==============================================================================
void NoviChordsProcessor::pushCapture (const CaptureEvent& e)
{
    if (! recording.load())
        return;
    auto scope = captureFifo.write (1);
    if (scope.blockSize1 > 0) captureBuf[(size_t) scope.startIndex1] = e;
    else if (scope.blockSize2 > 0) captureBuf[(size_t) scope.startIndex2] = e;
}

void NoviChordsProcessor::noteOn (int n, int velocity, int channel, juce::int64 time, juce::MidiBuffer& out, int pos)
{
    if (held[(size_t) n].active)
        noteOff (n, out, pos, time);

    const auto s = currentSettings();
    if (s != engine.settings())
        engine.update (s);

    const int slot = n % 12;
    const int octShift = juce::jlimit (-2, 2, n / 12 - 5);   // MIDI 60-71 = home octave
    const int centre = juce::jlimit (36, 96, 60 + 12 * (juce::roundToInt (pOctave->load()) + octShift));
    const bool voiceLead = pVoiceLead->load() > 0.5f;

    std::vector<int> prev;
    if (voiceLead)
    {
        prev = lastUpper;   // shift the previous right hand into the new register
        for (auto& x : prev) x += centre - lastCentre;
    }

    const auto voiced = engine.voice (slot, centre, prev, voiceLead);
    lastUpper = voiced.upper;
    lastCentre = centre;

    auto notes = voiced.all();
    std::sort (notes.begin(), notes.end());

    const double strumSamples = pStrum->load() / 1000.0 * sr;
    const float human = pHumanize->load() / 100.0f;
    channel = juce::jlimit (1, 16, channel);

    auto& h = held[(size_t) n];
    h.active = true;
    h.channel = channel;
    h.count = 0;

    for (size_t i = 0; i < notes.size() && h.count < (int) h.notes.size(); ++i)
    {
        const int note = notes[i];
        const float jitter = 1.0f + human * (rng.nextFloat() * 2.0f - 1.0f);
        const int vel = juce::jlimit (1, 127, juce::roundToInt ((float) velocity * jitter));

        h.notes[(size_t) h.count++] = note;

        // If another held chord is already sounding this note, re-strike it so every
        // chord gets a full, consistent attack (no silently "skipped" notes).
        if (refCount[(size_t) channel][(size_t) note]++ > 0)
            out.addEvent (juce::MidiMessage::noteOff (channel, note), pos);

        auto msg = juce::MidiMessage::noteOn (channel, note, (juce::uint8) vel);
        const auto delay = (juce::int64) std::llround (strumSamples * (double) i);
        if (delay == 0)
            out.addEvent (msg, pos);
        else
            pending.push_back ({ time + delay, msg, n });
    }

    lastSlot.store (slot);
    const int c = juce::jmin ((int) notes.size(), (int) lastNotes.size());
    for (int i = 0; i < c; ++i) lastNotes[(size_t) i].store (notes[(size_t) i]);
    lastNoteCount.store (c);
    playCounter.fetch_add (1);

    if (recording.load())
    {
        CaptureEvent e;
        e.on = true; e.inputNote = n; e.slot = slot; e.velocity = velocity; e.settings = s;
        e.count = h.count; e.notes = h.notes; e.seconds = (double) time / sr;
        pushCapture (e);
    }
}

void NoviChordsProcessor::noteOff (int n, juce::MidiBuffer& out, int pos, juce::int64 time)
{
    auto& h = held[(size_t) n];
    if (! h.active)
        return;

    for (int i = 0; i < h.count; ++i)
    {
        const int note = h.notes[(size_t) i];

        bool wasPending = false;
        for (auto it = pending.begin(); it != pending.end();)
        {
            if (it->sourceNote == n && it->msg.getNoteNumber() == note) { wasPending = true; it = pending.erase (it); }
            else ++it;
        }

        auto& rc = refCount[(size_t) h.channel][(size_t) note];
        rc = juce::jmax (0, rc - 1);

        if (rc == 0)
        {
            if (! wasPending)
                out.addEvent (juce::MidiMessage::noteOff (h.channel, note), pos);
        }
        else if (wasPending)
        {
            out.addEvent (juce::MidiMessage::noteOn (h.channel, note, (juce::uint8) 100), pos);   // another chord still wants it
        }
    }

    if (recording.load())
    {
        CaptureEvent e;
        e.on = false; e.inputNote = n; e.seconds = (double) time / sr;
        pushCapture (e);
    }

    h = Held {};
}

void NoviChordsProcessor::allOff (juce::MidiBuffer& out, int pos)
{
    for (int n = 0; n < 128; ++n)
        if (held[(size_t) n].active)
            noteOff (n, out, pos, sampleClock + pos);
    pending.clear();
    auditionHeld.fill (false);
}

void NoviChordsProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();   // NoviChords makes no sound itself — it drives your instrument over MIDI

    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
            if (auto bpm = pos->getBpm())
                if (*bpm > 20.0 && *bpm < 400.0)
                    hostBpm.store (*bpm);

    const int numSamples = buffer.getNumSamples();
    juce::MidiBuffer out;

    // Chords clicked in the UI
    {
        auto scope = auditionFifo.read (auditionFifo.getNumReady());
        auto handle = [&] (int start, int size) {
            for (int i = 0; i < size; ++i)
            {
                const int v = auditionBuf[(size_t) (start + i)];
                const int slot = std::abs (v) - 1;
                const int note = 60 + slot;
                if (v > 0) { auditionHeld[(size_t) slot] = true; noteOn (note, 100, 1, sampleClock, out, 0); }
                else if (auditionHeld[(size_t) slot]) { auditionHeld[(size_t) slot] = false; noteOff (note, out, 0, sampleClock); }
            }
        };
        handle (scope.startIndex1, scope.blockSize1);
        handle (scope.startIndex2, scope.blockSize2);
    }

    runSequencer (out, numSamples);

    for (const auto meta : midi)
    {
        const auto msg = meta.getMessage();
        const int pos = meta.samplePosition;

        if (msg.isNoteOn())
            noteOn (msg.getNoteNumber(), msg.getVelocity(), msg.getChannel(), sampleClock + pos, out, pos);
        else if (msg.isNoteOff())
            noteOff (msg.getNoteNumber(), out, pos, sampleClock + pos);
        else if (msg.isAllNotesOff() || msg.isAllSoundOff())
        {
            allOff (out, pos);
            out.addEvent (msg, pos);
        }
        else if (msg.isPitchWheel())
            continue;                  // FL Studio mangles pitch bend from VST3 MIDI out — bend on the instrument
        else
            out.addEvent (msg, pos);   // sustain pedal, mod wheel, other CCs pass straight through
    }

    // Strummed notes that fall inside this block
    const juce::int64 blockEnd = sampleClock + numSamples;
    for (auto it = pending.begin(); it != pending.end();)
    {
        if (it->time < blockEnd)
        {
            out.addEvent (it->msg, (int) juce::jlimit<juce::int64> (0, numSamples - 1, it->time - sampleClock));
            it = pending.erase (it);
        }
        else ++it;
    }

    midi.swapWith (out);
    sampleClock = blockEnd;
}

//==============================================================================
// Recorder (message thread)
void NoviChordsProcessor::startRecording()
{
    {
        const juce::ScopedLock sl (takeLock);
        take.clear();
        takeStartSeconds = -1.0;
        takeBpm = hostBpm.load();
    }
    captureFifo.reset();
    recording.store (true);
    takeVersion.fetch_add (1);
}

void NoviChordsProcessor::stopRecording()
{
    recording.store (false);
    timerCallback();   // drain what is left

    const juce::ScopedLock sl (takeLock);
    // Close chords that were still held when recording stopped
    double last = 0.0;
    for (auto& c : take) last = juce::jmax (last, c.start, c.end);
    for (auto& c : take)
        if (c.end < 0.0) c.end = juce::jmax (c.start + 0.25, last);
    takeVersion.fetch_add (1);
}

void NoviChordsProcessor::clearRecording()
{
    recording.store (false);
    captureFifo.reset();
    const juce::ScopedLock sl (takeLock);
    take.clear();
    takeStartSeconds = -1.0;
    takeVersion.fetch_add (1);
}

std::vector<CapturedChord> NoviChordsProcessor::getTake() const
{
    const juce::ScopedLock sl (takeLock);
    return take;
}

void NoviChordsProcessor::timerCallback()
{
    const int ready = captureFifo.getNumReady();
    if (ready == 0)
        return;

    auto scope = captureFifo.read (ready);
    const juce::ScopedLock sl (takeLock);

    auto handle = [&] (int start, int size) {
        for (int i = 0; i < size; ++i)
        {
            const auto& e = captureBuf[(size_t) (start + i)];
            if (takeStartSeconds < 0.0)
            {
                if (! e.on) continue;          // ignore releases of keys held before recording
                takeStartSeconds = e.seconds;
                takeBpm = hostBpm.load();
            }
            const double beat = (e.seconds - takeStartSeconds) * takeBpm / 60.0;

            if (e.on)
            {
                if (e.settings != nameEngine.settings())
                    nameEngine.update (e.settings);
                CapturedChord c;
                c.name = juce::String::fromUTF8 (nameEngine.chordName (e.slot).c_str());
                c.roman = juce::String::fromUTF8 (nameEngine.roman (e.slot).c_str());
                c.notes.assign (e.notes.begin(), e.notes.begin() + e.count);
                c.velocity = e.velocity;
                c.start = beat;
                c.inputNote = e.inputNote;
                take.push_back (std::move (c));
            }
            else
            {
                for (auto it = take.rbegin(); it != take.rend(); ++it)
                    if (it->inputNote == e.inputNote && it->end < 0.0) { it->end = juce::jmax (it->start + 0.05, beat); break; }
            }
        }
    };
    handle (scope.startIndex1, scope.blockSize1);
    handle (scope.startIndex2, scope.blockSize2);
    takeVersion.fetch_add (1);
}

const juce::StringArray& NoviChordsProcessor::quantizeNames()
{
    static const juce::StringArray names { "No snap", "Snap 1/16", "Snap 1/8", "Snap 1/4", "Snap 1/2", "Snap 1 bar" };
    return names;
}

static double quantizeGrid (int index)
{
    static const double grid[] = { 0.0, 0.25, 0.5, 1.0, 2.0, 4.0 };
    return grid[juce::jlimit (0, 5, index)];
}

juce::File NoviChordsProcessor::writeTakeToMidiFile (const juce::File& file, int qIndex) const
{
    const auto chords = getTake();
    if (chords.empty())
        return {};

    constexpr int ppq = 960;
    const double grid = quantizeGrid (qIndex);
    auto snap = [grid] (double b) { return grid > 0.0 ? std::round (b / grid) * grid : b; };

    juce::MidiMessageSequence seq;
    seq.addEvent (juce::MidiMessage::tempoMetaEvent (juce::roundToInt (60000000.0 / takeBpm)), 0.0);
    seq.addEvent (juce::MidiMessage::timeSignatureMetaEvent (4, 4), 0.0);
    seq.addEvent (juce::MidiMessage::textMetaEvent (3, "NoviChords"), 0.0);

    double lastEnd = 0.0;
    for (const auto& c : chords)
    {
        double start = snap (c.start);
        double end = c.end < 0.0 ? c.start + 1.0 : c.end;
        end = snap (end);
        if (end <= start) end = start + (grid > 0.0 ? grid : 0.25);
        lastEnd = juce::jmax (lastEnd, end);

        for (int note : c.notes)
        {
            seq.addEvent (juce::MidiMessage::noteOn (1, note, (juce::uint8) juce::jlimit (1, 127, c.velocity)), start * ppq);
            seq.addEvent (juce::MidiMessage::noteOff (1, note), end * ppq);
        }
    }
    seq.addEvent (juce::MidiMessage::endOfTrack(), lastEnd * ppq);
    seq.updateMatchedPairs();
    seq.sort();

    juce::MidiFile mf;
    mf.setTicksPerQuarterNote (ppq);
    mf.addTrack (seq);

    file.getParentDirectory().createDirectory();
    file.deleteFile();
    juce::FileOutputStream os (file);
    if (! os.openedOk() || ! mf.writeTo (os))
        return {};
    os.flush();
    return file;
}

juce::File NoviChordsProcessor::writeTakeToTempMidi (int qIndex) const
{
    const auto chords = getTake();
    juce::String label = "NoviChords";
    for (size_t i = 0; i < chords.size() && i < 4; ++i)
        label << " " << chords[i].name;
    label = juce::File::createLegalFileName (label.replace (juce::String::fromUTF8 ("♯"), "#")
                                                  .replace (juce::String::fromUTF8 ("♭"), "b")
                                                  .replace (juce::String::fromUTF8 ("°"), "dim")
                                                  .replace (juce::String::fromUTF8 ("ø"), "hd")
                                                  .replace ("/", "-"));
    auto dir = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("NoviChords");
    return writeTakeToMidiFile (dir.getChildFile (label + ".mid"), qIndex);
}

//==============================================================================
juce::AudioProcessorEditor* NoviChordsProcessor::createEditor()
{
    return new NoviChordsEditor (*this);
}

void NoviChordsProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    auto state = apvts.copyState();
    state.setProperty ("quantize", quantizeIndex, nullptr);
    state.setProperty ("mapView", mapView, nullptr);
    state.setProperty ("sigLength", sigLengthIndex, nullptr);
    state.setProperty ("sigLoop", sigLoop, nullptr);
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, dest);
}

void NoviChordsProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType()))
        {
            auto tree = juce::ValueTree::fromXml (*xml);
            quantizeIndex = (int) tree.getProperty ("quantize", 2);
            mapView = juce::jlimit (0, 2, (int) tree.getProperty ("mapView", 0));
            sigLengthIndex = juce::jlimit (0, 2, (int) tree.getProperty ("sigLength", 1));
            sigLoop = (bool) tree.getProperty ("sigLoop", false);
            apvts.replaceState (tree);
        }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new NoviChordsProcessor();
}
