// Prints the full key map, voicings and suggestions for every style, and runs sanity checks.
#include "../Source/ChordEngine.h"
#include <cassert>
#include <cstdio>
#include <set>

using namespace neo;

static int failures = 0;
#define CHECK(cond, ...) do { if (! (cond)) { ++failures; std::printf ("FAIL: " __VA_ARGS__); std::printf ("\n"); } } while (0)

int main()
{
    ChordEngine e;

    for (int st = 0; st < (int) Style::Count; ++st)
    {
        Settings s;
        s.key = 0; s.style = (Style) st; s.scale = Scale::Ionian;
        e.update (s);
        std::printf ("\n=== %s (C Ionian, lush, spread) ===\n", styleName ((Style) st));
        std::vector<int> prev;
        for (int k = 0; k < 12; ++k)
        {
            auto v = e.voice (k, 60, prev, true);
            std::printf ("  %-3s %-9s %-14s  notes:", ChordEngine::keyLabel (k).c_str(), e.roman (k).c_str(), e.chordName (k).c_str());
            for (int n : v.all()) std::printf (" %d", n);
            std::printf ("\n");
        }
        for (auto& p : e.styleSet().progressions)
        {
            std::printf ("  prog %-28s:", p.name.c_str());
            for (int k : p.keys) std::printf (" %s", e.chordName (k).c_str());
            std::printf ("\n");
        }
        for (int from : { 0, 2, 7 })
        {
            std::printf ("  after %s ->", e.chordName (from).c_str());
            for (auto& sg : e.suggest (from)) std::printf (" [%s: %s]", e.chordName (sg.slot).c_str(), sg.reason.c_str());
            std::printf ("\n");
        }
    }

    // Exhaustive sanity checks over every combination
    for (int key = 0; key < 12; ++key)
     for (int sc = 0; sc < (int) Scale::Count; ++sc)
      for (int st = 0; st < (int) Style::Count; ++st)
       for (int cx = 0; cx < (int) Complexity::Count; ++cx)
        for (int vo = 0; vo < (int) Voicing::Count; ++vo)
        {
            Settings s { key, (Scale) sc, (Style) st, (Complexity) cx, (Voicing) vo };
            e.update (s);
            std::vector<int> prev;
            for (int k = 0; k < 12; ++k)
            {
                for (bool vl : { false, true })
                {
                    auto v = e.voice (k, 60, prev, vl);
                    auto all = v.all();
                    CHECK (all.size() >= 3, "too few notes key=%d sc=%d st=%d cx=%d vo=%d slot=%d", key, sc, st, cx, vo, k);
                    std::set<int> pcsVoiced, pcsChord;
                    for (int n : all) { CHECK (n >= 20 && n <= 100, "note out of range %d", n); pcsVoiced.insert (n % 12); }
                    for (int pc : e.pitchClasses (k)) pcsChord.insert (pc);
                    for (int pc : pcsVoiced) CHECK (pcsChord.count (pc), "wrong note in voicing slot=%d", k);
                    std::set<int> uniq (all.begin(), all.end());
                    CHECK (uniq.size() == all.size(), "duplicate notes slot=%d st=%d vo=%d", k, st, vo);
                    if (! v.upper.empty())
                        CHECK (v.upper.back() - v.upper.front() < 26, "RH too wide slot=%d", k);
                    prev = v.upper;
                }
                auto sug = e.suggest (k);
                CHECK (! sug.empty(), "no suggestions slot=%d st=%d sc=%d", k, st, sc);
                for (auto& sg : sug) CHECK (sg.slot != k && ! sg.reason.empty(), "bad suggestion");
            }
        }

    // Every mode: print white-key chords in D
    for (int sc = 0; sc < (int) Scale::Count; ++sc)
    {
        Settings s; s.key = 2; s.scale = (Scale) sc; s.style = Style::NeoSoul; e.update (s);
        std::printf ("D %-20s:", scaleName ((Scale) sc));
        for (int k : { 0, 2, 4, 5, 7, 9, 11 }) std::printf (" %s(%s)", e.chordName (k).c_str(), e.roman (k).c_str());
        std::printf ("\n");
    }
    // Minor-flavour style sets
    for (int st = 0; st < (int) Style::Count; ++st)
    {
        Settings s; s.key = 9; s.scale = Scale::Aeolian; s.style = (Style) st; e.update (s);
        std::printf ("A minor %s black:", styleName ((Style) st));
        for (int k : { 1, 3, 6, 8, 10 }) std::printf (" %s", e.chordName (k).c_str());
        for (auto& p : e.styleSet().progressions) { std::printf (" | %s:", p.name.c_str()); for (int k : p.keys) std::printf (" %s", e.chordName (k).c_str()); }
        std::printf ("\n");
    }
    // Deterministic voicing when voice leading is off
    {
        Settings s; e.update (s);
        auto a = e.voice (2, 60, {}, false).all();
        auto b = e.voice (2, 60, e.voice (9, 60, {}, false).upper, false).all();
        CHECK (a == b, "voicing not deterministic with voice leading off");
    }

    // Recipes: every recipe in every key lays out, voices and names cleanly
    {
        auto nm = [] (int n) { static const char* x[] = { "C","C#","D","D#","E","F","F#","G","G#","A","A#","B" }; return std::string (x[n % 12]) + std::to_string (n / 12 - 1); };
        for (int r = 0; r < (int) recipes().size(); ++r)
            for (int key = 0; key < 12; ++key)
            {
                Settings s; s.key = key; s.recipeA = r; e.update (s);
                auto lay = recipeLayout (r, -1);
                CHECK (lay.stepsA.size() == recipes()[(size_t) r].steps.size(), "recipe %d steps lost", r);
                for (int slot : lay.stepsA)
                {
                    auto v = e.voice (slot, 60, {}, false);
                    auto all = v.all();
                    std::set<int> uniq (all.begin(), all.end());
                    CHECK (uniq.size() == all.size() && all.size() >= 3, "recipe %d slot %d bad voicing", r, slot);
                    std::set<int> pcs; for (int pc : e.pitchClasses (slot)) pcs.insert (pc);
                    for (int n : all) CHECK (pcs.count (n % 12), "recipe %d wrong note", r);
                }
                if (key == recipes()[(size_t) r].origKey)
                {
                    std::printf ("R%-2d %-30s:", r, recipes()[(size_t) r].name.c_str());
                    for (int slot : lay.stepsA) std::printf (" %s(%s)", e.chordName (slot).c_str(), e.roman (slot).c_str());
                    std::printf ("\n");
                }
            }
        // p.11 voicing check: Dsus7(9) = LH D, RH E-G-C-E
        int sus = -1;
        for (int r = 0; r < (int) recipes().size(); ++r) if (recipes()[(size_t) r].name == "Sus vamp") sus = r;
        Settings s; s.key = 2; s.recipeA = sus; e.update (s);
        auto v = e.voice (recipeLayout (sus, -1).stepsA[0], 60, {}, false);
        std::printf ("Dsus7(9) voicing:"); for (int n : v.all()) std::printf (" %s", nm (n).c_str()); std::printf ("\n");
        auto v2 = e.voice (recipeLayout (sus, -1).stepsA[1], 60, {}, false);
        std::printf ("Dsus7(9,11) voicing:"); for (int n : v2.all()) std::printf (" %s", nm (n).c_str()); std::printf ("\n");
        int chain = -1;
        for (int r = 0; r < (int) recipes().size(); ++r) if (recipes()[(size_t) r].name == "Hugo voicing chain") chain = r;
        s.key = 5; s.recipeA = chain; e.update (s);
        for (int slot : recipeLayout (chain, -1).stepsA)
        { auto vv = e.voice (slot, 60, {}, false); std::printf ("%s:", e.chordName (slot).c_str()); for (int n : vv.all()) std::printf (" %s", nm (n).c_str()); std::printf ("\n"); }
    }

    // Moving C major -> F major key should transpose names
    Settings f; f.key = 5; e.update (f);
    CHECK (e.chordName (0).rfind ("F", 0) == 0, "key F tonic should start with F, got %s", e.chordName (0).c_str());

    std::printf ("\n%s (%d failures)\n", failures ? "TESTS FAILED" : "ALL TESTS PASSED", failures);
    return failures ? 1 : 0;
}
