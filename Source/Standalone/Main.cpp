#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_audio_plugin_client/Standalone/juce_StandaloneFilterWindow.h>

// ---- About dialog -----------------------------------------------------------

class AboutComponent final : public juce::Component
{
public:
    AboutComponent()
    {
        setSize (420, 210);
    }

    void paint (juce::Graphics& g) override
    {
        const juce::Colour kBg     { 0xff1e1e28 };
        const juce::Colour kCyan   { 0xff00d9ff };
        const juce::Colour kLight  { 0xffccccdd };
        const juce::Colour kDim    { 0xff888899 };
        const juce::Colour kBorder { 0xff333350 };

        g.fillAll (kBg);

        int y = 20;
        g.setColour (kCyan);
        g.setFont (juce::FontOptions (20.0f).withStyle ("Bold"));
        g.drawText ("Drawwave Vocoder", 20, y, getWidth() - 40, 26,
                    juce::Justification::centredLeft);

        y += 28;
        g.setColour (kDim);
        g.setFont (juce::FontOptions (11.0f));
        g.drawText (juce::String ("v") + JucePlugin_VersionString
                    + "   © 2025 TIM And Friends",
                    20, y, getWidth() - 40, 16, juce::Justification::centredLeft);

        y += 24;
        g.setColour (kBorder);
        g.drawHorizontalLine (y, 20.0f, (float)(getWidth() - 20));
        y += 12;

        auto drawLib = [&](const juce::String& name, const juce::String& detail)
        {
            g.setColour (kLight);
            g.setFont (juce::FontOptions (11.5f).withStyle ("Bold"));
            g.drawText (name, 20, y, getWidth() - 40, 16,
                        juce::Justification::centredLeft);
            y += 17;
            g.setColour (kDim);
            g.setFont (juce::FontOptions (10.5f));
            g.drawText (detail, 28, y, getWidth() - 48, 15,
                        juce::Justification::centredLeft);
            y += 22;
        };

        drawLib ("JUCE",
                 "© Raw Material Software Limited  —  JUCE Personal Licence  —  juce.com");
        drawLib ("nlohmann/json",
                 "© 2013-2022 Niels Lohmann  —  MIT License");
    }
};

static void showAboutDialog()
{
    auto* content = new AboutComponent();
    juce::DialogWindow::LaunchOptions opts;
    opts.content.setOwned (content);
    opts.dialogTitle                  = "About Drawwave Vocoder";
    opts.dialogBackgroundColour       = juce::Colour (0xff1e1e28);
    opts.escapeKeyTriggersCloseButton = true;
    opts.useNativeTitleBar            = false;
    opts.resizable                    = false;
    opts.launchAsync();
}

// ---- Custom window: adds "About" to the Options popup -----------------------

class DrawwaveVocoderWindow final : public juce::StandaloneFilterWindow
{
public:
    DrawwaveVocoderWindow (const juce::String& name, juce::Colour bg,
                           std::unique_ptr<juce::StandalonePluginHolder> holder)
        : juce::StandaloneFilterWindow (name, bg, std::move (holder))
    {}

    // Override the Options button handler to inject "About..."
    void buttonClicked (juce::Button*) override
    {
        juce::PopupMenu m;
        m.addItem (1, TRANS ("Audio/MIDI Settings..."));
        m.addSeparator();
        m.addItem (2, TRANS ("Save current state..."));
        m.addItem (3, TRANS ("Load a saved state..."));
        m.addSeparator();
        m.addItem (4, TRANS ("Reset to default state"));
        m.addSeparator();
        m.addItem (5, "About...");

        m.showMenuAsync (juce::PopupMenu::Options(), [this] (int result)
        {
            switch (result)
            {
                case 1:  getPluginHolder()->showAudioSettingsDialog(); break;
                case 2:  getPluginHolder()->askUserToSaveState();      break;
                case 3:  getPluginHolder()->askUserToLoadState();      break;
                case 4:  resetToDefaultState();                        break;
                case 5:  showAboutDialog();                            break;
                default: break;
            }
        });
    }
};

// ---- macOS native menu bar --------------------------------------------------

