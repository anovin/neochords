#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace neo;

static juce::StringArray namesOf (int count, const char* (*fn) (int))
{
    juce::StringArray a;
    for (int i = 0; i < count; ++i) a.add (juce::String::fromUTF8 (fn (i)));
    return a;
}

juce::AudioProcessorValueTreeState::ParameterLayout NeoChordsProcessor::createLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout l;

    StringArray keys { "C", "C#/Db", "D", "D#/Eb", "E", "F", "F#/Gb", "G", "G#/Ab", "A", "A#/Bb", "B" };

    l.add (std::make_unique<AudioParameterChoice> (ParameterID { ParamID::key, 1 }, "Key", keys, 0));
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { ParamID::scale, 1 }, "Scale",
            namesOf ((int) Scale::Count, [] (int i) { return scaleName ((Scale) i); }), 0));
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { ParamID::style, 1 }, "Style",
            namesOf ((int) Style::Count, [] (int i) { return styleName ((Style) i); }), (int) Style::NeoSoul));
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { ParamID::complexity, 1 }, "Complexity",
            namesOf ((int) Complexity::Count, [] (int i) { return complexityName ((Complexity) i); }), (int) Complexity::Lush));
    l.add (std::make_unique<AudioParameterChoice> (ParameterID { ParamID::voicing, 1 }, "Voicing",
            namesOf ((int) Voicing::Count, [] (int i) { return voicingName ((Voicing) i); }), 0));
    l.add (std::make_unique<AudioParameterBool> (ParameterID { ParamID::voiceLead, 1 }, "Voice Leading", true));
    l.add (std::make_unique<AudioParameterInt> (ParameterID { ParamID::octave, 1 }, "Octave", -2, 2, 0));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { ParamID::strum, 1 }, "Strum",
            NormalisableRange<float> (0.0f, 120.0f, 1.0f), 0.0f, AudioParameterFloatAttributes().withLabel ("ms")));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { ParamID::humanize, 1 }, "Humanize",
            NormalisableRange<float> (0.0f, 40.0f, 1.0f), 8.0f, AudioParameterFloatAttributes().withLabel ("%")));
    return l;
}

NeoChordsProcessor::NeoChordsProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "NeoChords", createLayout())
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
}

Settings NeoChordsProcessor::currentSettings() const
{
    Settings s;
    s.key        = juce::roundToInt (pKey->load());
    s.scale      = (Scale) juce::roundToInt (pScale->load());
    s.style      = (Style) juce::roundToInt (pStyle->load());
    s.complexity = (Complexity) juce::roundToInt (pComplexity->load());
    s.voicing    = (Voicing) juce::roundToInt (pVoicing->load());
    return s;
}

bool NeoChordsProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    return (out == juce::AudioChannelSet::stereo() || out == juce::AudioChannelSet::mono())
        && layouts.getMainInputChannelSet().isDisabled();
}

void NeoChordsProcessor::prepareToPlay (double sampleRate, int)
{
    sr = sampleRate;
    pending.clear();
    for (auto& h : held) h = Held {};
    for (auto& ch : refCount) ch.fill (0);
    lastUpper.clear();
}

void NeoChordsProcessor::auditionSlot (int slot, bool on)
{
    const juce::SpinLock::ScopedLockType lock (auditionWriteLock);
    auto scope = auditionFifo.write (1);
    if (scope.blockSize1 > 0) auditionBuf[(size_t) scope.startIndex1] = on ? slot + 1 : -(slot + 1);
    else if (scope.blockSize2 > 0) auditionBuf[(size_t) scope.startIndex2] = on ? slot + 1 : -(slot + 1);
}

//==============================================================================
void NeoChordsProcessor::noteOn (int n, int velocity, int channel, juce::int64 time, juce::MidiBuffer& out, int pos)
{
    if (held[(size_t) n].active)
        noteOff (n, out, pos);

    const auto s = currentSettings();
    if (s != engine.settings())
        engine.update (s);

    const int slot = n % 12;
    const int octShift = juce::jlimit (-2, 2, n / 12 - 5);   // MIDI 60-71 = home octave
    const int centre = juce::jlimit (36, 96, 60 + 12 * (juce::roundToInt (pOctave->load()) + octShift));

    // Shift the previous right hand into the new register so voice leading still works across octaves.
    std::vector<int> prev (lastUpper);
    for (auto& x : prev) x += centre - lastCentre;

    const auto voiced = engine.voice (slot, centre, prev, pVoiceLead->load() > 0.5f);
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

        if (refCount[(size_t) channel][(size_t) note]++ > 0)
            continue;   // already sounding from another held chord

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
}

void NeoChordsProcessor::noteOff (int n, juce::MidiBuffer& out, int pos)
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

    h = Held {};
}

void NeoChordsProcessor::allOff (juce::MidiBuffer& out, int pos)
{
    for (int n = 0; n < 128; ++n)
        if (held[(size_t) n].active)
            noteOff (n, out, pos);
    pending.clear();
    auditionHeld.fill (false);
}

void NeoChordsProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();   // NeoChords makes no sound itself — it drives your instrument over MIDI

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
                else if (auditionHeld[(size_t) slot]) { auditionHeld[(size_t) slot] = false; noteOff (note, out, 0); }
            }
        };
        handle (scope.startIndex1, scope.blockSize1);
        handle (scope.startIndex2, scope.blockSize2);
    }

    for (const auto meta : midi)
    {
        const auto msg = meta.getMessage();
        const int pos = meta.samplePosition;

        if (msg.isNoteOn())
            noteOn (msg.getNoteNumber(), msg.getVelocity(), msg.getChannel(), sampleClock + pos, out, pos);
        else if (msg.isNoteOff())
            noteOff (msg.getNoteNumber(), out, pos);
        else if (msg.isAllNotesOff() || msg.isAllSoundOff())
        {
            allOff (out, pos);
            out.addEvent (msg, pos);
        }
        else if (msg.isPitchWheel())
            continue;                  // FL Studio mangles pitch bend from VST3 MIDI out (pins it low) — keep it on the controller side
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
juce::AudioProcessorEditor* NeoChordsProcessor::createEditor()
{
    return new NeoChordsEditor (*this);
}

void NeoChordsProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, dest);
}

void NeoChordsProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new NeoChordsProcessor();
}
