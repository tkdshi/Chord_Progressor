/*
  ==============================================================================

   This file is part of the JUCE examples.
   Copyright (c) 2020 - Raw Material Software Limited

   The code included in this file is provided under the terms of the ISC license
   http://www.isc.org/downloads/software-support-policy/isc-license. Permission
   To use, copy, modify, and/or distribute this software for any purpose with or
   without fee is hereby granted provided that the above copyright notice and
   this permission notice appear in all copies.

   THE SOFTWARE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES,
   WHETHER EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR
   PURPOSE, ARE DISCLAIMED.

  ==============================================================================
*/

/*******************************************************************************
 The block below describes the properties of this PIP. A PIP is a short snippet
 of code that can be read by the Projucer and used to generate a JUCE project.

 BEGIN_JUCE_PIP_METADATA

 name:                  AudioPluginDemo
 version:               1.0.0
 vendor:                JUCE
 website:               http://juce.com
 description:           Synthesiser audio plugin.

 dependencies:          juce_audio_basics, juce_audio_devices, juce_audio_formats,
						juce_audio_plugin_client, juce_audio_processors,
						juce_audio_utils, juce_core, juce_data_structures,
						juce_events, juce_graphics, juce_gui_basics, juce_gui_extra
 exporters:             xcode_mac, vs2017, vs2019, linux_make, xcode_iphone, androidstudio

 moduleFlags:           JUCE_STRICT_REFCOUNTEDPOINTER=1

 type:                  AudioProcessor
 mainClass:             JuceDemoPluginAudioProcessor

 useLocalCopy:          1

 pluginCharacteristics: pluginIsSynth, pluginWantsMidiIn, pluginProducesMidiOut,
						pluginEditorRequiresKeys
 extraPluginFormats:    AUv3

 END_JUCE_PIP_METADATA

*******************************************************************************/

#pragma once

int Chord_Value[8][2] = { {5,0},{7,0},{9,1},{9,1},{5,0},{7,0},{9,1},{9,1} };


//==============================================================================
/** A demo synth sound that's just a basic sine wave.. */
class SineWaveSound : public SynthesiserSound
{
public:
	SineWaveSound() {}

	bool appliesToNote(int /*midiNoteNumber*/) override { return true; }
	bool appliesToChannel(int /*midiChannel*/) override { return true; }
};



//==============================================================================
/** As the name suggest, this class does the actual audio processing. */
class JuceDemoPluginAudioProcessor : public AudioProcessor
{

public:
	//==============================================================================
	JuceDemoPluginAudioProcessor()
		: AudioProcessor(getBusesProperties()),
		state(*this, nullptr, "state",
			{ std::make_unique<AudioParameterFloat>("gain",  "Gain",           NormalisableRange<float>(0.0f, 1.0f), 0.9f),
			  std::make_unique<AudioParameterFloat>("delay", "Delay Feedback", NormalisableRange<float>(0.0f, 1.0f), 0.5f) })
	{
		// Add a sub-tree to store the state of our UI
		lastPosInfo.resetToDefault();

		state.state.addChild({ "uiState", { { "width",  400 }, { "height", 200 } }, {} }, -1, nullptr);
		loadAudioFile();
	}

	~JuceDemoPluginAudioProcessor() {}

	//==============================================================================
	bool isBusesLayoutSupported(const BusesLayout& layouts) const override
	{
		// Only mono/stereo and input/output must have same layout
		const auto& mainOutput = layouts.getMainOutputChannelSet();
		const auto& mainInput = layouts.getMainInputChannelSet();

		// input and output layout must either be the same or the input must be disabled altogether
		if (!mainInput.isDisabled() && mainInput != mainOutput)
			return false;

		// do not allow disabling the main buses
		if (mainOutput.isDisabled())
			return false;

		// only allow stereo and mono
		if (mainOutput.size() > 2)
			return false;

		return true;
	}

	juce::AudioParameterFloat* speed;
	int currentNote, lastNoteValue;
	int time;
	float rate;
	juce::SortedSet<int> notes;


