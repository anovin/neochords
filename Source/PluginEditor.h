#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"

class NeoChordsEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit NeoChordsEditor (NeoChordsProcessor&);
    ~NeoChordsEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;

private:
    void timerCallback() override;
    void refreshEngine();
    int hitTestSlot (juce::Point<float>) const;

    void paintKeyboard (juce::Graphics&);
    void paintNowPlaying (juce::Graphics&, juce::Rectangle<float>);
    void paintSuggestions (juce::Graphics&, juce::Rectangle<float>);
    void paintStyle (juce::Graphics&, juce::Rectangle<float>);

    NeoChordsProcessor& proc;
    neo::ChordEngine engine;
    neo::Settings shownSettings;
    std::vector<neo::Suggestion> suggestions;
    int shownCounter = -1;
    int currentSlot = -1;
    int pressedSlot = -1;

    struct Theme : juce::LookAndFeel_V4
    {
        Theme();
        void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool, bool) override;
    } theme;

    juce::ComboBox keyBox, scaleBox, styleBox, complexityBox, voicingBox;
    juce::Slider octaveSlider, strumSlider, humanizeSlider;
    juce::ToggleButton voiceLeadButton { "Voice leading" };
    juce::OwnedArray<juce::Label> labels;

    using ComboAttach  = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using SliderAttach = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttach = juce::AudioProcessorValueTreeState::ButtonAttachment;
    std::vector<std::unique_ptr<ComboAttach>> comboAttachments;
    std::vector<std::unique_ptr<SliderAttach>> sliderAttachments;
    std::unique_ptr<ButtonAttach> voiceLeadAttachment;

    juce::Rectangle<float> keyboardArea, nowArea, nextArea, styleArea;
    std::array<juce::Rectangle<float>, 12> keyRects;
    std::vector<std::pair<juce::Rectangle<float>, int>> suggestionRects;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NeoChordsEditor)
};
