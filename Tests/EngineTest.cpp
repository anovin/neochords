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
        s.key = 0; s.style = (Style) st; s.scale = Scale::Major;
        e.update (s);
        std::printf ("\n=== %s (C major, lush, spread) ===\n", styleName ((Style) st));
        std::vector<int> prev;
        for (int k = 0; k < 12; ++k)
        {
            auto v = e.voice (k, 60, prev, true);
            std::printf ("  %-3s %-9s %-14s  notes:", ChordEngine::keyLabel (k).c_str(), e.roman (k).c_str(), e.chordName (k).c_str());
            for (int n : v.all()) std::printf (" %d", n);
            std::printf ("\n");
        }
        for (auto& p : styleInfo ((Style) st).progressions)
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

    // Moving C major -> F major key should transpose names
    Settings f; f.key = 5; e.update (f);
    CHECK (e.chordName (0).rfind ("F", 0) == 0, "key F tonic should start with F, got %s", e.chordName (0).c_str());

    std::printf ("\n%s (%d failures)\n", failures ? "TESTS FAILED" : "ALL TESTS PASSED", failures);
    return failures ? 1 : 0;
}
