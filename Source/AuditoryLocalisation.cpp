#include "AuditoryLocalisation.h"
#include <random>

AuditoryLocalisation::AuditoryLocalisation()
	: m_oscTxRx(nullptr)
	, m_player(nullptr)
	, m_renderer(nullptr)
	, m_lsRenderer(nullptr)
{
	formatManager.registerBasicFormats();

	m_chooseStimuliFolder.setButtonText("Select Stimuli Folder");
	m_chooseStimuliFolder.addListener(this);
	addAndMakeVisible(m_chooseStimuliFolder);

	m_personalizedHRTF.setButtonText("Choose Personalized HRTF");
	m_personalizedHRTF.addListener(this);
	addAndMakeVisible(m_personalizedHRTF);

	m_genericHRTF.setButtonText("Choose Generic HRTF");
	m_genericHRTF.addListener(this);
	addAndMakeVisible(m_genericHRTF);

	m_demo.setButtonText("Currently in demo mode");
	m_demo.addListener(this);
	m_demo.setToggleState(true, dontSendNotification);
	addAndMakeVisible(m_demo);

	m_soundVis.setButtonText("Sound source visible");
	m_soundVis.addListener(this);
	m_soundVis.setToggleState(true, dontSendNotification);
	addAndMakeVisible(m_soundVis);

	m_startTest.setButtonText("Start Test");
	m_startTest.setToggleState(false, dontSendNotification);
	m_startTest.addListener(this);
	addAndMakeVisible(m_startTest);

	m_prevTrial.setButtonText("Previous Trial");
	m_prevTrial.addListener(this);
	addAndMakeVisible(m_prevTrial);

	m_nextTrial.setButtonText("Next Trial");
	m_nextTrial.addListener(this);
	addAndMakeVisible(m_nextTrial);

	/*
	m_createRndSubjectIDButton.setButtonText("Random Subject ID");
	m_createRndSubjectIDButton.addListener(this);
	addAndMakeVisible(m_createRndSubjectIDButton);
	*/

	m_labelNumTrials.setText("Number of trials:", NotificationType::dontSendNotification);
	addAndMakeVisible(m_labelNumTrials);
	addAndMakeVisible(m_editNumTrials);

	// osc logging
	startTimerHz(60);

	m_saveLogButton.setButtonText("Save Log");
	m_saveLogButton.addListener(this);
	m_saveLogButton.setEnabled(false);
	//addAndMakeVisible(m_saveLogButton);

	addAndMakeVisible(messageCounter);


}

AuditoryLocalisation::~AuditoryLocalisation()
{
	saveSettings();
}

void AuditoryLocalisation::init(OscTransceiver* oscTxRx, StimulusPlayer* player, BinauralRenderer* renderer, LoudspeakerRenderer* lsRenderer)
{
	m_lsRenderer = lsRenderer;
	m_renderer = renderer;
	m_player = player;
	m_player->addChangeListener(this);
	m_oscTxRx = oscTxRx;
	//m_oscTxRx->addListener(this);

	// load settings
	initSettings();

	if (TestSessionFormSettings.getUserSettings()->getBoolValue("loadSettingsFile"))
	{
		loadSettings();
	}
}

void AuditoryLocalisation::paint(Graphics& g)
{
	g.fillAll(getLookAndFeel().findColour(ResizableWindow::backgroundColourId));

	g.setColour(Colours::black);
	g.drawRect(getLocalBounds(), 1);

	g.setColour(Colours::white);
	g.drawText(audioFilesDir.getFullPathName(), 180, 20, 440, 25, Justification::centredLeft);
	g.drawText(genericHRTF_file.getFullPathName(), 180, 50, 440, 25, Justification::centredLeft);
	g.drawText(personalizedHRTF_file.getFullPathName(), 180, 80, 440, 25, Justification::centredLeft);
	g.drawText("Number of trials: " + String(audioFilesArray.size()), 180, 200, 440, 25, Justification::centredLeft);
	
	if(audioFilesArray.size() > 0)
		g.drawText("Current trial: " + String(currentTrialIndex + 1) + " of " + String(audioFilesArray.size()), 180, 230, 440, 25, Justification::centredLeft);
}

void AuditoryLocalisation::resized()
{
	m_chooseStimuliFolder.setBounds(20, 20, 150, 25);
	m_genericHRTF.setBounds(20, 50, 150, 25);
	m_personalizedHRTF.setBounds(20, 80, 150, 25);

	m_labelNumTrials.setBounds(20, 110, 120, 25);
	m_editNumTrials.setBounds(135, 110, 250, 25);

	m_soundVis.setBounds(20, 140, 150, 25);

	m_demo.setBounds(20, 170, 150, 25);

	m_startTest.setBounds(20, 200, 150, 25);
	m_saveLogButton.setBounds(20, 230, 150, 25);

	m_prevTrial.setBounds(20, 260, 100, 25);
	m_nextTrial.setBounds(140, 260, 100, 25);

}

