#include "MainComponent.h"

MainComponent::MainComponent()
	: m_audioSetup(deviceManager)
	, m_maxSamplesPerBlock(0)
	, showOnlyTestInterface(false)
{
	// add and make visible the stimulus player object
	addAndMakeVisible(m_stimulusPlayer);
	m_stimulusPlayer.addChangeListener(this);

	// setup loudspeaker renderer
	m_loudspeakerRenderer.addChangeListener(this);

	// setup binaural renderer, pass the osc transceiver
	oscTxRx.addListener(&m_binauralRenderer);
	m_binauralRenderer.addListener(&m_rendererView);
	m_binauralRenderer.addChangeListener(this);

	m_rendererView.init(&m_loudspeakerRenderer, &m_binauralRenderer);
	m_rendererView.addChangeListener(this);
	addAndMakeVisible(m_rendererView);

	// initialize headphone compensation
	addAndMakeVisible(m_headphoneCompensation);

	// set size of the main app window
	setSize(1400, 800);

	// add logo
	Image logo = ImageFileFormat::loadFrom(BinaryData::logo_180px_png, BinaryData::logo_180px_pngSize);

	if (logo.isValid())
		imageComponent.setImage(logo);

	addAndMakeVisible(&imageComponent);

	// set number of output channels to 64 (rendering using binaural playback or loudspeaker rig)
	setAudioChannels(0, 64);

	// OSC labels
	clientTxIpLabel.setEditable(false, true, false);
	clientTxPortLabel.setEditable(false, true, false);
	clientRxPortLabel.setEditable(false, true, false);
	clientTxIpLabel.setText("127.0.0.1", dontSendNotification);
	clientTxPortLabel.setText("6000", dontSendNotification);
	clientRxPortLabel.setText("9000", dontSendNotification);
	clientTxIpLabel.setColour(Label::outlineColourId, Colours::black);
	clientTxPortLabel.setColour(Label::outlineColourId, Colours::black);
	clientRxPortLabel.setColour(Label::outlineColourId, Colours::black);
	clientTxIpLabel.setJustificationType(Justification::centred);
	clientTxPortLabel.setJustificationType(Justification::centred);
	clientRxPortLabel.setJustificationType(Justification::centred);
	addAndMakeVisible(clientTxIpLabel);
	addAndMakeVisible(clientTxPortLabel);
	addAndMakeVisible(clientRxPortLabel);

	// load settings
	initSettings();
	if (Settings.getUserSettings()->getBoolValue("loadSettingsFile"))
	{
		loadSettings();
	}

	connectOscButton.setButtonText("Connect OSC");
	connectOscButton.addListener(this);
	connectOscButton.triggerClick(); // connect on startup
	addAndMakeVisible(&connectOscButton);

	openAudioDeviceManager.setButtonText("Audio device setup");
	openAudioDeviceManager.addListener(this);
	addAndMakeVisible(&openAudioDeviceManager);

	// load settings file if available
	String filePath = File::getSpecialLocation(File::SpecialLocationType::currentApplicationFile).getParentDirectory().getFullPathName();
	audioSettingsFile = File(filePath + "/" + "SALTEAudioSettings.conf");

	if (audioSettingsFile.existsAsFile())
		loadAudioSettings();

	m_testSessionForm.init(&m_testSession);
	m_testSessionForm.addListener(this);
	addAndMakeVisible(m_testSessionForm);

	m_mixedMethods.init(&oscTxRx, &m_testSession, &m_stimulusPlayer, &m_binauralRenderer);
	m_mixedMethods.addListener(this);
	m_mixedMethods.addChangeListener(this);
	addChildComponent(m_mixedMethods);

	// localisation component temporarily on top of the session form and mixed methods
	m_localisationComponent.init(&oscTxRx, &m_stimulusPlayer, &m_binauralRenderer, &m_loudspeakerRenderer);
	m_localisationComponent.addChangeListener(this);
	addChildComponent(m_localisationComponent);

	// log window
	logWindow.setMultiLine(true);
	logWindow.setReadOnly(true);
	logWindow.setCaretVisible(false);
	logWindow.setScrollbarsShown(true);
	addAndMakeVisible(logWindow);

	showMixedComp.setButtonText("Mixed Methods");
	showMixedComp.addListener(this);
	addAndMakeVisible(showMixedComp);

	showLocComp.setButtonText("Localisation");
	showLocComp.addListener(this);
	addAndMakeVisible(showLocComp);

	showTestInterface.setButtonText("Show test interface");
	showTestInterface.setClickingTogglesState(true);
	showTestInterface.addListener(this);
	addAndMakeVisible(showTestInterface);

	openRouter.setButtonText("Output Routing");
	openRouter.addListener(this);
	addAndMakeVisible(&openRouter);

	LookAndFeel& lookAndFeel = getLookAndFeel();
	Colour bckgnd = Colour(25, 50, 77);
	lookAndFeel.setColour(ResizableWindow::backgroundColourId, bckgnd);
	lookAndFeel.setColour(ComboBox::backgroundColourId, bckgnd.darker());
	lookAndFeel.setColour(TextEditor::backgroundColourId, bckgnd.darker());
	lookAndFeel.setColour(Slider::backgroundColourId, bckgnd.darker());
	lookAndFeel.setColour(TextButton::buttonColourId, Colour(12, 25, 39));
	lookAndFeel.setColour(Slider::trackColourId, Colour(12, 25, 39));
}

