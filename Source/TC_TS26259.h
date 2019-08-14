#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include <vector>
#include <random>

using std::vector;

class TestTrial
{
public:
	TestTrial()
	{
	}
	~TestTrial()
	{
	}

	void setFilepath(int fileindex, String filepath) { filepathArray.set(fileindex, filepath); }
	String getFilepath(int fileindex) { return filepathArray[fileindex]; }
	int getNumberOfConditions() { return filepathArray.size(); }
	void setLastPlaybackHeadPosition(float time) { lastPlaybackHeadPosition = time; }
	float getLastPlaybackHeadPosition() { return lastPlaybackHeadPosition; }
	void setLoopStart(float startTime) { loopStartTime = startTime; }
	float getLoopStart() { return loopStartTime; }
	void setLoopEnd(float endTime) { loopEndTime = endTime; }
	float getLoopEnd() { return loopEndTime; }

private:
	Array<String> filepathArray;
	// results vector needs to be added
	float lastPlaybackHeadPosition = 0.0f;
	float loopStartTime = 0.0f, loopEndTime = 0.0f;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TestTrial)
};

class TC_TS26259 : public Component,
					private Button::Listener,
					private Slider::Listener
{
public:
	TC_TS26259();
	~TC_TS26259();

	void paint(Graphics&) override;
	void resized() override;
	void buttonClicked(Button* buttonThatWasClicked) override;
	void sliderValueChanged(Slider* sliderThatWasChanged) override;


private:
	// OSCSender sender;
	TextButton playButton, stopButton, loopButton;
	TextButton selectAButton, selectBButton;
	TextButton prevTrialButton, nextTrialButton;
	OwnedArray<Slider> ratingSliderArray;
	OwnedArray<TestTrial> testTrialArray;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TC_TS26259)
};
