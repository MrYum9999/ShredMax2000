#include "PluginEditor.h"

namespace
{
    constexpr int W = 1120, H = 620;

    // Main regions
    const juce::Rectangle<float> bodyR   { 14.0f, 34.0f, (float) W - 28.0f, (float) H - 46.0f };
    const juce::Rectangle<float> grilleR { 50.0f, 66.0f, (float) W - 100.0f, 234.0f };
    const juce::Rectangle<float> panelR  { 50.0f, 316.0f, (float) W - 100.0f, 264.0f };

    constexpr float knobY = 432.0f, ratioY = 440.0f;
    constexpr float knobSize = 70.0f, ratioSize = 96.0f;
    const juce::Rectangle<float> eardrumCol { 60.0f, 326.0f, 116.0f, 244.0f };
    const juce::Rectangle<float> audioCol   { 944.0f, 326.0f, 116.0f, 244.0f };

    const juce::Colour gold      { 0xffc29a45 };
    const juce::Colour ink       { 0xff1d1813 };
    const juce::Colour cream     { 0xffeadfbf };
    const juce::Colour ledRed    { 0xffff3b26 };
    const juce::Colour ledAmber  { 0xffffa21c };

    float ratioAngle (int i)
    {
        return juce::MathConstants<float>::pi * (1.25f + 1.5f * (float) i / 7.0f);
    }

    juce::Colour vary (juce::Colour c, juce::Random& r, float amt = 0.08f)
    {
        return c.withMultipliedBrightness (1.0f + (r.nextFloat() - 0.5f) * 2.0f * amt);
    }

    void screw (juce::Graphics& g, juce::Point<float> c, float rad, juce::Random& r)
    {
        g.setColour (juce::Colours::black.withAlpha (0.5f));
        g.fillEllipse (juce::Rectangle<float> (rad * 2.2f, rad * 2.2f).withCentre (c.translated (1.0f, 1.5f)));
        juce::ColourGradient grad (juce::Colour (0xffd8d4cc), c.x - rad, c.y - rad, juce::Colour (0xff5a5650), c.x + rad, c.y + rad, false);
        g.setGradientFill (grad);
        g.fillEllipse (juce::Rectangle<float> (rad * 2.0f, rad * 2.0f).withCentre (c));
        // rust bloom
        for (int i = 0; i < 6; ++i)
        {
            g.setColour (juce::Colour (0xff7a3e18).withAlpha (0.25f + 0.3f * r.nextFloat()));
            const float a = r.nextFloat() * 6.28f, d = rad * (0.4f + 0.7f * r.nextFloat());
            g.fillEllipse (juce::Rectangle<float> (1.6f, 1.6f).withCentre (c + juce::Point<float> (std::cos (a) * d, std::sin (a) * d)));
        }
        const float a = r.nextFloat() * 3.14f;
        g.setColour (juce::Colour (0xff2a2724));
        g.drawLine (juce::Line<float> (c.getPointOnCircumference (rad * 0.75f, a), c.getPointOnCircumference (rad * 0.75f, a + 3.14159f)), rad * 0.35f);
    }

    // Lettering that has been rubbed half off by years of gigs
    void wornText (juce::Graphics& g, const juce::String& text, juce::Rectangle<float> area, juce::Font f,
                   juce::Justification j, juce::Colour c, juce::Random& r, float wear = 0.35f)
    {
        g.setFont (f);
        g.setColour (c.withAlpha (0.88f));
        g.drawFittedText (text, area.toNearestInt(), j, 2, 0.8f);
        const int specks = (int) (area.getWidth() * area.getHeight() * 0.05f * wear);
        for (int i = 0; i < specks; ++i)
        {
            g.setColour (vary (gold, r, 0.12f).withAlpha (0.55f + 0.35f * r.nextFloat()));
            const float s = 0.8f + 1.6f * r.nextFloat();
            g.fillEllipse (area.getX() + r.nextFloat() * area.getWidth(), area.getY() + r.nextFloat() * area.getHeight(), s, s * 0.7f);
        }
    }
}

//==============================================================================
juce::Font AmpLookAndFeel::font (float height, const juce::StringArray& families, const juce::String& style)
{
    static const juce::StringArray installed = juce::Font::findAllTypefaceNames();
    juce::String face = juce::Font::getDefaultSansSerifFontName();
    for (auto& f : families)
        if (installed.contains (f)) { face = f; break; }
    return juce::Font (juce::FontOptions().withName (face).withHeight (height).withStyle (style));
}

juce::Font AmpLookAndFeel::stencil (float height)
{
    return font (height, { "Arial Black", "Franklin Gothic Heavy", "Helvetica Neue", "Inter", "DejaVu Sans" }, "Bold");
}

juce::Font AmpLookAndFeel::marker (float height)
{
    return font (height, { "Ink Free", "Segoe Print", "Bradley Hand", "Comic Sans MS", "Inter", "DejaVu Sans" }, "Bold");
}

//------------------------------------------------------------------------------
// Cream bakelite chicken-head knob
void AmpLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h,
                                       float pos, float startAngle, float endAngle, juce::Slider& s)
{
    const auto b = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h);
    const auto c = b.getCentre();
    const float R = juce::jmin (b.getWidth(), b.getHeight()) * 0.5f - 3.0f;
    const float angle = startAngle + pos * (endAngle - startAngle);
    const bool big = (bool) s.getProperties().getWithDefault ("big", false);

    // knurled black skirt
    const float skirt = R * 0.6f;
    g.setColour (juce::Colours::black.withAlpha (0.55f));
    g.fillEllipse (juce::Rectangle<float> (skirt * 2.0f, skirt * 2.0f).withCentre (c.translated (2.0f, 3.5f)));
    juce::ColourGradient sk (juce::Colour (0xff3a3632), c.x - skirt, c.y - skirt, juce::Colour (0xff0b0a09), c.x + skirt, c.y + skirt, false);
    g.setGradientFill (sk);
    g.fillEllipse (juce::Rectangle<float> (skirt * 2.0f, skirt * 2.0f).withCentre (c));
    g.setColour (juce::Colour (0xff4a4540).withAlpha (0.8f));
    for (int i = 0; i < 48; ++i)
    {
        const float a = (float) i / 48.0f * juce::MathConstants<float>::twoPi + angle;
        g.drawLine (juce::Line<float> (c.getPointOnCircumference (skirt * 0.86f, a), c.getPointOnCircumference (skirt * 0.98f, a)), 1.0f);
    }

    // pointer shape, drawn pointing up then rotated
    juce::Path p;
    p.startNewSubPath (0.0f, -R);
    p.cubicTo ( R * 0.16f, -R * 0.78f,  R * 0.36f, -R * 0.12f,  R * 0.36f,  R * 0.14f);
    p.cubicTo ( R * 0.36f,  R * 0.52f, -R * 0.36f,  R * 0.52f, -R * 0.36f,  R * 0.14f);
    p.cubicTo (-R * 0.36f, -R * 0.12f, -R * 0.16f, -R * 0.78f,  0.0f,      -R);
    p.closeSubPath();
    const auto t = juce::AffineTransform::rotation (angle).translated (c);

    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.fillPath (p, t.translated (3.0f, 4.5f));

    juce::ColourGradient body (juce::Colour (0xfff3ead0), c.x - R * 0.4f, c.y - R * 0.5f,
                               juce::Colour (0xffb9a473), c.x + R * 0.5f, c.y + R * 0.5f, false);
    body.addColour (0.55, juce::Colour (0xffdccb9c));
    g.setGradientFill (body);
    g.fillPath (p, t);

    // grime in the creases and a glossy highlight
    g.setColour (juce::Colour (0xff5b4a2c).withAlpha (0.55f));
    g.strokePath (p, juce::PathStrokeType (1.4f), t);
    g.setColour (juce::Colours::white.withAlpha (0.35f));
    g.fillEllipse (juce::Rectangle<float> (R * 0.28f, R * 0.18f).withCentre (c.translated (-R * 0.12f, -R * 0.1f)));

    // engraved index line
    juce::Path line;
    line.startNewSubPath (0.0f, -R * 0.9f);
    line.lineTo (0.0f, -R * 0.2f);
    g.setColour (juce::Colour (0xff201a12));
    g.strokePath (line, juce::PathStrokeType (big ? 3.0f : 2.4f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded), t);

    // brass centre screw
    g.setColour (juce::Colour (0xff8a6d32));
    g.fillEllipse (juce::Rectangle<float> (R * 0.16f, R * 0.16f).withCentre (c));
}

