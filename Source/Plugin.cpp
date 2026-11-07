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
// Visual: preto + amarelo, fonte pesada, knobs grossos e um visor que mostra
// a onda sendo "esmagada" pelos parametros de Bits / Taxa / Pressao.
//==============================================================================
namespace Look
{
    static const Colour yellow  { 0xffffe600 };
    static const Colour black   { 0xff0b0b0b };
    static const Colour panel   { 0xff1a1a1a };
    static const Colour dim     { 0xff3a3400 };

    // Impact/Arial Black/Bahnschrift existem no Windows; se faltar, o JUCE usa a fonte padrao.
    static Font titleFont(float h)  { return Font(FontOptions("Impact", h, Font::plain)); }
    static Font labelFont(float h)  { return Font(FontOptions("Bahnschrift", h, Font::bold)); }
    static Font valueFont(float h)  { return Font(FontOptions("Consolas", h, Font::bold)); }
}

class CrusherLookAndFeel : public LookAndFeel_V4
{
public:
    void drawRotarySlider(Graphics& g, int x, int y, int w, int h, float pos,
                          float startAngle, float endAngle, Slider&) override
    {
        auto bounds = Rectangle<float>((float) x, (float) y, (float) w, (float) h).reduced(10.0f);
        const float radius = jmin(bounds.getWidth(), bounds.getHeight()) / 2.0f;
        const auto centre = bounds.getCentre();
        const float angle = startAngle + pos * (endAngle - startAngle);
        const float thick = radius * 0.28f;
        const float arcR  = radius - thick / 2.0f;

        // corpo do knob
        g.setColour(Look::panel);
        g.fillEllipse(centre.x - radius, centre.y - radius, radius * 2.0f, radius * 2.0f);

        // trilho apagado
        Path track;
        track.addCentredArc(centre.x, centre.y, arcR, arcR, 0.0f, startAngle, endAngle, true);
        g.setColour(Look::dim);
        g.strokePath(track, PathStrokeType(thick, PathStrokeType::curved, PathStrokeType::butt));

        // arco do valor
        Path value;
        value.addCentredArc(centre.x, centre.y, arcR, arcR, 0.0f, startAngle, angle, true);
        g.setColour(Look::yellow);
        g.strokePath(value, PathStrokeType(thick, PathStrokeType::curved, PathStrokeType::butt));

        // ponteiro
        Path pointer;
        const float len = radius * 0.55f;
        pointer.addRectangle(-2.5f, -len, 5.0f, len * 0.8f);
        g.setColour(Colours::white);
        g.fillPath(pointer, AffineTransform::rotation(angle).translated(centre.x, centre.y));

        // miolo
        g.setColour(Look::black);
        g.fillEllipse(centre.x - 5.0f, centre.y - 5.0f, 10.0f, 10.0f);
    }

    Label* createSliderTextBox(Slider& s) override
    {
        auto* l = LookAndFeel_V4::createSliderTextBox(s);
        l->setFont(Look::valueFont(16.0f));
        l->setColour(Label::textColourId, Colours::white);
        l->setColour(Label::outlineColourId, Colours::transparentBlack);
        l->setColour(Label::backgroundColourId, Colours::transparentBlack);
        return l;
    }
};

//==============================================================================
class CrusherEditor : public AudioProcessorEditor
{
public:
    explicit CrusherEditor(CrusherProcessor& p) : AudioProcessorEditor(&p), proc(p)
    {
        setLookAndFeel(&lnf);
        setSize(580, 360);

        const char* ids[]    = { "drive", "bits", "rate", "mix", "out" };
        const char* labels[] = { "PRESSAO", "BITS", "TAXA", "MIX", "SAIDA" };

        for (int i = 0; i < 5; ++i)
        {
            auto& k = knobs[i];
            k.setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
            k.setRotaryParameters(MathConstants<float>::pi * 1.2f,
                                  MathConstants<float>::pi * 2.8f, true);
            k.setTextBoxStyle(Slider::TextBoxBelow, false, 80, 22);
            k.onValueChange = [this] { repaint(scopeArea); };
            addAndMakeVisible(k);

            auto& l = names[i];
            l.setText(labels[i], dontSendNotification);
            l.setJustificationType(Justification::centred);
            l.setColour(Label::textColourId, Look::yellow);
            l.setFont(Look::labelFont(16.0f));
            addAndMakeVisible(l);

            attachments.add(new AudioProcessorValueTreeState::SliderAttachment(proc.apvts, ids[i], k));
        }

        loadBackground();
    }

    ~CrusherEditor() override { setLookAndFeel(nullptr); }

