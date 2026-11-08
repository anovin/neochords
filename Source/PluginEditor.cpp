#include "PluginEditor.h"

using namespace neo;

namespace
{
    // NoviChords palette: #A30000 red, #FF7700 orange, #004777 deep blue, #EFD28D sand, #00AFB5 teal
    const juce::Colour red       { 0xffa30000 };
    const juce::Colour orange    { 0xffff7700 };
    const juce::Colour deepBlue  { 0xff004777 };
    const juce::Colour sand      { 0xffefd28d };
    const juce::Colour teal      { 0xff00afb5 };

    const juce::Colour navy      = deepBlue.darker (0.45f);                   // dark base (panels, text on light keys)
    const juce::Colour cyan      = teal;
    const juce::Colour blue      = red;                                       // your key's chords on the circle
    const juce::Colour green     = teal;                                      // "MIDI captured", drag strip
    const juce::Colour yellow    = orange;
    const juce::Colour plum      = navy;
    const juce::Colour indigo    = deepBlue;                                  // raised surfaces, borders, black keys
    const juce::Colour mint      = juce::Colours::white.interpolatedWith (sand, 0.35f);

    const juce::Colour bg        = deepBlue.darker (0.8f);                    // window background
    const juce::Colour panel     = navy;                                      // panels
    const juce::Colour panelEdge = indigo;                                    // borders
    const juce::Colour cream     = mint;                                      // main text
    const juce::Colour muted     = sand.interpolatedWith (navy, 0.4f);        // secondary text
    const juce::Colour gold      = orange;                                    // accent / active
    const juce::Colour rose      = teal;                                      // suggestion badges, colour chords
    const juce::Colour recRed    = red.brighter (0.6f);
    const juce::Colour ivory     = juce::Colours::white.interpolatedWith (sand, 0.5f);                                      // white keys
    const juce::Colour ebony     = indigo;                                    // black keys

    constexpr int W = 1200, H = 780;

    juce::String u (const std::string& s) { return juce::String::fromUTF8 (s.c_str()); }
    juce::String u8 (const char* s) { return juce::String::fromUTF8 (s); }

    juce::Font font (float size, bool bold = false)
    {
        return juce::Font (juce::FontOptions (size, bold ? juce::Font::bold : juce::Font::plain));
    }

    juce::String noteWithOctave (int n)
    {
        static const char* names[] = { "C", "C♯", "D", "E♭", "E", "F", "F♯", "G", "A♭", "A", "B♭", "B" };
        return u8 (names[n % 12]) + juce::String (n / 12 - 1);
    }

    // Circle of fifths labels, clockwise from the top
    const char* kMajorNames[12] = { "C", "G", "D", "A", "E", "B", "F♯", "D♭", "A♭", "E♭", "B♭", "F" };
    const char* kMinorNames[12] = { "Am", "Em", "Bm", "F♯m", "C♯m", "G♯m", "E♭m", "B♭m", "Fm", "Cm", "Gm", "Dm" };
    const char* kSigs[12]       = { "", "1♯", "2♯", "3♯", "4♯", "5♯", "6♯/6♭", "5♭", "4♭", "3♭", "2♭", "1♭" };

    bool majorLike (Family f)
    {
        return f == Family::Major || f == Family::Dominant || f == Family::Sus || f == Family::AugMaj;
    }

    void drawPanel (juce::Graphics& g, juce::Rectangle<float> r, const juce::String& title)
    {
        g.setColour (panel);
        g.fillRoundedRectangle (r, 10.0f);
        g.setColour (panelEdge);
        g.drawRoundedRectangle (r, 10.0f, 1.0f);
        g.setColour (muted);
        g.setFont (font (11.0f, true));
        g.drawText (title, r.reduced (16.0f, 12.0f).removeFromTop (14.0f), juce::Justification::centredLeft);
    }
}

//==============================================================================
NoviChordsEditor::Theme::Theme()
{
    setColour (juce::ComboBox::backgroundColourId, panel);
    setColour (juce::ComboBox::textColourId, cream);
    setColour (juce::ComboBox::outlineColourId, panelEdge);
    setColour (juce::ComboBox::arrowColourId, gold);
    setColour (juce::ComboBox::focusedOutlineColourId, gold);
    setColour (juce::PopupMenu::backgroundColourId, panel);
    setColour (juce::PopupMenu::textColourId, cream);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, gold);
    setColour (juce::PopupMenu::highlightedTextColourId, bg);
    setColour (juce::Slider::backgroundColourId, panel);
    setColour (juce::Slider::trackColourId, gold.withAlpha (0.55f));
    setColour (juce::Slider::textBoxTextColourId, cream);
    setColour (juce::Slider::textBoxOutlineColourId, panelEdge);
    setColour (juce::Label::textColourId, muted);
    setColour (juce::TextButton::buttonColourId, ebony);
    setColour (juce::TextButton::buttonOnColourId, recRed);
    setColour (juce::TextButton::textColourOffId, cream);
    setColour (juce::TextButton::textColourOnId, cream);
}

void NoviChordsEditor::Theme::drawToggleButton (juce::Graphics& g, juce::ToggleButton& b, bool hover, bool)
{
    auto r = b.getLocalBounds().toFloat();
    auto pill = r.removeFromLeft (38.0f).withSizeKeepingCentre (34.0f, 18.0f);
    const bool on = b.getToggleState();
    g.setColour (on ? gold : panelEdge);
    g.fillRoundedRectangle (pill, 9.0f);
    g.setColour (on ? bg : (hover ? cream : muted));
    const float x = on ? pill.getRight() - 16.0f : pill.getX() + 2.0f;
    g.fillEllipse (x, pill.getY() + 2.0f, 14.0f, 14.0f);
    g.setColour (cream);
    g.setFont (font (13.0f));
    g.drawText (b.getButtonText(), r.withTrimmedLeft (6.0f), juce::Justification::centredLeft);
}

void NoviChordsEditor::Theme::drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour& base, bool hover, bool down)
{
    auto r = b.getLocalBounds().toFloat().reduced (0.5f);
    auto c = b.getToggleState() ? b.findColour (juce::TextButton::buttonOnColourId) : base;
    if (! b.isEnabled()) c = c.withAlpha (0.4f);
    else if (down)       c = c.brighter (0.2f);
    else if (hover)      c = c.brighter (0.1f);
    g.setColour (c);
    g.fillRoundedRectangle (r, 6.0f);
    g.setColour (panelEdge.brighter (0.2f));
    g.drawRoundedRectangle (r, 6.0f, 1.0f);
}

//==============================================================================
NoviChordsEditor::NoviChordsEditor (NoviChordsProcessor& p)
    : AudioProcessorEditor (&p), proc (p)
{
    setLookAndFeel (&theme);

    auto addLabel = [this] (juce::Component& c, const juce::String& text) {
        auto* l = labels.add (new juce::Label ({}, text.toUpperCase()));
        l->setFont (font (11.0f, true));
        l->attachToComponent (&c, false);
        addAndMakeVisible (l);
    };

    auto setupCombo = [this, addLabel] (juce::ComboBox& box, const char* paramId, const juce::String& label) {
        auto* param = dynamic_cast<juce::AudioParameterChoice*> (proc.apvts.getParameter (paramId));
        box.addItemList (param->choices, 1);
        addAndMakeVisible (box);
        comboAttachments.push_back (std::make_unique<ComboAttach> (proc.apvts, paramId, box));
        addLabel (box, label);
    };

    setupCombo (keyBox,        ParamID::key,        "Key");
    setupCombo (scaleBox,      ParamID::scale,      "Scale / mode");
    setupCombo (styleBox,      ParamID::style,      "Style");
    setupCombo (complexityBox, ParamID::complexity, "Complexity");
    setupCombo (voicingBox,    ParamID::voicing,    "Voicing");

    auto setupSlider = [this, addLabel] (juce::Slider& s, const char* paramId, const juce::String& label, const juce::String& suffix) {
        s.setSliderStyle (juce::Slider::LinearBar);
        s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 0, 0);
        s.setTextValueSuffix (suffix);
        s.setColour (juce::Slider::textBoxTextColourId, cream);
        addAndMakeVisible (s);
        sliderAttachments.push_back (std::make_unique<SliderAttach> (proc.apvts, paramId, s));
        addLabel (s, label);
    };

    setupSlider (octaveSlider,   ParamID::octave,   "Octave", "");
    setupSlider (strumSlider,    ParamID::strum,    "Strum",  " ms");
    setupSlider (humanizeSlider, ParamID::humanize, "Humanize", " %");

    addAndMakeVisible (voiceLeadButton);
    voiceLeadAttachment = std::make_unique<ButtonAttach> (proc.apvts, ParamID::voiceLead, voiceLeadButton);
    voiceLeadButton.setTooltip ("Off: every key plays the same voicing each time. On: inversions follow the previous chord.");

    // Recorder
    quantizeBox.addItemList (NoviChordsProcessor::quantizeNames(), 1);
    quantizeBox.setSelectedItemIndex (proc.quantizeIndex, juce::dontSendNotification);
    quantizeBox.onChange = [this] { proc.quantizeIndex = quantizeBox.getSelectedItemIndex(); repaint(); };
    addAndMakeVisible (quantizeBox);

    recButton.setClickingTogglesState (false);
    recButton.onClick   = [this] { recorderMessage.clear(); proc.startRecording(); updateRecorderButtons(); };
    stopButton.onClick  = [this] { proc.stopRecording(); updateRecorderButtons(); };
    clearButton.onClick = [this] { recorderMessage.clear(); proc.clearRecording(); updateRecorderButtons(); };
    saveButton.onClick  = [this] { saveMidiToMusicFolder(); };
    for (auto* b : { &recButton, &stopButton, &clearButton, &saveButton })
        addAndMakeVisible (b);

    take = proc.getTake();
    shownTakeVersion = proc.getTakeVersion();

    setSize (W, H);
    refreshEngine();
    updateRecorderButtons();
    startTimerHz (30);
}

