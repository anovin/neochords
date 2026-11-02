#include "PluginEditor.h"

using namespace neo;

namespace
{
    // Warm "late-night studio" palette: espresso background, cream text, brass accent.
    const juce::Colour bg        { 0xff15100e };
    const juce::Colour panel     { 0xff211915 };
    const juce::Colour panelEdge { 0xff3a2c24 };
    const juce::Colour cream     { 0xfff3e6d3 };
    const juce::Colour muted     { 0xff9f8d7b };
    const juce::Colour gold      { 0xffe3b04b };
    const juce::Colour rose      { 0xffd0785e };
    const juce::Colour ivory     { 0xffeee1cb };
    const juce::Colour ebony     { 0xff2c211c };

    constexpr int W = 980, H = 650;

    juce::String u (const std::string& s) { return juce::String::fromUTF8 (s.c_str()); }

    juce::Font font (float size, bool bold = false)
    {
        return juce::Font (juce::FontOptions (size, bold ? juce::Font::bold : juce::Font::plain));
    }

    juce::String noteWithOctave (int n)
    {
        static const char* names[] = { "C", "C♯", "D", "E♭", "E", "F", "F♯", "G", "A♭", "A", "B♭", "B" };
        return juce::String::fromUTF8 (names[n % 12]) + juce::String (n / 12 - 1);
    }
}

//==============================================================================
NeoChordsEditor::Theme::Theme()
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
}

void NeoChordsEditor::Theme::drawToggleButton (juce::Graphics& g, juce::ToggleButton& b, bool hover, bool)
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

//==============================================================================
NeoChordsEditor::NeoChordsEditor (NeoChordsProcessor& p)
    : AudioProcessorEditor (&p), proc (p)
{
    setLookAndFeel (&theme);

    auto setupCombo = [this] (juce::ComboBox& box, const char* paramId, const juce::String& label) {
        auto* param = dynamic_cast<juce::AudioParameterChoice*> (proc.apvts.getParameter (paramId));
        box.addItemList (param->choices, 1);
        addAndMakeVisible (box);
        comboAttachments.push_back (std::make_unique<ComboAttach> (proc.apvts, paramId, box));
        auto* l = labels.add (new juce::Label ({}, label.toUpperCase()));
        l->setFont (font (11.0f, true));
        l->attachToComponent (&box, false);
        addAndMakeVisible (l);
    };

    setupCombo (keyBox,        ParamID::key,        "Key");
    setupCombo (scaleBox,      ParamID::scale,      "Scale");
    setupCombo (styleBox,      ParamID::style,      "Style");
    setupCombo (complexityBox, ParamID::complexity, "Complexity");
    setupCombo (voicingBox,    ParamID::voicing,    "Voicing");

    auto setupSlider = [this] (juce::Slider& s, const char* paramId, const juce::String& label, const juce::String& suffix) {
        s.setSliderStyle (juce::Slider::LinearBar);
        s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 0, 0);
        s.setTextValueSuffix (suffix);
        s.setColour (juce::Slider::textBoxTextColourId, cream);
        addAndMakeVisible (s);
        sliderAttachments.push_back (std::make_unique<SliderAttach> (proc.apvts, paramId, s));
        auto* l = labels.add (new juce::Label ({}, label.toUpperCase()));
        l->setFont (font (11.0f, true));
        l->attachToComponent (&s, false);
        addAndMakeVisible (l);
    };

    setupSlider (octaveSlider,   ParamID::octave,   "Octave", "");
    setupSlider (strumSlider,    ParamID::strum,    "Strum",  " ms");
    setupSlider (humanizeSlider, ParamID::humanize, "Humanize", " %");

    addAndMakeVisible (voiceLeadButton);
    voiceLeadAttachment = std::make_unique<ButtonAttach> (proc.apvts, ParamID::voiceLead, voiceLeadButton);

    setSize (W, H);
    refreshEngine();
    startTimerHz (30);
}

NeoChordsEditor::~NeoChordsEditor()
{
    stopTimer();
    if (pressedSlot >= 0)
        proc.auditionSlot (pressedSlot, false);
    setLookAndFeel (nullptr);
}

void NeoChordsEditor::refreshEngine()
{
    shownSettings = proc.currentSettings();
    engine.update (shownSettings);
    currentSlot = proc.lastSlot.load();
    suggestions = engine.suggest (currentSlot >= 0 ? currentSlot : 0, 5);
}

