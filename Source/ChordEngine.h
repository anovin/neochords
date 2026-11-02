#pragma once
// NoviChords chord engine — pure C++ (no JUCE), so it can be unit-tested on its own.
//
// Mapping (one key = one chord), repeated in every octave:
//   White keys C D E F G A B -> the 7 diatonic chords of the chosen key + scale (degrees 1..7)
//   Black keys C# D# F# G# A# -> 5 "colour" chords chosen per style
//                                (passing diminished, secondary dominants, borrowed chords...)

#include <array>
#include <string>
#include <vector>

namespace neo
{

enum class Family { Major, Minor, Dominant, Sus, HalfDim, Dim, MinMaj, AugMaj };

enum class Quality
{
    Maj, Min, Dim, Aug, Sus4,
    Maj7, Maj9, Maj9s11, Maj13, SixNine, Add9,
    Min7, Min9, Min11, MinMaj7, MinMaj9,
    Dom7, Dom9, Dom13, Dom7s9, Dom7b9, Dom13b9,
    Dom7sus4, Dom9sus4, Dom13sus,
    HalfDim7, HalfDim9, Dim7, AugMaj7,
    Count
};

struct QualityInfo
{
    const char* suffix;
    std::vector<int> intervals;   // semitones above the root (may exceed 12 for extensions)
    Family family;
};

const QualityInfo& info (Quality q);

enum class Scale { Ionian, Dorian, Phrygian, Lydian, Mixolydian, Aeolian, Locrian, HarmonicMinor, MelodicMinor, Count };
enum class Style { Gospel, Soul, NeoSoul, Count };
enum class Complexity { Triads, Sevenths, Ninths, Lush, Count };
enum class Voicing { Spread, RootlessBass, Close, Drop2, Count };

const char* scaleName (Scale);
const char* styleName (Style);
const char* complexityName (Complexity);
const char* voicingName (Voicing);
bool scaleIsMinor (Scale);         // true when the scale has a minor 3rd

struct Chord
{
    int root = 0;                 // pitch class relative to the key (0..11)
    Quality quality = Quality::Maj;
    int bass = -1;                // slash-bass pitch class relative to key, -1 = none
    bool diatonic = true;
};

struct Settings
{
    int key = 0;                  // 0 = C ... 11 = B
    Scale scale = Scale::Ionian;
    Style style = Style::NeoSoul;
    Complexity complexity = Complexity::Lush;
    Voicing voicing = Voicing::Spread;

    bool operator== (const Settings& o) const
    {
        return key == o.key && scale == o.scale && style == o.style
            && complexity == o.complexity && voicing == o.voicing;
    }
    bool operator!= (const Settings& o) const { return ! (*this == o); }
};

struct Progression
{
    std::string name;
    std::vector<int> keys;        // keyboard slots (0 = C key ... 11 = B key)
};

struct StyleSet                   // one flavour (major or minor) of a style
{
    std::string tip;
    std::array<Chord, 5> colour;  // black-key chords (C#, D#, F#, G#, A#), at "lush" quality
    std::vector<Progression> progressions;
};

struct StyleInfo
{
    std::string name;
    std::string blurb;
    StyleSet major, minor;
};

const StyleInfo& styleInfo (Style);

struct Suggestion
{
    int slot = 0;
    int score = 0;
    std::string reason;
};

struct Voiced
{
    std::vector<int> bass;        // left-hand notes
    std::vector<int> upper;       // right-hand notes (used for voice leading)

    std::vector<int> all() const
    {
        std::vector<int> v (bass);
        v.insert (v.end(), upper.begin(), upper.end());
        return v;
    }
};

class ChordEngine
{
public:
    ChordEngine() { update (Settings {}); }

    void update (const Settings&);
    const Settings& settings() const { return s; }
    bool minorMode() const { return scaleIsMinor (s.scale); }
    const StyleSet& styleSet() const;

    const Chord& chord (int slot) const { return chords[(size_t) (((slot % 12) + 12) % 12)]; }

    std::string chordName (int slot) const;
    std::string roman (int slot) const;
    std::vector<int> pitchClasses (int slot) const;   // absolute pitch classes, bass first
    int absoluteRoot (int slot) const { return (s.key + chord (slot).root) % 12; }

    // centre = MIDI note the right hand should sit around.
    // voiceLead = false gives the same voicing every time for a given key and octave.
    Voiced voice (int slot, int centre, const std::vector<int>& previousUpper, bool voiceLead) const;

    std::vector<Suggestion> suggest (int fromSlot, int count = 5) const;

    std::string noteName (int pitchClass) const;      // spelled for the current key
    static std::string keyLabel (int slot);           // "C", "C#", ...
    static bool isBlackKey (int slot);

private:
    Settings s;
    std::array<Chord, 12> chords;
    bool useSharps = false;

    Chord diatonic (int degree) const;
};

} // namespace neo