NoviChordsEditor::~NoviChordsEditor()
{
    stopTimer();
    if (pressedSlot >= 0)
        proc.auditionSlot (pressedSlot, false);
    setLookAndFeel (nullptr);
}

void NoviChordsEditor::refreshEngine()
{
    shownSettings = proc.currentSettings();
    engine.update (shownSettings);
    currentSlot = proc.lastSlot.load();
    suggestions = engine.suggest (currentSlot >= 0 ? currentSlot : 0, 5);
}

void NoviChordsEditor::updateRecorderButtons()
{
    const bool rec = proc.isRecording();
    const bool hasTake = ! proc.getTake().empty();
    recButton.setToggleState (rec, juce::dontSendNotification);
    recButton.setButtonText (rec ? u8 ("● Rec") : u8 ("● Rec"));
    stopButton.setEnabled (rec);
    clearButton.setEnabled (rec || hasTake);
    saveButton.setEnabled (! rec && hasTake);
}

void NoviChordsEditor::timerCallback()
{
    const int counter = proc.playCounter.load();
    const int tv = proc.getTakeVersion();
    bool dirty = false;

    if (proc.currentSettings() != shownSettings || counter != shownCounter)
    {
        shownCounter = counter;
        refreshEngine();
        dirty = true;
    }
    if (tv != shownTakeVersion)
    {
        shownTakeVersion = tv;
        take = proc.getTake();
        updateRecorderButtons();
        dirty = true;
    }

    if (proc.seqRow.load() != shownSeqRow || proc.seqStep.load() != shownSeqStep)
    {
        shownSeqRow = proc.seqRow.load();
        shownSeqStep = proc.seqStep.load();
        dirty = true;
    }

    if (dirty)
        repaint();
    else if (proc.isRecording())
        repaint (recArea.toNearestInt());   // blink the record light
}

//==============================================================================
void NoviChordsEditor::resized()
{
    const int top = 88, h = 30;
    int x = 24;
    auto place = [&] (juce::Component& c, int w) { c.setBounds (x, top, w, h); x += w + 12; };
    place (keyBox, 76);
    place (scaleBox, 176);
    place (styleBox, 120);
    place (complexityBox, 122);
    place (voicingBox, 150);
    place (octaveSlider, 60);
    place (strumSlider, 70);
    place (humanizeSlider, 70);

    voiceLeadButton.setBounds (W - 24 - 190, 20, 190, 26);

    const float rightX = (float) W - 24.0f - 300.0f;
    keyboardArea = { 24.0f, 136.0f, rightX - 16.0f - 24.0f, 220.0f };

    const float py = 372.0f, ph = (float) H - py - 20.0f;
    nowArea   = { 24.0f, py, 200.0f, ph };
    nextArea  = { 236.0f, py, 296.0f, ph };
    styleArea = { 544.0f, py, keyboardArea.getRight() - 544.0f, ph };

    circleArea = { rightX, 128.0f, 300.0f, 342.0f };
    recArea    = { rightX, 482.0f, 300.0f, (float) H - 482.0f - 20.0f };

    // harmony map: view tabs in the title row, content between title and hint line
    {
        const float tw = 46.0f, gap = 4.0f;
        float tx = circleArea.getRight() - 14.0f - 3.0f * tw - 2.0f * gap;
        for (auto& t : viewTabs) { t = { tx, circleArea.getY() + 9.0f, tw, 19.0f }; tx += tw + gap; }
        mapContent = circleArea.withTrimmedTop (36.0f).withTrimmedBottom (28.0f).reduced (10.0f, 0.0f);
    }

    // circle geometry
    const float d = juce::jmin (circleArea.getWidth() - 48.0f, circleArea.getHeight() - 70.0f);
    circleCentre = { circleArea.getCentreX(), circleArea.getY() + 44.0f + d * 0.5f };
    rOuter = d * 0.5f;
    rMid = rOuter * 0.69f;
    rInner = rOuter * 0.40f;

    // recorder layout
    auto r = recArea.reduced (14.0f, 12.0f).withTrimmedTop (22.0f);
    auto row = r.removeFromTop (28.0f).toNearestInt();
    recButton.setBounds (row.removeFromLeft (60));   row.removeFromLeft (6);
    stopButton.setBounds (row.removeFromLeft (50));  row.removeFromLeft (6);
    clearButton.setBounds (row.removeFromLeft (50)); row.removeFromLeft (8);
    quantizeBox.setBounds (row);
    r.removeFromTop (24.0f);                          // status line
    rollArea = r.removeFromTop (78.0f);
    r.removeFromTop (8.0f);
    dragStrip = r.removeFromTop (48.0f);
    r.removeFromTop (8.0f);
    saveButton.setBounds (r.removeFromTop (24.0f).removeFromRight (96.0f).toNearestInt());

    // Key rectangles: white first (C D E F G A B), then black on top.
    const float kw = keyboardArea.getWidth() / 7.0f;
    const int whites[7] = { 0, 2, 4, 5, 7, 9, 11 };
    for (int i = 0; i < 7; ++i)
        keyRects[(size_t) whites[i]] = { keyboardArea.getX() + kw * i, keyboardArea.getY(), kw, keyboardArea.getHeight() };

    const int blacks[5] = { 1, 3, 6, 8, 10 };
    const int afterWhite[5] = { 0, 1, 3, 4, 5 };
    const float bw = kw * 0.68f, bh = keyboardArea.getHeight() * 0.56f;
    for (int i = 0; i < 5; ++i)
    {
        const float cx = keyboardArea.getX() + kw * (afterWhite[i] + 1);
        keyRects[(size_t) blacks[i]] = { cx - bw * 0.5f, keyboardArea.getY(), bw, bh };
    }
}

//==============================================================================
int NoviChordsEditor::hitTestSlot (juce::Point<float> p) const
{
    for (int s : { 1, 3, 6, 8, 10 })
        if (keyRects[(size_t) s].contains (p)) return s;
    for (int s : { 0, 2, 4, 5, 7, 9, 11 })
        if (keyRects[(size_t) s].contains (p)) return s;
    for (auto& [r, slot] : suggestionRects)
        if (r.contains (p)) return slot;
    return -1;
}

std::pair<int, int> NoviChordsEditor::hitTestCircle (juce::Point<float> p) const
{
    if (proc.mapView != 0)
        return { -1, -1 };
    const auto d = p - circleCentre;
    const float r = std::hypot (d.x, d.y);
    int ring = -1;
    if (r >= rMid && r <= rOuter) ring = 0;
    else if (r >= rInner && r < rMid) ring = 1;
    if (ring < 0) return { -1, -1 };

    float a = std::atan2 (d.x, -d.y);   // clockwise from 12 o'clock
    if (a < 0) a += juce::MathConstants<float>::twoPi;
    const int idx = (int) std::floor ((a + juce::MathConstants<float>::pi / 12.0f) / (juce::MathConstants<float>::pi / 6.0f)) % 12;
    return { ring, idx };
}

