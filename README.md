# NoviChords

A one-key-per-chord MIDI plugin for gospel, soul and neo-soul. It plays your other instruments (Keyscape, Kontakt, Serum, FL's own synths and so on): you press one key and NoviChords sends a fully voiced chord out over MIDI. It also suggests the next chord and shows each style's signature moves.

## How the keys work

The mapping repeats in every octave. The octave you play in sets the register.

| Keys | What you get |
|---|---|
| **White keys** C D E F G A B | The 7 chords of your chosen key and scale (I ii iii IV V vi vii in major) |
| **Black keys** C♯ D♯ F♯ G♯ A♯ | 5 "colour" chords picked for the current style |

Change the key or scale and every chord follows. All 12 keys work with all 7 modes (Ionian, Dorian, Phrygian, Lydian, Mixolydian, Aeolian, Locrian) plus harmonic and melodic minor.

| Style | Major-key colour chords | Minor-key colour chords |
|---|---|---|
| Gospel | ♯I°7, III7♯9, ♯IV°7, VI13♭9, IV/V | ♭IImaj7, V7♭9, ♯ivø, IV9, II9 |
| Soul | II9, ♭IIImaj7, iv m7, ♭VI9, ♭VII9 | ♭IImaj7, IV9, V7♯9, ♭VI9, II9 |
| Neo-Soul | ♭IImaj9, ♭IIImaj9, iv m9, ♭VImaj9, ♭VII13 | ♭IImaj9, IV9, v m9, ♭VI9, V7♯9 |

The style also changes the diatonic chords. For example, Gospel's tonic is a 6/9, Soul uses 9th chords, and Neo-Soul uses 13sus dominants and maj9♯11 colours.

## Controls

- **Key / Scale**: any of the 12 keys, 9 scales
- **Style**: Gospel, Soul, Neo-Soul
- **Complexity**: Triads, 7ths, 9ths, or Lush (11ths and 13ths)
- **Voicing**: Spread (LH + RH), Rootless + bass, Close, Drop 2
- **Smooth voice leading**: off by default, so a key plays the identical chord every time. Turn it on to have inversions follow the previous chord.
- **Octave**, **Strum** (0–120 ms), **Humanize** (velocity variation, off by default)

**Try Next** ranks five follow-up chords and says why each fits.

**Circle of fifths**: majors on the outer ring, relative minors on the inner ring. Click a segment to change key. Your key's chords are lit with their numerals, the style's colour chords are outlined in rose, the chord you're playing is gold, and the numbered suggestions appear where they sit on the circle. The centre shows the relative key and the neighbouring keys (one step either way is the smoothest key change).

**Chord recorder**:
1. Press **● Rec**, play your chords, then **Stop**. The panel shows "MIDI captured" with a mini piano roll.
2. Pick a **Snap** grid (or No snap).
3. Drag the **Drag MIDI into FL Studio** strip onto a channel, the piano roll or the playlist. **Save .mid** writes the file to `Music/NoviChords` and shows it in Finder.

The recorder captures the full voiced chords at the project tempo.

## Install (Mac)

1. Download `NoviChords-mac.zip` from the repo's **Releases** page ("latest") and unzip it.
2. Right-click `Install-NoviChords.command` and choose **Open**. You only need to right-click the first time, because the build isn't notarised by Apple.
3. In FL Studio, go to **Options → Manage plugins → Find installed plugins**.

## Wire it to an instrument in FL Studio

1. Add **NoviChords** (VST3) to the Channel Rack and select that channel when you play.
2. Open NoviChords and click the **gear icon** in the plugin window's top bar to open the wrapper settings. Under **MIDI**, set **Output port** to a free number, for example **1**.
3. Open your instrument's wrapper settings (gear icon) and set its MIDI **Input port** to the same number.
4. Play keys on the NoviChords channel. Your instrument plays the chords.

**Recording:** record your one-finger part into a pattern on the NoviChords channel. NoviChords turns it into chords on playback, so you can change the key, style or voicing later without re-recording anything.

## Notes

- **Pitch bend** is not forwarded. FL Studio mishandles pitch bend sent from VST3 plugins, so bend on the instrument directly. Sustain pedal, mod wheel and other CCs are forwarded.
- **Use the VST3 version** in FL Studio. The AU build is there for other Mac hosts such as Logic.

## Building from source

The GitHub Actions workflow builds a universal (Apple Silicon + Intel) VST3 and AU on every push to `main`. To build locally you need CMake 3.22+ and Xcode command line tools:

```
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --target NoviChords_VST3 NoviChords_AU
```

The engine has a standalone test: `cmake --build build --target EngineTest && ./build/EngineTest`.

The plugin is built with JUCE 8, which is free for personal use under its licence terms (AGPLv3, or JUCE's free personal licence).