MainComponent::~MainComponent()
{
	saveAudioSettings();
	saveSettings();
	oscTxRx.disconnectTxRx();
	m_rendererView.deinit();
	shutdownAudio();
}

//==============================================================================
void MainComponent::prepareToPlay(int samplesPerBlockExpected, double sampleRate)
{
	// prepare stimulus player object
	m_stimulusPlayer.prepareToPlay(samplesPerBlockExpected, sampleRate);
	m_loudspeakerRenderer.prepareToPlay(samplesPerBlockExpected, sampleRate);
	m_binauralRenderer.prepareToPlay(samplesPerBlockExpected, sampleRate);
	m_headphoneCompensation.prepareToPlay(samplesPerBlockExpected, sampleRate);
	m_lspkRouter.prepareToPlay(samplesPerBlockExpected, sampleRate);

	if (samplesPerBlockExpected != m_maxSamplesPerBlock)
	{
		m_maxSamplesPerBlock = samplesPerBlockExpected;
		processBuffer.setSize(64, samplesPerBlockExpected);
	}
}

void MainComponent::getNextAudioBlock(const AudioSourceChannelInfo& bufferToFill)
{
	AudioSourceChannelInfo newinfo(processBuffer);

	// pass the buffer into the stimulus player to be filled with required audio
	m_stimulusPlayer.getNextAudioBlock(newinfo);

	// pass the buffer to the loudspeaker renderer to replace ambisonic signals with loudspeaker feeds
	m_loudspeakerRenderer.processBlock(*newinfo.buffer);

	// pass the buffer to the binaural rendering object to replace ambisonic signals with binaural audio
	m_binauralRenderer.processBlock(*newinfo.buffer);

	m_headphoneCompensation.processBlock(*newinfo.buffer);

	m_lspkRouter.processBlock(*newinfo.buffer);

	AudioBuffer<float>* sourceBuffer = bufferToFill.buffer;

	for (int c = 0; c < sourceBuffer->getNumChannels(); ++c)
	{
		sourceBuffer->copyFrom(c, 0, *newinfo.buffer, c, 0, sourceBuffer->getNumSamples());
	}
}

void MainComponent::releaseResources()
{
	// relese resources taken by stimulus player object
	m_stimulusPlayer.releaseResources();
	m_loudspeakerRenderer.releaseResources();
	m_binauralRenderer.releaseResources();
	m_headphoneCompensation.releaseResources();
	m_lspkRouter.releaseResources();
}

//==============================================================================
void MainComponent::paint(Graphics& g)
{
	g.fillAll(getLookAndFeel().findColour(ResizableWindow::backgroundColourId));

	// RECTANGULAR OUTLINE
	g.setColour(Colours::black);
	g.drawRect(getLocalBounds(), 1);

	g.setColour(getLookAndFeel().findColour(Label::textColourId));
	g.drawMultiLineText(String("Build date and time:\n" + String(__DATE__) + " " + String(__TIME__)), 20, 140, 150, Justification::left);

	g.setFont(18.0f);
	g.drawMultiLineText("Spatial\nAudio\nListening\nTest\nEnvironment", 120, 33, 120, Justification::left, 1.2f);


	if (showOnlyTestInterface)
	{

	}
	else
	{
		juce::Rectangle<int> oscRect(250, 10, 400, 150);        // osc status / vr interface status

		g.setColour(Colours::black);
		g.drawRect(oscRect, 1);


		// OSC WINDOW
		g.setColour(getLookAndFeel().findColour(Label::textColourId));
		g.setFont(14.0f);
		g.drawText("IP", 310, 10, 50, 25, Justification::centredLeft, true);
		g.drawText("Send to", 435, 10, 60, 25, Justification::centredLeft, true);
		g.drawText("Receive at", 490, 10, 60, 25, Justification::centredLeft, true);
		g.drawText("Client", 260, 35, 50, 25, Justification::centredLeft, true);
	}
}