juce::Point<float> NoviChordsEditor::circlePoint (int ring, int index, float radialOffset) const
{
    const float r = (ring == 0 ? (rMid + rOuter) * 0.5f : (rInner + rMid) * 0.5f) + radialOffset;
    const float a = (float) index * juce::MathConstants<float>::pi / 6.0f;
    return { circleCentre.x + r * std::sin (a), circleCentre.y - r * std::cos (a) };
}

std::pair<int, int> NoviChordsEditor::circlePosFor (int pc, Family f) const
{
    pc = ((pc % 12) + 12) % 12;
    if (majorLike (f))
        return { 0, (pc * 7) % 12 };
    return { 1, (((pc - 9 + 12) % 12) * 7) % 12 };
}

int NoviChordsEditor::hitTestMap (juce::Point<float> p) const
{
    if (proc.mapView == 0 || ! mapContent.contains (p))
        return -1;
    for (auto it = mapHits.rbegin(); it != mapHits.rend(); ++it)
        if (it->first.contains (p))
            return it->second;
    return -1;
}

void NoviChordsEditor::mouseMove (const juce::MouseEvent& e)
{
    const int hm = hitTestMap (e.position);
    bool overTab = false;
    for (auto& t : viewTabs) overTab |= t.contains (e.position);
    for (auto& sh : sigHits) overTab |= sh.r.contains (e.position);
    if (hm != hoverMapSlot)
    {
        hoverMapSlot = hm;
        repaint (circleArea.toNearestInt());
    }
    const auto h = hitTestCircle (e.position);
    if (h != hoverCircle)
    {
        hoverCircle = h;
        repaint (circleArea.toNearestInt());
    }
    const bool overStrip = dragStrip.contains (e.position) && ! take.empty() && ! proc.isRecording();
    setMouseCursor (overStrip ? juce::MouseCursor::DraggingHandCursor
                              : (h.first >= 0 || hm >= 0 || overTab || hitTestSlot (e.position) >= 0 ? juce::MouseCursor::PointingHandCursor
                                                                                : juce::MouseCursor::NormalCursor));
}

void NoviChordsEditor::mouseDown (const juce::MouseEvent& e)
{
    dragStarted = false;
    dragFromStrip = dragStrip.contains (e.position) && ! take.empty() && ! proc.isRecording();
    if (dragFromStrip)
        return;

    for (int i = 0; i < 3; ++i)
        if (viewTabs[(size_t) i].contains (e.position))
        {
            proc.mapView = i;
            hoverCircle = { -1, -1 };
            hoverMapSlot = -1;
            repaint (circleArea.toNearestInt());
            return;
        }

    for (const auto& sh : sigHits)
    {
        if (! sh.r.contains (e.position))
            continue;
        if (sh.type == 0)
        {
            if (proc.seqRow.load() == sh.index)
                proc.stopProgression();
            else
            {
                static const double beats[] = { 2.0, 4.0, 8.0 };
                const auto& progs = engine.styleSet().progressions;
                if (sh.index < (int) progs.size())
                    proc.playProgression (sh.index, progs[(size_t) sh.index].keys, beats[juce::jlimit (0, 2, proc.sigLengthIndex)], proc.sigLoop);
            }
        }
        else if (sh.type == 1)
        {
            pressedSlot = sh.index;
            proc.auditionSlot (sh.index, true);
        }
        else if (sh.type == 2) proc.sigLengthIndex = sh.index;
        else if (sh.type == 3) proc.sigLoop = ! proc.sigLoop;
        repaint (styleArea.toNearestInt());
        return;
    }

    if (const int mapSlot = hitTestMap (e.position); mapSlot >= 0)
    {
        pressedSlot = mapSlot;
        proc.auditionSlot (mapSlot, true);
        return;
    }

    const auto [ring, idx] = hitTestCircle (e.position);
    if (ring >= 0)
    {
        const bool minorNow = scaleIsMinor (shownSettings.scale);
        if (ring == 0)
        {
            const int pc = (idx * 7) % 12;
            proc.setKeyAndScale (pc, minorNow ? Scale::Ionian : shownSettings.scale);
        }
        else
        {
            const int pc = (idx * 7 + 9) % 12;
            proc.setKeyAndScale (pc, minorNow ? shownSettings.scale : Scale::Aeolian);
        }
        return;
    }

    const int slot = hitTestSlot (e.position);
    if (slot >= 0)
    {
        pressedSlot = slot;
        proc.auditionSlot (slot, true);
    }
}

void NoviChordsEditor::mouseDrag (const juce::MouseEvent& e)
{
    if (! dragFromStrip || dragStarted || e.getDistanceFromDragStart() < 4)
        return;

    dragStarted = true;
    const auto file = proc.writeTakeToTempMidi (proc.quantizeIndex);
    if (file.existsAsFile())
    {
        recorderMessage = "Dropped: " + file.getFileName();
        juce::DragAndDropContainer::performExternalDragDropOfFiles ({ file.getFullPathName() }, false, this);
    }
    else
    {
        recorderMessage = "Could not write the MIDI file";
    }
    repaint (recArea.toNearestInt());
}

void NoviChordsEditor::mouseUp (const juce::MouseEvent&)
{
    if (pressedSlot >= 0)
    {
        proc.auditionSlot (pressedSlot, false);
        pressedSlot = -1;
    }
    dragFromStrip = false;
}

void NoviChordsEditor::saveMidiToMusicFolder()
{
    auto temp = proc.writeTakeToTempMidi (proc.quantizeIndex);
    if (! temp.existsAsFile())
        return;
    auto dir = juce::File::getSpecialLocation (juce::File::userMusicDirectory).getChildFile ("NoviChords");
    dir.createDirectory();
    auto dest = dir.getNonexistentChildFile (temp.getFileNameWithoutExtension(), ".mid", false);
    if (temp.copyFileTo (dest))
    {
        recorderMessage = "Saved to Music/NoviChords";
        dest.revealToUser();
    }
    repaint (recArea.toNearestInt());
}

//==============================================================================
void NoviChordsEditor::paint (juce::Graphics& g)
{
    g.fillAll (bg);

    g.setColour (cream);
    g.setFont (font (26.0f, true));
    g.drawText ("NoviChords", 24, 14, 200, 32, juce::Justification::centredLeft);
    g.setColour (muted);
    g.setFont (font (13.0f));
    g.drawText (u8 ("gospel · soul · neo-soul chord engine"), 170, 20, 400, 24, juce::Justification::centredLeft);
    g.setColour (panelEdge);
    g.fillRect (24.0f, 58.0f, (float) W - 48.0f, 1.0f);

    paintKeyboard (g);
    paintNowPlaying (g, nowArea);
    paintSuggestions (g, nextArea);
    paintStyle (g, styleArea);
    paintMapPanel (g, circleArea);
    paintRecorder (g, recArea);
}