    void paint(Graphics& g) override
    {
        g.fillAll(Look::black);

        // imagem de fundo opcional (ver loadBackground), colada na direita e no fundo
        if (background.isValid())
        {
            const float scale = (float) getHeight() / (float) background.getHeight();
            const float w = (float) background.getWidth() * scale;
            g.setOpacity(0.55f);
            g.drawImage(background, Rectangle<float>((float) getWidth() - w, 0.0f, w, (float) getHeight()),
                        RectanglePlacement::stretchToFit);
            g.setOpacity(1.0f);
        }

        // listras de aviso no topo e no fundo
        drawStripes(g, 0, 10);
        drawStripes(g, getHeight() - 10, 10);

        // slogan
        g.setColour(Look::yellow);
        g.setFont(Look::titleFont(18.0f));
        g.drawText(String(CharPointer_UTF8("ESSA BEAT \xC3\x89 GLORIOSA")), 0, 12, getWidth(), 22,
                   Justification::centred);

        // titulo
        g.setColour(Colours::white);
        g.setFont(Look::titleFont(44.0f));
        g.drawText("RENAN SANTOS CRUSHER", 0, 32, getWidth(), 50, Justification::centred);

        g.setColour(Look::yellow);
        g.setFont(Look::labelFont(13.0f));
        g.drawText("DISTORCAO  +  BITCRUSHER", 0, 80, getWidth(), 18, Justification::centred);

        drawScope(g);
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced(20, 0);
        area.removeFromTop(106);
        area.removeFromBottom(20);

        scopeArea = area.removeFromBottom(60).reduced(10, 0);

        const int w = area.getWidth() / 5;
        for (int i = 0; i < 5; ++i)
        {
            auto col = area.removeFromLeft(w);
            names[i].setBounds(col.removeFromTop(24));
            knobs[i].setBounds(col);
        }
    }

private:
    // Procura a imagem em:  %APPDATA%\RenanSantosCrusher\fundo.png  (ou fundo.jpg)
    // Se nao existir, o plugin fica sem imagem. Nada e embutido no plugin.
    void loadBackground()
    {
        const auto folder = File::getSpecialLocation(File::userApplicationDataDirectory)
                                .getChildFile("RenanSantosCrusher");

        for (auto name : { "fundo.png", "fundo.jpg", "fundo.jpeg" })
        {
            const auto f = folder.getChildFile(name);
            if (f.existsAsFile())
            {
                background = ImageFileFormat::loadFrom(f);
                if (background.isValid())
                    return;
            }
        }
    }

    static void drawStripes(Graphics& g, int y, int h)
    {
        g.saveState();
        g.reduceClipRegion(0, y, 4000, h);
        g.setColour(Look::yellow);
        g.fillRect(0, y, 4000, h);
        g.setColour(Look::black);
        for (int x = -h; x < 4000; x += h * 2)
        {
            Path p;
            p.startNewSubPath((float) x, (float) (y + h));
            p.lineTo((float) (x + h), (float) y);
            p.lineTo((float) (x + h * 2), (float) y);
            p.lineTo((float) (x + h), (float) (y + h));
            p.closeSubPath();
            g.fillPath(p);
        }
        g.restoreState();
    }

    // Mostra uma senoide passando pela distorcao + quantizacao + hold de amostras
    void drawScope(Graphics& g)
    {
        auto r = scopeArea.toFloat();
        g.setColour(Look::panel);
        g.fillRoundedRectangle(r, 4.0f);

        const float drive  = knobs[0].getValue();
        const float bits   = (float) knobs[1].getValue();
        const int   rate   = jmax(1, (int) knobs[2].getValue());
        const float levels = std::pow(2.0f, bits) * 0.5f;

        const int n = 160;
        Path wave;
        float held = 0.0f;
        for (int i = 0; i < n; ++i)
        {
            if (i % rate == 0)
            {
                const float in = std::sin(MathConstants<float>::twoPi * 2.0f * (float) i / (float) n);
                float x = std::tanh(in * drive);
                held = std::round(x * levels) / levels;
            }
            const float px = r.getX() + r.getWidth() * (float) i / (float) (n - 1);
            const float py = r.getCentreY() - held * (r.getHeight() * 0.42f);
            if (i == 0) wave.startNewSubPath(px, py);
            else        wave.lineTo(px, py);
        }

        g.setColour(Look::dim);
        g.drawHorizontalLine((int) r.getCentreY(), r.getX(), r.getRight());
        g.setColour(Look::yellow);
        g.strokePath(wave, PathStrokeType(2.0f));
    }

    CrusherLookAndFeel lnf;
    CrusherProcessor& proc;
    Rectangle<int> scopeArea;
    Image background;
    Slider knobs[5];
    Label names[5];
    OwnedArray<AudioProcessorValueTreeState::SliderAttachment> attachments;
};

AudioProcessorEditor* CrusherProcessor::createEditor() { return new CrusherEditor(*this); }

AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new CrusherProcessor(); }