//------------------------------------------------------------------------------
// Chrome bat-handle toggle: lever up = on
void AmpLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& b, bool highlighted, bool)
{
    const auto r = b.getLocalBounds().toFloat();
    const auto c = r.getCentre();
    const bool on = b.getToggleState();

    // hex nut
    juce::Path hex;
    for (int i = 0; i < 6; ++i)
    {
        const auto p = c.getPointOnCircumference (11.0f, (float) i * juce::MathConstants<float>::pi / 3.0f + 0.52f);
        if (i == 0) hex.startNewSubPath (p); else hex.lineTo (p);
    }
    hex.closeSubPath();
    g.setColour (juce::Colours::black.withAlpha (0.45f));
    g.fillPath (hex, juce::AffineTransform::translation (1.5f, 2.5f));
    juce::ColourGradient ng (juce::Colour (0xffe6e3dc), c.x - 10.0f, c.y - 10.0f, juce::Colour (0xff6c6862), c.x + 10.0f, c.y + 10.0f, false);
    g.setGradientFill (ng);
    g.fillPath (hex);
    g.setColour (juce::Colour (0xff3b3833));
    g.strokePath (hex, juce::PathStrokeType (0.8f));
    g.setColour (juce::Colour (0xff2b2925));
    g.fillEllipse (juce::Rectangle<float> (9.0f, 9.0f).withCentre (c));

    // lever
    const float dir = on ? -1.0f : 1.0f;
    const auto tip = c.translated (0.0f, dir * (r.getHeight() * 0.5f - 6.0f));
    g.setColour (juce::Colours::black.withAlpha (0.4f));
    g.drawLine (juce::Line<float> (c.translated (2.0f, 3.0f), tip.translated (3.0f, 4.0f)), 5.0f);
    juce::ColourGradient lg (juce::Colour (0xfff4f2ee), c.x - 3.0f, c.y, juce::Colour (0xff7d7973), c.x + 3.0f, c.y, false);
    g.setGradientFill (lg);
    g.drawLine (juce::Line<float> (c, tip), 5.0f);
    g.fillEllipse (juce::Rectangle<float> (8.0f, 8.0f).withCentre (tip));
    g.setColour (juce::Colours::white.withAlpha (highlighted ? 0.9f : 0.6f));
    g.fillEllipse (juce::Rectangle<float> (3.0f, 3.0f).withCentre (tip.translated (-1.0f, -1.0f)));
}

//==============================================================================
ShredMaxxEditor::ShredMaxxEditor (ShredMaxxProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    setLookAndFeel (&lnf);

    setupKnob (knobs[0], "input",    "INPUT",    227.0f);
    setupKnob (knobs[1], "attack",   "ATTACK",   325.0f);
    setupKnob (knobs[2], "release",  "RELEASE",  423.0f);
    setupKnob (knobs[3], "output",   "OUTPUT",   697.0f);
    setupKnob (knobs[4], "headroom", "HEADROOM", 795.0f);
    setupKnob (knobs[5], "mix",      "MIX",      893.0f);
    setupKnob (ratio,    "ratio",    "RATIO",    560.0f, true);

    setupSwitch (eardrum[0], "schpf",      "HPF",          "100 Hz sidechain");
    setupSwitch (eardrum[1], "bandbeater", "BAND BEATER",  "6 kHz sidechain");
    setupSwitch (eardrum[2], "link",       "STEREO LINK",  "sums control volts");
    setupSwitch (audio[0],   "audiohpf",   "HPF",          "80 Hz 18 dB/oct");
    setupSwitch (audio[1],   "dist2",      "DIST DEUX",    "2nd harmonic tube");
    setupSwitch (audio[2],   "dist3",      "DIST TROIS",   "3rd harmonic tape");

    setSize (W, H);
    startTimerHz (30);
}

ShredMaxxEditor::~ShredMaxxEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void ShredMaxxEditor::setupKnob (Knob& k, const juce::String& paramId, const juce::String& label, float cx, bool big)
{
    k.label = label;
    k.centre = { cx, big ? ratioY : knobY };
    k.slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    k.slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    k.slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f, juce::MathConstants<float>::pi * 2.75f, true);
    k.slider.getProperties().set ("big", big);
    k.slider.setMouseDragSensitivity (big ? 400 : 250);
    addAndMakeVisible (k.slider);
    k.attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, paramId, k.slider);
    if (auto* prm = processor.apvts.getParameter (paramId))
        k.slider.setDoubleClickReturnValue (true, prm->convertFrom0to1 (prm->getDefaultValue()));
}

void ShredMaxxEditor::setupSwitch (Switch& s, const juce::String& paramId, const juce::String& title, const juce::String& subtitle)
{
    s.title = title;
    s.subtitle = subtitle;
    addAndMakeVisible (s.button);
    s.attachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (processor.apvts, paramId, s.button);
}

void ShredMaxxEditor::resized()
{
    for (auto& k : knobs)
        k.slider.setBounds (juce::Rectangle<float> (knobSize, knobSize).withCentre (k.centre).toNearestInt());
    ratio.slider.setBounds (juce::Rectangle<float> (ratioSize, ratioSize).withCentre (ratio.centre).toNearestInt());

    for (int i = 0; i < 3; ++i)
    {
        const float rowY = eardrumCol.getY() + 54.0f + (float) i * 64.0f;
        eardrum[(size_t) i].button.setBounds (juce::Rectangle<float> (34.0f, 52.0f).withCentre ({ eardrumCol.getRight() - 22.0f, rowY }).toNearestInt());
        audio[(size_t) i].button.setBounds   (juce::Rectangle<float> (34.0f, 52.0f).withCentre ({ audioCol.getRight() - 22.0f, rowY }).toNearestInt());
    }

    const auto plate = juce::Rectangle<float> (634.0f, 82.0f, 418.0f, 202.0f);
    vuFace   = juce::Rectangle<float> (plate.getX() + 22.0f, plate.getY() + 22.0f, 252.0f, 158.0f);
    lampArea = juce::Rectangle<float> (vuFace.getRight() + 16.0f, plate.getY() + 14.0f, plate.getRight() - vuFace.getRight() - 30.0f, plate.getHeight() - 28.0f);

    paintBackground();
}

juce::Rectangle<float> ShredMaxxEditor::tapeRect (const Knob& k) const
{
    return juce::Rectangle<float> (84.0f, 22.0f).withCentre (k.centre.translated (0.0f, 66.0f));
}

