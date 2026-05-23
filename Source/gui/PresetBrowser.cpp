#include "PresetBrowser.h"
#include "../PluginProcessor.h"

static const juce::Colour kBg      { 0xff1e1e28 };
static const juce::Colour kCyan    { 0xff00d9ff };
static const juce::Colour kFactory { 0xffaaaacc };
static const juce::Colour kUser    { 0xffeeeeff };
static const juce::Colour kSelBg   { 0xff2a2a40 };
static const juce::Colour kBorder  { 0xff333350 };

// ---- ListBoxModel -----------------------------------------------------------

int PresetBrowser::Model::getNumRows()
{
    return (int)owner->presetManager.getPresets().size();
}

void PresetBrowser::Model::paintListBoxItem(int row, juce::Graphics& g,
                                             int w, int h, bool sel)
{
    const auto& presets = owner->presetManager.getPresets();
    if (row < 0 || row >= (int)presets.size()) return;

    const auto& info = presets[(size_t)row];
    const bool isCurrent = (info.name == owner->presetManager.getCurrentName());

    if (sel)
        g.fillAll(kSelBg);
    else if (row % 2 == 0)
        g.fillAll(kBg.brighter(0.05f));
    else
        g.fillAll(kBg);

    g.setFont(juce::FontOptions(isCurrent ? 11.5f : 11.0f)
                  .withStyle(isCurrent ? "Bold" : "Regular"));
    g.setColour(info.isFactory ? kFactory : kUser);
    g.drawText(info.name, 8, 0, w - 16, h, juce::Justification::centredLeft);

    if (isCurrent)
    {
        g.setColour(kCyan);
        g.fillRect(0, 0, 3, h);
    }
}

void PresetBrowser::Model::listBoxItemClicked(int row, const juce::MouseEvent&)
{
    owner->selectedRow = row;
    owner->updateButtons();
}

void PresetBrowser::Model::listBoxItemDoubleClicked(int row, const juce::MouseEvent&)
{
    owner->loadRow(row);
}

// ---- PresetBrowser ----------------------------------------------------------

PresetBrowser::PresetBrowser(DrawwaveVocoderProcessor& p)
    : proc(p), presetManager(p),
      btnSave("Save"), btnDelete("Delete"), btnCapA("Cap A"), btnRecA("Rec A"),
      btnCapB("Cap B"), btnRecB("Rec B")
{
    model.owner = this;

    listBox.setModel(&model);
    listBox.setColour(juce::ListBox::backgroundColourId, kBg);
    listBox.setColour(juce::ListBox::outlineColourId,    kBorder);
    listBox.setOutlineThickness(1);
    listBox.setRowHeight(20);
    addAndMakeVisible(listBox);

    auto styleBtn = [&](juce::TextButton& b) {
        b.setColour(juce::TextButton::buttonColourId,   juce::Colour(0xff252535));
        b.setColour(juce::TextButton::buttonOnColourId, kCyan.darker(0.4f));
        b.setColour(juce::TextButton::textColourOffId,  kFactory);
        b.setColour(juce::TextButton::textColourOnId,   kCyan);
        addAndMakeVisible(b);
    };

    styleBtn(btnSave);
    styleBtn(btnDelete);
    styleBtn(btnCapA);
    styleBtn(btnRecA);
    styleBtn(btnCapB);
    styleBtn(btnRecB);

    btnDelete.setColour(juce::TextButton::textColourOffId, juce::Colour(0xffff6666));

    btnSave.onClick   = [this] { onSave(); };
    btnDelete.onClick = [this] { onDelete(); };
    btnCapA.onClick   = [this] { onCapA(); };
    btnRecA.onClick   = [this] { onRecA(); };
    btnCapB.onClick   = [this] { onCapB(); };
    btnRecB.onClick   = [this] { onRecB(); };

    updateButtons();
}

PresetBrowser::~PresetBrowser()
{
    listBox.setModel(nullptr);
}

void PresetBrowser::refreshList()
{
    presetManager.refreshUserPresets();
    listBox.updateContent();
    listBox.repaint();
    updateButtons();
}