void NoviChordsEditor::paintKeyboard (juce::Graphics& g)
{
    std::array<int, 12> rank {};
    for (size_t i = 0; i < suggestions.size(); ++i)
        rank[(size_t) suggestions[i].slot] = (int) i + 1;

    auto drawBadge = [&] (juce::Rectangle<float> keyR, int r) {
        if (r == 0) return;
        auto b = juce::Rectangle<float> (keyR.getRight() - 24.0f, keyR.getY() + 6.0f, 18.0f, 18.0f);
        g.setColour (rose);
        g.fillEllipse (b);
        g.setColour (bg);
        g.setFont (font (11.0f, true));
        g.drawText (juce::String (r), b, juce::Justification::centred);
    };

    auto drawLabels = [&] (int slot, juce::Rectangle<float> area, bool active, bool dark) {
        const auto textCol = active ? bg : (dark ? cream : ebony);
        g.setColour (active ? navy : (dark ? gold : navy));
        g.setFont (font (dark ? 12.0f : 14.0f, true));
        g.drawText (u (engine.roman (slot)), area.removeFromTop (dark ? 16.0f : 20.0f), juce::Justification::centred);
        g.setColour (textCol);
        g.setFont (font (dark ? 13.0f : 16.0f, true));
        g.drawFittedText (u (engine.chordName (slot)), area.removeFromTop (dark ? 34.0f : 24.0f).toNearestInt(),
                          juce::Justification::centred, 2, 0.8f);
        g.setColour (active ? bg.withAlpha (0.7f) : (dark ? muted : navy.withAlpha (0.6f)));
        g.setFont (font (11.0f));
        g.drawText ("key " + u (ChordEngine::keyLabel (slot)), area.removeFromTop (16.0f), juce::Justification::centred);
    };

    for (int s : { 0, 2, 4, 5, 7, 9, 11 })
    {
        auto r = keyRects[(size_t) s].reduced (2.0f, 0.0f);
        const bool active = s == currentSlot;
        g.setColour (active ? gold : ivory);
        g.fillRoundedRectangle (r, 6.0f);
        if (active)
        {
            g.setColour (gold.withAlpha (0.35f));
            g.drawRoundedRectangle (r.expanded (2.0f), 8.0f, 3.0f);
        }
        drawLabels (s, r.withTop (r.getBottom() - 74.0f).reduced (4.0f, 0.0f), active, false);
        drawBadge (r.withTop (r.getY() + keyboardArea.getHeight() * 0.56f), rank[(size_t) s]);
    }

    for (int s : { 1, 3, 6, 8, 10 })
    {
        auto r = keyRects[(size_t) s];
        const bool active = s == currentSlot;
        g.setColour (juce::Colours::black.withAlpha (0.35f));
        g.fillRoundedRectangle (r.translated (0.0f, 3.0f), 6.0f);
        g.setColour (active ? gold : ebony);
        g.fillRoundedRectangle (r, 6.0f);
        g.setColour (active ? gold.brighter (0.3f) : panelEdge);
        g.drawRoundedRectangle (r, 6.0f, 1.0f);
        drawLabels (s, r.reduced (4.0f, 0.0f).withTop (r.getBottom() - 72.0f), active, true);
        drawBadge (r, rank[(size_t) s]);
    }
}

void NoviChordsEditor::paintNowPlaying (juce::Graphics& g, juce::Rectangle<float> r)
{
    drawPanel (g, r, "NOW PLAYING");
    auto a = r.reduced (16.0f, 12.0f).withTrimmedTop (22.0f);

    if (currentSlot < 0)
    {
        g.setColour (cream);
        g.setFont (font (14.0f));
        g.drawFittedText (u8 ("Play a key on your controller, or click the keys above.\n\nWhite keys = the 7 chords of your key and mode.\nBlack keys = the style's colour chords."),
                          a.toNearestInt(), juce::Justification::topLeft, 10);
        return;
    }

    g.setColour (cream);
    g.setFont (font (32.0f, true));
    g.drawFittedText (u (engine.chordName (currentSlot)), a.removeFromTop (42.0f).toNearestInt(), juce::Justification::centredLeft, 1, 0.5f);
    g.setColour (gold);
    g.setFont (font (17.0f, true));
    g.drawText (u (engine.roman (currentSlot)) + u8 ("  · key ") + u (ChordEngine::keyLabel (currentSlot)),
                a.removeFromTop (26.0f), juce::Justification::centredLeft);

    a.removeFromTop (12.0f);
    g.setColour (muted);
    g.setFont (font (11.0f, true));
    g.drawText ("NOTES SENT", a.removeFromTop (16.0f), juce::Justification::centredLeft);

    juce::StringArray notes;
    std::vector<int> noteNums;
    const int n = proc.lastNoteCount.load();
    for (int i = 0; i < n; ++i)
    {
        noteNums.push_back (proc.lastNotes[(size_t) i].load());
        notes.add (noteWithOctave (noteNums.back()));
    }
    g.setColour (cream);
    g.setFont (font (13.0f));
    g.drawFittedText (notes.joinIntoString ("  "), a.removeFromTop (36.0f).toNearestInt(), juce::Justification::topLeft, 2);
    a.removeFromTop (4.0f);
    drawMiniKeys (g, a.removeFromTop (54.0f), noteNums);

    g.setColour (muted);
    g.setFont (font (12.0f));
    g.drawFittedText (u8 (scaleName (shownSettings.scale)) + "\n" + complexityName (shownSettings.complexity)
                      + "\n" + u8 (voicingName (shownSettings.voicing)),
                      a.toNearestInt(), juce::Justification::bottomLeft, 3);
}

void NoviChordsEditor::paintSuggestions (juce::Graphics& g, juce::Rectangle<float> r)
{
    const juce::String from = currentSlot >= 0 ? u (engine.chordName (currentSlot)) : u (engine.chordName (0));
    drawPanel (g, r, u8 ("TRY NEXT  →  after ") + from);
    auto a = r.reduced (12.0f, 12.0f).withTrimmedTop (24.0f);

    suggestionRects.clear();
    const float rowH = juce::jmin (48.0f, a.getHeight() / 5.0f);

    for (size_t i = 0; i < suggestions.size(); ++i)
    {
        const auto& sg = suggestions[i];
        auto row = a.removeFromTop (rowH).reduced (0.0f, 2.0f);
        suggestionRects.push_back ({ row, sg.slot });

        const bool pressed = sg.slot == pressedSlot;
        g.setColour (pressed ? gold.withAlpha (0.25f) : juce::Colours::white.withAlpha (i % 2 ? 0.0f : 0.025f));
        g.fillRoundedRectangle (row, 6.0f);

        auto badge = row.removeFromLeft (30.0f).withSizeKeepingCentre (20.0f, 20.0f);
        g.setColour (rose);
        g.fillEllipse (badge);
        g.setColour (bg);
        g.setFont (font (11.0f, true));
        g.drawText (juce::String ((int) i + 1), badge, juce::Justification::centred);

        auto keyTag = row.removeFromRight (60.0f);
        g.setColour (gold);
        g.setFont (font (12.0f, true));
        g.drawText ("key " + u (ChordEngine::keyLabel (sg.slot)), keyTag, juce::Justification::centredRight);

        auto top = row.removeFromTop (row.getHeight() * 0.52f);
        g.setColour (cream);
        g.setFont (font (15.0f, true));
        g.drawText (u (engine.chordName (sg.slot)), top.withTrimmedLeft (6.0f), juce::Justification::bottomLeft);
        g.setColour (gold.withAlpha (0.85f));
        g.setFont (font (12.0f, true));
        g.drawText (u (engine.roman (sg.slot)), top.withTrimmedLeft (150.0f), juce::Justification::bottomLeft);

        g.setColour (muted);
        g.setFont (font (11.5f));
        g.drawText (u (sg.reason), row.withTrimmedLeft (6.0f), juce::Justification::topLeft, true);
    }
}

void NoviChordsEditor::drawMiniKeys (juce::Graphics& g, juce::Rectangle<float> r, const std::vector<int>& notes)
{
    if (notes.empty()) return;
    const int lo = (*std::min_element (notes.begin(), notes.end()) / 12) * 12;
    int hi = (*std::max_element (notes.begin(), notes.end()) / 12) * 12 + 11;
    hi = juce::jmax (hi, lo + 23);
    auto isBlack = [] (int n) { const int pc = n % 12; return pc == 1 || pc == 3 || pc == 6 || pc == 8 || pc == 10; };
    auto on = [&] (int n) { return std::find (notes.begin(), notes.end(), n) != notes.end(); };

    int whites = 0;
    for (int n = lo; n <= hi; ++n) whites += isBlack (n) ? 0 : 1;
    const float kw = r.getWidth() / (float) whites;

    std::vector<std::pair<int, float>> blacks;
    float x = r.getX();
    for (int n = lo; n <= hi; ++n)
    {
        if (isBlack (n)) { blacks.push_back ({ n, x }); continue; }
        auto k = juce::Rectangle<float> (x, r.getY(), kw, r.getHeight()).reduced (0.5f, 0.0f);
        g.setColour (on (n) ? gold : ivory);
        g.fillRoundedRectangle (k, 2.0f);
        if (n % 12 == 0)
        {
            g.setColour (navy.withAlpha (0.55f));
            g.setFont (font (7.5f));
            g.drawText ("C" + juce::String (n / 12 - 1), k.withTop (k.getBottom() - 10.0f), juce::Justification::centred);
        }
        x += kw;
    }
    for (auto [n, bx] : blacks)
    {
        auto k = juce::Rectangle<float> (bx - kw * 0.32f, r.getY(), kw * 0.64f, r.getHeight() * 0.6f);
        g.setColour (on (n) ? gold : navy);
        g.fillRoundedRectangle (k, 1.5f);
    }
}

