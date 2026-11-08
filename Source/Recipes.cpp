// Producer recipes, transcribed from "The Neptunes + Sade Chord Progressions" field guide.
// Every chord is written relative to the record's tonic (0), so recipes play in any key.
// Reference records are named for orientation only; loops marked "Loop" are original
// progressions written to the documented patterns, not transcriptions.

#include "ChordEngine.h"

#include <algorithm>

namespace neo
{

namespace
{
    using Q = Quality;
    constexpr auto N = RecipeFamily::Neptunes;
    constexpr auto S = RecipeFamily::Sade;

    // pitch classes for original keys
    constexpr int C = 0, Db = 1, D = 2, Eb = 3, E = 4, F = 5, Gb = 6, G = 7, Ab = 8, A = 9, Bb = 10, B = 11;
}

const std::vector<Recipe>& recipes()
{
    static const std::vector<Recipe> list = {
        // ---------------- The Neptunes: the eight patterns ----------------
        { N, "Two-chord minor vamp", "P1 · i9 – iv11 · like Frontin', Wanna Love You Girl", F, 100, 4.0, true, false,
          { { 0, Q::Min9 }, { 5, Q::Min11 } } },
        { N, u8"Minor vamp to ♭VII", u8"P1 · i9 – ♭VIImaj9 · like Pass the Courvoisier II", Eb, 98, 4.0, true, false,
          { { 0, Q::Min9 }, { 10, Q::Maj9 } } },
        { N, "One chord, held", "P2 · one root, voiced rich · like Drop It Like It's Hot", Eb, 96, 16.0, true, false,
          { { 0, Q::Min11 } } },
        { N, "Dominant-first vamp", u8"P3 · V7♭9 – i9 – iv11 – III11 · like Señorita", Eb, 100, 4.0, true, false,
          { { 7, Q::Dom7b9 }, { 0, Q::Min9 }, { 5, Q::Min11 }, { 3, Q::Dom11 } } },
        { N, "Dorian climb", "P4 · i – ii – III · like Beautiful", E, 101, 4.0, true, false,
          { { 0, Q::Min9 }, { 2, Q::Min7 }, { 3, Q::Maj7 }, { 0, Q::Min9 } } },
        { N, "Slash-chord groove", "P5 · chords over the next bass note · like Rock Your Body", E, 104, 4.0, true, false,
          { { 1, Q::Maj, 3 }, { 3, Q::Maj, 5 }, { 0, Q::Min7 } } },
        { N, "Soul loop", "P6 · Imaj9 – vi9 – ii9 – V · like Signs, Say Somethin'", Ab, 96, 4.0, true, false,
          { { 0, Q::Maj9 }, { 9, Q::Min9 }, { 2, Q::Min9 }, { 7, Q::Dom9 } } },
        { N, u8"Bridge lift ♭VI – ♭VII – I", "P7 · minor verse, major sunshine · like Happy", F, 100, 4.0, false, true,
          { { 8, Q::Maj7 }, { 10, Q::Dom9 }, { 0, Q::Maj9 } } },
        { N, "Bridge that flies", "P7 · VImaj7 – v7 – i7, then ii7 – V7 · like Like I Love You", B, 112, 4.0, false, true,
          { { 8, Q::Maj7 }, { 7, Q::Min7 }, { 0, Q::Min7 }, { 8, Q::Maj7 }, { 7, Q::Min7 }, { 2, Q::Min7 }, { 7, Q::Dom7 } } },
        { N, "Phrygian nudge", u8"P8 · i – ♭II over a pedal · like Hot In Herre", E, 107, 4.0, true, false,
          { { 0, Q::Min7 }, { 1, Q::Maj7, 0 } } },

        // ---------------- The Neptunes: original loops (p. 12) ----------------
        { N, "Usher-style verse", u8"Loop · i9 – iv11 vamp · save a ♭VI for the bridge", F, 100, 4.0, true, false,
          { { 0, Q::Min9 }, { 5, Q::Min11 }, { 0, Q::Min9 }, { 5, Q::Min11 } } },
        { N, "Start away from home", "Loop · vi-led, never quite lands · Frontin' energy", Db, 102, 4.0, true, false,
          { { 9, Q::Min9 }, { 2, Q::Min9 } } },
        { N, "Grown minor hook", "Loop · i – iv – v, all minor · Luv U Better feel", Eb, 98, 4.0, true, false,
          { { 0, Q::Min9 }, { 5, Q::Min9 }, { 7, Q::Min7 }, { 0, Q::Min9 } } },
        { N, "Sus vamp", "Loop · 9th on top, then 11th · N.E.R.D. Am I High voicings", D, 96, 8.0, true, false,
          { { 0, Q::Dom9sus4 }, { 0, Q::Dom11 } } },
        { N, "Neo-soul rotation", "Loop · minor verse that begs for a bridge", G, 92, 4.0, true, false,
          { { 0, Q::Min9 }, { 5, Q::Min11 }, { 3, Q::Maj9 }, { 7, Q::Dom7sus4 } } },
        { N, "Female R&B hook", "Loop · Charlie Wilson-style harmonies", Ab, 94, 4.0, true, false,
          { { 0, Q::Maj9 }, { 9, Q::Min9 }, { 5, Q::Maj9 }, { 7, Q::Dom9sus4 } } },
        { N, "Hugo voicing chain", "Loop · sus9 – maj7(6/9) – maj7(9,#11) · She Wants To Move", F, 100, 4.0, true, false,
          { { 0, Q::Dom9sus4 }, { 1, Q::Maj7Six9 }, { 11, Q::Maj9s11 } } },
        { N, "Bridge after a one-chord verse", "Loop · i – VImaj7 – v7 – i7 · use as the bridge", B, 112, 4.0, false, true,
          { { 0, Q::Min9 }, { 8, Q::Maj7 }, { 7, Q::Min7 }, { 0, Q::Min7 } } },
        { N, "Chorus lift in minor", u8"Loop · ♭VImaj7 – v7 – Isus – I · the Happy shape", F, 160, 4.0, false, true,
          { { 8, Q::Maj7 }, { 7, Q::Min7 }, { 0, Q::Dom7sus4 }, { 0, Q::Dom7 } } },

        // ---------------- Sade: the six patterns ----------------
        { S, "Two-chord vamp", "S1 · i9 – iv7 · like Nothing Can Come Between Us", F, 104, 8.0, true, false,
          { { 0, Q::Min9 }, { 5, Q::Min7 } } },
        { S, "Dorian i – IV", "S1 · minor with a major IV · like Paradise", F, 97, 8.0, true, false,
          { { 0, Q::Min9 }, { 5, Q::Dom9 } } },
        { S, "Pedal under moving chords", "S2 · every chord over one bass note · like Cherish the Day", C, 92, 4.0, true, false,
          { { 0, Q::Min7 }, { 2, Q::Min7, 0 }, { 5, Q::Min7, 0 }, { 7, Q::Min7, 0 } } },
        { S, "Walking bass", u8"S2 · chord holds, bass walks down by semitones · Smooth Operator idea", D, 117, 4.0, true, false,
          { { 0, Q::Min9 }, { 0, Q::Min9, 11 }, { 5, Q::Min9, 10 }, { 5, Q::Min9, 9 } } },
        { S, "Minor soul loop", "S2 · i7 – v7 – iv7 – v7 · like Smooth Operator", D, 117, 4.0, true, false,
          { { 0, Q::Min7 }, { 7, Q::Min7 }, { 5, Q::Min7 }, { 7, Q::Min7 } } },
        { S, "Major-7th loop", "S3 · Imaj7 – IVmaj7 · like Your Love Is King", A, 90, 8.0, true, false,
          { { 0, Q::Maj7 }, { 5, Q::Maj7 } } },
        { S, "The one rising bridge", "S3 · ii – iii – IVmaj9 · Your Love Is King bridge", A, 90, 4.0, false, true,
          { { 2, Q::Min7 }, { 4, Q::Min7 }, { 5, Q::Maj9 } } },
        { S, "Jazz ballad", "S4 · i – iv – V7, a real resolution · like Is It a Crime?", C, 82, 4.0, true, false,
          { { 0, Q::Min7 }, { 5, Q::Min7 }, { 7, Q::Dom7 }, { 0, Q::Min7 } } },
        { S, "One-riff pedal", "S5 · one minor chord, the groove does the rest", B, 96, 16.0, true, false,
          { { 0, Q::Min9 } } },
        { S, "One borrowed chord", "S6 · I – vi – V – IV, then a minor iv shadow · like By Your Side", B, 73, 4.0, false, true,
          { { 0, Q::Maj7 }, { 9, Q::Min7 }, { 7, Q::Maj }, { 5, Q::Maj7 }, { 5, Q::Min7 } } },

        // ---------------- Sade: original loops (p. 18) ----------------
        { S, "Major warmth loop", "Loop · Imaj7 – vi7 – IVmaj7 – V · Kiss of Life feel", A, 96, 4.0, true, false,
          { { 0, Q::Maj7 }, { 9, Q::Min7 }, { 5, Q::Maj7 }, { 7, Q::Maj9 } } },
        { S, "Slow minor cadence", "Loop · i – iv – V – i as m9s · The Moon and the Sky feel", B, 76, 8.0, true, false,
          { { 0, Q::Min9 }, { 5, Q::Min9 }, { 7, Q::Dom7 }, { 0, Q::Min9 } } },
    };
    return list;
}

RecipeLayout recipeLayout (int a, int b)
{
    RecipeLayout lay;
    const auto& list = recipes();

    auto slotFor = [&] (const RecipeChord& rc, RecipeFamily fam) {
        for (size_t i = 0; i < lay.unique.size(); ++i)
        {
            const auto& u = lay.unique[i];
            if (u.root == rc.root && u.quality == rc.q && u.bass == rc.bass)
                return kRecipeSlotOrder[i];
        }
        if (lay.unique.size() >= 12)
            return -1;
        Chord c;
        c.root = ((rc.root % 12) + 12) % 12;
        c.quality = rc.q;
        c.bass = rc.bass;
        c.diatonic = true;
        c.voicing = (int) (fam == RecipeFamily::Neptunes ? Voicing::ExtensionOnTop : Voicing::Spread);
        lay.unique.push_back (c);
        return kRecipeSlotOrder[lay.unique.size() - 1];
    };

    if (a >= 0 && a < (int) list.size())
        for (auto& rc : list[(size_t) a].steps)
            if (const int sl = slotFor (rc, list[(size_t) a].family); sl >= 0) lay.stepsA.push_back (sl);

    if (b >= 0 && b < (int) list.size())
        for (auto& rc : list[(size_t) b].steps)
            if (const int sl = slotFor (rc, list[(size_t) b].family); sl >= 0) lay.stepsB.push_back (sl);

    return lay;
}

} // namespace neo