juce::Point<float> ShredMaxxEditor::ratioLed (int i) const
{
    return ratio.centre.getPointOnCircumference (58.0f, ratioAngle (i));
}

//==============================================================================
void ShredMaxxEditor::paintBackground()
{
    background = juce::Image (juce::Image::ARGB, W, H, true);
    juce::Graphics g (background);
    juce::Random r (2000);

    // Stage floor behind the amp
    g.fillAll (juce::Colour (0xff0d0c0b));

    //---- Tolex body ---------------------------------------------------------
    g.setColour (juce::Colours::black.withAlpha (0.7f));
    g.fillRoundedRectangle (bodyR.translated (0.0f, 6.0f), 20.0f);
    g.setColour (juce::Colour (0xff1d1c1a));
    g.fillRoundedRectangle (bodyR, 20.0f);
    {
        juce::Graphics::ScopedSaveState ss (g);
        juce::Path clip; clip.addRoundedRectangle (bodyR, 20.0f);
        g.reduceClipRegion (clip);

        // levant grain
        for (int i = 0; i < 46000; ++i)
        {
            const float x = bodyR.getX() + r.nextFloat() * bodyR.getWidth();
            const float y = bodyR.getY() + r.nextFloat() * bodyR.getHeight();
            g.setColour ((r.nextBool() ? juce::Colour (0xff3a3835) : juce::Colours::black).withAlpha (0.35f + 0.3f * r.nextFloat()));
            g.fillEllipse (x, y, 1.0f + 2.0f * r.nextFloat(), 1.0f + 1.4f * r.nextFloat());
        }

        // edge scuffs where it has been dragged in and out of vans
        for (int i = 0; i < 900; ++i)
        {
            const int side = r.nextInt (4);
            const float d = std::pow (r.nextFloat(), 2.0f) * 22.0f;
            float x, y;
            if (side == 0)      { x = bodyR.getX() + r.nextFloat() * bodyR.getWidth(); y = bodyR.getY() + d; }
            else if (side == 1) { x = bodyR.getX() + r.nextFloat() * bodyR.getWidth(); y = bodyR.getBottom() - d; }
            else if (side == 2) { x = bodyR.getX() + d; y = bodyR.getY() + r.nextFloat() * bodyR.getHeight(); }
            else                { x = bodyR.getRight() - d; y = bodyR.getY() + r.nextFloat() * bodyR.getHeight(); }
            g.setColour (juce::Colour (0xff6b665e).withAlpha (0.08f + 0.18f * r.nextFloat()));
            const float a = r.nextFloat() * 3.14f, len = 3.0f + 10.0f * r.nextFloat();
            g.drawLine (x, y, x + std::cos (a) * len, y + std::sin (a) * len, 0.8f + 1.5f * r.nextFloat());
        }

        // rips showing the plywood underneath
        auto rip = [&] (float cx, float cy, float rw, float rh)
        {
            juce::Path p;
            const int pts = 14;
            for (int i = 0; i < pts; ++i)
            {
                const float a = (float) i / pts * juce::MathConstants<float>::twoPi;
                const float k = 0.65f + 0.45f * r.nextFloat();
                const juce::Point<float> pt (cx + std::cos (a) * rw * k, cy + std::sin (a) * rh * k);
                if (i == 0) p.startNewSubPath (pt); else p.lineTo (pt);
            }
            p.closeSubPath();
            g.setColour (juce::Colour (0xff8c6b45));
            g.fillPath (p);
            for (int i = 0; i < 18; ++i)
            {
                g.setColour (juce::Colour (0xff5c4128).withAlpha (0.5f));
                const float yy = cy - rh + r.nextFloat() * rh * 2.0f;
                g.drawLine (cx - rw, yy, cx + rw, yy + (r.nextFloat() - 0.5f) * 3.0f, 0.7f);
            }
            g.setColour (juce::Colour (0xff4a4640));
            g.strokePath (p, juce::PathStrokeType (1.6f));
            g.setColour (juce::Colours::black.withAlpha (0.5f));
            g.strokePath (p, juce::PathStrokeType (1.0f), juce::AffineTransform::translation (0.0f, 1.5f));
        };
        rip (bodyR.getX() + 340.0f, bodyR.getBottom() - 7.0f, 34.0f, 7.0f);
        rip (bodyR.getRight() - 9.0f, bodyR.getY() + 190.0f, 7.0f, 26.0f);
        rip (bodyR.getX() + 760.0f, bodyR.getY() + 9.0f, 22.0f, 6.0f);
        rip (bodyR.getX() + 8.0f, bodyR.getY() + 420.0f, 6.0f, 15.0f);
    }

    //---- Grille cloth -------------------------------------------------------
    {
        juce::Graphics::ScopedSaveState ss (g);
        juce::Path clip; clip.addRoundedRectangle (grilleR, 8.0f);
        g.reduceClipRegion (clip);

        g.setColour (juce::Colour (0xff4f1d18));
        g.fillRect (grilleR);
        const float cell = 6.0f;
        for (float y = grilleR.getY(); y < grilleR.getBottom(); y += cell)
            for (float x = grilleR.getX(); x < grilleR.getRight(); x += cell)
            {
                const bool horiz = (((int) ((x - grilleR.getX()) / cell) + (int) ((y - grilleR.getY()) / cell)) % 2) == 0;
                const juce::Colour thread = horiz ? juce::Colour (0xff7a3a2c) : juce::Colour (0xff9a6a48);
                g.setColour (vary (thread, r, 0.15f).withAlpha (0.85f));
                if (horiz) g.fillRoundedRectangle (x + 0.5f, y + 1.5f, cell - 1.0f, cell - 3.0f, 1.5f);
                else       g.fillRoundedRectangle (x + 1.5f, y + 0.5f, cell - 3.0f, cell - 1.0f, 1.5f);
            }

        // sun fading at the top, smoke and beer stains
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xffd9b48a).withAlpha (0.22f), 0.0f, grilleR.getY(),
                                                 juce::Colours::transparentBlack, 0.0f, grilleR.getY() + 120.0f, false));
        g.fillRect (grilleR);
        for (int i = 0; i < 16; ++i)
        {
            const float cx = grilleR.getX() + r.nextFloat() * grilleR.getWidth();
            const float cy = grilleR.getY() + r.nextFloat() * grilleR.getHeight();
            const float rad = 18.0f + 60.0f * r.nextFloat();
            g.setGradientFill (juce::ColourGradient (juce::Colour (0xff1a0d08).withAlpha (0.28f), cx, cy,
                                                     juce::Colours::transparentBlack, cx + rad, cy, true));
            g.fillEllipse (cx - rad, cy - rad, rad * 2.0f, rad * 2.0f);
        }
        g.setColour (juce::Colour (0xff2a140c).withAlpha (0.35f));     // bottle ring
        g.drawEllipse (468.0f, 222.0f, 58.0f, 52.0f, 3.0f);
        g.drawEllipse (472.0f, 226.0f, 50.0f, 45.0f, 1.0f);

        // pulled threads
        for (int i = 0; i < 7; ++i)
        {
            const float x = grilleR.getX() + r.nextFloat() * grilleR.getWidth();
            const float y = grilleR.getY() + r.nextFloat() * grilleR.getHeight();
            juce::Path t; t.startNewSubPath (x, y);
            t.quadraticTo (x + 10.0f, y + 14.0f * r.nextFloat(), x + 26.0f * r.nextFloat(), y + 22.0f);
            g.setColour (juce::Colour (0xffb78a5e).withAlpha (0.8f));
            g.strokePath (t, juce::PathStrokeType (1.2f));
        }
        g.setGradientFill (juce::ColourGradient (juce::Colours::transparentBlack, grilleR.getCentreX(), grilleR.getCentreY(),
                                                 juce::Colours::black.withAlpha (0.45f), grilleR.getX(), grilleR.getY(), true));
        g.fillRect (grilleR);
    }
    // piping
    g.setColour (juce::Colours::black.withAlpha (0.6f));
    g.drawRoundedRectangle (grilleR.expanded (3.0f).translated (0.0f, 1.5f), 10.0f, 4.0f);
    g.setColour (juce::Colour (0xffcfc6ae));
    g.drawRoundedRectangle (grilleR.expanded (3.0f), 10.0f, 3.2f);
    for (int i = 0; i < 260; ++i)    // grime on the piping
    {
        const auto e = grilleR.expanded (3.0f);
        const int side = r.nextInt (4);
        const float x = side < 2 ? e.getX() + r.nextFloat() * e.getWidth() : (side == 2 ? e.getX() : e.getRight());
        const float y = side >= 2 ? e.getY() + r.nextFloat() * e.getHeight() : (side == 0 ? e.getY() : e.getBottom());
        g.setColour (juce::Colour (0xff3b3125).withAlpha (0.3f + 0.4f * r.nextFloat()));
        g.fillEllipse (x - 1.5f, y - 1.5f, 3.0f, 3.0f);
    }

    //---- Logo ----------------------------------------------------------------
    {
        juce::GlyphArrangement ga;
        ga.addLineOfText (AmpLookAndFeel::font (78.0f, { "Impact", "Arial Black", "Inter", "DejaVu Sans" }, "Bold"),
                          "ShredMaxx", 0.0f, 0.0f);
        juce::Path logo; ga.createPath (logo);
        const auto lb = logo.getBounds();
        const auto lt = juce::AffineTransform::translation (-lb.getX(), -lb.getY())
                            .followedBy (juce::AffineTransform::shear (-0.22f, 0.0f))
                            .translated (96.0f, 98.0f);
        logo.applyTransform (lt);
        const auto b = logo.getBounds();

        g.setColour (juce::Colours::black.withAlpha (0.65f));
        g.fillPath (logo, juce::AffineTransform::translation (3.0f, 5.0f));
        juce::ColourGradient chrome (juce::Colour (0xfff7f6f2), 0.0f, b.getY(), juce::Colour (0xff8d8a84), 0.0f, b.getBottom(), false);
        chrome.addColour (0.45, juce::Colour (0xffd9d6cf));
        chrome.addColour (0.52, juce::Colour (0xff57534c));
        chrome.addColour (0.70, juce::Colour (0xffb7b3ab));
        g.setGradientFill (chrome);
        g.fillPath (logo);
        g.setColour (juce::Colour (0xff1c1a17));
        g.strokePath (logo, juce::PathStrokeType (1.6f));

        // pitting on the chrome
        {
            juce::Graphics::ScopedSaveState ss (g);
            g.reduceClipRegion (logo);
            for (int i = 0; i < 260; ++i)
            {
                g.setColour ((r.nextBool() ? juce::Colour (0xff4b3a26) : juce::Colours::white).withAlpha (0.35f));
                g.fillEllipse (b.getX() + r.nextFloat() * b.getWidth(), b.getY() + r.nextFloat() * b.getHeight(), 1.6f, 1.6f);
            }
        }

        // "2000" badge
        auto badge = juce::Rectangle<float> (b.getRight() - 132.0f, b.getBottom() + 8.0f, 126.0f, 40.0f);
        g.setColour (juce::Colours::black.withAlpha (0.5f));
        g.fillRoundedRectangle (badge.translated (2.0f, 3.0f), 5.0f);
        juce::ColourGradient red (juce::Colour (0xffd2321f), 0.0f, badge.getY(), juce::Colour (0xff6e130b), 0.0f, badge.getBottom(), false);
        g.setGradientFill (red);
        g.fillRoundedRectangle (badge, 5.0f);
        g.setColour (juce::Colour (0xffd9d6cf));
        g.drawRoundedRectangle (badge.reduced (2.0f), 4.0f, 1.5f);
        g.setFont (AmpLookAndFeel::font (30.0f, { "Impact", "Arial Black", "Inter", "DejaVu Sans" }, "Bold"));
        g.setColour (juce::Colours::black.withAlpha (0.5f));
        g.drawText ("2000", badge.translated (1.5f, 1.5f), juce::Justification::centred);
        g.setColour (cream);
        g.drawText ("2000", badge, juce::Justification::centred);

        g.setFont (AmpLookAndFeel::stencil (12.5f));
        g.setColour (cream.withAlpha (0.8f));
        g.drawText ("DISTORTING COMPRESSOR  -  MODEL SM-2K", juce::Rectangle<float> (98.0f, badge.getY() + 8.0f, 360.0f, 20.0f),
                    juce::Justification::centredLeft);
    }

    //---- Gaffer tape on the grille --------------------------------------------
    {
        juce::Graphics::ScopedSaveState ss (g);
        g.addTransform (juce::AffineTransform::rotation (-0.05f, 200.0f, 262.0f));
        auto tape = juce::Rectangle<float> (70.0f, 246.0f, 270.0f, 36.0f);
        juce::Path tp;
        tp.startNewSubPath (tape.getX(), tape.getY());
        tp.lineTo (tape.getRight(), tape.getY());
        for (int i = 0; i <= 6; ++i) tp.lineTo (tape.getRight() + ((i % 2) ? 4.0f : -2.0f), tape.getY() + tape.getHeight() * (float) i / 6.0f);
        tp.lineTo (tape.getX(), tape.getBottom());
        for (int i = 6; i >= 0; --i) tp.lineTo (tape.getX() + ((i % 2) ? -4.0f : 2.0f), tape.getY() + tape.getHeight() * (float) i / 6.0f);
        tp.closeSubPath();
        g.setColour (juce::Colours::black.withAlpha (0.4f));
        g.fillPath (tp, juce::AffineTransform::translation (1.5f, 2.5f));
        g.setColour (juce::Colour (0xff8d8f8c));
        g.fillPath (tp);
        for (int i = 0; i < 160; ++i)
        {
            g.setColour (juce::Colour (0xff5f615e).withAlpha (0.3f));
            const float x = tape.getX() + r.nextFloat() * tape.getWidth();
            g.drawLine (x, tape.getY(), x + 2.0f, tape.getBottom(), 0.6f);
        }
        g.setFont (AmpLookAndFeel::marker (20.0f));
        g.setColour (juce::Colour (0xff121212).withAlpha (0.85f));
        g.drawText ("ROOM MICS = KABOOM!!", tape, juce::Justification::centred);
    }

    //---- Meter plate ---------------------------------------------------------
    {
        const auto plate = juce::Rectangle<float> (634.0f, 82.0f, 418.0f, 202.0f);
        g.setColour (juce::Colours::black.withAlpha (0.6f));
        g.fillRoundedRectangle (plate.translated (3.0f, 5.0f), 8.0f);
        juce::ColourGradient pg (juce::Colour (0xff2c2a27), 0.0f, plate.getY(), juce::Colour (0xff121110), 0.0f, plate.getBottom(), false);
        g.setGradientFill (pg);
        g.fillRoundedRectangle (plate, 8.0f);
        for (int i = 0; i < 200; ++i)
        {
            g.setColour (juce::Colour (0xff8a857c).withAlpha (0.12f + 0.15f * r.nextFloat()));
            const float x = plate.getX() + r.nextFloat() * plate.getWidth(), y = plate.getY() + r.nextFloat() * plate.getHeight();
            g.drawLine (x, y, x + (r.nextFloat() - 0.5f) * 30.0f, y + (r.nextFloat() - 0.5f) * 6.0f, 0.6f);
        }
        g.setColour (juce::Colour (0xff4b4741));
        g.drawRoundedRectangle (plate, 8.0f, 1.5f);
        screw (g, plate.getTopLeft().translated (10.0f, 10.0f), 4.5f, r);
        screw (g, plate.getTopRight().translated (-10.0f, 10.0f), 4.5f, r);
        screw (g, plate.getBottomLeft().translated (10.0f, -10.0f), 4.5f, r);
        screw (g, plate.getBottomRight().translated (-10.0f, -10.0f), 4.5f, r);

        // VU meter: chrome bezel + yellowed face
        const auto bezel = vuFace.expanded (7.0f);
        juce::ColourGradient bz (juce::Colour (0xffe9e6df), bezel.getX(), bezel.getY(), juce::Colour (0xff4f4b45), bezel.getRight(), bezel.getBottom(), false);
        g.setGradientFill (bz);
        g.fillRoundedRectangle (bezel, 9.0f);
        juce::ColourGradient face (juce::Colour (0xfff2e2b0), vuFace.getCentreX(), vuFace.getY(),
                                   juce::Colour (0xffcfae6a), vuFace.getCentreX(), vuFace.getBottom(), false);
        g.setGradientFill (face);
        g.fillRoundedRectangle (vuFace, 5.0f);
        for (int i = 0; i < 8; ++i)    // water stains on the face
        {
            const float cx = vuFace.getX() + r.nextFloat() * vuFace.getWidth(), cy = vuFace.getY() + r.nextFloat() * vuFace.getHeight();
            const float rad = 8.0f + 26.0f * r.nextFloat();
            g.setColour (juce::Colour (0xff9a7638).withAlpha (0.12f));
            g.drawEllipse (cx - rad, cy - rad * 0.8f, rad * 2.0f, rad * 1.6f, 1.2f);
        }

        // scale
        const juce::Point<float> pivot (vuFace.getCentreX(), vuFace.getBottom() + 34.0f);
        const float arcR = 150.0f;
        auto gToAngle = [] (float gr) { return juce::jmap (std::pow (juce::jlimit (0.0f, 20.0f, gr) / 20.0f, 0.6f), 0.8f, -0.8f); };
        juce::Path arc;
        arc.addCentredArc (pivot.x, pivot.y, arcR, arcR, 0.0f, -0.8f, 0.8f, true);
        g.setColour (ink);
        g.strokePath (arc, juce::PathStrokeType (1.6f));
        juce::Path redArc;
        redArc.addCentredArc (pivot.x, pivot.y, arcR + 4.0f, arcR + 4.0f, 0.0f, -0.8f, gToAngle (10.0f), true);
        g.setColour (juce::Colour (0xffb3241a));
        g.strokePath (redArc, juce::PathStrokeType (5.0f));
        g.setFont (AmpLookAndFeel::stencil (11.0f));
        for (float gr : { 0.0f, 1.0f, 2.0f, 3.0f, 5.0f, 7.0f, 10.0f, 15.0f, 20.0f })
        {
            const float a = gToAngle (gr);
            g.setColour (ink);
            g.drawLine (juce::Line<float> (pivot.getPointOnCircumference (arcR - 8.0f, a), pivot.getPointOnCircumference (arcR + 2.0f, a)), 1.6f);
            g.drawText (juce::String ((int) gr), juce::Rectangle<float> (30.0f, 14.0f).withCentre (pivot.getPointOnCircumference (arcR + 15.0f, a)),
                        juce::Justification::centred);
        }
        g.setFont (AmpLookAndFeel::stencil (12.0f));
        g.drawText ("GAIN REDUCTION  dB", vuFace.withTrimmedTop (vuFace.getHeight() - 52.0f).withHeight (18.0f), juce::Justification::centred);
        g.setFont (AmpLookAndFeel::stencil (9.0f));
        g.setColour (ink.withAlpha (0.7f));
        g.drawText ("SM-2K", vuFace.withTrimmedTop (vuFace.getHeight() - 34.0f).withHeight (14.0f), juce::Justification::centred);

        // lamp labels
        g.setFont (AmpLookAndFeel::stencil (10.5f));
        g.setColour (cream.withAlpha (0.85f));
        const char* names[4] = { "THD 1%", "THD 3%", "OPTO CELL", "POWER" };
        for (int i = 0; i < 4; ++i)
        {
            const float y = lampArea.getY() + 12.0f + (float) i * 44.0f;
            g.drawText (names[i], juce::Rectangle<float> (lampArea.getX() + 36.0f, y - 8.0f, lampArea.getWidth() - 36.0f, 16.0f),
                        juce::Justification::centredLeft);
            // lamp sockets
            g.setColour (juce::Colour (0xff8f8a82));
            g.fillEllipse (juce::Rectangle<float> (i == 3 ? 28.0f : 22.0f, i == 3 ? 28.0f : 22.0f).withCentre ({ lampArea.getX() + 16.0f, y }));
            g.setColour (juce::Colour (0xff1a1816));
            g.fillEllipse (juce::Rectangle<float> (i == 3 ? 22.0f : 16.0f, i == 3 ? 22.0f : 16.0f).withCentre ({ lampArea.getX() + 16.0f, y }));
            g.setColour (cream.withAlpha (0.85f));
        }
    }

    //---- Control panel: brushed gold, badly worn -------------------------------
    g.setColour (juce::Colours::black.withAlpha (0.65f));
    g.fillRoundedRectangle (panelR.translated (0.0f, 4.0f), 6.0f);
    {
        juce::Graphics::ScopedSaveState ss (g);
        juce::Path clip; clip.addRoundedRectangle (panelR, 6.0f);
        g.reduceClipRegion (clip);

        juce::ColourGradient pg (juce::Colour (0xffd8b25a), 0.0f, panelR.getY(), juce::Colour (0xffa47d31), 0.0f, panelR.getBottom(), false);
        g.setGradientFill (pg);
        g.fillRect (panelR);
        for (int i = 0; i < 2600; ++i)      // brushing
        {
            const float y = panelR.getY() + r.nextFloat() * panelR.getHeight();
            const float x = panelR.getX() + r.nextFloat() * panelR.getWidth();
            g.setColour ((r.nextBool() ? juce::Colour (0xfff2d48a) : juce::Colour (0xff7d5e22)).withAlpha (0.08f + 0.12f * r.nextFloat()));
            g.drawLine (x, y, x + 40.0f + 160.0f * r.nextFloat(), y + (r.nextFloat() - 0.5f) * 0.6f, 0.7f);
        }

        // gold rubbed through to the silver under-plate around the knobs
        auto rub = [&] (juce::Point<float> c, float rad)
        {
            for (int i = 0; i < 4; ++i)
            {
                const auto cc = c.translated ((r.nextFloat() - 0.5f) * 10.0f, (r.nextFloat() - 0.5f) * 10.0f);
                const float rr = rad * (0.8f + 0.5f * r.nextFloat());
                g.setGradientFill (juce::ColourGradient (juce::Colour (0xffd9d4c6).withAlpha (0.35f), cc.x, cc.y,
                                                         juce::Colours::transparentWhite, cc.x + rr, cc.y, true));
                g.fillEllipse (cc.x - rr, cc.y - rr, rr * 2.0f, rr * 2.0f);
            }
        };
        for (auto& k : knobs) rub (k.centre, 50.0f);
        rub (ratio.centre, 66.0f);
        

        // scratches
        for (int i = 0; i < 160; ++i)
        {
            const float x = panelR.getX() + r.nextFloat() * panelR.getWidth(), y = panelR.getY() + r.nextFloat() * panelR.getHeight();
            const float a = r.nextFloat() * 3.14f, len = 4.0f + 50.0f * std::pow (r.nextFloat(), 2.0f);
            g.setColour (juce::Colour (0xfff8eccd).withAlpha (0.25f + 0.3f * r.nextFloat()));
            g.drawLine (x, y, x + std::cos (a) * len, y + std::sin (a) * len, 0.6f);
            g.setColour (juce::Colour (0xff5a4218).withAlpha (0.2f));
            g.drawLine (x, y + 0.8f, x + std::cos (a) * len, y + 0.8f + std::sin (a) * len, 0.5f);
        }
        // grime collecting along the bottom and corrosion spots
        g.setGradientFill (juce::ColourGradient (juce::Colours::transparentBlack, 0.0f, panelR.getBottom() - 70.0f,
                                                 juce::Colour (0xff2b1d0a).withAlpha (0.5f), 0.0f, panelR.getBottom(), false));
        g.fillRect (panelR);
        for (int i = 0; i < 40; ++i)
        {
            const float x = panelR.getX() + r.nextFloat() * panelR.getWidth();
            const float y = panelR.getY() + r.nextFloat() * panelR.getHeight();
            const float s = 1.5f + 4.0f * r.nextFloat();
            g.setColour (juce::Colour (0xff5d4a2a).withAlpha (0.35f + 0.3f * r.nextFloat()));
            g.fillEllipse (x, y, s, s * (0.6f + 0.6f * r.nextFloat()));
        }
        // old sticker residue
        g.setColour (juce::Colour (0xffe8dcc0).withAlpha (0.18f));
        g.fillRoundedRectangle (470.0f, 548.0f, 120.0f, 26.0f, 3.0f);
        g.setColour (juce::Colour (0xff6e5a34).withAlpha (0.25f));
        g.drawRoundedRectangle (470.0f, 548.0f, 120.0f, 26.0f, 3.0f, 1.0f);
    }
    g.setColour (juce::Colour (0xff4a3714));
    g.drawRoundedRectangle (panelR, 6.0f, 2.0f);
    g.setColour (juce::Colour (0xfff3d995).withAlpha (0.5f));
    g.drawLine (panelR.getX() + 6.0f, panelR.getY() + 1.5f, panelR.getRight() - 6.0f, panelR.getY() + 1.5f, 1.0f);
    screw (g, panelR.getTopLeft().translated (12.0f, 12.0f), 5.0f, r);
    screw (g, panelR.getTopRight().translated (-12.0f, 12.0f), 5.0f, r);
    screw (g, panelR.getBottomLeft().translated (12.0f, -12.0f), 5.0f, r);
    screw (g, panelR.getBottomRight().translated (-12.0f, -12.0f), 5.0f, r);

    //---- Silkscreen ----------------------------------------------------------
    // section boxes
    g.setColour (ink.withAlpha (0.7f));
    g.drawRoundedRectangle (eardrumCol, 5.0f, 1.6f);
    g.drawRoundedRectangle (audioCol, 5.0f, 1.6f);

    auto header = [&] (const juce::String& t, juce::Rectangle<float> col)
    {
        auto hb = juce::Rectangle<float> (col.getWidth() - 30.0f, 18.0f).withCentre ({ col.getCentreX(), col.getY() });
        g.setColour (ink);
        g.fillRect (hb);
        wornText (g, t, hb, AmpLookAndFeel::stencil (13.0f), juce::Justification::centred, gold.brighter (0.2f), r, 0.15f);
    };
    header ("EARDRUM", eardrumCol);
    header ("AUDIO", audioCol);

    auto switchLabels = [&] (std::array<Switch, 3>& sw, juce::Rectangle<float> col)
    {
        for (int i = 0; i < 3; ++i)
        {
            const float rowY = col.getY() + 54.0f + (float) i * 64.0f;
            auto area = juce::Rectangle<float> (col.getX() + 8.0f, rowY - 22.0f, col.getWidth() - 50.0f, 44.0f);
            wornText (g, sw[(size_t) i].title, area.withHeight (20.0f), AmpLookAndFeel::stencil (sw[(size_t) i].title.length() > 8 ? 11.5f : 14.0f),
                      juce::Justification::centredLeft, ink, r);
            wornText (g, sw[(size_t) i].subtitle, area.withTrimmedTop (20.0f).withHeight (24.0f),
                      AmpLookAndFeel::font (9.5f, { "Arial", "Inter", "DejaVu Sans" }, "Bold"),
                      juce::Justification::topLeft, ink.withAlpha (0.8f), r, 0.2f);
            const float tx = col.getRight() - 22.0f;
            g.setFont (AmpLookAndFeel::stencil (8.0f));
            g.setColour (ink.withAlpha (0.75f));
            g.drawText ("ON",  juce::Rectangle<float> (30.0f, 10.0f).withCentre ({ tx, rowY - 30.0f }), juce::Justification::centred);
        }
    };
    switchLabels (eardrum, eardrumCol);
    switchLabels (audio, audioCol);

    // knob labels and 0..10 scales
    auto scale = [&] (const Knob& k, float rad)
    {
        wornText (g, k.label, juce::Rectangle<float> (96.0f, 18.0f).withCentre (k.centre.translated (0.0f, -rad - 20.0f)),
                  AmpLookAndFeel::stencil (k.label.length() > 6 ? 12.5f : 14.0f), juce::Justification::centred, ink, r);
        g.setFont (AmpLookAndFeel::stencil (9.5f));
        for (int i = 0; i <= 10; ++i)
        {
            const float a = juce::MathConstants<float>::pi * (1.25f + 1.5f * (float) i / 10.0f);
            g.setColour (ink.withAlpha (0.85f));
            g.drawLine (juce::Line<float> (k.centre.getPointOnCircumference (rad - 7.0f, a), k.centre.getPointOnCircumference (rad - 2.0f, a)), i % 5 == 0 ? 2.0f : 1.2f);
            if (i % 2 == 0)
                g.drawText (juce::String (i), juce::Rectangle<float> (16.0f, 12.0f).withCentre (k.centre.getPointOnCircumference (rad + 5.0f, a)),
                            juce::Justification::centred);
        }
    };
    for (auto& k : knobs) scale (k, 40.0f);

    // ratio selector legend
    wornText (g, "RATIO", juce::Rectangle<float> (120.0f, 18.0f).withCentre (ratio.centre.translated (0.0f, -102.0f)),
              AmpLookAndFeel::stencil (14.0f), juce::Justification::centred, ink, r);
    for (int i = 0; i < 8; ++i)
    {
        const float a = ratioAngle (i);
        const auto p = ratio.centre.getPointOnCircumference (i == 7 ? 86.0f : 80.0f, a);
        const auto& name = ShredMaxxProcessor::ratioNames()[i];
        const bool kab = i == 7;
        auto f = AmpLookAndFeel::stencil (kab ? 11.0f : 12.0f);
        auto box = juce::Rectangle<float> (kab ? 64.0f : 40.0f, 14.0f).withCentre (p);
        if (kab)
        {
            g.setColour (juce::Colour (0xffa3180e));
            g.fillRoundedRectangle (box.expanded (2.0f, 1.0f), 3.0f);
            wornText (g, name, box, f, juce::Justification::centred, cream, r, 0.2f);
        }
        else
            wornText (g, name, box, f, juce::Justification::centred, ink, r, 0.25f);
        if (i == 5)
            wornText (g, "OPTO", box.translated (0.0f, -13.0f).withSizeKeepingCentre (40.0f, 11.0f),
                      AmpLookAndFeel::stencil (8.0f), juce::Justification::centred, ink.withAlpha (0.8f), r, 0.1f);
        // LED bezel
        g.setColour (juce::Colour (0xff6a6458));
        g.fillEllipse (juce::Rectangle<float> (11.0f, 11.0f).withCentre (ratioLed (i)));
        g.setColour (juce::Colour (0xff2a0705));
        g.fillEllipse (juce::Rectangle<float> (7.0f, 7.0f).withCentre (ratioLed (i)));
    }

    // footer lettering
    wornText (g, "FIXED THRESHOLD  -  PUSH INPUT TO SQUASH & SATURATE  -  HEADROOM SETS THE RAILS",
              juce::Rectangle<float> (panelR.getX() + 150.0f, panelR.getBottom() - 30.0f, 740.0f, 16.0f),
              AmpLookAndFeel::stencil (9.5f), juce::Justification::centred, ink.withAlpha (0.8f), r, 0.4f);

    //---- Corner protectors --------------------------------------------------------
    auto corner = [&] (juce::Point<float> pt, float sx, float sy)
    {
        juce::Path p;
        p.startNewSubPath (0.0f, 0.0f);
        p.lineTo (54.0f, 0.0f);
        p.quadraticTo (54.0f, 18.0f, 30.0f, 24.0f);
        p.quadraticTo (24.0f, 52.0f, 0.0f, 54.0f);
        p.closeSubPath();
        const auto t = juce::AffineTransform::scale (sx, sy).translated (pt);
        g.setColour (juce::Colours::black.withAlpha (0.6f));
        g.fillPath (p, t.translated (2.0f, 3.0f));
        const auto bb = p.getBoundsTransformed (t);
        juce::ColourGradient cg (juce::Colour (0xffd2cfc8), bb.getX(), bb.getY(), juce::Colour (0xff4a4640), bb.getRight(), bb.getBottom(), false);
        cg.addColour (0.5, juce::Colour (0xff9a968e));
        g.setGradientFill (cg);
        g.fillPath (p, t);
        {
            juce::Graphics::ScopedSaveState ss (g);
            juce::Path pp = p; pp.applyTransform (t);
            g.reduceClipRegion (pp);
            for (int i = 0; i < 70; ++i)
            {
                const float x = bb.getX() + r.nextFloat() * bb.getWidth(), y = bb.getY() + r.nextFloat() * bb.getHeight();
                g.setColour ((r.nextFloat() < 0.4f ? juce::Colour (0xff6d3814) : juce::Colours::white).withAlpha (0.25f));
                g.drawLine (x, y, x + (r.nextFloat() - 0.5f) * 12.0f, y + (r.nextFloat() - 0.5f) * 12.0f, 0.7f);
            }
        }
        g.setColour (juce::Colour (0xff2e2b27));
        g.strokePath (p, juce::PathStrokeType (1.2f), t);
        screw (g, juce::Point<float> (16.0f, 16.0f).transformedBy (t), 3.5f, r);
    };
    corner (bodyR.getTopLeft(), 1.0f, 1.0f);
    corner (bodyR.getTopRight(), -1.0f, 1.0f);
    corner (bodyR.getBottomLeft(), 1.0f, -1.0f);
    corner (bodyR.getBottomRight(), -1.0f, -1.0f);

    //---- Leather strap handle ------------------------------------------------
    {
        const float cx = (float) W * 0.5f;
        for (float side : { -1.0f, 1.0f })
        {
            auto bracket = juce::Rectangle<float> (34.0f, 22.0f).withCentre ({ cx + side * 128.0f, 36.0f });
            g.setColour (juce::Colours::black.withAlpha (0.6f));
            g.fillRoundedRectangle (bracket.translated (2.0f, 3.0f), 4.0f);
            juce::ColourGradient bg (juce::Colour (0xffcfcbc3), bracket.getX(), bracket.getY(), juce::Colour (0xff4e4a44), bracket.getRight(), bracket.getBottom(), false);
            g.setGradientFill (bg);
            g.fillRoundedRectangle (bracket, 4.0f);
            screw (g, bracket.getCentre().translated (-8.0f, 0.0f), 3.0f, r);
            screw (g, bracket.getCentre().translated (8.0f, 0.0f), 3.0f, r);
        }
        juce::Path strap;
        strap.startNewSubPath (cx - 120.0f, 30.0f);
        strap.cubicTo (cx - 90.0f, 4.0f, cx + 90.0f, 4.0f, cx + 120.0f, 30.0f);
        g.setColour (juce::Colours::black.withAlpha (0.6f));
        g.strokePath (strap, juce::PathStrokeType (22.0f, juce::PathStrokeType::curved, juce::PathStrokeType::butt), juce::AffineTransform::translation (2.0f, 4.0f));
        g.setColour (juce::Colour (0xff2a1d14));
        g.strokePath (strap, juce::PathStrokeType (22.0f, juce::PathStrokeType::curved, juce::PathStrokeType::butt));
        g.setColour (juce::Colour (0xff5a3d26).withAlpha (0.8f));
        g.strokePath (strap, juce::PathStrokeType (12.0f, juce::PathStrokeType::curved, juce::PathStrokeType::butt));
        juce::Path stitch;
        const float dashes[2] = { 4.0f, 3.0f };
        juce::PathStrokeType (1.0f).createDashedStroke (stitch, strap, dashes, 2, juce::AffineTransform::translation (0.0f, -7.0f));
        g.setColour (juce::Colour (0xffc9b48a).withAlpha (0.7f));
        g.fillPath (stitch);
        juce::Path stitch2;
        juce::PathStrokeType (1.0f).createDashedStroke (stitch2, strap, dashes, 2, juce::AffineTransform::translation (0.0f, 7.0f));
        g.fillPath (stitch2);
    }
}