void NoviChordsEditor::paintStyle (juce::Graphics& g, juce::Rectangle<float> r)
{
    const auto& si = styleInfo (shownSettings.style);
    const auto& set = engine.styleSet();
    drawPanel (g, r, u8 ("SIGNATURE MOVES  ·  ") + u (si.name).toUpperCase() + (engine.minorMode() ? u8 (" · MINOR") : u8 (" · MAJOR")));
    sigHits.clear();
    auto a = r.reduced (14.0f, 12.0f).withTrimmedTop (22.0f);

    g.setColour (cream.withAlpha (0.8f));
    g.setFont (font (11.0f));
    g.drawFittedText (u (si.blurb), a.removeFromTop (30.0f).toNearestInt(), juce::Justification::topLeft, 2, 0.8f);
    a.removeFromTop (4.0f);

    // playback controls: chord length + loop
    {
        auto row = a.removeFromTop (22.0f);
        g.setColour (muted);
        g.setFont (font (10.0f, true));
        g.drawText ("EACH CHORD", row.removeFromLeft (70.0f), juce::Justification::centredLeft);
        static const char* lens[] = { u8"½ bar", "1 bar", "2 bars" };
        for (int i = 0; i < 3; ++i)
        {
            auto pill = row.removeFromLeft (50.0f).reduced (2.0f, 1.0f);
            const bool sel = proc.sigLengthIndex == i;
            g.setColour (sel ? gold : panelEdge.withAlpha (0.6f));
            g.fillRoundedRectangle (pill, 9.0f);
            g.setColour (sel ? bg : cream);
            g.setFont (font (10.5f, sel));
            g.drawText (juce::String::fromUTF8 (lens[i]), pill, juce::Justification::centred);
            sigHits.push_back ({ pill, 2, i });
        }
        auto loop = row.removeFromRight (56.0f).reduced (2.0f, 1.0f);
        g.setColour (proc.sigLoop ? rose : panelEdge.withAlpha (0.6f));
        g.fillRoundedRectangle (loop, 9.0f);
        g.setColour (proc.sigLoop ? bg : cream);
        g.setFont (font (10.5f, proc.sigLoop));
        g.drawText (u8 ("⟳ Loop"), loop, juce::Justification::centred);
        sigHits.push_back ({ loop, 3, 0 });
    }
    a.removeFromTop (6.0f);

    const int playingRow = proc.seqRow.load();
    const int playingStep = proc.seqStep.load();
    const float rowH = a.getHeight() / (float) juce::jmax<size_t> (1, set.progressions.size());

    for (size_t pi = 0; pi < set.progressions.size(); ++pi)
    {
        const auto& p = set.progressions[pi];
        auto row = a.removeFromTop (rowH).reduced (0.0f, 2.0f);
        const bool playing = playingRow == (int) pi;

        g.setColour (playing ? gold.withAlpha (0.10f) : juce::Colours::white.withAlpha (0.025f));
        g.fillRoundedRectangle (row, 7.0f);
        row = row.reduced (6.0f, 3.0f);

        // play / stop button + name
        auto head = row.removeFromTop (20.0f);
        auto btn = head.removeFromLeft (20.0f).withSizeKeepingCentre (20.0f, 20.0f);
        g.setColour (playing ? gold : rose);
        g.fillEllipse (btn);
        g.setColour (bg);
        if (playing)
            g.fillRect (btn.withSizeKeepingCentre (7.0f, 7.0f));
        else
        {
            juce::Path tri;
            const auto c = btn.getCentre();
            tri.addTriangle (c.x - 3.0f, c.y - 5.0f, c.x - 3.0f, c.y + 5.0f, c.x + 5.5f, c.y);
            g.fillPath (tri);
        }
        sigHits.push_back ({ btn.expanded (3.0f), 0, (int) pi });

        g.setColour (cream);
        g.setFont (font (12.5f, true));
        g.drawText (u (p.name), head.withTrimmedLeft (8.0f), juce::Justification::centredLeft, true);
        // the whole title line also toggles playback
        sigHits.push_back ({ head, 0, (int) pi });

        row.removeFromTop (3.0f);
        auto chips = row.withHeight (juce::jmin (40.0f, row.getHeight()));
        const int n = (int) p.keys.size();
        const float gap = 4.0f;
        const float cw = (chips.getWidth() - gap * (float) (n - 1)) / (float) juce::jmax (1, n);
        for (int ci = 0; ci < n; ++ci)
        {
            const int slot = p.keys[(size_t) ci];
            auto chip = juce::Rectangle<float> (chips.getX() + (float) ci * (cw + gap), chips.getY(), cw, chips.getHeight());
            const bool lit = (playing && playingStep == ci) || (! playing && slot == pressedSlot);
            const bool diat = engine.chord (slot).diatonic;
            g.setColour (lit ? gold : (diat ? blue : navy.interpolatedWith (cyan, 0.12f)));
            g.fillRoundedRectangle (chip, 6.0f);
            if (! diat && ! lit)
            {
                g.setColour (rose);
                g.drawRoundedRectangle (chip.reduced (0.5f), 6.0f, 1.2f);
            }
            auto t = chip.reduced (3.0f, 3.0f);
            g.setColour (lit ? bg : cream);
            g.setFont (font (11.5f, true));
            g.drawFittedText (u (engine.chordName (slot)), t.removeFromTop (t.getHeight() * 0.56f).toNearestInt(),
                              juce::Justification::centred, 1, 0.55f);
            g.setColour (lit ? bg : (diat ? sand : rose));
            g.setFont (font (9.0f, true));
            g.drawFittedText (u (engine.roman (slot)) + u8 (" · ") + u (ChordEngine::keyLabel (slot)), t.toNearestInt(),
                              juce::Justification::centred, 1, 0.6f);
            sigHits.push_back ({ chip, 1, slot });
        }
    }
}

//==============================================================================
void NoviChordsEditor::paintMapPanel (juce::Graphics& g, juce::Rectangle<float> area)
{
    static const char* titles[] = { "CIRCLE OF FIFTHS", "HARMONIC GRID", "CHORD FLOW" };
    static const char* tabs[]   = { "Circle", "Grid", "Flow" };
    static const char* hints[]  = { "Click a key on the circle to change key",
                                    "Touching triangles share two notes · click to play",
                                    "Home → Away → Tension → Home · click to play" };
    const int view = juce::jlimit (0, 2, proc.mapView);

    drawPanel (g, area, titles[view]);

    for (int i = 0; i < 3; ++i)
    {
        auto t = viewTabs[(size_t) i];
        const bool on = i == view;
        g.setColour (on ? gold : panelEdge.withAlpha (0.6f));
        g.fillRoundedRectangle (t, 9.5f);
        g.setColour (on ? bg : cream);
        g.setFont (font (11.0f, on));
        g.drawText (tabs[i], t, juce::Justification::centred);
    }

    g.setColour (muted);
    g.setFont (font (10.5f));
    g.drawFittedText (u8 (hints[view]), area.withTop (area.getBottom() - 26.0f).reduced (14.0f, 4.0f).toNearestInt(),
                      juce::Justification::centred, 1, 0.8f);

    mapHits.clear();
    if (view == 0)      paintCircle (g, area);
    else if (view == 1) paintGrid (g, mapContent);
    else                paintFlow (g, mapContent);
}