void NeoChordsEditor::timerCallback()
{
    const int counter = proc.playCounter.load();
    if (proc.currentSettings() != shownSettings || counter != shownCounter)
    {
        shownCounter = counter;
        refreshEngine();
        repaint();
    }
}

//==============================================================================
void NeoChordsEditor::resized()
{
    const int top = 88, h = 30;
    int x = 24;
    auto place = [&] (juce::Component& c, int w) { c.setBounds (x, top, w, h); x += w + 12; };
    place (keyBox, 78);
    place (scaleBox, 128);
    place (styleBox, 160);
    place (complexityBox, 118);
    place (voicingBox, 146);
    place (octaveSlider, 60);
    place (strumSlider, 70);
    place (humanizeSlider, 70);

    voiceLeadButton.setBounds (W - 24 - 140, 20, 140, 26);

    keyboardArea = { 24.0f, 136.0f, (float) W - 48.0f, 220.0f };

    const float py = 372.0f, ph = (float) H - py - 20.0f;
    nowArea   = { 24.0f, py, 250.0f, ph };
    nextArea  = { 286.0f, py, 330.0f, ph };
    styleArea = { 628.0f, py, (float) W - 628.0f - 24.0f, ph };

    // Key rectangles: white first (C D E F G A B), then black on top.
    const float kw = keyboardArea.getWidth() / 7.0f;
    const int whites[7] = { 0, 2, 4, 5, 7, 9, 11 };
    for (int i = 0; i < 7; ++i)
        keyRects[(size_t) whites[i]] = { keyboardArea.getX() + kw * i, keyboardArea.getY(), kw, keyboardArea.getHeight() };

    const int blacks[5] = { 1, 3, 6, 8, 10 };
    const int afterWhite[5] = { 0, 1, 3, 4, 5 };
    const float bw = kw * 0.66f, bh = keyboardArea.getHeight() * 0.56f;
    for (int i = 0; i < 5; ++i)
    {
        const float cx = keyboardArea.getX() + kw * (afterWhite[i] + 1);
        keyRects[(size_t) blacks[i]] = { cx - bw * 0.5f, keyboardArea.getY(), bw, bh };
    }
}

int NeoChordsEditor::hitTestSlot (juce::Point<float> p) const
{
    for (int s : { 1, 3, 6, 8, 10 })
        if (keyRects[(size_t) s].contains (p)) return s;
    for (int s : { 0, 2, 4, 5, 7, 9, 11 })
        if (keyRects[(size_t) s].contains (p)) return s;
    for (auto& [r, slot] : suggestionRects)
        if (r.contains (p)) return slot;
    return -1;
}

void NeoChordsEditor::mouseDown (const juce::MouseEvent& e)
{
    const int slot = hitTestSlot (e.position);
    if (slot >= 0)
    {
        pressedSlot = slot;
        proc.auditionSlot (slot, true);
    }
}

void NeoChordsEditor::mouseUp (const juce::MouseEvent&)
{
    if (pressedSlot >= 0)
    {
        proc.auditionSlot (pressedSlot, false);
        pressedSlot = -1;
    }
}

//==============================================================================
void NeoChordsEditor::paint (juce::Graphics& g)
{
    g.fillAll (bg);

    // Header
    g.setColour (cream);
    g.setFont (font (26.0f, true));
    g.drawText ("NeoChords", 24, 14, 200, 32, juce::Justification::centredLeft);
    g.setColour (muted);
    g.setFont (font (13.0f));
    g.drawText (juce::String::fromUTF8 ("gospel · soul · neo-soul chord engine"), 170, 20, 400, 24, juce::Justification::centredLeft);
    g.setColour (panelEdge);
    g.fillRect (24.0f, 58.0f, (float) W - 48.0f, 1.0f);

    paintKeyboard (g);
    paintNowPlaying (g, nowArea);
    paintSuggestions (g, nextArea);
    paintStyle (g, styleArea);
}