	// プラグインをロードした時やホスト側のセットアップ処理を実行した時にホストから呼び出される。
	void prepareToPlay(double newSampleRate, int samplesPerBlock) override
	{
		// Synthesiserオブジェクトにホストアプリケーションのサンプリングレートをセットする
		synth.setCurrentPlaybackSampleRate(newSampleRate);
		// MidiKeyboardStateオブジェクトの状態を初期化する
		keyboardState.reset();


	}
	// プラグインを非アクティブ化した時や削除する時にホストから呼び出される。
	void releaseResources() override
	{
		// MidiKeyboardStateオブジェクトをオール・ノートOFF状態にする
		keyboardState.allNotesOff(0);
		// MidiKeyboardStateオブジェクトの状態を初期化する
		keyboardState.reset();
	}

	void reset() override
	{
		// Use this method as the place to clear any delay lines, buffers, etc, as it
		// means there's been a break in the audio's continuity.
		delayBufferFloat.clear();
		delayBufferDouble.clear();
	}

	//==============================================================================
	int beat_position[4] = { 0,0,0,0 };
	int Chord_key[5] = { 0,0,0,0,0 };

	//アプリケーションからオーディオバッファとMIDIバッファの参照を取得してオーディオレンダリングを実行
	void processBlock(AudioBuffer<float>& buffer, MidiBuffer& midiMessages) override
	{




		if (isChanging) {
			return;
		}

		ScopedNoDenormals noDenormals;

		int totalNumInputChannels = getTotalNumInputChannels();
		int totalNumOutputChannels = getTotalNumOutputChannels();
		int key_num = 60;
		MidiMessage message[4];

		//midiメッセージを追加

			//ノートオン
		if (beat_position[0] != beat_position[1]) {
			/*
			for (int i = 0; Chord_key[i] != -1; i++) {
				message[i] = juce::MidiMessage::noteOff(1, key_num + Chord_Value[beat_position[1]][0] + Chord_key[i]noteNumber);
				midiMessages.addEvent(message[i], 0);
			}*/
			int Chord_key[5] = { 0,4,7,-1,-1 };

			keyboardState.reset();
			switch (Chord_Value[beat_position[0]][1]) {
			case 0://major
				break;
			case 1://miner
				Chord_key[1] = 3;
				break;
			case 2://M7
				Chord_key[3] = 11;
				break;
			case 3://m7
				Chord_key[1] = 3;
				Chord_key[3] = 10;
				break;
			default:
				break;

			}




			for (int i = 0; Chord_key[i] != -1; i++) {
				message[i] = juce::MidiMessage::noteOn(1, key_num + Chord_Value[beat_position[0]][0] + Chord_key[i]/*noteNumber*/, (uint8)127);
				midiMessages.addEvent(message[i], 0/*sample number*/);
			}
		}
		else {
			juce::MidiMessage::allNotesOff(1);

		}
		//ノートオフ
		/*
		auto messageOff = juce::MidiMessage::noteOff(message.getChannel(), message.getNoteNumber());
		addMessageToBuffer(messageOff);
		*/







		// MidiKeyboardStateオブジェクトのMIDIメッセージとMIDIバッファのMIDIメッセージをマージする
		keyboardState.processNextMidiBuffer(midiMessages, 0, buffer.getNumSamples(), true);

		// オーディオバッファのサンプルデータをクリア
		for (auto i = totalNumInputChannels; i < totalNumOutputChannels; i++) {
			buffer.clear(i, 0, buffer.getNumSamples());
		}
		//    // Synthesiserオブジェクトにオーディオバッファの参照とMIDIバッファの参照を渡して、オーディオレンダリング
		synth.renderNextBlock(buffer, midiMessages, 0, buffer.getNumSamples());

		updateCurrentTimeInfoFromHost(beat_position);
	}



	//==============================================================================
	bool hasEditor() const override { return true; }

	AudioProcessorEditor* createEditor() override
	{
		return new JuceDemoPluginAudioProcessorEditor(*this);
	}

	//==============================================================================
	const String getName() const override { return "Chord Progressor"; }
	bool acceptsMidi() const override { return true; }
	bool producesMidi() const override { return true; }
	double getTailLengthSeconds() const override { return 0.0; }

	//==============================================================================
	int getNumPrograms() override { return 0; }
	int getCurrentProgram() override { return 0; }
	void setCurrentProgram(int) override {}
	const String getProgramName(int) override { return {}; }
	void changeProgramName(int, const String&) override {}

	//==============================================================================
	void getStateInformation(MemoryBlock& destData) override
	{
		// Store an xml representation of our state.
		if (auto xmlState = state.copyState().createXml())
			copyXmlToBinary(*xmlState, destData);
	}