//==============================================================================
// Harmonic grid (Tonnetz): right = up a fifth, up-right = up a major third, down-right = up a minor third.
// Upward triangles are major triads, downward triangles are minor triads.
void NoviChordsEditor::paintGrid (juce::Graphics& g, juce::Rectangle<float> area)
{
    juce::Graphics::ScopedSaveState save (g);
    g.reduceClipRegion (area.toNearestInt());

    const bool minorNow = engine.minorMode();
    const int key = shownSettings.key;
    const float dx = 64.0f, dy = dx * 0.866f;

    auto pcAt = [key] (int c, int r) { return (((key + 7 * c + 4 * r) % 12) + 12) % 12; };
    const juce::Point<float> tonicCentroidOffset { 0.5f * dx, minorNow ? dy / 3.0f : -dy / 3.0f };
    const auto origin = area.getCentre() - tonicCentroidOffset;
    auto pos = [&] (int c, int r) { return juce::Point<float> { origin.x + ((float) c + (float) r * 0.5f) * dx, origin.y - (float) r * dy }; };

    struct Tri
    {
        juce::Path path;
        juce::Point<float> centroid;
        bool major = true;
        int root = 0;
        juce::String roman;
        bool diatonic = false, colour = false, current = false;
        int suggestion = 0, slot = -1;
    };
    std::vector<Tri> tris;

    for (int r = -5; r <= 5; ++r)
        for (int c = -9; c <= 9; ++c)
            for (int major = 0; major < 2; ++major)
            {
                const auto a = pos (c, r), b = pos (c + 1, r);
                const auto d = major ? pos (c, r + 1) : pos (c + 1, r - 1);
                Tri t;
                t.major = major == 1;
                t.root = pcAt (c, r);
                t.centroid = (a + b + d) / 3.0f;
                if (! area.expanded (14.0f).contains (t.centroid))
                    continue;
                t.path.addTriangle (a, b, d);
                tris.push_back (std::move (t));
            }

    // Place each of the 12 playable chords on its nearest matching triangle
    auto nearest = [&] (bool major, int root) -> Tri* {
        Tri* best = nullptr;
        float bestD = 1e9f;
        for (auto& t : tris)
            if (t.major == major && t.root == root
                && area.reduced (14.0f, 10.0f).withTrimmedBottom (14.0f).contains (t.centroid))
            {
                const float dd = t.centroid.getDistanceFrom (area.getCentre());
                if (dd < bestD) { bestD = dd; best = &t; }
            }
        return best;
    };
    auto triFor = [&] (int slot) -> Tri* {
        const auto fam = info (engine.chord (slot).quality).family;
        if (fam == Family::Dim || fam == Family::HalfDim)
            return nullptr;   // diminished chords are not triangles on this grid
        return nearest (majorLike (fam), engine.absoluteRoot (slot));
    };

    for (int slot = 0; slot < 12; ++slot)
        if (auto* t = triFor (slot))
        {
            const auto rn = u (engine.roman (slot));
            if (t->roman.isEmpty()) t->roman = rn;
            else if (! t->roman.contains (rn)) t->roman << " " << rn;
            if (engine.chord (slot).diatonic) t->diatonic = true; else t->colour = true;
            if (slot == currentSlot) { t->current = true; t->slot = slot; }
            if (t->slot < 0 || (t->diatonic && ! engine.chord (t->slot).diatonic)) t->slot = slot;
        }
    for (size_t i = 0; i < suggestions.size(); ++i)
        if (auto* t = triFor (suggestions[i].slot))
            if (t->suggestion == 0) t->suggestion = (int) i + 1;

    // Triangles
    for (auto& t : tris)
    {
        juce::Colour fill = t.major ? navy.interpolatedWith (cyan, 0.13f) : navy.interpolatedWith (cyan, 0.06f);
        if (t.diatonic) fill = blue;
        if (t.current)  fill = gold;
        if (t.slot >= 0 && t.slot == hoverMapSlot) fill = fill.brighter (0.2f);
        g.setColour (fill);
        g.fillPath (t.path);
        g.setColour (bg);
        g.strokePath (t.path, juce::PathStrokeType (1.5f));
        if (t.colour && ! t.current)
        {
            g.setColour (rose);
            auto inset = t.path;
            inset.applyTransform (juce::AffineTransform::scale (0.82f, 0.82f, t.centroid.x, t.centroid.y));
            g.strokePath (inset, juce::PathStrokeType (1.4f));
        }

        const bool marked = t.slot >= 0;
        if (marked)
            mapHits.push_back ({ t.path, t.slot });

        const auto name = u (engine.noteName (t.root)) + (t.major ? "" : "m");
        const float lift = marked ? 5.0f : 0.0f;
        g.setColour (t.current ? bg : (marked ? cream : muted.withAlpha (0.5f)));
        g.setFont (font (marked ? 12.5f : 10.5f, marked));
        g.drawText (name, juce::Rectangle<float> (40.0f, 13.0f).withCentre ({ t.centroid.x, t.centroid.y - lift }), juce::Justification::centred);
        if (marked)
        {
            g.setColour (t.current ? bg : (t.colour && ! t.diatonic ? rose : (t.diatonic ? sand : gold)));
            g.setFont (font (9.0f, true));
            g.drawFittedText (t.roman, juce::Rectangle<float> (36.0f, 11.0f).withCentre ({ t.centroid.x, t.centroid.y + 7.0f }).toNearestInt(),
                              juce::Justification::centred, 1, 0.6f);
        }
        if (t.suggestion > 0)
        {
            const auto bp = t.centroid + juce::Point<float> (17.0f, t.major ? 8.0f : -8.0f);
            auto b = juce::Rectangle<float> (13.0f, 13.0f).withCentre (bp);
            g.setColour (rose);
            g.fillEllipse (b);
            g.setColour (bg);
            g.setFont (font (9.0f, true));
            g.drawText (juce::String (t.suggestion), b, juce::Justification::centred);
        }
    }

    // Note names on the grid points
    for (int r = -5; r <= 5; ++r)
        for (int c = -9; c <= 9; ++c)
        {
            const auto p = pos (c, r);
            if (! area.expanded (6.0f).contains (p)) continue;
            auto dot = juce::Rectangle<float> (17.0f, 17.0f).withCentre (p);
            const bool tonic = pcAt (c, r) == key;
            g.setColour (tonic ? gold : bg);
            g.fillEllipse (dot);
            g.setColour (tonic ? bg : muted);
            g.setFont (font (8.5f, tonic));
            g.drawText (u (engine.noteName (pcAt (c, r))), dot, juce::Justification::centred);
        }

    // axis legend
    g.setColour (panel.withAlpha (0.85f));
    auto legend = area.withTop (area.getBottom() - 16.0f).withWidth (190.0f);
    g.fillRoundedRectangle (legend, 4.0f);
    g.setColour (muted);
    g.setFont (font (9.5f));
    g.drawText (u8 ("→ 5th   ↗ maj 3rd   ↘ min 3rd"), legend.reduced (5.0f, 0.0f), juce::Justification::centredLeft);
}

