#pragma once

#include "Gui.h"

// Toute la face avant est dessinée dans un espace fixe de 1500 x 800 (ProphorcelContent),
// que l'éditeur met à l'échelle quand on redimensionne la fenêtre.
class ProphorcelContent : public juce::Component, private juce::Timer
{
public:
    explicit ProphorcelContent (ProphorcelProcessor&);
    ~ProphorcelContent() override;

    void paint (juce::Graphics&) override;

private:
    void timerCallback() override;
    void updateOctLabel();
    juce::String hintText() const;

    template <typename T, typename... Args>
    T& make (Args&&... args)
    {
        auto p = std::make_unique<T> (std::forward<Args> (args)...);
        auto& ref = *p;
        addAndMakeVisible (ref);
        owned.push_back (std::move (p));
        return ref;
    }

    struct Module
    {
        juce::Rectangle<int> r;
        juce::String title;
        juce::Colour colour;
    };

    gui::PanelLookAndFeel laf; // déclaré en premier : détruit en dernier

    ProphorcelProcessor& proc;
    std::vector<Module> modules;
    std::vector<juce::Rectangle<int>> oscCols;
    std::vector<std::unique_ptr<juce::Component>> owned;

    juce::ComboBox presetBox, waveBox[4];
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> waveAtt[4];
    gui::VectorPad* pad = nullptr;
    gui::Wheel* pitchWheel = nullptr;
    gui::Wheel* modWheel = nullptr;
    gui::PanelKeyboard keyboard;
    juce::TextButton octDown { "Oct -" }, octUp { "Oct +" };
    juce::String octText, lastHint;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ProphorcelContent)
};

class ProphorcelEditor : public juce::AudioProcessorEditor
{
public:
    explicit ProphorcelEditor (ProphorcelProcessor&);
    void resized() override;

private:
    ProphorcelContent content;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ProphorcelEditor)
};
