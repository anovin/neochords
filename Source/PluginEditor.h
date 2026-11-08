#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"

class NoviChordsEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit NoviChordsEditor (NoviChordsProcessor&);
    ~NoviChordsEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;

private:
    void timerCallback() override;
    void refreshEngine();
    int hitTestSlot (juce::Point<float>) const;

    // circle of fifths: returns ring (0 = major/outer, 1 = minor/inner, -1 = none) and segment index
    std::pair<int, int> hitTestCircle (juce::Point<float>) const;
    juce::Point<float> circlePoint (int ring, int index, float radialOffset = 0.0f) const;
    std::pair<int, int> circlePosFor (int pitchClass, neo::Family) const;

    void paintKeyboard (juce::Graphics&);
    void paintNowPlaying (juce::Graphics&, juce::Rectangle<float>);
    void paintSuggestions (juce::Graphics&, juce::Rectangle<float>);
    void paintStyle (juce::Graphics&, juce::Rectangle<float>);
    void paintMapPanel (juce::Graphics&, juce::Rectangle<float>);
    void paintCircle (juce::Graphics&, juce::Rectangle<float>);
    void paintGrid (juce::Graphics&, juce::Rectangle<float>);
    void paintFlow (juce::Graphics&, juce::Rectangle<float>);
    int hitTestMap (juce::Point<float>) const;   // grid / flow: slot under the mouse, or -1
    void drawMiniKeys (juce::Graphics&, juce::Rectangle<float>, const std::vector<int>& notes);
    struct SigHit { juce::Rectangle<float> r; int type = 0; int index = 0; };  // 0 play, 1 chord chip, 2 length, 3 loop
    void paintRecorder (juce::Graphics&, juce::Rectangle<float>);

    void updateRecorderButtons();
    void saveMidiToMusicFolder();

    NoviChordsProcessor& proc;
    neo::ChordEngine engine;
    neo::Settings shownSettings;
    std::vector<neo::Suggestion> suggestions;
    std::vector<CapturedChord> take;
    int shownCounter = -1;
    int shownTakeVersion = -1;
    int currentSlot = -1;
    int pressedSlot = -1;
    std::pair<int, int> hoverCircle { -1, -1 };
    bool dragStarted = false;
    bool dragFromStrip = false;
    juce::String recorderMessage;

    struct Theme : juce::LookAndFeel_V4
    {
        Theme();
        void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool, bool) override;
        void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool, bool) override;
    } theme;

    juce::ComboBox keyBox, scaleBox, styleBox, complexityBox, voicingBox, quantizeBox;
    juce::Slider octaveSlider, strumSlider, humanizeSlider;
    juce::ToggleButton voiceLeadButton { "Smooth voice leading" };
    juce::TextButton recButton { "Rec" }, stopButton { "Stop" }, clearButton { "Clear" }, saveButton { "Save .mid" };
    juce::OwnedArray<juce::Label> labels;
    std::unique_ptr<juce::FileChooser> chooser;

    using ComboAttach  = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using SliderAttach = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttach = juce::AudioProcessorValueTreeState::ButtonAttachment;
    std::vector<std::unique_ptr<ComboAttach>> comboAttachments;
    std::vector<std::unique_ptr<SliderAttach>> sliderAttachments;
    std::unique_ptr<ButtonAttach> voiceLeadAttachment;

    juce::Rectangle<float> keyboardArea, nowArea, nextArea, styleArea, circleArea, recArea;
    juce::Rectangle<float> rollArea, dragStrip;
    juce::Point<float> circleCentre;
    float rOuter = 0, rMid = 0, rInner = 0;
    std::array<juce::Rectangle<float>, 12> keyRects;
    std::vector<std::pair<juce::Rectangle<float>, int>> suggestionRects;
    std::array<juce::Rectangle<float>, 3> viewTabs;
    juce::Rectangle<float> mapContent;
    std::vector<std::pair<juce::Path, int>> mapHits;
    int hoverMapSlot = -1;
    std::vector<SigHit> sigHits;
    int shownSeqRow = -2, shownSeqStep = -2;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NoviChordsEditor)
};