void NeoChordsEditor::paintKeyboard (juce::Graphics& g)
{
    // suggestion rank per slot (1..5)
    std::array<int, 12> rank {};
    for (size_t i = 0; i < suggestions.size(); ++i)
        rank[(size_t) suggestions[i].slot] = (int) i + 1;

    auto drawBadge = [&] (juce::Rectangle<float> keyR, int r, bool onDark) {
        if (r == 0) return;
        auto b = juce::Rectangle<float> (keyR.getRight() - 24.0f, keyR.getY() + 6.0f, 18.0f, 18.0f);
        g.setColour (rose);
        g.fillEllipse (b);
        g.setColour (onDark ? bg : bg);
        g.setFont (font (11.0f, true));
        g.drawText (juce::String (r), b, juce::Justification::centred);
    };

    auto drawLabels = [&] (int slot, juce::Rectangle<float> area, bool active, bool dark) {
        const auto textCol = active ? bg : (dark ? cream : ebony);
        g.setColour (active ? bg : (dark ? gold : juce::Colour (0xff9a6a1c)));
        g.setFont (font (dark ? 12.0f : 14.0f, true));
        g.drawText (u (engine.roman (slot)), area.removeFromTop (dark ? 16.0f : 20.0f), juce::Justification::centred);
        g.setColour (textCol);
        g.setFont (font (dark ? 13.0f : 16.0f, true));
        g.drawFittedText (u (engine.chordName (slot)), area.removeFromTop (dark ? 34.0f : 24.0f).toNearestInt(),
                          juce::Justification::centred, 2, 0.8f);
        g.setColour (active ? bg.withAlpha (0.7f) : (dark ? muted : juce::Colour (0xff8a7764)));
        g.setFont (font (11.0f));
        g.drawText ("key " + u (ChordEngine::keyLabel (slot)), area.removeFromTop (16.0f), juce::Justification::centred);
    };

    // White keys
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
        auto labelArea = r.withTop (r.getBottom() - 74.0f).reduced (4.0f, 0.0f);
        drawLabels (s, labelArea, active, false);
        drawBadge (r.withTop (r.getY() + keyboardArea.getHeight() * 0.56f), rank[(size_t) s], false);
    }

    // Black keys
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
        auto labelArea = r.reduced (4.0f, 0.0f).withTop (r.getBottom() - 72.0f);
        drawLabels (s, labelArea, active, true);
        drawBadge (r, rank[(size_t) s], true);
    }
}

static void drawPanel (juce::Graphics& g, juce::Rectangle<float> r, const juce::String& title)
{
    g.setColour (panel);
    g.fillRoundedRectangle (r, 10.0f);
    g.setColour (panelEdge);
    g.drawRoundedRectangle (r, 10.0f, 1.0f);
    g.setColour (muted);
    g.setFont (font (11.0f, true));
    g.drawText (title, r.reduced (16.0f, 12.0f).removeFromTop (14.0f), juce::Justification::centredLeft);
}

void NeoChordsEditor::paintNowPlaying (juce::Graphics& g, juce::Rectangle<float> r)
{
    drawPanel (g, r, "NOW PLAYING");
    auto a = r.reduced (16.0f, 12.0f).withTrimmedTop (22.0f);

    if (currentSlot < 0)
    {
        g.setColour (cream);
        g.setFont (font (15.0f));
        g.drawFittedText (juce::String::fromUTF8 ("Play a key on your controller — or click the map above.\n\nWhite keys = the 7 chords of your key.\nBlack keys = style colour chords."),
                          a.toNearestInt(), juce::Justification::topLeft, 8);
        return;
    }

    g.setColour (cream);
    g.setFont (font (34.0f, true));
    g.drawFittedText (u (engine.chordName (currentSlot)), a.removeFromTop (44.0f).toNearestInt(), juce::Justification::centredLeft, 1, 0.6f);
    g.setColour (gold);
    g.setFont (font (18.0f, true));
    g.drawText (u (engine.roman (currentSlot)) + "   " + juce::String::fromUTF8 ("· key ") + u (ChordEngine::keyLabel (currentSlot)),
                a.removeFromTop (26.0f), juce::Justification::centredLeft);

    a.removeFromTop (12.0f);
    g.setColour (muted);
    g.setFont (font (11.0f, true));
    g.drawText ("NOTES SENT", a.removeFromTop (16.0f), juce::Justification::centredLeft);

    juce::StringArray notes;
    const int n = proc.lastNoteCount.load();
    for (int i = 0; i < n; ++i)
        notes.add (noteWithOctave (proc.lastNotes[(size_t) i].load()));
    g.setColour (cream);
    g.setFont (font (14.0f));
    g.drawFittedText (notes.joinIntoString ("  "), a.removeFromTop (44.0f).toNearestInt(), juce::Justification::topLeft, 3);

    g.setColour (muted);
    g.setFont (font (12.0f));
    g.drawFittedText (juce::String (scaleName (shownSettings.scale)) + " - " + complexityName (shownSettings.complexity)
                      + "\n" + juce::String::fromUTF8 (voicingName (shownSettings.voicing)),
                      a.toNearestInt(), juce::Justification::bottomLeft, 2);
}