//==============================================================================
void ShredMaxxEditor::paint (juce::Graphics& g)
{
    if (background.isValid())
        g.drawImageAt (background, 0, 0);

    drawVu (g);
    drawLamps (g);

    // ratio LEDs
    const int sel = juce::jlimit (0, 7, (int) std::round (processor.apvts.getRawParameterValue ("ratio")->load()));
    for (int i = 0; i < 8; ++i)
        if (i == sel)
        {
            const auto c = ratioLed (i);
            g.setGradientFill (juce::ColourGradient (ledRed.withAlpha (0.55f), c.x, c.y, ledRed.withAlpha (0.0f), c.x + 14.0f, c.y, true));
            g.fillEllipse (juce::Rectangle<float> (28.0f, 28.0f).withCentre (c));
            g.setColour (ledRed);
            g.fillEllipse (juce::Rectangle<float> (7.0f, 7.0f).withCentre (c));
            g.setColour (juce::Colours::white.withAlpha (0.8f));
            g.fillEllipse (juce::Rectangle<float> (2.5f, 2.5f).withCentre (c.translated (-1.0f, -1.0f)));
        }

    for (int i = 0; i < 6; ++i)
        drawTape (g, knobs[(size_t) i], i);
}

void ShredMaxxEditor::drawTape (juce::Graphics& g, Knob& k, int index)
{
    // embossed label-maker tape
    const auto rc = tapeRect (k);
    const float rot = ((index * 37) % 7 - 3) * 0.012f;
    juce::Graphics::ScopedSaveState ss (g);
    g.addTransform (juce::AffineTransform::rotation (rot, rc.getCentreX(), rc.getCentreY()));
    g.setColour (juce::Colours::black.withAlpha (0.45f));
    g.fillRect (rc.translated (1.0f, 2.0f));
    const juce::Colour tape = index % 3 == 1 ? juce::Colour (0xff7a1410) : juce::Colour (0xff141414);
    g.setColour (tape);
    g.fillRect (rc);
    g.setColour (juce::Colours::white.withAlpha (0.08f));
    g.drawLine (rc.getX(), rc.getY() + 1.0f, rc.getRight(), rc.getY() + 1.0f, 1.0f);
    const auto text = k.slider.getTextFromValue (k.slider.getValue()).toUpperCase();
    g.setFont (AmpLookAndFeel::stencil (11.5f).withExtraKerningFactor (0.08f));
    g.setColour (juce::Colours::white.withAlpha (0.92f));
    g.drawText (text, rc, juce::Justification::centred, false);
}