#if JUCE_MAC
class AppMenuBarModel final : public juce::MenuBarModel
{
public:
    juce::StringArray getMenuBarNames() override              { return {}; }
    juce::PopupMenu   getMenuForIndex (int, const juce::String&) override { return {}; }
    void menuItemSelected (int id, int) override
    {
        if (id == 1) showAboutDialog();
    }
};
#endif

// ---- Application ------------------------------------------------------------

class DrawwaveVocoderApp final : public juce::JUCEApplication
{
public:
    DrawwaveVocoderApp()
    {
        juce::PropertiesFile::Options options;
        options.applicationName     = juce::CharPointer_UTF8 (JucePlugin_Name);
        options.filenameSuffix      = ".settings";
        options.osxLibrarySubFolder = "Application Support";
        options.folderName          = "";
        appProperties.setStorageParameters (options);
    }

    const juce::String getApplicationName() override    { return juce::CharPointer_UTF8 (JucePlugin_Name); }
    const juce::String getApplicationVersion() override { return JucePlugin_VersionString; }
    bool moreThanOneInstanceAllowed() override          { return true; }
    void anotherInstanceStarted (const juce::String&) override {}

    void initialise (const juce::String&) override
    {
        const juce::Array<juce::StandalonePluginHolder::PluginInOuts> channelConfig;
        auto holder = std::make_unique<juce::StandalonePluginHolder> (
            appProperties.getUserSettings(), false, juce::String{}, nullptr, channelConfig, false);

        holder->getMuteInputValue().setValue (false);
        holder->processorHasPotentialFeedbackLoop = false;

        mainWindow.reset (new DrawwaveVocoderWindow (
            getApplicationName(),
            juce::LookAndFeel::getDefaultLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId),
            std::move (holder)));

        mainWindow->setVisible (true);

#if JUCE_MAC
        menuBarModel = std::make_unique<AppMenuBarModel>();
        juce::PopupMenu appleItems;
        appleItems.addItem (1, "About " + getApplicationName() + "...");
        juce::MenuBarModel::setMacMainMenu (menuBarModel.get(), &appleItems);
#endif

        juce::Timer::callAfterDelay (300, [this]() { loadSessionState(); });
    }

    void shutdown() override
    {
#if JUCE_MAC
        juce::MenuBarModel::setMacMainMenu (nullptr);
        menuBarModel.reset();
#endif
        saveSessionState();
        mainWindow = nullptr;
        appProperties.saveIfNeeded();
    }

    void systemRequestedQuit() override
    {
        if (juce::ModalComponentManager::getInstance()->cancelAllModalComponents())
        {
            juce::Timer::callAfterDelay (100, []() {
                if (auto* app = juce::JUCEApplicationBase::getInstance())
                    app->systemRequestedQuit();
            });
        }
        else
        {
            quit();
        }
    }

private:
    juce::ApplicationProperties appProperties;
    std::unique_ptr<DrawwaveVocoderWindow> mainWindow;

#if JUCE_MAC
    std::unique_ptr<AppMenuBarModel> menuBarModel;
#endif

    static juce::File getSessionFile()
    {
        return juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
                   .getChildFile ("Drawwave Vocoder.session");
    }

    void saveSessionState()
    {
        if (mainWindow == nullptr) return;
        auto* holder = mainWindow->getPluginHolder();
        if (holder == nullptr || holder->processor == nullptr) return;

        juce::MemoryBlock data;
        holder->processor->getStateInformation (data);
        if (data.getSize() > 0)
            getSessionFile().replaceWithData (data.getData(), data.getSize());
    }

    void loadSessionState()
    {
        if (mainWindow == nullptr) return;
        auto* holder = mainWindow->getPluginHolder();
        if (holder == nullptr || holder->processor == nullptr) return;

        auto file = getSessionFile();
        if (!file.existsAsFile()) return;

        juce::MemoryBlock data;
        if (file.loadFileAsData (data) && data.getSize() > 0)
            holder->processor->setStateInformation (data.getData(), (int) data.getSize());
    }
};

START_JUCE_APPLICATION (DrawwaveVocoderApp)