void AuditoryLocalisation::buttonClicked(Button* buttonThatWasClicked)
{
	if (buttonThatWasClicked == &m_chooseStimuliFolder)
	{
		selectSrcPath();
	}
	else if (buttonThatWasClicked == &m_demo) // Tell unity to save csv file or not
	{
		if (m_demo.getToggleState())
		{
			m_demo.setToggleState(false, dontSendNotification);
			m_demo.setButtonText("Start Demo");
			m_oscTxRx->addListener(this);
			m_oscTxRx->sendOscMessage("/LogData", true);
			m_oscTxRx->removeListener(this);
		}
		else
		{
			m_demo.setToggleState(true, dontSendNotification);
			m_demo.setButtonText("Currently in demo mode");
			m_oscTxRx->addListener(this);
			m_oscTxRx->sendOscMessage("/LogData", false);
			m_oscTxRx->removeListener(this);
		}
	}
	else if (buttonThatWasClicked == &m_soundVis) // Tell unity to save csv file or not
	{
		if (m_soundVis.getToggleState())
		{
			m_soundVis.setToggleState(false, dontSendNotification);
			m_soundVis.setButtonText("Sound source invisible");
			m_oscTxRx->addListener(this);
			m_oscTxRx->sendOscMessage("/soundVis", false);
			m_oscTxRx->removeListener(this);
		}
		else
		{
			m_soundVis.setToggleState(true, dontSendNotification);
			m_soundVis.setButtonText("Sound source visible");
			m_oscTxRx->addListener(this);
			m_oscTxRx->sendOscMessage("/soundVis", true);
			m_oscTxRx->removeListener(this);
		}
	}
	else if (buttonThatWasClicked == &m_startTest)
	{
		if (audioFilesArray.isEmpty())
		{
			indexAudioFiles();
		}

		if (m_startTest.getToggleState())
		{
			currentTrialIndex = 0;
			m_startTest.setToggleState(false, dontSendNotification);
			m_startTest.setButtonText("Start Test");
			m_oscTxRx->removeListener(this);
		}
		else
		{
			indexAudioFiles();

			oscMessageList.clear();
			m_oscTxRx->addListener(this);
			activationTime = Time::getMillisecondCounterHiRes();

			loadFile();
			m_startTest.setToggleState(true, dontSendNotification);
			m_startTest.setButtonText("Stop Test");
		}
	}
	else if (buttonThatWasClicked == &m_prevTrial)
	{
		if(currentTrialIndex > 0)
		{
			currentTrialIndex--;
			loadFile();
		}
	}
	else if (buttonThatWasClicked == &m_nextTrial)
	{
		if (currentTrialIndex < audioFilesArray.size() - 1)
		{
			currentTrialIndex++;
			loadFile();
		}
		else
		{
			m_startTest.triggerClick();
			m_demo.setToggleState(false, dontSendNotification);
			m_demo.triggerClick();
		}
	}
	else if (buttonThatWasClicked == &m_saveLogButton)
	{
		if (oscMessageList.size() > 0)
		{
			saveLog();
		}
	}
	else if (buttonThatWasClicked == &m_genericHRTF)
	{
		// player configuration
		//m_player->pause();
		//m_player->loadSourceToTransport("C:\MEng Project\Stimulus_generation\noises1\stim_vis_0_1.wav");
		//juce::String audioFile = "C:\MEng Project\SALTE\SALTE_DEMO_TEST\AmbisonicScenes\Speech_1_3OA_cm2_256.wav";
		//m_player->loadSourceToTransport(audioFilesArray[currentTrialIndex].getFullPathName());

		FileChooser fc("Select Ambix Config file to open...",
			File::getCurrentWorkingDirectory(),
			"*.config",
			true);

		if (fc.browseForFileToOpen())
		{
			genericHRTF_file = fc.getResult();

			//m_renderer->setOrder(5);
			//m_renderer->loadAmbixFile(chosenFile);
		}
		//m_player->setGain(0);
		//m_player->play();
	}
	else if (buttonThatWasClicked == &m_personalizedHRTF)
	{
		FileChooser fc("Select Ambix Config file to open...",
			File::getCurrentWorkingDirectory(),
			"*.config",
			true);

		if (fc.browseForFileToOpen())
		{
			personalizedHRTF_file = fc.getResult();
		}

	}

	repaint();
}

