#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "OscTransceiver.h"
#include "StimulusPlayer.h"
#include "BinauralRenderer.h"

class AuditoryLocalisation	:	public Component
							//,	public OSCReceiver
							,	public OSCReceiver::Listener<OSCReceiver::RealtimeCallback>
							,	private Button::Listener
							,	private ChangeListener
							,	public ChangeBroadcaster
							,	public Timer
{
public:
	AuditoryLocalisation();
	~AuditoryLocalisation();

	void init(OscTransceiver* oscTxRx, StimulusPlayer* player, BinauralRenderer* renderer);

	void paint(Graphics& g) override;
	void resized() override;
	void buttonClicked(Button* buttonThatWasClicked) override;
	void oscMessageReceived(const OSCMessage& message) override;
	void oscBundleReceived(const OSCBundle& bundle) override;
	void changeListenerCallback(ChangeBroadcaster* source) override;
	void timerCallback() override;

	String currentMessage;
private:
	OscTransceiver* m_oscTxRx;
	StimulusPlayer* m_player;
	BinauralRenderer* m_renderer;

	TextButton m_chooseStimuliFolder;
	TextButton m_demo;
	TextButton m_startTest;
	TextButton m_prevTrial, m_nextTrial;
	TextButton m_confirmPointer;
	TextButton m_personalizedHRTF;
	TextButton m_genericHRTF;

	File genericHRTF_file;
	File personalizedHRTF_file;

	TextButton m_createRndSubjectIDButton;
	void createRandomSubjectID();
	Label m_labelComment;
	TextEditor m_editComment;

	Label m_labelNumTrials;
	TextEditor m_editNumTrials;

	void selectSrcPath();
	void indexAudioFiles();
	String returnHHMMSS(double lengthInSeconds);
	File audioFilesDir;
	Array<File> audioFilesArray;
	AudioFormatManager formatManager;
	double totalTimeOfAudioFiles = 0;

	int currentTrialIndex = 0;

	void loadFile();
	void changeAudioOrientation(float roll, float pitch, float yaw);
	void sendMsgToLogWindow(String message);

	// OSC logging
	void processOscMessage(const OSCMessage& message);
	void saveLog();
	int port = 9000;
	double activationTime = 0.0f;
	TextButton m_saveLogButton;
	Label messageCounter;
	StringArray oscMessageList;

	// settings
	void initSettings();
	void loadSettings();
	void saveSettings();
	ApplicationProperties TestSessionFormSettings;
};