void PresetBrowser::paint(juce::Graphics& g)
{
    g.fillAll(kBg);
    g.setColour(kBorder);
    g.drawRect(getLocalBounds(), 1);

    g.setColour(kCyan.withAlpha(0.9f));
    g.setFont(juce::FontOptions(10.5f).withStyle("Bold"));
    g.drawText("PRESETS", 8, 2, getWidth() - 16, 16,
               juce::Justification::centredLeft);

    g.setColour(kBorder);
    g.drawHorizontalLine(18, 1.0f, (float)(getWidth() - 1));
}

void PresetBrowser::resized()
{
    constexpr int btnH = 24, gap = 4, titleH = 20;
    const int w = getWidth();
    const int h = getHeight();

    const int listBottom = h - btnH - gap * 2;
    listBox.setBounds(1, titleH, w - 2, listBottom - titleH);

    const int btnY  = listBottom + gap;
    const int btnW  = (w - gap * 7) / 6;

    int x = gap;
    for (auto* btn : { &btnSave, &btnDelete, &btnCapA, &btnRecA, &btnCapB, &btnRecB })
    {
        btn->setBounds(x, btnY, btnW, btnH);
        x += btnW + gap;
    }
}

// ---- Button actions ---------------------------------------------------------

void PresetBrowser::loadRow(int row)
{
    const auto& presets = presetManager.getPresets();
    if (row < 0 || row >= (int)presets.size()) return;
    presetManager.loadPreset(presets[(size_t)row]);
    listBox.repaint();
    updateButtons();
    if (onPresetLoaded) onPresetLoaded();
}

void PresetBrowser::onSave()
{
    auto* aw = new juce::AlertWindow("Save Preset", "Enter preset name:",
                                      juce::MessageBoxIconType::NoIcon);
    aw->addTextEditor("name", presetManager.getCurrentName());
    aw->addButton("Save",   1);
    aw->addButton("Cancel", 0);

    aw->enterModalState(true,
        juce::ModalCallbackFunction::create([this, aw](int result) {
            if (result == 1)
            {
                const juce::String name = aw->getTextEditorContents("name");
                if (name.isNotEmpty())
                {
                    presetManager.savePreset(name);
                    listBox.updateContent();
                    listBox.repaint();
                    updateButtons();
                }
            }
        }), true);
}

void PresetBrowser::onDelete()
{
    const auto& presets = presetManager.getPresets();
    if (selectedRow < 0 || selectedRow >= (int)presets.size()) return;
    const auto& info = presets[(size_t)selectedRow];
    if (info.isFactory) return;

    juce::AlertWindow::showAsync(
        juce::MessageBoxOptions()
            .withIconType(juce::MessageBoxIconType::WarningIcon)
            .withTitle("Delete Preset")
            .withMessage("Delete \"" + info.name + "\"?")
            .withButton("Delete")
            .withButton("Cancel"),
        [this](int result) {
            if (result != 1) return;
            const auto& p = presetManager.getPresets();
            if (selectedRow < 0 || selectedRow >= (int)p.size()) return;
            presetManager.deleteUserPreset(p[(size_t)selectedRow]);
            selectedRow = -1;
            listBox.updateContent();
            listBox.repaint();
            updateButtons();
        });
}

void PresetBrowser::onCapA() { presetManager.captureA(); updateButtons(); }
void PresetBrowser::onCapB() { presetManager.captureB(); updateButtons(); }

void PresetBrowser::onRecA()
{
    presetManager.recallA();
    listBox.repaint();
    if (onPresetLoaded) onPresetLoaded();
}

void PresetBrowser::onRecB()
{
    presetManager.recallB();
    listBox.repaint();
    if (onPresetLoaded) onPresetLoaded();
}

void PresetBrowser::updateButtons()
{
    const auto& presets = presetManager.getPresets();
    const bool validUser = selectedRow >= 0
                           && selectedRow < (int)presets.size()
                           && !presets[(size_t)selectedRow].isFactory;
    btnDelete.setEnabled(validUser);
    btnRecA.setEnabled(presetManager.hasSlotA());
    btnRecB.setEnabled(presetManager.hasSlotB());
}