void NeoChordsEditor::paintSuggestions (juce::Graphics& g, juce::Rectangle<float> r)
{
    const juce::String from = currentSlot >= 0 ? u (engine.chordName (currentSlot)) : u (engine.chordName (0));
    drawPanel (g, r, "TRY NEXT  " + juce::String::fromUTF8 ("→") + "  after " + from);
    auto a = r.reduced (12.0f, 12.0f).withTrimmedTop (24.0f);

    suggestionRects.clear();
    const float rowH = juce::jmin (44.0f, a.getHeight() / 5.0f);

    for (size_t i = 0; i < suggestions.size(); ++i)
    {
        const auto& sg = suggestions[i];
        auto row = a.removeFromTop (rowH);
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

        auto keyTag = row.removeFromRight (64.0f);
        g.setColour (gold);
        g.setFont (font (12.0f, true));
        g.drawText ("key " + u (ChordEngine::keyLabel (sg.slot)), keyTag, juce::Justification::centredRight);

        auto top = row.removeFromTop (row.getHeight() * 0.52f);
        g.setColour (cream);
        g.setFont (font (15.0f, true));
        g.drawText (u (engine.chordName (sg.slot)) + "   ", top.withTrimmedLeft (6.0f), juce::Justification::bottomLeft);
        g.setColour (gold.withAlpha (0.85f));
        g.setFont (font (12.0f, true));
        g.drawText (u (engine.roman (sg.slot)), top.withTrimmedLeft (150.0f), juce::Justification::bottomLeft);

        g.setColour (muted);
        g.setFont (font (11.5f));
        g.drawText (u (sg.reason), row.withTrimmedLeft (6.0f), juce::Justification::topLeft, true);
    }
}

void NeoChordsEditor::paintStyle (juce::Graphics& g, juce::Rectangle<float> r)
{
    const auto& si = styleInfo (shownSettings.style);
    drawPanel (g, r, "SIGNATURE MOVES  " + juce::String::fromUTF8 ("·") + "  " + u (si.name).toUpperCase());
    auto a = r.reduced (16.0f, 12.0f).withTrimmedTop (22.0f);

    g.setColour (cream.withAlpha (0.85f));
    g.setFont (font (12.0f));
    g.drawFittedText (u (si.blurb), a.removeFromTop (44.0f).toNearestInt(), juce::Justification::topLeft, 3, 0.9f);
    g.setColour (gold);
    g.setFont (font (11.0f));
    g.drawFittedText (u (si.tip), a.removeFromTop (28.0f).toNearestInt(), juce::Justification::topLeft, 2, 0.9f);
    a.removeFromTop (4.0f);

    const float rowH = a.getHeight() / (float) juce::jmax<size_t> (1, si.progressions.size());
    for (auto& p : si.progressions)
    {
        auto row = a.removeFromTop (rowH);
        juce::StringArray chordsTxt, keysTxt;
        for (int k : p.keys)
        {
            chordsTxt.add (u (engine.chordName (k)));
            keysTxt.add (u (ChordEngine::keyLabel (k)));
        }
        auto head = row.removeFromTop (16.0f);
        g.setColour (muted);
        g.setFont (font (11.0f));
        g.drawText ("keys " + keysTxt.joinIntoString (juce::String::fromUTF8 ("–")), head, juce::Justification::centredRight);
        g.setColour (cream);
        g.setFont (font (12.5f, true));
        g.drawText (u (p.name), head, juce::Justification::centredLeft);
        g.setColour (cream.withAlpha (0.75f));
        g.setFont (font (12.0f));
        g.drawFittedText (chordsTxt.joinIntoString (juce::String::fromUTF8 (" → ")), row.removeFromTop (16.0f).toNearestInt(),
                          juce::Justification::centredLeft, 1, 0.7f);
    }
}