void AuditoryLocalisation::timerCallback()
{
	messageCounter.setText(String(oscMessageList.size()), dontSendNotification);

	if(oscMessageList.size() > 0)
		m_saveLogButton.setEnabled(true);
	else
		m_saveLogButton.setEnabled(false);
}

void AuditoryLocalisation::oscMessageReceived(const OSCMessage& message)
{
	processOscMessage(message);
}

void AuditoryLocalisation::oscBundleReceived(const OSCBundle& bundle)
{
	OSCBundle::Element elem = bundle[0];
	processOscMessage(elem.getMessage());
}

void AuditoryLocalisation::processOscMessage(const OSCMessage& message)
{
	String arguments;
	
	for (int i = 0; i < message.size(); ++i)
	{
		if (message[i].isString()) arguments += "," + message[i].getString();
		else if (message[i].isFloat32()) arguments += "," + String(message[i].getFloat32());
		else if (message[i].isInt32()) arguments += "," + String(message[i].getInt32());
	}

	double time = Time::getMillisecondCounterHiRes() - activationTime;
	String messageText = String(time) + "," + String(currentTrialIndex + 1) + ",";
	
	if (audioFilesArray[currentTrialIndex].exists() && m_player->checkPlaybackStatus())
	{
		messageText += audioFilesArray[currentTrialIndex].getFileName() + "," + message.getAddressPattern().toString() + arguments + "\n";
	}
	else
	{
		messageText += "no stimulus present," + message.getAddressPattern().toString() + arguments + "\n";
	}
	
	oscMessageList.add(messageText);
	if (message.getAddressPattern() == "/startPlayback")
	{
		m_player->play();
	}
	if (message.getAddressPattern() == "/StopPlayback")
	{
		m_player->pause();
	}
	if (message.getAddressPattern() == "/changeSoundPos" && message.size() == 2 && message[0].isFloat32() && message[1].isFloat32())
	{
		float azi = message[0].getFloat32();
		float ele = message[1].getFloat32();

		while (azi >= 180)
		{
			azi -= 360;
		}
		while (azi <= -180)
		{
			azi += 360;
		}

		changeAudioOrientation(0, ele, azi);
	}
	if (message.getAddressPattern() == "/loadNextSample")
	{
		m_nextTrial.triggerClick();
		
	}
}

void AuditoryLocalisation::saveLog()
{
	FileChooser fc("Select or create results export file...",
		File::getCurrentWorkingDirectory(),
		"*.csv",
		true);

	if (fc.browseForFileToSave(true))
	{
		File logFile;

		logFile = fc.getResult();

		if (!logFile.exists())
			logFile.create();

		FileOutputStream fos(logFile);

		// create csv file header
		fos << "time,trial_index,stimulus,osc_pattern,et_el,et_az,et_rot,et_dist,et_conf\n";
		for (int i = 0; i < oscMessageList.size(); ++i)
			fos << oscMessageList[i];
	}
}

void AuditoryLocalisation::changeListenerCallback(ChangeBroadcaster* source)
{
	if (source == m_player)
	{
		if (!m_player->checkPlaybackStatus() && m_startTest.getToggleState())
		{
			//m_nextTrial.triggerClick();
		}
	}
}

void AuditoryLocalisation::selectSrcPath()
{
	FileChooser fc("Select the stimuli folder...",
		File::getSpecialLocation(File::userHomeDirectory));
	
	if (fc.browseForDirectory())
	{
		audioFilesDir = fc.getResult();
		indexAudioFiles();
		saveSettings();
	}
}

void AuditoryLocalisation::indexAudioFiles()
{
	audioFilesArray.clear();
	Array<File> audioFilesInDir;

	// create the test file array (audio stimuli)
	audioFilesInDir.clear();
	DirectoryIterator iter2(audioFilesDir, false, "stim_*.wav");
	while (iter2.next())
		audioFilesInDir.add(iter2.getFile());

	int numTrials = m_editNumTrials.getText().getIntValue();
	for (int i = 0; i < numTrials; ++i)
	{
		std::random_device seed;
		std::mt19937 rng(seed());
		std::shuffle(audioFilesInDir.begin(), audioFilesInDir.end(), rng);
		audioFilesArray.addArray(audioFilesInDir);
	}

	// load files to the player
	m_player->clearPlayer();
	for (auto& file : audioFilesArray)
		m_player->cacheFileToPlayer(file.getFullPathName());

	totalTimeOfAudioFiles = 0.0f;
	for (auto& file : audioFilesArray)
	{
		if (auto * reader = formatManager.createReaderFor(file))
		{
			totalTimeOfAudioFiles += reader->lengthInSamples / reader->sampleRate;
			reader->~AudioFormatReader();
		}

	}
}

