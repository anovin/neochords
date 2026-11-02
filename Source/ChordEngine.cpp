#include "ChordEngine.h"

#include <algorithm>
#include <climits>
#include <cmath>

namespace neo
{

//==============================================================================
// Chord qualities
const QualityInfo& info (Quality q)
{
    static const std::vector<QualityInfo> table = {
        { "",          { 0, 4, 7 },                 Family::Major },     // Maj
        { "m",         { 0, 3, 7 },                 Family::Minor },     // Min
        { "dim",       { 0, 3, 6 },                 Family::Dim },       // Dim
        { "aug",       { 0, 4, 8 },                 Family::AugMaj },    // Aug
        { "sus4",      { 0, 5, 7 },                 Family::Sus },       // Sus4
        { "maj7",      { 0, 4, 7, 11 },             Family::Major },     // Maj7
        { "maj9",      { 0, 4, 7, 11, 14 },         Family::Major },     // Maj9
        { "maj9♯11",   { 0, 4, 7, 11, 14, 18 },     Family::Major },     // Maj9s11
        { "maj13",     { 0, 4, 7, 11, 14, 21 },     Family::Major },     // Maj13
        { "6/9",       { 0, 4, 7, 9, 14 },          Family::Major },     // SixNine
        { "add9",      { 0, 4, 7, 14 },             Family::Major },     // Add9
        { "m7",        { 0, 3, 7, 10 },             Family::Minor },     // Min7
        { "m9",        { 0, 3, 7, 10, 14 },         Family::Minor },     // Min9
        { "m11",       { 0, 3, 7, 10, 14, 17 },     Family::Minor },     // Min11
        { "m(maj7)",   { 0, 3, 7, 11 },             Family::MinMaj },    // MinMaj7
        { "m(maj9)",   { 0, 3, 7, 11, 14 },         Family::MinMaj },    // MinMaj9
        { "7",         { 0, 4, 7, 10 },             Family::Dominant },  // Dom7
        { "9",         { 0, 4, 7, 10, 14 },         Family::Dominant },  // Dom9
        { "13",        { 0, 4, 7, 10, 14, 21 },     Family::Dominant },  // Dom13
        { "7♯9",       { 0, 4, 7, 10, 15 },         Family::Dominant },  // Dom7s9
        { "7♭9",       { 0, 4, 7, 10, 13 },         Family::Dominant },  // Dom7b9
        { "13♭9",      { 0, 4, 7, 10, 13, 21 },     Family::Dominant },  // Dom13b9
        { "7sus4",     { 0, 5, 7, 10 },             Family::Sus },       // Dom7sus4
        { "9sus4",     { 0, 5, 7, 10, 14 },         Family::Sus },       // Dom9sus4
        { "13sus4",    { 0, 5, 7, 10, 14, 21 },     Family::Sus },       // Dom13sus
        { "m7♭5",      { 0, 3, 6, 10 },             Family::HalfDim },   // HalfDim7
        { "m9♭5",      { 0, 3, 6, 10, 14 },         Family::HalfDim },   // HalfDim9
        { "°7",        { 0, 3, 6, 9 },              Family::Dim },       // Dim7
        { "maj7♯5",    { 0, 4, 8, 11 },             Family::AugMaj },    // AugMaj7
    };
    return table[(size_t) q];
}

const char* scaleName (Scale s)
{
    static const char* n[] = { "Ionian (Major)", "Dorian", "Phrygian", "Lydian", "Mixolydian",
                               "Aeolian (Nat. Minor)", "Locrian", "Harmonic Minor", "Melodic Minor" };
    return n[(int) s];
}

bool scaleIsMinor (Scale s)
{
    return ! (s == Scale::Ionian || s == Scale::Lydian || s == Scale::Mixolydian);
}

const char* styleName (Style s)
{
    static const char* n[] = { "Gospel", "Soul", "Neo-Soul" };
    return n[(int) s];
}

const char* complexityName (Complexity c)
{
    static const char* n[] = { "Triads", "7ths", "9ths", "Lush (11/13)" };
    return n[(int) c];
}

const char* voicingName (Voicing v)
{
    static const char* n[] = { "Spread (LH + RH)", "Rootless + bass", "Close", "Drop 2" };
    return n[(int) v];
}

//==============================================================================
// Style data. Colour chords are listed in black-key order: C#, D#, F#, G#, A#.
// Roots are semitones above the key. Progressions reference keyboard slots.
static Chord col (int root, Quality q, int bass = -1) { return Chord { root, q, bass, false }; }

const StyleInfo& styleInfo (Style st)
{
    using Q = Quality;
    // Major-flavour progressions use white keys as I ii iii IV V vi vii;
    // minor-flavour ones as i ii iii iv v vi vii of the chosen minor mode.
    static const std::vector<StyleInfo> styles = {
        { "Gospel",
          "Church harmony: passing diminished chords, secondary dominants, 13ths and the 'gospel sus'. Built for walk-ups, turnarounds and big Amen endings.",
          { "Black keys: ♯I°7, III7♯9, ♯IV°7, VI13♭9, IV/V.",
            { col (1, Q::Dim7), col (4, Q::Dom7s9), col (6, Q::Dim7), col (9, Q::Dom13b9), col (5, Q::Maj9, 7) },
            { { "Walk-up to the 2",          { 0, 1, 2, 7, 0 } },
              { "Amen ending",               { 5, 6, 0 } },
              { "6-2-5-1 turnaround",        { 8, 2, 10, 0 } },
              { "3-6-2-5-1",                 { 3, 9, 2, 10, 0 } } } },
          { "Black keys: ♭IImaj7, V7♭9, ♯ivø, IV9, II9 (V of V).",
            { col (1, Q::Maj7), col (7, Q::Dom7b9), col (6, Q::HalfDim7), col (5, Q::Dom9), col (2, Q::Dom9) },
            { { "Minor 2-5-1",               { 2, 3, 0 } },
              { "Neapolitan cadence",        { 1, 3, 0 } },
              { "Minor walk-down",           { 0, 11, 9, 3 } },
              { "Dorian lift",               { 0, 8, 0 } } } } },

        { "Soul",
          "Classic soul and R&B: 9th-chord dominants, the II7 and ♭VII9 lifts, I-vi-ii-V changes and backdoor cadences. Warm, gritty and singable.",
          { "Black keys: II9, ♭IIImaj7, iv m7, ♭VI9, ♭VII9.",
            { col (2, Q::Dom9), col (3, Q::Maj7), col (5, Q::Min7), col (8, Q::Dom9), col (10, Q::Dom9) },
            { { "I-vi-ii-V",                 { 0, 9, 2, 7 } },
              { "I-II9-V-I",                 { 0, 1, 7, 0 } },
              { "Backdoor (IV-♭VII9-I)",     { 5, 10, 0 } },
              { "♭VI9-♭VII9-I",              { 8, 10, 0 } } } },
          { "Black keys: ♭IImaj7, IV9, V7♯9, ♭VI9, II9 (V of V).",
            { col (1, Q::Maj7), col (5, Q::Dom9), col (7, Q::Dom7s9), col (8, Q::Dom9), col (2, Q::Dom9) },
            { { "i-IV9 vamp",                { 0, 3 } },
              { "i-♭VII-♭VI-V",              { 0, 11, 9, 6 } },
              { "♭VI9-V7♯9-i",               { 8, 6, 0 } },
              { "Neapolitan to V",           { 1, 6, 0 } } } } },

        { "Neo-Soul",
          "Lush m9/maj9 colours, 13sus dominants and chords borrowed from the parallel minor. Warm, behind-the-beat Rhodes harmony.",
          { "Black keys: ♭IImaj9, ♭IIImaj9, iv m9, ♭VImaj9, ♭VII13.",
            { col (1, Q::Maj9), col (3, Q::Maj9), col (5, Q::Min9), col (8, Q::Maj9), col (10, Q::Dom13) },
            { { "2-5-1-6 loop",              { 2, 7, 0, 9 } },
              { "IV-iii-ii-I descent",       { 5, 4, 2, 0 } },
              { "Backdoor home (iv-♭VII-I)", { 6, 10, 0 } },
              { "Chromatic slide down",      { 0, 3, 2, 1, 0 } } } },
          { "Black keys: ♭IImaj9, IV9, v m9, ♭VI9, V7♯9.",
            { col (1, Q::Maj9), col (5, Q::Dom9), col (7, Q::Min9), col (8, Q::Dom9), col (7, Q::Dom7s9) },
            { { "i-IV9 Dorian vamp",         { 0, 3 } },
              { "♭VI-V7♯9-i",                { 9, 10, 0 } },
              { "i-v-♭VI",                   { 0, 6, 8 } },
              { "Phrygian ♭II",              { 0, 1, 0 } } } } },
    };
    return styles[(size_t) st];
}

//==============================================================================
static const std::array<std::array<int, 7>, 9> kScales = { {
    { 0, 2, 4, 5, 7, 9, 11 },   // Ionian
    { 0, 2, 3, 5, 7, 9, 10 },   // Dorian
    { 0, 1, 3, 5, 7, 8, 10 },   // Phrygian
    { 0, 2, 4, 6, 7, 9, 11 },   // Lydian
    { 0, 2, 4, 5, 7, 9, 10 },   // Mixolydian
    { 0, 2, 3, 5, 7, 8, 10 },   // Aeolian
    { 0, 1, 3, 5, 6, 8, 10 },   // Locrian
    { 0, 2, 3, 5, 7, 8, 11 },   // Harmonic minor
    { 0, 2, 3, 5, 7, 9, 11 },   // Melodic minor
} };

const StyleSet& ChordEngine::styleSet() const
{
    const auto& si = styleInfo (s.style);
    return minorMode() ? si.minor : si.major;
}

static const int kWhiteSlots[7] = { 0, 2, 4, 5, 7, 9, 11 };
static const int kBlackSlots[5] = { 1, 3, 6, 8, 10 };

bool ChordEngine::isBlackKey (int slot)
{
    slot = ((slot % 12) + 12) % 12;
    return slot == 1 || slot == 3 || slot == 6 || slot == 8 || slot == 10;
}

std::string ChordEngine::keyLabel (int slot)
{
    static const char* n[] = { "C", "C♯", "D", "D♯", "E", "F", "F♯", "G", "G♯", "A", "A♯", "B" };
    return n[((slot % 12) + 12) % 12];
}

std::string ChordEngine::noteName (int pc) const
{
    static const char* sharps[] = { "C", "C♯", "D", "D♯", "E", "F", "F♯", "G", "G♯", "A", "A♯", "B" };
    static const char* flats[]  = { "C", "D♭", "D", "E♭", "E", "F", "G♭", "G", "A♭", "A", "B♭", "B" };
    pc = ((pc % 12) + 12) % 12;
    return useSharps ? sharps[pc] : flats[pc];
}

// Reduce a "lush" quality to the chosen complexity.
static Quality reduce (Quality q, Complexity cx)
{
    const Family f = info (q).family;

    if (cx == Complexity::Triads)
    {
        switch (f)
        {
            case Family::Major:    return Quality::Maj;
            case Family::Minor:    return Quality::Min;
            case Family::Dominant: return Quality::Maj;
            case Family::Sus:      return Quality::Sus4;
            case Family::HalfDim:  return Quality::Dim;
            case Family::Dim:      return Quality::Dim;
            case Family::MinMaj:   return Quality::Min;
            case Family::AugMaj:   return Quality::Aug;
        }
    }

    if (cx == Complexity::Sevenths)
    {
        switch (f)
        {
            case Family::Major:    return q == Quality::Add9 ? Quality::Add9 : Quality::Maj7;
            case Family::Minor:    return Quality::Min7;
            case Family::Dominant: return (q == Quality::Dom7s9 || q == Quality::Dom7b9) ? q
                                        : (q == Quality::Dom13b9 ? Quality::Dom7b9 : Quality::Dom7);
            case Family::Sus:      return Quality::Dom7sus4;
            case Family::HalfDim:  return Quality::HalfDim7;
            case Family::Dim:      return Quality::Dim7;
            case Family::MinMaj:   return Quality::MinMaj7;
            case Family::AugMaj:   return Quality::AugMaj7;
        }
    }

    if (cx == Complexity::Ninths)
    {
        switch (q)
        {
            case Quality::Maj13:
            case Quality::Maj9s11:  return Quality::Maj9;
            case Quality::Min11:    return Quality::Min9;
            case Quality::Dom13:    return Quality::Dom9;
            case Quality::Dom13b9:  return Quality::Dom7b9;
            case Quality::Dom13sus: return Quality::Dom9sus4;
            default:                return q;
        }
    }

    return q;
}

Chord ChordEngine::diatonic (int degree) const
{
    const auto& sc = kScales[(size_t) s.scale];
    auto iv = [&] (int steps) {
        const int a = sc[(size_t) ((degree + steps) % 7)];
        return (a - sc[(size_t) degree] + 12) % 12;
    };

    const int third = iv (2), fifth = iv (4), seventh = iv (6);
    const int ninth = iv (1), eleventh = iv (3), thirteenth = iv (5);
    const bool nat9 = ninth == 2;

    Chord c;
    c.root = sc[(size_t) degree];
    c.diatonic = true;

    Quality lush = Quality::Maj7;
    const Style st = s.style;

    if (third == 4 && fifth == 7 && seventh == 11)              // major 7
    {
        if (! nat9)                         lush = Quality::Maj7;
        else if (eleventh == 6 && (degree == 0 || st == Style::NeoSoul))
                                            lush = Quality::Maj9s11;     // Lydian colour
        else switch (st)
        {
            case Style::Gospel:     lush = degree == 0 ? Quality::SixNine : Quality::Maj13; break;
            case Style::NeoSoul:    lush = Quality::Maj13; break;
            default:                lush = Quality::Maj9; break;
        }
    }
    else if (third == 4 && fifth == 7 && seventh == 10)         // dominant
    {
        if (! nat9)                         lush = thirteenth == 9 ? Quality::Dom13b9 : Quality::Dom7b9;
        else switch (st)
        {
            case Style::NeoSoul:    lush = Quality::Dom13sus; break;
            case Style::Soul:       lush = Quality::Dom9; break;
            default:                lush = thirteenth == 9 ? Quality::Dom13 : Quality::Dom9; break;
        }
    }
    else if (third == 3 && fifth == 7 && seventh == 10)         // minor 7
    {
        lush = nat9 ? (st == Style::Soul ? Quality::Min9 : Quality::Min11) : Quality::Min7;
    }
    else if (third == 3 && fifth == 6 && seventh == 10)         // half-diminished
    {
        lush = nat9 ? Quality::HalfDim9 : Quality::HalfDim7;
    }
    else if (third == 3 && fifth == 6)                          // fully diminished
    {
        lush = Quality::Dim7;
    }
    else if (third == 3 && fifth == 7 && seventh == 11)         // minor-major
    {
        lush = nat9 ? Quality::MinMaj9 : Quality::MinMaj7;
    }
    else if (third == 4 && fifth == 8)                          // augmented
    {
        lush = Quality::AugMaj7;
    }

    c.quality = reduce (lush, s.complexity);
    return c;
}

void ChordEngine::update (const Settings& settings)
{
    s = settings;
    s.key = ((s.key % 12) + 12) % 12;

    // Spell with sharps for sharp keys (judged from the relative major for minor modes).
    // Find the parent major key so notes are spelled the way the key signature would.
    static const int toParentMajor[] = { 0, 10, 8, 7, 5, 3, 1, 3, 3 };   // Ionian .. Locrian, HarmMin, MelMin
    const int relMajor = (s.key + toParentMajor[(int) s.scale]) % 12;
    useSharps = relMajor == 7 || relMajor == 2 || relMajor == 9 || relMajor == 4 || relMajor == 11;

    for (int d = 0; d < 7; ++d)
        chords[(size_t) kWhiteSlots[d]] = diatonic (d);

    const auto& set = styleSet();
    for (int i = 0; i < 5; ++i)
    {
        Chord c = set.colour[(size_t) i];
        c.quality = reduce (c.quality, s.complexity);
        chords[(size_t) kBlackSlots[i]] = c;
    }
}

std::string ChordEngine::chordName (int slot) const
{
    const Chord& c = chord (slot);
    // Passing diminished chords read better spelled with sharps (C♯°7 rather than D♭°7).
    // The leading tone (C♯ in D minor) is spelled sharp as well.
    const Family fam = info (c.quality).family;
    const bool sharpDim = ((fam == Family::Dim || fam == Family::HalfDim) && (c.root == 1 || c.root == 6 || c.root == 8))
                          || c.root == 11;
    static const char* sharps[] = { "C", "C♯", "D", "D♯", "E", "F", "F♯", "G", "G♯", "A", "A♯", "B" };
    std::string n = (sharpDim && ! useSharps ? std::string (sharps[(s.key + c.root) % 12]) : noteName (s.key + c.root))
                    + info (c.quality).suffix;
    if (c.bass >= 0)
        n += "/" + noteName (s.key + c.bass);
    return n;
}

std::string ChordEngine::roman (int slot) const
{
    const Chord& c = chord (slot);
    const Family f = info (c.quality).family;

    static const char* up[]  = { "I", "♭II", "II", "♭III", "III", "IV", "♯IV", "V", "♭VI", "VI", "♭VII", "VII" };
    static const char* low[] = { "i", "♭ii", "ii", "♭iii", "iii", "iv", "♯iv", "v", "♭vi", "vi", "♭vii", "vii" };

    std::string r;
    const bool lower = f == Family::Minor || f == Family::HalfDim || f == Family::Dim || f == Family::MinMaj;

    if (c.root == 6 && minorMode() && ! (f == Family::Dim || f == Family::HalfDim))
        r = lower ? "♭v" : "♭V";
    else if (f == Family::Dim && c.root == 1)  r = "♯i";
    else if (f == Family::Dim && c.root == 8)  r = "♯v";
    else                                       r = lower ? low[c.root] : up[c.root];

    switch (f)
    {
        case Family::Dim:      r += "°"; break;
        case Family::HalfDim:  r += "ø"; break;
        case Family::AugMaj:   r += "+"; break;
        case Family::Dominant: r += c.quality == Quality::Maj ? "" : "7"; break;
        case Family::Sus:      r += "sus"; break;
        default: break;
    }

    if (c.bass >= 0)
        r += std::string ("/") + up[c.bass];

    return r;
}

std::vector<int> ChordEngine::pitchClasses (int slot) const
{
    const Chord& c = chord (slot);
    std::vector<int> pcs;
    const int root = (s.key + c.root) % 12;
    if (c.bass >= 0)
        pcs.push_back ((s.key + c.bass) % 12);
    for (int i : info (c.quality).intervals)
    {
        const int pc = (root + i) % 12;
        if (std::find (pcs.begin(), pcs.end(), pc) == pcs.end())
            pcs.push_back (pc);
    }
    return pcs;
}

//==============================================================================
// Voicing
static int placeInRange (int pc, int low)   // lowest pitch with this pitch class that is >= low
{
    return low + (((pc - low) % 12) + 12) % 12;
}

static std::vector<int> stackFrom (const std::vector<int>& pcs, int startIndex, int startPitch)
{
    std::vector<int> notes;
    int prev = startPitch - 1;
    for (size_t k = 0; k < pcs.size(); ++k)
    {
        const int pc = pcs[(startIndex + k) % pcs.size()];
        const int n = placeInRange (pc, prev + 1);
        notes.push_back (n);
        prev = n;
    }
    return notes;
}

static double motionCost (const std::vector<int>& a, const std::vector<int>& b)
{
    double cost = 0;
    for (int x : a)
    {
        int best = INT_MAX;
        for (int y : b) best = std::min (best, std::abs (x - y));
        cost += best;
    }
    for (int y : b)
    {
        int best = INT_MAX;
        for (int x : a) best = std::min (best, std::abs (x - y));
        cost += best;
    }
    // keep the top voice smooth — it is what the ear follows
    if (! a.empty() && ! b.empty())
        cost += 1.5 * std::abs (a.back() - b.back());
    return cost;
}

Voiced ChordEngine::voice (int slot, int centre, const std::vector<int>& previousUpper, bool voiceLead) const
{
    const Chord& c = chord (slot);
    const auto& ivs = info (c.quality).intervals;
    const int rootPc = (s.key + c.root) % 12;
    const int bassPc = c.bass >= 0 ? (s.key + c.bass) % 12 : rootPc;

    // Chord tones as pitch classes, ordered by interval above the root (mod 12), unique.
    std::vector<std::pair<int, int>> tones;   // (interval mod 12, raw interval)
    for (int i : ivs)
    {
        const int m = i % 12;
        bool dup = false;
        for (auto& t : tones) dup |= t.first == m;
        if (! dup) tones.push_back ({ m, i });
    }
    std::sort (tones.begin(), tones.end());

    auto hasInterval = [&] (int m) { for (auto& t : tones) if (t.first == m) return true; return false; };

    Voiced v;
    std::vector<int> upperPcs;
    const int bassLow = centre - 26;

    switch (s.voicing)
    {
        case Voicing::Spread:
        {
            // LH: bass + 7th (or 5th); RH: the remaining colour tones
            const int b = placeInRange (bassPc, bassLow);
            v.bass.push_back (b);
            int lhTone = -1;
            if (c.bass < 0)
            {
                if (hasInterval (10))      lhTone = 10;
                else if (hasInterval (11)) lhTone = 11;
                else if (hasInterval (7))  lhTone = 7;
                if (lhTone >= 0) v.bass.push_back (b + lhTone);
            }
            for (auto& t : tones)
                if (t.first != 0 && t.first != lhTone)
                    upperPcs.push_back ((rootPc + t.first) % 12);
            if (upperPcs.size() < 3 && c.bass < 0)       // triads: keep the root on top too
                upperPcs.push_back (rootPc);
            if (c.bass >= 0)                              // slash chord: full chord over the bass
            {
                upperPcs.clear();
                for (auto& t : tones) upperPcs.push_back ((rootPc + t.first) % 12);
            }
            break;
        }
        case Voicing::RootlessBass:
        {
            v.bass.push_back (placeInRange (bassPc, bassLow));
            const bool dropFifth = tones.size() >= 5;
            for (auto& t : tones)
                if (t.first != 0 && ! (dropFifth && t.first == 7))
                    upperPcs.push_back ((rootPc + t.first) % 12);
            if (upperPcs.size() < 3)
                upperPcs.push_back (rootPc);
            break;
        }
        case Voicing::Close:
        case Voicing::Drop2:
        default:
            for (auto& t : tones) upperPcs.push_back ((rootPc + t.first) % 12);
            break;
    }

    // Choose the inversion / octave for the right hand.
    std::vector<int> best;
    double bestCost = 1e9;
    const int n = (int) upperPcs.size();
    for (int rot = 0; rot < n; ++rot)
    {
        for (int shift = -12; shift <= 12; shift += 12)
        {
            auto notes = stackFrom (upperPcs, rot, centre - 6 + shift);
            const double mean = [&] { double m = 0; for (int x : notes) m += x; return m / notes.size(); }();
            double cost;
            if (voiceLead && ! previousUpper.empty())
                cost = motionCost (notes, previousUpper) + 0.4 * std::abs (mean - centre);
            else
                cost = std::abs (mean - (centre + 2)) + (s.voicing == Voicing::Close && rot != 0 ? 2.0 : 0.0);
            if (notes.front() < centre - 14 || notes.back() > centre + 19)
                cost += 50;
            if (cost < bestCost) { bestCost = cost; best = notes; }
        }
    }

    if (s.voicing == Voicing::Drop2 && best.size() >= 4)
    {
        best[best.size() - 2] -= 12;
        std::sort (best.begin(), best.end());
    }

    // Close / drop-2 slash chords still need their bass note.
    if ((s.voicing == Voicing::Close || s.voicing == Voicing::Drop2) && c.bass >= 0)
        v.bass.push_back (placeInRange (bassPc, best.front() - 12));

    v.upper = best;

    for (auto* list : { &v.bass, &v.upper })
        for (auto& x : *list) x = std::clamp (x, 0, 127);

    return v;
}

//==============================================================================
// Suggestions
std::vector<Suggestion> ChordEngine::suggest (int fromSlot, int count) const
{
    fromSlot = ((fromSlot % 12) + 12) % 12;
    const Chord& a = chords[(size_t) fromSlot];
    const Family fa = info (a.quality).family;
    const Style st = s.style;
    const auto& set = styleSet();

    const bool stepStyle    = st == Style::NeoSoul;
    const bool mediantStyle = st == Style::NeoSoul;
    const bool vampStyle    = st == Style::Soul || st == Style::NeoSoul || minorMode();
    const bool domA = fa == Family::Dominant || fa == Family::Sus || a.bass == 7;   // IV/V acts as V

    std::vector<Suggestion> out;

    for (int t = 0; t < 12; ++t)
    {
        if (t == fromSlot) continue;
        const Chord& b = chords[(size_t) t];
        if (b.root == a.root && b.quality == a.quality && b.bass == a.bass) continue;

        const Family fb = info (b.quality).family;
        const int d = (b.root - a.root + 12) % 12;

        int score = 0, bestPts = 0;
        std::string reason;
        auto add = [&] (int pts, const std::string& why) {
            score += pts;
            if (pts > bestPts) { bestPts = pts; reason = why; }
        };

        // Signature progressions of the style carry the most weight.
        for (auto& p : set.progressions)
            for (size_t i = 0; i + 1 < p.keys.size(); ++i)
                if (p.keys[i] == fromSlot && p.keys[i + 1] == t)
                    add (7, "Next chord in “" + p.name + "”");

        // Functional pulls
        if (domA)
        {
            const int target = a.bass >= 0 ? (a.bass - 7 + 12) % 12 : (a.root + 5) % 12;
            if (b.root == target)                               add (6, "Dominant resolves down a fifth");
            if (fa == Family::Dominant && d == 11)              add (5, "Half-step resolution (tritone-sub move)");
            if (a.root == 7 && b.root == 9 && fb == Family::Minor) add (3, "Deceptive move to the relative minor");
        }
        if (fa == Family::Dim)
        {
            if (d == 1)                                         add (6, "Passing diminished resolves up a half step");
            if (a.root == 6 && b.root == 0)                     add (6, "♯IV°7 settles home — the gospel ‘Amen’");
            if (d == 11)                                        add (2, "Diminished slips back down a half step");
        }
        if (fa == Family::HalfDim && d == 5)                    add (5, "Minor ii–V motion");
        if (fa == Family::Minor && (fb == Family::Dominant || fb == Family::Sus) && d == 5)
                                                                add (4, "ii–V setup");

        if (d == 5)                                             add (3, "Circle-of-fifths motion");
        if (d == 7)                                             add (1, "Up a fifth");
        if (d == 1 || d == 11)                                  add (stepStyle ? 3 : 1, "Chromatic slide");
        if (d == 2 || d == 10)                                  add (stepStyle ? 3 : 1, "Stepwise planing");

        if (a.root == 5 && b.root == 0)
            add (st == Style::Gospel ? 5 : 3, fa == Family::Minor ? "Minor plagal (iv → I) — bittersweet resolve"
                                                                  : "Plagal ‘Amen’ resolution");

        if ((d == 3 || d == 4 || d == 8 || d == 9) && fa == Family::Major && fb == Family::Major)
            add (mediantStyle ? 4 : 1, "Chromatic-mediant shift");

        if (! a.diatonic && b.root == 0 && (a.root == 3 || a.root == 8 || a.root == 10))
            add (4, "Borrowed chord resolving home");

        if (vampStyle && (a.root == 0 || b.root == 0) && (d == 5 || d == 7 || d == 2 || d == 10)
            && (fa == Family::Minor || fb == Family::Minor || fb == Family::Dominant))
            add (3, "Two-chord vamp — loop it");

        if (st == Style::Gospel && fb == Family::Dominant && b.root != 7 && ! b.diatonic)
            add (2, "Secondary dominant setup");

        if (b.root == 0 && b.diatonic)                          add (1, "Back home to the tonic");

        bool dup = false;   // the same chord can sit on two keys in some modes — list it once
        for (auto& o : out)
        {
            const Chord& oc = chords[(size_t) o.slot];
            if (oc.root == b.root && oc.quality == b.quality && oc.bass == b.bass)
            {
                dup = true;
                if (score > o.score) { o = { t, score, reason }; }
            }
        }
        if (score > 0 && ! dup)
            out.push_back ({ t, score, reason });
    }

    std::stable_sort (out.begin(), out.end(), [] (const Suggestion& x, const Suggestion& y) { return x.score > y.score; });
    if ((int) out.size() > count)
        out.resize ((size_t) count);
    return out;
}

} // namespace neo
