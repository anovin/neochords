# NeoChords

A one-key-per-chord MIDI plugin for gospel, soul and neo-soul. It plays your other instruments (Keyscape, Kontakt, Serum, FL's own synths and so on): you press one key and NeoChords sends a fully voiced chord out over MIDI. It also suggests the next chord and shows each style's signature moves.

## How the keys work

The mapping repeats in every octave. The octave you play in sets the register.

| Keys | What you get |
|---|---|
| **White keys** C D E F G A B | The 7 chords of your chosen key and scale (I ii iii IV V vi vii in major) |
| **Black keys** C♯ D♯ F♯ G♯ A♯ | 5 "colour" chords picked for the current style |

Change the Key and Scale and every chord follows, so all 12 keys work in every mode. Each style has its own black-key colour chords:

| Style | Black-key colour chords | Character |
|---|---|---|
| Gospel | ♯I°7, III7♯9, ♯IV°7, VI13♭9, IV/V | Walk-ups, passing diminished chords, secondary dominants, Amen endings |
| Neo-Soul | ♭IImaj9, ♭IIImaj9, iv m9, ♭VImaj9, ♭VII13 | Lush borrowed colours, sus dominants, backdoor cadences |
| Sade | ♭IImaj7, IV9, v m9, ♭VImaj9, ♭VII6/9 | Minor and Dorian vamps, quiet-storm smoothness |
| Frank Ocean | II add9, IIImaj9, iv m9, ♭VImaj9, ♭VII add9 | Open add9 colours, Lydian II, bittersweet minor iv |
| Pharrell / Neptunes | ♭II, ♭III and ♭VI maj9♯11, ♯ivø, ♭VII13 | Lydian sparkle, chromatic-mediant jumps |
| Tyler, the Creator | III7♯9, ♭IIImaj9, iv m9, ♭VImaj9, ♭VII add9 | Jazzy-pop borrowed chords, chromatic moves |
| OutKast | ♭II13, ♭III9, IV9, ♭VI9, ♭VII9 | Funk dominants, Dorian vamps, tritone turnarounds |

The style also changes the diatonic chords. For example, Gospel's tonic is a 6/9 and its V is a 13, Neo-Soul and Sade use 13sus4 dominants, and Pharrell uses maj9♯11.

These profiles are a musical interpretation of each artist's typical harmonic vocabulary, not transcriptions of specific songs.

## Controls

- **Key / Scale**: Major, Natural Minor, Dorian, Mixolydian, Lydian, Harmonic Minor, Melodic Minor
- **Style**: the seven profiles above
- **Complexity**: Triads, 7ths, 9ths, or Lush (11ths and 13ths)
- **Voicing**:
  - Spread (LH + RH): bass and 7th in the left hand, colour tones in the right. Classic neo-soul piano.
  - Rootless + bass
  - Close
  - Drop 2
- **Voice leading**: picks the inversion that moves least from the last chord, so progressions glide
- **Octave**: shifts the whole register
- **Strum**: rolls the chord from the bass upward (0–120 ms)
- **Humanize**: random velocity variation

The **Try Next** panel ranks five follow-up chords and gives the reason for each (resolutions, chromatic slides, the style's signature progressions). The same five are numbered on the key map. You can click any key or suggestion to hear it.

## Install (Mac)

1. Download `NeoChords-mac.zip` from the repo's **Releases** page ("latest") and unzip it.
2. Right-click `Install-NeoChords.command` and choose **Open**. You only need to right-click the first time, because the build isn't notarised by Apple.
3. In FL Studio, go to **Options → Manage plugins → Find installed plugins**.

## Wire it to an instrument in FL Studio

1. Add **NeoChords** (VST3) to the Channel Rack and select that channel when you play.
2. Open NeoChords and click the **gear icon** in the plugin window's top bar to open the wrapper settings. Under **MIDI**, set **Output port** to a free number, for example **1**.
3. Open your instrument's wrapper settings (gear icon) and set its MIDI **Input port** to the same number.
4. Play keys on the NeoChords channel. Your instrument plays the chords.

**Recording:** record your one-finger part into a pattern on the NeoChords channel. NeoChords turns it into chords on playback, so you can change the key, style or voicing later without re-recording anything.

## Notes

- **Pitch bend** is not forwarded. FL Studio mishandles pitch bend sent from VST3 plugins, so bend on the instrument directly. Sustain pedal, mod wheel and other CCs are forwarded.
- **Use the VST3 version** in FL Studio. The AU build is there for other Mac hosts such as Logic.

## Building from source

The GitHub Actions workflow builds a universal (Apple Silicon + Intel) VST3 and AU on every push to `main`. To build locally you need CMake 3.22+ and Xcode command line tools:

```
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --target NeoChords_VST3 NeoChords_AU
```

The engine has a standalone test: `cmake --build build --target EngineTest && ./build/EngineTest`.

The plugin is built with JUCE 8, which is free for personal use under its licence terms (AGPLv3, or JUCE's free personal licence).