	void setStateInformation(const void* data, int sizeInBytes) override
	{
		// Restore our plug-in's state from the xml representation stored in the above
		// method.
		if (auto xmlState = getXmlFromBinary(data, sizeInBytes))
			state.replaceState(ValueTree::fromXml(*xmlState));
	}

	//==============================================================================
	void updateTrackProperties(const TrackProperties& properties) override
	{
		{
			const ScopedLock sl(trackPropertiesLock);
			trackProperties = properties;
		}

		MessageManager::callAsync([this]
			{
				if (auto* editor = dynamic_cast<JuceDemoPluginAudioProcessorEditor*> (getActiveEditor()))
					editor->updateTrackProperties();
			});
	}



	//synthsizer setup
	void setupSampler(AudioFormatReader& newReader) {
		isChanging = true;

		synth.clearSounds();
		synth.clearVoices();

		BigInteger allNotes;
		allNotes.setRange(0, 128, true);

		synth.addSound(new SamplerSound("default", newReader, allNotes, 60, 0, 0.1, 10.0));

		for (int i = 0; i < 128; i++) {
			synth.addVoice(new SamplerVoice());
		}

		isChanging = false;
	}

	void loadAudioFile() {

		AudioFormatManager formatManager;
		formatManager.registerBasicFormats();

		MemoryInputStream* inputStream = new MemoryInputStream(BinaryData::piano_mp3, BinaryData::piano_mp3Size, true);

		AudioFormatReader* reader = formatManager.createReaderFor(inputStream);

		if (reader != nullptr) {
			setupSampler(*reader);
			delete reader;
		}


	}

	void loadSampleFile() {
		AudioFormatManager formatManager;
		formatManager.registerBasicFormats();

		FileChooser chooser("Open audio file to play.", File::nonexistent, formatManager.getWildcardForAllFormats());

		if (chooser.browseForFileToOpen()) {
			File file(chooser.getResult());
			AudioFormatReader* reader = formatManager.createReaderFor(file);

			if (reader != nullptr) {
				setupSampler(*reader);
				delete reader;
			}
		}


	}



	MidiKeyboardState& getMidiKeyboardState() {
		return keyboardState;
	}




	TrackProperties getTrackProperties() const
	{
		const ScopedLock sl(trackPropertiesLock);
		return trackProperties;
	}

	class SpinLockedPosInfo
	{
	public:
		SpinLockedPosInfo() { info.resetToDefault(); }

		// Wait-free, but setting new info may fail if the main thread is currently
		// calling `get`. This is unlikely to matter in practice because
		// we'll be calling `set` much more frequently than `get`.
		void set(const AudioPlayHead::CurrentPositionInfo& newInfo)
		{

			const juce::SpinLock::ScopedTryLockType lock(mutex);

			if (lock.isLocked())
				info = newInfo;

		}

		AudioPlayHead::CurrentPositionInfo get() const noexcept
		{
			const juce::SpinLock::ScopedLockType lock(mutex);
			return info;
		}


	private:
		juce::SpinLock mutex;
		AudioPlayHead::CurrentPositionInfo info;





		//==============================================================================
	};





	//==============================================================================
	// These properties are public so that our editor component can access them
	// A bit of a hacky way to do it, but it's only a demo! Obviously in your own
	// code you'll do this much more neatly..
		//processblock skip

	bool isChanging;

	// this is kept up to date with the midi messages that arrive, and the UI component
	// registers with it so it can represent the incoming messages
	MidiKeyboardState keyboardState;

	// this keeps a copy of the last set of time info that was acquired during an audio
	// callback - the UI component will read this and display it.
	AudioPlayHead::CurrentPositionInfo lastPosInfo;