//==============================================================================
// Chord flow: chords grouped by job — Home (tonic), Away (pre-dominant), Tension (dominant) —
// plus the style's colour chords, with arrows from what's playing to the suggestions.
void NoviChordsEditor::paintFlow (juce::Graphics& g, juce::Rectangle<float> area)
{
    static const char* heads[] = { "HOME", "AWAY", "TENSION" };
    const std::vector<std::vector<int>> columns = { { 0, 9, 4 }, { 5, 2 }, { 7, 11 } };
    const int colourSlots[5] = { 1, 3, 6, 8, 10 };

    std::array<juce::Rectangle<float>, 12> rects {};
    const float colW = area.getWidth() / 3.0f;
    auto header = area.removeFromTop (18.0f);
    auto colourRow = area.removeFromBottom (54.0f);
    area.removeFromBottom (6.0f);

    // headers with flow arrows between them
    for (int i = 0; i < 3; ++i)
    {
        auto h = header.withX (header.getX() + colW * (float) i).withWidth (colW);
        g.setColour (muted);
        g.setFont (font (10.0f, true));
        g.drawText (heads[i], h, juce::Justification::centred);
        if (i < 2)
        {
            g.setColour (muted.withAlpha (0.6f));
            g.drawText (u8 ("→"), h.withX (h.getRight() - 8.0f).withWidth (16.0f), juce::Justification::centred);
        }
    }

    for (int ci = 0; ci < 3; ++ci)
    {
        const auto& col = columns[(size_t) ci];
        const float nodeH = 38.0f, gapY = 12.0f;
        const float total = (float) col.size() * nodeH + (float) (col.size() - 1) * gapY;
        float y = area.getCentreY() - total * 0.5f;
        for (int slot : col)
        {
            rects[(size_t) slot] = { area.getX() + colW * (float) ci + 5.0f, y, colW - 10.0f, nodeH };
            y += nodeH + gapY;
        }
    }

    g.setColour (muted);
    g.setFont (font (9.5f, true));
    g.drawText ("COLOUR CHORDS", colourRow.removeFromTop (14.0f), juce::Justification::centredLeft);
    {
        const float gap = 5.0f;
        const float w = (colourRow.getWidth() - 4.0f * gap) / 5.0f;
        for (int i = 0; i < 5; ++i)
            rects[(size_t) colourSlots[i]] = { colourRow.getX() + (float) i * (w + gap), colourRow.getY() + 2.0f, w, 36.0f };
    }

    // arrows: playing chord -> each suggestion (or Home -> Away -> Tension -> Home when idle)
    auto edgePoint = [] (juce::Rectangle<float> r, juce::Point<float> towards) {
        const auto c = r.getCentre();
        auto d = towards - c;
        const float tx = std::abs (d.x) > 0.001f ? (r.getWidth() * 0.5f) / std::abs (d.x) : 1e9f;
        const float ty = std::abs (d.y) > 0.001f ? (r.getHeight() * 0.5f) / std::abs (d.y) : 1e9f;
        return c + d * juce::jmin (tx, ty);
    };
    auto drawArrow = [&] (int from, int to, juce::Colour col, float thick) {
        const auto a = rects[(size_t) from], b = rects[(size_t) to];
        const auto p0 = edgePoint (a, b.getCentre());
        const auto p1 = edgePoint (b, a.getCentre());
        juce::Path arrow;
        arrow.addArrow ({ p0, p1 }, thick, thick * 4.5f, thick * 4.5f);
        g.setColour (col);
        g.fillPath (arrow);
    };

    if (currentSlot >= 0)
    {
        for (int i = (int) suggestions.size() - 1; i >= 0; --i)
            drawArrow (currentSlot, suggestions[(size_t) i].slot, rose.withAlpha (i == 0 ? 0.95f : 0.45f), i == 0 ? 2.2f : 1.4f);
    }
    else
    {
        drawArrow (0, 5, muted.withAlpha (0.35f), 1.2f);
        drawArrow (5, 7, muted.withAlpha (0.35f), 1.2f);
        drawArrow (7, 0, muted.withAlpha (0.35f), 1.2f);
    }

    std::array<int, 12> rank {};
    for (size_t i = 0; i < suggestions.size(); ++i)
        rank[(size_t) suggestions[i].slot] = (int) i + 1;

    for (int slot = 0; slot < 12; ++slot)
    {
        auto r = rects[(size_t) slot];
        if (r.isEmpty()) continue;
        const auto& c = engine.chord (slot);
        const bool current = slot == currentSlot;

        juce::Colour fill = c.diatonic ? blue : navy.interpolatedWith (cyan, 0.12f);
        if (current) fill = gold;
        if (slot == hoverMapSlot) fill = fill.brighter (0.2f);
        g.setColour (fill);
        g.fillRoundedRectangle (r, 7.0f);
        if (! c.diatonic && ! current)
        {
            g.setColour (rose);
            g.drawRoundedRectangle (r.reduced (0.5f), 7.0f, 1.3f);
        }

        juce::Path hit;
        hit.addRoundedRectangle (r, 7.0f);
        mapHits.push_back ({ hit, slot });

        auto t = r.reduced (4.0f, 3.0f);
        g.setColour (current ? bg : cream);
        g.setFont (font (c.diatonic ? 12.0f : 10.5f, true));
        g.drawFittedText (u (engine.chordName (slot)), t.removeFromTop (t.getHeight() * 0.55f).toNearestInt(),
                          juce::Justification::centred, 1, 0.6f);
        g.setColour (current ? bg : (c.diatonic ? sand : rose));
        g.setFont (font (9.5f, true));
        g.drawFittedText (u (engine.roman (slot)), t.toNearestInt(), juce::Justification::centred, 1, 0.6f);

        if (rank[(size_t) slot] > 0)
        {
            auto b = juce::Rectangle<float> (14.0f, 14.0f).withCentre ({ r.getRight() - 2.0f, r.getY() + 2.0f });
            g.setColour (rose);
            g.fillEllipse (b);
            g.setColour (bg);
            g.setFont (font (9.0f, true));
            g.drawText (juce::String (rank[(size_t) slot]), b, juce::Justification::centred);
        }
    }
}

void NoviChordsEditor::paintCircle (juce::Graphics& g, juce::Rectangle<float> area)
{
    juce::ignoreUnused (area);
    const auto pi = juce::MathConstants<float>::pi;
    const bool minorNow = engine.minorMode();

    // What sits where: chords of the key (white keys), colour chords, current chord, suggestions
    struct Mark { juce::String roman; bool diatonic = false; bool colour = false; bool current = false; int suggestion = 0; };
    std::array<std::array<Mark, 12>, 2> marks {};

    for (int slot = 0; slot < 12; ++slot)
    {
        const auto& c = engine.chord (slot);
        const auto [ring, idx] = circlePosFor (engine.absoluteRoot (slot), info (c.quality).family);
        auto& m = marks[(size_t) ring][(size_t) idx];
        auto rn = u (engine.roman (slot));
        if (m.roman.isEmpty()) m.roman = rn;
        else if (! m.roman.contains (rn)) m.roman << " " << rn;
        if (c.diatonic) m.diatonic = true; else m.colour = true;
        if (slot == currentSlot) m.current = true;
    }
    for (size_t i = 0; i < suggestions.size(); ++i)
    {
        const int slot = suggestions[i].slot;
        const auto [ring, idx] = circlePosFor (engine.absoluteRoot (slot), info (engine.chord (slot).quality).family);
        auto& m = marks[(size_t) ring][(size_t) idx];
        if (m.suggestion == 0) m.suggestion = (int) i + 1;
    }

    const auto tonicPos = minorNow ? std::pair<int, int> { 1, (((shownSettings.key - 9 + 12) % 12) * 7) % 12 }
                                   : std::pair<int, int> { 0, (shownSettings.key * 7) % 12 };

    for (int ring = 0; ring < 2; ++ring)
    {
        const float r0 = ring == 0 ? rMid : rInner;
        const float r1 = ring == 0 ? rOuter : rMid;
        for (int i = 0; i < 12; ++i)
        {
            const auto& m = marks[(size_t) ring][(size_t) i];
            const bool tonic = tonicPos == std::pair<int, int> { ring, i };
            const bool hover = hoverCircle == std::pair<int, int> { ring, i };

            juce::Path seg;
            seg.addPieSegment (circleCentre.x - r1, circleCentre.y - r1, r1 * 2.0f, r1 * 2.0f,
                               ((float) i - 0.5f) * pi / 6.0f, ((float) i + 0.5f) * pi / 6.0f, r0 / r1);

            juce::Colour fill = ring == 0 ? navy.interpolatedWith (cyan, 0.12f) : navy.interpolatedWith (cyan, 0.06f);
            if (m.diatonic) fill = blue;
            if (m.current)  fill = gold;
            if (hover)      fill = fill.brighter (0.15f);
            g.setColour (fill);
            g.fillPath (seg);

            g.setColour (bg);
            g.strokePath (seg, juce::PathStrokeType (1.5f));

            if (m.colour && ! m.current)
            {
                g.setColour (rose.withAlpha (0.9f));
                juce::Path inset;
                inset.addPieSegment (circleCentre.x - r1 + 3, circleCentre.y - r1 + 3, (r1 - 3) * 2.0f, (r1 - 3) * 2.0f,
                                     ((float) i - 0.42f) * pi / 6.0f, ((float) i + 0.42f) * pi / 6.0f, (r0 + 3) / (r1 - 3));
                g.strokePath (inset, juce::PathStrokeType (1.2f));
            }
            if (tonic)
            {
                g.setColour (gold);
                g.strokePath (seg, juce::PathStrokeType (2.5f));
            }

            // Labels
            const auto textCol = m.current ? bg : (m.diatonic || tonic ? cream : muted);
            const bool hasRoman = m.roman.isNotEmpty();
            auto c = circlePoint (ring, i, 0.0f);
            g.setColour (textCol);
            g.setFont (font (ring == 0 ? 13.0f : 11.5f, m.diatonic || tonic || m.current));
            const float lift = hasRoman ? 5.0f : 0.0f;
            g.drawText (u8 (ring == 0 ? kMajorNames[i] : kMinorNames[i]),
                        juce::Rectangle<float> (60.0f, 16.0f).withCentre ({ c.x, c.y - lift }), juce::Justification::centred);
            if (hasRoman)
            {
                g.setColour (m.current ? bg : (m.colour && ! m.diatonic ? rose : (m.diatonic ? sand : gold)));
                g.setFont (font (9.5f, true));
                g.drawFittedText (m.roman, juce::Rectangle<float> (52.0f, 12.0f).withCentre ({ c.x, c.y + 9.0f }).toNearestInt(),
                                  juce::Justification::centred, 1, 0.6f);
            }
            if (m.suggestion > 0)
            {
                auto bp = circlePoint (ring, i, ring == 0 ? (rOuter - rMid) * 0.5f - 9.0f : (rMid - rInner) * 0.5f - 8.0f);
                const float a = ((float) i + 0.33f) * pi / 6.0f;
                const float rr = std::hypot (bp.x - circleCentre.x, bp.y - circleCentre.y);
                juce::Point<float> p { circleCentre.x + rr * std::sin (a), circleCentre.y - rr * std::cos (a) };
                auto b = juce::Rectangle<float> (14.0f, 14.0f).withCentre (p);
                g.setColour (rose);
                g.fillEllipse (b);
                g.setColour (bg);
                g.setFont (font (9.5f, true));
                g.drawText (juce::String (m.suggestion), b, juce::Justification::centred);
            }
        }
    }

    // key-signature ticks on the outer edge
    g.setFont (font (9.0f));
    g.setColour (muted);
    for (int i = 0; i < 12; ++i)
    {
        if (kSigs[i][0] == 0) continue;
        const float a = (float) i * pi / 6.0f;
        const float rr = rOuter + 11.0f;
        juce::Point<float> p { circleCentre.x + rr * std::sin (a), circleCentre.y - rr * std::cos (a) };
        g.drawText (u8 (kSigs[i]), juce::Rectangle<float> (36.0f, 10.0f).withCentre (p), juce::Justification::centred);
    }

    // centre: key, mode and nearest keys
    g.setColour (panel);
    g.fillEllipse (circleCentre.x - rInner + 2, circleCentre.y - rInner + 2, (rInner - 2) * 2, (rInner - 2) * 2);
    auto inner = juce::Rectangle<float> (rInner * 1.7f, rInner * 1.5f).withCentre (circleCentre);
    g.setColour (cream);
    g.setFont (font (22.0f, true));
    g.drawText (u (engine.noteName (shownSettings.key)) + (minorNow ? "m" : ""), inner.removeFromTop (inner.getHeight() * 0.42f),
                juce::Justification::centredBottom);
    g.setColour (gold);
    g.setFont (font (10.5f, true));
    juce::String mode = u8 (scaleName (shownSettings.scale)).upToFirstOccurrenceOf (" (", false, false);
    g.drawText (mode, inner.removeFromTop (14.0f), juce::Justification::centred);
    g.setColour (muted);
    g.setFont (font (9.5f));
    const int k = shownSettings.key;
    const juce::String rel = minorNow ? u8 ("rel. ") + u (engine.noteName (k + 3))
                                      : u8 ("rel. ") + u (engine.noteName (k + 9)) + "m";
    g.drawText (rel, inner.removeFromTop (13.0f), juce::Justification::centred);
    g.drawText (u8 ("◀ ") + u (engine.noteName (k + 5)) + "  " + u (engine.noteName (k + 7)) + u8 (" ▶"),
                inner.removeFromTop (13.0f), juce::Justification::centred);
}