String AuditoryLocalisation::returnHHMMSS(double lengthInSeconds)
{
	int hours = static_cast<int>(lengthInSeconds / (60 * 60));
	int minutes = (static_cast<int>(lengthInSeconds / 60)) % 60;
	int seconds = (static_cast<int>(lengthInSeconds)) % 60;
	int millis = static_cast<int>(floor((lengthInSeconds - floor(lengthInSeconds)) * 100));

	String output = String(hours).paddedLeft('0', 2) + ":" +
		String(minutes).paddedLeft('0', 2) + ":" +
		String(seconds).paddedLeft('0', 2) + "." +
		String(millis).paddedLeft('0', 2);
	return output;
}

void AuditoryLocalisation::loadFile()
{
	m_player->pause();

	m_player->loadSourceToTransport(audioFilesArray[currentTrialIndex].getFullPathName());
	sendMsgToLogWindow("Loaded Audio File: " + audioFilesArray[currentTrialIndex].getFullPathName());
	m_player->setGain(0);
	m_player->loop(true);

	// Load HRTF based on naming convention of stimulus
	String filename = m_player->getCurrentSourceFileName();
	String HRTF_idx = filename.fromFirstOccurrenceOf("HRTF_", false, false).upToFirstOccurrenceOf("_", false, false);
	File HRTF_file;
	if (HRTF_idx == "1")
		HRTF_file = genericHRTF_file;
	else if (HRTF_idx == "2")
		HRTF_file = personalizedHRTF_file;

	// Send whether it's generic or personal HRTF to Unity
	String HRTFstr = (String)HRTF_file.getFullPathName();
	if (m_lsRenderer->isRendererEnabled() == true)
	{
		m_oscTxRx->sendOscMessage("/HRTF_name", (String)"Loudspeakers");
	}
	else if (HRTFstr.contains("SADIE") != std::string::npos) 
	{
		m_oscTxRx->sendOscMessage("/HRTF_name", (String)"Generic Headphones");
	}
	else
	{
		m_oscTxRx->sendOscMessage("/HRTF_name", (String)"Personal Headphones");
	}

	m_renderer->setOrder(5);
	m_renderer->loadAmbixFile(HRTF_file);

	// Set azimuth (yaw) and elevation (pitch) angles
	//m_player->changeOrientation(0, ele, azi);
	
	//sendMsgToLogWindow(vis + " / " + azi + " / " + audioFilesArray[currentTrialIndex].getFileName());

	String vis = filename.fromFirstOccurrenceOf("vis_", false, false).upToFirstOccurrenceOf("_", false, false);
	//sendMsgToLogWindow(vis + " / " + azi + " / " + audioFilesArray[currentTrialIndex].getFileName());
	//m_oscTxRx->sendOscMessage("/targetVisAzEl", (int)vis.getFloatValue(), azi, (float)0);
}

// For changing orientation programatically 
void AuditoryLocalisation::changeAudioOrientation(float roll, float pitch, float yaw)
{
	const MessageManagerLock mmLock;
	m_player->changeOrientation(roll, pitch, yaw);
	sendMsgToLogWindow("Azimuth: " + String(yaw));
	sendMsgToLogWindow("Elevation: " + String(pitch));
}

void AuditoryLocalisation::sendMsgToLogWindow(String message)
{
	currentMessage += message + "\n";
	sendChangeMessage();  // broadcast change message to inform and update the editor
}

void AuditoryLocalisation::initSettings()
{
	PropertiesFile::Options options;
	options.applicationName = "SALTELocalisationTestSettings";
	options.filenameSuffix = ".conf";
	options.osxLibrarySubFolder = "Application Support";
	options.folderName = File::getSpecialLocation(File::SpecialLocationType::currentApplicationFile).getParentDirectory().getFullPathName();
	options.storageFormat = PropertiesFile::storeAsXML;
	TestSessionFormSettings.setStorageParameters(options);
}

void AuditoryLocalisation::loadSettings()
{
	audioFilesDir = TestSessionFormSettings.getUserSettings()->getValue("audioFilesSrcPath");
	if (File(audioFilesDir).exists()) indexAudioFiles();
}

void AuditoryLocalisation::saveSettings()
{
	TestSessionFormSettings.getUserSettings()->setValue("audioFilesSrcPath", audioFilesDir.getFullPathName());
	TestSessionFormSettings.getUserSettings()->setValue("loadSettingsFile", true);
}