void ShredMaxxEditor::drawVu (juce::Graphics& g)
{
    const juce::Point<float> pivot (vuFace.getCentreX(), vuFace.getBottom() + 34.0f);
    const float a = juce::jmap (std::pow (juce::jlimit (0.0f, 20.0f, needle) / 20.0f, 0.6f), 0.8f, -0.8f);
    juce::Graphics::ScopedSaveState ss (g);
    g.reduceClipRegion (vuFace.toNearestInt());
    const auto tip = pivot.getPointOnCircumference (148.0f, a);
    const auto base = pivot.getPointOnCircumference (40.0f, a);
    g.setColour (juce::Colours::black.withAlpha (0.25f));
    g.drawLine (juce::Line<float> (base.translated (3.0f, 3.0f), tip.translated (3.0f, 3.0f)), 1.6f);
    g.setColour (juce::Colour (0xff15110c));
    g.drawLine (juce::Line<float> (base, tip), 1.6f);
    // cracked glass glare
    g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.25f), vuFace.getX(), vuFace.getY(),
                                             juce::Colours::transparentWhite, vuFace.getX() + 120.0f, vuFace.getY() + 90.0f, false));
    g.fillRect (vuFace);
    g.setColour (juce::Colours::white.withAlpha (0.35f));
    juce::Path crack;
    crack.startNewSubPath (vuFace.getRight() - 4.0f, vuFace.getY() + 18.0f);
    crack.lineTo (vuFace.getRight() - 34.0f, vuFace.getY() + 44.0f);
    crack.lineTo (vuFace.getRight() - 46.0f, vuFace.getY() + 41.0f);
    crack.lineTo (vuFace.getRight() - 78.0f, vuFace.getY() + 70.0f);
    crack.startNewSubPath (vuFace.getRight() - 34.0f, vuFace.getY() + 44.0f);
    crack.lineTo (vuFace.getRight() - 30.0f, vuFace.getY() + 76.0f);
    g.strokePath (crack, juce::PathStrokeType (0.9f));
    // needle pivot cover
    g.setColour (juce::Colour (0xff2a2622));
    g.fillRect (vuFace.withTrimmedTop (vuFace.getHeight() - 16.0f));
}