//==============================================================================
void NoviChordsEditor::paintRecorder (juce::Graphics& g, juce::Rectangle<float> area)
{
    drawPanel (g, area, "CHORD RECORDER");
    const bool rec = proc.isRecording();

    // status line
    auto status = juce::Rectangle<float> (area.getX() + 14.0f, (float) recButton.getBottom() + 4.0f, area.getWidth() - 28.0f, 18.0f);
    double beats = 0.0;
    for (auto& c : take) beats = juce::jmax (beats, c.end < 0.0 ? c.start + 1.0 : c.end);
    const double bars = beats / 4.0;

    if (rec)
    {
        const bool blink = (juce::Time::getMillisecondCounter() / 450) % 2 == 0;
        g.setColour (blink ? recRed : recRed.withAlpha (0.35f));
        g.fillEllipse (status.getX(), status.getCentreY() - 5.0f, 10.0f, 10.0f);
        g.setColour (cream);
        g.setFont (font (12.5f, true));
        g.drawText (take.empty() ? u8 ("Recording — play some chords")
                                 : u8 ("Recording · ") + juce::String ((int) take.size()) + (take.size() == 1 ? " chord" : " chords"),
                    status.withTrimmedLeft (16.0f), juce::Justification::centredLeft);
    }
    else if (! take.empty())
    {
        g.setColour (green);
        g.setFont (font (12.5f, true));
        g.drawText (u8 ("✓ MIDI captured · ") + juce::String ((int) take.size()) + " chords · "
                    + juce::String (bars, bars < 10 ? 1 : 0) + " bars @ " + juce::String (juce::roundToInt (proc.getTakeBpm())) + " BPM",
                    status, juce::Justification::centredLeft);
    }
    else
    {
        g.setColour (muted);
        g.setFont (font (12.0f));
        g.drawText (u8 ("Press ● Rec, play chords, then Stop"), status, juce::Justification::centredLeft);
    }

    // mini piano roll
    g.setColour (bg);
    g.fillRoundedRectangle (rollArea, 6.0f);
    if (! take.empty())
    {
        int lo = 127, hi = 0;
        for (auto& c : take) for (int n : c.notes) { lo = juce::jmin (lo, n); hi = juce::jmax (hi, n); }
        const float span = (float) juce::jmax (12, hi - lo + 1);
        const double total = juce::jmax (4.0, std::ceil (beats / 4.0) * 4.0);
        auto inner = rollArea.reduced (6.0f, 6.0f);

        // bar lines
        g.setColour (panelEdge.withAlpha (0.8f));
        for (double b = 0.0; b <= total + 0.001; b += 4.0)
        {
            const float x = inner.getX() + (float) (b / total) * inner.getWidth();
            g.drawVerticalLine (juce::roundToInt (x), inner.getY(), inner.getBottom());
        }

        const float noteH = juce::jmax (1.5f, inner.getHeight() / span);
        for (size_t i = 0; i < take.size(); ++i)
        {
            const auto& c = take[i];
            const double end = c.end < 0.0 ? c.start + 0.5 : c.end;
            const float x0 = inner.getX() + (float) (c.start / total) * inner.getWidth();
            const float x1 = inner.getX() + (float) (end / total) * inner.getWidth();
            const auto col = (i % 2 == 0 ? gold : rose).withAlpha (c.end < 0.0 ? 0.6f : 0.95f);
            g.setColour (col);
            for (int n : c.notes)
            {
                const float y = inner.getBottom() - ((float) (n - lo) + 1.0f) / span * inner.getHeight();
                g.fillRoundedRectangle (x0, y, juce::jmax (2.0f, x1 - x0 - 1.0f), juce::jmax (1.5f, noteH - 0.5f), 1.0f);
            }
            if (x1 - x0 > 26.0f)
            {
                g.setColour (cream);
                g.setFont (font (9.5f, true));
                g.drawText (c.name, juce::Rectangle<float> (x0 + 2.0f, inner.getY(), x1 - x0 - 2.0f, 11.0f),
                            juce::Justification::centredLeft, true);
            }
        }
    }
    else
    {
        g.setColour (muted.withAlpha (0.6f));
        g.setFont (font (11.0f));
        g.drawText ("No MIDI captured yet", rollArea, juce::Justification::centred);
    }

    // drag strip
    const bool canDrag = ! take.empty() && ! rec;
    g.setColour (canDrag ? green.withAlpha (0.14f) : panelEdge.withAlpha (0.25f));
    g.fillRoundedRectangle (dragStrip, 8.0f);
    juce::Path dash;
    dash.addRoundedRectangle (dragStrip.reduced (1.0f), 8.0f);
    juce::Path dashed;
    const float pattern[] = { 5.0f, 4.0f };
    juce::PathStrokeType (1.3f).createDashedStroke (dashed, dash, pattern, 2);
    g.setColour (canDrag ? green : muted.withAlpha (0.5f));
    g.fillPath (dashed);

    auto t = dragStrip.reduced (12.0f, 6.0f);
    g.setColour (canDrag ? cream : muted);
    g.setFont (font (14.0f, true));
    g.drawText (u8 ("⇣  Drag MIDI into FL Studio"), t.removeFromTop (t.getHeight() * 0.55f), juce::Justification::centredBottom);
    g.setColour (canDrag ? muted.brighter (0.2f) : muted.withAlpha (0.6f));
    g.setFont (font (10.5f));
    g.drawText (canDrag ? "drop on a channel, the piano roll or the playlist" : "record some chords first",
                t, juce::Justification::centredTop);

    if (recorderMessage.isNotEmpty())
    {
        g.setColour (muted);
        g.setFont (font (10.5f));
        g.drawFittedText (recorderMessage, saveButton.getBounds().withX ((int) area.getX() + 14)
                                                .withRight (saveButton.getX() - 8),
                          juce::Justification::centredLeft, 1, 0.8f);
    }
}