void MainComponent::resized()
{
	imageComponent.setBounds(20, 20, 90, 90);

	m_testSessionForm.setBounds(10, 170, 640, 480);
	m_mixedMethods.setBounds(10, 170, 640, 480);
	m_localisationComponent.setBounds(10, 170, 640, 480);

	if (showOnlyTestInterface)
	{
		m_stimulusPlayer.setBounds(660, 170, 730, 330);
		showTestInterface.setBounds(getWidth() - 20, getHeight() - 20, 10, 10);
	}
	else
	{
		m_audioSetup.setCentrePosition(getWidth() / 2, getHeight() / 2);
		m_stimulusPlayer.setBounds(660, 10, 730, 330);
		m_rendererView.setBounds(660, 350, 730, 245);
		m_headphoneCompensation.setBounds(660+365, 605, 365, 185);
		m_lspkRouter.setCentrePosition(getWidth() / 2, getHeight() / 2);

		connectOscButton.setBounds(560, 20, 80, 40);
		openAudioDeviceManager.setBounds(310, 70, 240, 25);

		clientTxIpLabel.setBounds(310, 35, 120, 25);
		clientTxPortLabel.setBounds(435, 35, 55, 25);
		clientRxPortLabel.setBounds(495, 35, 55, 25);

		logWindow.setBounds(10, 660, 640, 130);

		showMixedComp.setBounds(310, 105, 115, 25);
		showLocComp.setBounds(435, 105, 115, 25);
		showTestInterface.setBounds(560, 70, 80, 60);

		openRouter.setBounds(660, 605, 240, 25);
	}
}

void MainComponent::buttonClicked(Button* buttonThatWasClicked)
{
	if (buttonThatWasClicked == &openAudioDeviceManager)
	{
		addAndMakeVisible(m_audioSetup);
		m_audioSetup.m_shouldBeVisible = true;
	}
	else if (buttonThatWasClicked == &connectOscButton)
	{
		if (!oscTxRx.isConnected())
		{
			// OSC sender and receiver connect
			String clientIp = clientTxIpLabel.getText();
			int clientSendToPort = clientTxPortLabel.getText().getIntValue();
			int clientReceiveAtPort = clientRxPortLabel.getText().getIntValue();
			oscTxRx.connectTxRx(clientIp, clientSendToPort, clientReceiveAtPort);
		}
		else
		{
			oscTxRx.disconnectTxRx();
		}

		if (oscTxRx.isConnected())
		{
			connectOscButton.setColour(TextButton::buttonColourId, Colours::green);
			connectOscButton.setButtonText("OSC connected");
		}
		else
		{
			connectOscButton.setColour(TextButton::buttonColourId, Component::findColour(TextButton::buttonColourId));
			connectOscButton.setButtonText("Connect OSC");
		}
	}
	else if (buttonThatWasClicked == &showMixedComp)
	{
		m_testSessionForm.setVisible(true);
		m_localisationComponent.setVisible(false);
	}
	else if (buttonThatWasClicked == &showLocComp)
	{
		m_testSessionForm.setVisible(false);
		m_localisationComponent.setVisible(true);
	}
	else if (buttonThatWasClicked == &showTestInterface)
	{
		bool show = showTestInterface.getToggleState();
		showOnlyTestInterface = show;

		m_stimulusPlayer.setShowTest(show);
		if (m_audioSetup.m_shouldBeVisible) m_audioSetup.setVisible(!show);
		m_rendererView.setVisible(!show);
		m_headphoneCompensation.setVisible(!show);
		openAudioDeviceManager.setVisible(!show);
		connectOscButton.setVisible(!show);
		clientTxIpLabel.setVisible(!show);
		clientTxPortLabel.setVisible(!show);
		clientRxPortLabel.setVisible(!show);
		logWindow.setVisible(!show);
		resized();
	}
	else if (buttonThatWasClicked == &openRouter)
	{
		addAndMakeVisible(m_lspkRouter);
		m_lspkRouter.m_shouldBeVisible = true;
	}

	repaint();
}

void MainComponent::formCompleted()
{
	m_mixedMethods.loadTestSession();
	m_mixedMethods.setVisible(true);
	m_rendererView.setTestInProgress(true);
}