	// Our plug-in's current state
	AudioProcessorValueTreeState state;


private:
	//==============================================================================
	/** This is the editor component that our filter will display. */
	//GUI‚Ì•ÒW
	class JuceDemoPluginAudioProcessorEditor : public AudioProcessorEditor,
		private Timer,
		private Value::Listener,
		private Button::Listener
	{
	public:


		int Chord_g1[16][8][2] = { {{0,3},{7,0},{9,1},{4,1},{3,0},{0,0},{4,1},{7,0}},
			{{5,0},{7,0},{9,0},{9,0},{5,0},{7,0},{9,0},{9,0} },
		{ {0,0},{9,1},{5,0},{7,0},{0,0},{9,1},{5,0},{7,0} },
		{ {0,3},{7,0},{9,1},{4,1},{3,0},{0,0},{4,1},{7,0} },
		{ {0,3},{7,0},{9,1},{4,1},{3,0},{0,0},{4,1},{7,0} },
		{ {0,3},{7,0},{9,1},{4,1},{3,0},{0,0},{4,1},{7,0} },
		{ {0,3},{7,0},{9,1},{4,1},{3,0},{0,0},{4,1},{7,0} },
		{ {0,3},{7,0},{9,1},{4,1},{3,0},{0,0},{4,1},{7,0} },
		{ {0,3},{7,0},{9,1},{4,1},{3,0},{0,0},{4,1},{7,0} },
		{ {5,0},{7,0},{9,0},{9,0},{5,0},{7,0},{9,0},{9,0} },
		{ {0,0},{9,1},{5,0},{7,0},{0,0},{9,1},{5,0},{7,0} },
		{ {0,3},{7,0},{9,1},{4,1},{3,0},{0,0},{4,1},{7,0} },
		{ {0,3},{7,0},{9,1},{4,1},{3,0},{0,0},{4,1},{7,0} },
		{ {0,3},{7,0},{9,1},{4,1},{3,0},{0,0},{4,1},{7,0} },
		{ {0,3},{7,0},{9,1},{4,1},{3,0},{0,0},{4,1},{7,0} },
		{ {0,3},{7,0},{9,1},{4,1},{3,0},{0,0},{4,1},{7,0} } };

		int g_push[8] = { 0,0,0,0,0,0,0,0 };



		int Page = 0;
		const String Chord_Name[12] = { "C","C#","D" ,"D#" ,"E" ,"F" ,"F#" ,"G" ,"G#" ,"A" ,"A#" ,"B" };
		const String Chord_Type[4] = { "","m","M7","m7" };



		JuceDemoPluginAudioProcessorEditor(JuceDemoPluginAudioProcessor& owner)
			: AudioProcessorEditor(owner),
			midiKeyboard(owner.keyboardState, MidiKeyboardComponent::horizontalKeyboard),
			gainAttachment(owner.state, "gain", gainSlider),
			delayAttachment(owner.state, "delay", delaySlider)
		{

			//Using Button Attach
			addAndMakeVisible(Button_c1);
			Button_c1.setButtonText(Chord_Name[Chord_Value[0 + Page][0]] + Chord_Type[Chord_Value[0 + Page][1]]);
			Button_c1.addListener(this);

			addAndMakeVisible(Button_c2);
			Button_c2.setButtonText(Chord_Name[Chord_Value[1 + Page][0]] + Chord_Type[Chord_Value[1 + Page][1]]);
			Button_c2.addListener(this);

			addAndMakeVisible(Button_c3);
			Button_c3.setButtonText(Chord_Name[Chord_Value[2 + Page][0]] + Chord_Type[Chord_Value[2 + Page][1]]);
			Button_c3.addListener(this);

			addAndMakeVisible(Button_c4);
			Button_c4.setButtonText(Chord_Name[Chord_Value[3 + Page][0]] + Chord_Type[Chord_Value[3 + Page][1]]);
			Button_c4.addListener(this);

			addAndMakeVisible(Button_r1);
			Button_r1.setButtonText("Normal");

			addAndMakeVisible(Button_r2);
			Button_r2.setButtonText("Normal");

			addAndMakeVisible(Button_r3);
			Button_r3.setButtonText("Normal");

			addAndMakeVisible(Button_r4);
			Button_r4.setButtonText("Normal");

			addAndMakeVisible(Button_g1);
			Button_g1.setButtonText("J-POP");
			Button_g1.addListener(this);

			addAndMakeVisible(Button_g2);
			Button_g2.setButtonText("Rock");
			Button_g2.addListener(this);

			addAndMakeVisible(Button_g3);
			Button_g3.setButtonText("Jazz");
			Button_g3.addListener(this);

			addAndMakeVisible(Button_g4);
			Button_g4.setButtonText("EDM");
			Button_g4.addListener(this);

			addAndMakeVisible(Button_g5);
			Button_g5.setButtonText("Idol");
			Button_g5.addListener(this);

			addAndMakeVisible(Button_g6);
			Button_g6.setButtonText("Ballade");
			Button_g6.addListener(this);

			addAndMakeVisible(Button_g7);
			Button_g7.setButtonText("Anime");
			Button_g7.addListener(this);

			addAndMakeVisible(Button_g8);
			Button_g8.setButtonText("Game");
			Button_g8.addListener(this);

			addAndMakeVisible(Button_L);
			Button_L.setButtonText("<");
			Button_L.addListener(this);

			addAndMakeVisible(Button_R);
			Button_R.setButtonText(">");
			Button_R.addListener(this);

			addAndMakeVisible(tempoDisplayLabel);
			TempoLabel.setFont(Font(Font::getDefaultMonospacedFontName(), 15.0f, Font::plain));

			// add some sliders..
			addAndMakeVisible(gainSlider);
			gainSlider.setSliderStyle(Slider::Rotary);

			addAndMakeVisible(delaySlider);
			delaySlider.setSliderStyle(Slider::Rotary);

			// add the midi keyboard component..
			addAndMakeVisible(midiKeyboard);

			// add a label that will display the current timecode and status..
			addAndMakeVisible(timecodeDisplayLabel);
			timecodeDisplayLabel.setFont(Font(Font::getDefaultMonospacedFontName(), 15.0f, Font::plain));

			// set resize limits for this plug-in
			setResizeLimits(800, 600, 800, 600);

			lastUIWidth.referTo(owner.state.state.getChildWithName("uiState").getPropertyAsValue("width", nullptr));
			lastUIHeight.referTo(owner.state.state.getChildWithName("uiState").getPropertyAsValue("height", nullptr));

			// set our component's initial size to be the last one that was stored in the filter's settings

			//setSize (lastUIWidth.getValue(), lastUIHeight.getValue());

			lastUIWidth.addListener(this);
			lastUIHeight.addListener(this);




			updateTrackProperties();

			// start a timer which will keep our timecode display updated
			startTimerHz(30);
		}

		~JuceDemoPluginAudioProcessorEditor() override {}

		//==============================================================================
	   //背景の描画
		void paint(Graphics& g) override
		{
			//g.setColour (backgroundColour);
			g.fillAll();

			//Imageオブジェクトの生成
			image_background = ImageCache::getFromMemory(BinaryData::background_png, BinaryData::background_pngSize);

			//Imageオブジェクトの描画
			g.drawImageWithin(image_background, 0, 0, image_background.getWidth(), image_background.getHeight(), RectanglePlacement::yTop, false);
		}

		//ボタンなどの描画
		void resized() override
		{
			// This lays out our child components...



			auto r = getLocalBounds().reduced(8);

			//ヘッダ部分
			auto headerArea = r.removeFromTop(80);

			//楽譜部分
			auto scoreArea = r.removeFromTop(160);
			midiKeyboard.setBounds(scoreArea.removeFromLeft(scoreArea.getWidth()));


			//左右のボタン
			auto sideWidth = 25;
			Button_L.setBounds(r.removeFromLeft(sideWidth));
			Button_R.setBounds(r.removeFromRight(sideWidth));


			auto margin1 = r.removeFromTop(30);

			//コード部分
			auto chordArea = r.removeFromTop(70);
			Button_c1.setBounds(chordArea.removeFromLeft(chordArea.getWidth() / 4));
			Button_c2.setBounds(chordArea.removeFromLeft(chordArea.getWidth() / 3));
			Button_c3.setBounds(chordArea.removeFromLeft(chordArea.getWidth() / 2));
			Button_c4.setBounds(chordArea.removeFromLeft(chordArea.getWidth() / 1));

			auto margin2 = r.removeFromTop(10);


			//コードの種類部分
			auto rythmArea = r.removeFromTop(40);
			Button_r1.setBounds(rythmArea.removeFromLeft(rythmArea.getWidth() / 4));
			Button_r2.setBounds(rythmArea.removeFromLeft(rythmArea.getWidth() / 3));
			Button_r3.setBounds(rythmArea.removeFromLeft(rythmArea.getWidth() / 2));
			Button_r4.setBounds(rythmArea.removeFromLeft(rythmArea.getWidth() / 1));


			auto margin3 = r.removeFromTop(10);

			//ジャンル部分
			auto genreArea = r.removeFromLeft(r.getWidth() / 2);
			auto genrerow1 = genreArea.removeFromTop(genreArea.getHeight() / 2);
			auto genrerow2 = genreArea.removeFromTop(genreArea.getHeight() / 1);
			Button_g1.setBounds(genrerow1.removeFromLeft(genrerow1.getWidth() / 4));
			Button_g2.setBounds(genrerow1.removeFromLeft(genrerow1.getWidth() / 3));
			Button_g3.setBounds(genrerow1.removeFromLeft(genrerow1.getWidth() / 2));
			Button_g4.setBounds(genrerow1.removeFromLeft(genrerow1.getWidth() / 1));
			Button_g5.setBounds(genrerow2.removeFromLeft(genrerow2.getWidth() / 4));
			Button_g6.setBounds(genrerow2.removeFromLeft(genrerow2.getWidth() / 3));
			Button_g7.setBounds(genrerow2.removeFromLeft(genrerow2.getWidth() / 2));
			Button_g8.setBounds(genrerow2.removeFromLeft(genrerow2.getWidth() / 1));


			//小節など
			auto stateArea = r.removeFromLeft(r.getWidth() / 1);
			auto staterow1 = stateArea.removeFromTop(stateArea.getHeight() / 2);
			auto staterow2 = stateArea.removeFromTop(stateArea.getHeight());

			timecodeDisplayLabel.setBounds(staterow1.removeFromLeft(staterow1.getWidth()));
			tempoDisplayLabel.setBounds(staterow2.removeFromLeft(staterow2.getWidth()));

			auto sliderArea = r.removeFromTop(60);
			//gainSlider.setBounds  (sliderArea.removeFromLeft (jmin (180, sliderArea.getWidth() / 2)));
			//delaySlider.setBounds (sliderArea.removeFromLeft (jmin (180, sliderArea.getWidth())));
			//midiKeyboard.setBounds(r.removeFromBottom(70));


			//ƒ{ƒgƒ€•”•ª


			lastUIWidth = getWidth();
			lastUIHeight = getHeight();

		}

		void timerCallback() override
		{
			updateTimecodeDisplay(getProcessor().lastPosInfo);
		}

		void hostMIDIControllerIsAvailable(bool controllerIsAvailable) override
		{
			midiKeyboard.setVisible(!controllerIsAvailable);
		}

		int getControlParameterIndex(Component& control) override
		{
			if (&control == &gainSlider)
				return 0;

			if (&control == &delaySlider)
				return 1;

			return -1;
		}

		//ボタンをクリックしたときの処理
		void buttonClicked(Button* clickedButton) override
		{
			int number;

			number = 0;
			if (clickedButton == &Button_g1) {
				g_push[number] = updateChordValue(number, g_push[number]);
			}

			number++;
			if (clickedButton == &Button_g2) {
				g_push[number] = updateChordValue(number, g_push[number]);
			}

			number++;
			if (clickedButton == &Button_g3) {
				g_push[number] = updateChordValue(number, g_push[number]);
			}

			number++;
			if (clickedButton == &Button_g4) {
				g_push[number] = updateChordValue(number, g_push[number]);
			}

			number++;
			if (clickedButton == &Button_g5) {
				g_push[number] = updateChordValue(number, g_push[number]);
			}

			number++;
			if (clickedButton == &Button_g6) {
				g_push[number] = updateChordValue(number, g_push[number]);
			}

			number++;
			if (clickedButton == &Button_g7) {
				g_push[number] = updateChordValue(number, g_push[number]);
			}

			number++;
			if (clickedButton == &Button_g8) {
				g_push[number] = updateChordValue(number, g_push[number]);
			}

			if (clickedButton == &Button_L && Page != 0) {
				Page = 0;
				updateChordLabel();
			}

			if (clickedButton == &Button_R && Page == 0) {
				Page = 4;
				updateChordLabel();
			}






		}

		void updateTrackProperties()
		{
			auto trackColour = getProcessor().getTrackProperties().colour;
			auto& lf = getLookAndFeel();

			backgroundColour = (trackColour == Colour() ? lf.findColour(ResizableWindow::backgroundColourId)
				: trackColour.withAlpha(1.0f).withBrightness(0.266f));
			repaint();
		}

		//コードの値の更新
		int updateChordValue(int n, int push) {
			if (push == 0) {
				push++;
				n = 2 * n;
			}
			else {
				push--;
				n = 2 * n + 1;

			}

			for (int i = 0; i < 8; i++) {
				for (int j = 0; j < 2; j++) {
					Chord_Value[i][j] = Chord_g1[n][i][j];
				}

			}

			updateChordLabel();


			return push;
		}


		//コードのボタン上のラベルを更新
		void updateChordLabel() {
			Button_c1.setButtonText(Chord_Name[Chord_Value[0 + Page][0]] + Chord_Type[Chord_Value[0 + Page][1]]);
			Button_c2.setButtonText(Chord_Name[Chord_Value[1 + Page][0]] + Chord_Type[Chord_Value[1 + Page][1]]);
			Button_c3.setButtonText(Chord_Name[Chord_Value[2 + Page][0]] + Chord_Type[Chord_Value[2 + Page][1]]);
			Button_c4.setButtonText(Chord_Name[Chord_Value[3 + Page][0]] + Chord_Type[Chord_Value[3 + Page][1]]);


		}




	private:



		//--------------------
		//midiŠÖ˜A‚ÌƒRƒ“ƒ|[ƒlƒ“ƒg
		static juce::String getMidiMessageDescription(const juce::MidiMessage& m)
		{
			if (m.isNoteOn())           return "Note on " + juce::MidiMessage::getMidiNoteName(m.getNoteNumber(), true, true, 3);
			if (m.isNoteOff())          return "Note off " + juce::MidiMessage::getMidiNoteName(m.getNoteNumber(), true, true, 3);
			if (m.isProgramChange())    return "Program change " + juce::String(m.getProgramChangeNumber());
			if (m.isPitchWheel())       return "Pitch wheel " + juce::String(m.getPitchWheelValue());
			if (m.isAftertouch())       return "After touch " + juce::MidiMessage::getMidiNoteName(m.getNoteNumber(), true, true, 3) + ": " + juce::String(m.getAfterTouchValue());
			if (m.isChannelPressure())  return "Channel pressure " + juce::String(m.getChannelPressureValue());
			if (m.isAllNotesOff())      return "All notes off";
			if (m.isAllSoundOff())      return "All sound off";
			if (m.isMetaEvent())        return "Meta event";

			if (m.isController())
			{
				juce::String name(juce::MidiMessage::getControllerName(m.getControllerNumber()));

				if (name.isEmpty())
					name = "[" + juce::String(m.getControllerNumber()) + "]";

				return "Controller " + name + ": " + juce::String(m.getControllerValue());
			}

			return juce::String::toHexString(m.getRawData(), m.getRawDataSize());
		}















		Label timecodeDisplayLabel, tempoDisplayLabel,
			gainLabel{ {}, "Throughput level:" },
			delayLabel{ {}, "Delay:" };

		//Žg—p‚·‚éƒRƒ“ƒ|[ƒlƒ“ƒg‚ÌéŒ¾
		MidiKeyboardComponent midiKeyboard;
		Label TempoLabel;
		Slider gainSlider, delaySlider;
		TextButton Button_c1;
		TextButton Button_c2;
		TextButton Button_c3;
		TextButton Button_c4;
		TextButton Button_r1;
		TextButton Button_r2;
		TextButton Button_r3;
		TextButton Button_r4;
		TextButton Button_g1;
		TextButton Button_g2;
		TextButton Button_g3;
		TextButton Button_g4;
		TextButton Button_g5;
		TextButton Button_g6;
		TextButton Button_g7;
		TextButton Button_g8;
		TextButton Button_L;
		TextButton Button_R;

		Image image_background;

		int midiChannel = 10;
		double startTime;

		AudioProcessorValueTreeState::SliderAttachment gainAttachment, delayAttachment;
		Colour backgroundColour;

		// these are used to persist the UI's size - the values are stored along with the
		// filter's other parameters, and the UI component will update them when it gets
		// resized.
		Value lastUIWidth, lastUIHeight;

		//==============================================================================
		JuceDemoPluginAudioProcessor& getProcessor() const
		{
			return static_cast<JuceDemoPluginAudioProcessor&> (processor);
		}

		//==============================================================================
		// quick-and-dirty function to format a timecode string
		static String timeToTimecodeString(double seconds)
		{
			auto millisecs = roundToInt(seconds * 1000.0);
			auto absMillisecs = std::abs(millisecs);

			return String::formatted("%02d:%02d:%02d.%03d",
				millisecs / 3600000,
				(absMillisecs / 60000) % 60,
				(absMillisecs / 1000) % 60,
				absMillisecs % 1000);
		}

		// quick-and-dirty function to format a bars/beats string
		static String quarterNotePositionToBarsBeatsString(double quarterNotes, int numerator, int denominator)
		{
			if (numerator == 0 || denominator == 0)
				return "1|1|000";

			auto quarterNotesPerBar = (numerator * 4 / denominator);
			auto beats = (fmod(quarterNotes, quarterNotesPerBar) / quarterNotesPerBar) * numerator;

			auto bar = ((int)quarterNotes) / quarterNotesPerBar + 1;
			auto beat = ((int)beats) + 1;
			auto ticks = ((int)(fmod(beats, 1.0) * 960.0 + 0.5));

			return String::formatted("%d|%d|%03d", bar, beat, ticks);
		}

		// Updates the text in our position label.
		void updateTimecodeDisplay(AudioPlayHead::CurrentPositionInfo pos)
		{
			MemoryOutputStream displayText;
			MemoryOutputStream displayText2; //ƒeƒ“ƒ|

			displayText << pos.timeSigNumerator << '/' << pos.timeSigDenominator
				<< "  -  " << timeToTimecodeString(pos.timeInSeconds)
				<< "  -  " << quarterNotePositionToBarsBeatsString(pos.ppqPosition,
					pos.timeSigNumerator,
					pos.timeSigDenominator);
			displayText2 << " Tempo: " << String(pos.bpm, 2);

			if (pos.isRecording)
				displayText << "  (recording)";
			else if (pos.isPlaying)
				displayText << "  (playing)";

			timecodeDisplayLabel.setText(displayText.toString(), dontSendNotification);
			tempoDisplayLabel.setText(displayText2.toString(), dontSendNotification);
		}

		// called when the stored window size changes
		void valueChanged(Value&) override
		{
			setSize(800, 600);
		}
	};

