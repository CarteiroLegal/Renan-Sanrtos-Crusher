#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <cmath>

using namespace juce;

//==============================================================================
class CrusherProcessor : public AudioProcessor
{
public:
    CrusherProcessor()
        : AudioProcessor(BusesProperties()
                             .withInput("Input", AudioChannelSet::stereo(), true)
                             .withOutput("Output", AudioChannelSet::stereo(), true)),
          apvts(*this, nullptr, "STATE", createLayout())
    {
    }

    static AudioProcessorValueTreeState::ParameterLayout createLayout()
    {
        AudioProcessorValueTreeState::ParameterLayout l;
        l.add(std::make_unique<AudioParameterFloat>("drive", "Pressao (Drive)",
                                                    NormalisableRange<float>(1.0f, 40.0f, 0.01f, 0.4f), 8.0f));
        l.add(std::make_unique<AudioParameterFloat>("bits", "Bits",
                                                    NormalisableRange<float>(2.0f, 16.0f, 0.1f), 10.0f));
        l.add(std::make_unique<AudioParameterFloat>("rate", "Reducao de Taxa",
                                                    NormalisableRange<float>(1.0f, 40.0f, 1.0f), 4.0f));
        l.add(std::make_unique<AudioParameterFloat>("mix", "Mix",
                                                    NormalisableRange<float>(0.0f, 1.0f, 0.001f), 1.0f));
        l.add(std::make_unique<AudioParameterFloat>("out", "Saida (dB)",
                                                    NormalisableRange<float>(-24.0f, 6.0f, 0.1f), -6.0f));
        return l;
    }

    const String getName() const override { return "Renan Santos Crusher"; }
    void prepareToPlay(double, int) override { held[0] = held[1] = 0.0f; counter = 0; }
    void releaseResources() override {}
    bool isBusesLayoutSupported(const BusesLayout& l) const override
    {
        return l.getMainOutputChannelSet() == l.getMainInputChannelSet()
            && (l.getMainOutputChannelSet() == AudioChannelSet::stereo()
                || l.getMainOutputChannelSet() == AudioChannelSet::mono());
    }

    void processBlock(AudioBuffer<float>& buffer, MidiBuffer&) override
    {
        ScopedNoDenormals noDenormals;

        const float drive = *apvts.getRawParameterValue("drive");
        const float bits  = *apvts.getRawParameterValue("bits");
        const int   rate  = (int) *apvts.getRawParameterValue("rate");
        const float mix   = *apvts.getRawParameterValue("mix");
        const float outG  = Decibels::decibelsToGain(apvts.getRawParameterValue("out")->load());
        const float levels = std::pow(2.0f, bits) * 0.5f;

        for (int s = 0; s < buffer.getNumSamples(); ++s)
        {
            const bool refresh = (counter % rate) == 0;
            ++counter;

            for (int ch = 0; ch < buffer.getNumChannels() && ch < 2; ++ch)
            {
                float* data = buffer.getWritePointer(ch);
                const float dry = data[s];

                if (refresh)
                {
                    float x = std::tanh(dry * drive);          // distorcao
                    x = std::round(x * levels) / levels;        // bitcrush
                    held[ch] = x;
                }

                data[s] = (dry * (1.0f - mix) + held[ch] * mix) * outG;
            }
        }
    }

    AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    double getTailLengthSeconds() const override { return 0.0; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const String getProgramName(int) override { return {}; }
    void changeProgramName(int, const String&) override {}

    void getStateInformation(MemoryBlock& dest) override
    {
        if (auto xml = apvts.copyState().createXml())
            copyXmlToBinary(*xml, dest);
    }
    void setStateInformation(const void* data, int size) override
    {
        if (auto xml = getXmlFromBinary(data, size))
            apvts.replaceState(ValueTree::fromXml(*xml));
    }

    AudioProcessorValueTreeState apvts;

private:
    float held[2] { 0.0f, 0.0f };
    int counter = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CrusherProcessor)
};

//==============================================================================
class CrusherEditor : public AudioProcessorEditor
{
public:
    explicit CrusherEditor(CrusherProcessor& p) : AudioProcessorEditor(&p), proc(p)
    {
        setSize(560, 320);

        const char* ids[]    = { "drive", "bits", "rate", "mix", "out" };
        const char* labels[] = { "PRESSAO", "BITS", "TAXA", "MIX", "SAIDA" };

        for (int i = 0; i < 5; ++i)
        {
            auto& k = knobs[i];
            k.setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
            k.setTextBoxStyle(Slider::TextBoxBelow, false, 70, 18);
            k.setColour(Slider::rotarySliderFillColourId, orange);
            k.setColour(Slider::thumbColourId, Colours::white);
            k.setColour(Slider::textBoxTextColourId, Colours::white);
            k.setColour(Slider::textBoxOutlineColourId, Colours::transparentBlack);
            addAndMakeVisible(k);

            auto& l = names[i];
            l.setText(labels[i], dontSendNotification);
            l.setJustificationType(Justification::centred);
            l.setColour(Label::textColourId, orange);
            l.setFont(Font(FontOptions(14.0f, Font::bold)));
            addAndMakeVisible(l);

            attachments.add(new AudioProcessorValueTreeState::SliderAttachment(proc.apvts, ids[i], k));
        }
    }

    void paint(Graphics& g) override
    {
        g.fillAll(Colour(0xff111111));

        g.setColour(orange);
        g.fillRect(0, 0, getWidth(), 6);
        g.fillRect(0, getHeight() - 6, getWidth(), 6);

        g.setColour(Colours::white);
        g.setFont(Font(FontOptions(30.0f, Font::bold)));
        g.drawText("RENAN SANTOS CRUSHER", 0, 20, getWidth(), 40, Justification::centred);

        g.setColour(orange);
        g.setFont(Font(FontOptions(13.0f, Font::italic)));
        g.drawText("distorcao + bitcrusher  |  plugin de brincadeira", 0, 58, getWidth(), 20,
                   Justification::centred);
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced(20).withTrimmedTop(80);
        const int w = area.getWidth() / 5;
        for (int i = 0; i < 5; ++i)
        {
            auto col = area.removeFromLeft(w);
            names[i].setBounds(col.removeFromTop(22));
            knobs[i].setBounds(col);
        }
    }

private:
    const Colour orange { 0xffff6a00 };
    CrusherProcessor& proc;
    Slider knobs[5];
    Label names[5];
    OwnedArray<AudioProcessorValueTreeState::SliderAttachment> attachments;
};

AudioProcessorEditor* CrusherProcessor::createEditor() { return new CrusherEditor(*this); }

AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new CrusherProcessor(); }