void MainComponent::testCompleted()
{
	m_testSessionForm.reset();
	m_testSessionForm.setVisible(true);
	m_rendererView.setTestInProgress(false);
}

void MainComponent::loadAudioSettings()
{
	XmlDocument asxmldoc(audioSettingsFile);
	std::unique_ptr<XmlElement> audioDeviceSettings(asxmldoc.getDocumentElement());
	deviceManager.initialise(0, 64, audioDeviceSettings.get(), true);
}

void MainComponent::saveAudioSettings()
{
	std::unique_ptr<XmlElement> audioDeviceSettings(deviceManager.createStateXml());
	if (audioDeviceSettings.get())
	{
		audioDeviceSettings->writeTo(audioSettingsFile);
	}
}

// LOG WINDOW
void MainComponent::changeListenerCallback(ChangeBroadcaster* source)
{
	String timeStamp = Time::getCurrentTime().formatted("%H:%M:%S") + ": ";

	if (source == &m_stimulusPlayer)
	{
		if (m_stimulusPlayer.currentMessage != "")
		{
			logWindowMessage += timeStamp + m_stimulusPlayer.currentMessage;
			m_stimulusPlayer.currentMessage.clear();
		}
	}
	else if (source == &m_loudspeakerRenderer)
	{
		if (m_loudspeakerRenderer.m_currentLogMessage != "")
		{
			logWindowMessage += timeStamp + m_loudspeakerRenderer.m_currentLogMessage;
			m_loudspeakerRenderer.m_currentLogMessage.clear();
		}
	}
	else if (source == &m_binauralRenderer)
	{
		if (m_binauralRenderer.m_currentLogMessage != "")
		{
			logWindowMessage += timeStamp + m_binauralRenderer.m_currentLogMessage;
			m_binauralRenderer.m_currentLogMessage.clear();
		}
	}
	else if (source == &m_rendererView)
	{
		if (m_rendererView.m_currentLogMessage != "")
		{
			logWindowMessage += timeStamp + m_rendererView.m_currentLogMessage;
			m_rendererView.m_currentLogMessage.clear();
		}
	}
	else if (source == &m_mixedMethods)
	{
		if (m_mixedMethods.currentMessage != "")
		{
			logWindowMessage += timeStamp + m_mixedMethods.currentMessage;
			m_mixedMethods.currentMessage.clear();
		}
	}
	else if (source == &m_localisationComponent)
	{
		if (m_localisationComponent.currentMessage != "")
		{
			logWindowMessage += timeStamp + m_localisationComponent.currentMessage;
			m_localisationComponent.currentMessage.clear();
		}
	}

	logWindow.setText(logWindowMessage);
	logWindow.moveCaretToEnd();
}

void MainComponent::initSettings()
{
	PropertiesFile::Options options;
	options.applicationName = "SALTESettings";
	options.filenameSuffix = ".conf";
	options.osxLibrarySubFolder = "Application Support";
	options.folderName = File::getSpecialLocation(File::SpecialLocationType::currentApplicationFile).getParentDirectory().getFullPathName();
	options.storageFormat = PropertiesFile::storeAsXML;
	Settings.setStorageParameters(options);
}

void MainComponent::loadSettings()
{
	clientTxIpLabel.setText(Settings.getUserSettings()->getValue("clientTxIp"), dontSendNotification);
	clientTxPortLabel.setText(Settings.getUserSettings()->getValue("clientTxPort"), dontSendNotification);
	clientRxPortLabel.setText(Settings.getUserSettings()->getValue("clientRxPort"), dontSendNotification);

	m_lspkRouter.loadRoutingFile(Settings.getUserSettings()->getValue("routingFile"));
	m_lspkRouter.loadCalibrationFile(Settings.getUserSettings()->getValue("calibrationFile"));
}

void MainComponent::saveSettings()
{
	Settings.getUserSettings()->setValue("clientTxIp", clientTxIpLabel.getText());
	Settings.getUserSettings()->setValue("clientTxPort", clientTxPortLabel.getText());
	Settings.getUserSettings()->setValue("clientRxPort", clientRxPortLabel.getText());

	Settings.getUserSettings()->setValue("routingFile", m_lspkRouter.getRoutingFilePath());
	Settings.getUserSettings()->setValue("calibrationFile", m_lspkRouter.getCalibrationFilePath());

	Settings.getUserSettings()->setValue("loadSettingsFile", true);
}