void ShredMaxxEditor::drawLamps (juce::Graphics& g)
{
    auto lamp = [&] (int i, juce::Colour c, float level)
    {
        const juce::Point<float> p (lampArea.getX() + 16.0f, lampArea.getY() + 12.0f + (float) i * 44.0f);
        const float size = i == 3 ? 22.0f : 16.0f;
        if (level > 0.01f)
        {
            g.setGradientFill (juce::ColourGradient (c.withAlpha (0.6f * level), p.x, p.y, c.withAlpha (0.0f), p.x + size * 1.6f, p.y, true));
            g.fillEllipse (juce::Rectangle<float> (size * 3.2f, size * 3.2f).withCentre (p));
        }
        juce::ColourGradient lens (c.interpolatedWith (juce::Colours::black, 0.8f - 0.75f * level), p.x - size * 0.3f, p.y - size * 0.3f,
                                   c.interpolatedWith (juce::Colours::black, 0.92f - 0.5f * level), p.x + size * 0.5f, p.y + size * 0.5f, true);
        g.setGradientFill (lens);
        g.fillEllipse (juce::Rectangle<float> (size, size).withCentre (p));
        if (i == 3)   // jewel facets
        {
            g.setColour (juce::Colours::white.withAlpha (0.18f));
            for (int k = 0; k < 6; ++k)
                g.drawLine (juce::Line<float> (p, p.getPointOnCircumference (size * 0.5f, (float) k * 1.047f)), 0.8f);
        }
        g.setColour (juce::Colours::white.withAlpha (0.55f));
        g.fillEllipse (juce::Rectangle<float> (size * 0.25f, size * 0.2f).withCentre (p.translated (-size * 0.18f, -size * 0.2f)));
    };
    lamp (0, ledAmber, thd1 ? 1.0f : 0.0f);
    lamp (1, ledRed,   thd3 ? 1.0f : 0.0f);
    lamp (2, juce::Colour (0xffffc35a), opto);
    lamp (3, juce::Colour (0xffff2a12), 1.0f);
}