	//==============================================================================


	template <typename FloatType>
	void applyGain(AudioBuffer<FloatType>& buffer, AudioBuffer<FloatType>& delayBuffer, float gainLevel)
	{
		ignoreUnused(delayBuffer);

		for (auto channel = 0; channel < getTotalNumOutputChannels(); ++channel)
			buffer.applyGain(channel, 0, buffer.getNumSamples(), gainLevel);
	}

	template <typename FloatType>
	void applyDelay(AudioBuffer<FloatType>& buffer, AudioBuffer<FloatType>& delayBuffer, float delayLevel)
	{
		auto numSamples = buffer.getNumSamples();

		auto delayPos = 0;

		for (auto channel = 0; channel < getTotalNumOutputChannels(); ++channel)
		{
			auto channelData = buffer.getWritePointer(channel);
			auto delayData = delayBuffer.getWritePointer(jmin(channel, delayBuffer.getNumChannels() - 1));
			delayPos = delayPosition;

			for (auto i = 0; i < numSamples; ++i)
			{
				auto in = channelData[i];
				channelData[i] += delayData[delayPos];
				delayData[delayPos] = (delayData[delayPos] + in) * delayLevel;

				if (++delayPos >= delayBuffer.getNumSamples())
					delayPos = 0;
			}
		}

		delayPosition = delayPos;
	}

	AudioBuffer<float> delayBufferFloat;
	AudioBuffer<double> delayBufferDouble;

	int delayPosition = 0;

	Synthesiser synth;

	CriticalSection trackPropertiesLock;
	TrackProperties trackProperties;



	void updateCurrentTimeInfoFromHost(int a[])
	{
		if (auto* ph = getPlayHead())
		{
			AudioPlayHead::CurrentPositionInfo newTime;

			if (ph->getCurrentPosition(newTime))
			{
				lastPosInfo = newTime;  // Successfully got the current time from the host..



				/*
				pos.ppqPosition,
					pos.timeSigNumerator,
					pos.timeSigDenominator*/

				auto quarterNotesPerBar = (newTime.timeSigNumerator * 4 / newTime.timeSigDenominator);
				auto beats = (fmod(newTime.ppqPosition, quarterNotesPerBar) / quarterNotesPerBar) * newTime.timeSigNumerator;

				int bar = (((((int)newTime.ppqPosition) / quarterNotesPerBar)) % 8); //0から7
				int beat = ((((int)beats)) % 16); //0から16

				a[1] = a[0];
				a[0] = bar;
				a[3] = a[2];
				a[2] = beat;


				return;
			}
		}

		// If the host fails to provide the current time, we'll just reset our copy to a default..
		lastPosInfo.resetToDefault();
	}

	static BusesProperties getBusesProperties()
	{
		return BusesProperties().withInput("Input", AudioChannelSet::stereo(), false)
			.withOutput("Output", AudioChannelSet::stereo(), true);
	}

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(JuceDemoPluginAudioProcessor);
};