//==============================================================================
void ShredMaxxEditor::timerCallback()
{
    const double now = juce::Time::getMillisecondCounterHiRes();

    // VU ballistics (about 300 ms)
    const float gr = processor.meterGr.load();
    needle += (gr - needle) * 0.42f;

    const float thd = processor.meterThd.load();
    if (thd >= 0.01f) thd1Hold = now + 250.0;
    if (thd >= 0.03f) thd3Hold = now + 250.0;
    const bool n1 = now < thd1Hold, n3 = now < thd3Hold;

    const int sel = (int) std::round (processor.apvts.getRawParameterValue ("ratio")->load());
    const float optoTarget = sel == sm::kRatio10 ? 0.25f + 0.75f * processor.meterOpto.load() : 0.0f;
    opto += (optoTarget - opto) * 0.2f;

    repaint (vuFace.expanded (2.0f).toNearestInt());
    if (n1 != thd1 || n3 != thd3 || std::abs (optoTarget - opto) > 0.002f)
    {
        thd1 = n1; thd3 = n3;
        repaint (lampArea.expanded (30.0f).toNearestInt());
    }
    if (sel != lastRatio)
    {
        lastRatio = sel;
        repaint (juce::Rectangle<float> (220.0f, 220.0f).withCentre (ratio.centre).toNearestInt());
        repaint (lampArea.expanded (30.0f).toNearestInt());
    }
    for (auto& k : knobs)
    {
        const auto t = k.slider.getTextFromValue (k.slider.getValue());
        if (t != k.lastText) { k.lastText = t; repaint (tapeRect (k).expanded (6.0f).toNearestInt()); }
    }
}
