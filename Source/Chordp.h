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


//==============================================================================
/** A demo synth sound that's just a basic sine wave.. */
class SineWaveSound : public SynthesiserSound
{
public:
    SineWaveSound() {}

    bool appliesToNote (int /*midiNoteNumber*/) override    { return true; }
    bool appliesToChannel (int /*midiChannel*/) override    { return true; }
};

//==============================================================================
/** A simple demo synth voice that just plays a sine wave.. */
class SineWaveVoice   : public SynthesiserVoice
{
public:
    SineWaveVoice() {}

    bool canPlaySound (SynthesiserSound* sound) override
    {
        return dynamic_cast<SineWaveSound*> (sound) != nullptr;
    }

    void startNote (int midiNoteNumber, float velocity,
                    SynthesiserSound* /*sound*/,
                    int /*currentPitchWheelPosition*/) override
    {
        currentAngle = 0.0;
        level = velocity * 0.15;
        tailOff = 0.0;

        auto cyclesPerSecond = MidiMessage::getMidiNoteInHertz (midiNoteNumber);
        auto cyclesPerSample = cyclesPerSecond / getSampleRate();

        angleDelta = cyclesPerSample * MathConstants<double>::twoPi;
    }

    void stopNote (float /*velocity*/, bool allowTailOff) override
    {
        if (allowTailOff)
        {
            // start a tail-off by setting this flag. The render callback will pick up on
            // this and do a fade out, calling clearCurrentNote() when it's finished.

            if (tailOff == 0.0) // we only need to begin a tail-off if it's not already doing so - the
                                // stopNote method could be called more than once.
                tailOff = 1.0;
        }
        else
        {
            // we're being told to stop playing immediately, so reset everything..

            clearCurrentNote();
            angleDelta = 0.0;
        }
    }

    void pitchWheelMoved (int /*newValue*/) override
    {
        // not implemented for the purposes of this demo!
    }

    void controllerMoved (int /*controllerNumber*/, int /*newValue*/) override
    {
        // not implemented for the purposes of this demo!
    }

    void renderNextBlock (AudioBuffer<float>& outputBuffer, int startSample, int numSamples) override
    {
        if (angleDelta != 0.0)
        {
            if (tailOff > 0.0)
            {
                while (--numSamples >= 0)
                {
                    auto currentSample = (float) (sin (currentAngle) * level * tailOff);

                    for (auto i = outputBuffer.getNumChannels(); --i >= 0;)
                        outputBuffer.addSample (i, startSample, currentSample);

                    currentAngle += angleDelta;
                    ++startSample;

                    tailOff *= 0.99;

                    if (tailOff <= 0.005)
                    {
                        // tells the synth that this voice has stopped
                        clearCurrentNote();

                        angleDelta = 0.0;
                        break;
                    }
                }
            }
            else
            {
                while (--numSamples >= 0)
                {
                    auto currentSample = (float) (sin (currentAngle) * level);

                    for (auto i = outputBuffer.getNumChannels(); --i >= 0;)
                        outputBuffer.addSample (i, startSample, currentSample);

                    currentAngle += angleDelta;
                    ++startSample;
                }
            }
        }
    }

    using SynthesiserVoice::renderNextBlock;

private:
    double currentAngle = 0.0;
    double angleDelta   = 0.0;
    double level        = 0.0;
    double tailOff      = 0.0;


};

//==============================================================================
/** As the name suggest, this class does the actual audio processing. */
class JuceDemoPluginAudioProcessor  : public AudioProcessor
{

public:
    //==============================================================================
    JuceDemoPluginAudioProcessor()
        : AudioProcessor (getBusesProperties()),
          state (*this, nullptr, "state",
                 { std::make_unique<AudioParameterFloat> ("gain",  "Gain",           NormalisableRange<float> (0.0f, 1.0f), 0.9f),
                   std::make_unique<AudioParameterFloat> ("delay", "Delay Feedback", NormalisableRange<float> (0.0f, 1.0f), 0.5f) })
    {
        // Add a sub-tree to store the state of our UI
        state.state.addChild ({ "uiState", { { "width",  400 }, { "height", 200 } }, {} }, -1, nullptr);

        initialiseSynth();
    }

    ~JuceDemoPluginAudioProcessor() override = default;

    //==============================================================================
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override
    {
        // Only mono/stereo and input/output must have same layout
        const auto& mainOutput = layouts.getMainOutputChannelSet();
        const auto& mainInput  = layouts.getMainInputChannelSet();

        // input and output layout must either be the same or the input must be disabled altogether
        if (! mainInput.isDisabled() && mainInput != mainOutput)
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



    void prepareToPlay (double newSampleRate, int /*samplesPerBlock*/) override
    {
        // Use this method as the place to do any pre-playback
        // initialisation that you need..
        synth.setCurrentPlaybackSampleRate (newSampleRate);
        keyboardState.reset();

        if (isUsingDoublePrecision())
        {
            delayBufferDouble.setSize (2, 12000);
            delayBufferFloat .setSize (1, 1);
        }
        else
        {
            delayBufferFloat .setSize (2, 12000);
            delayBufferDouble.setSize (1, 1);
        }

        reset();

        juce::AudioParameterFloat* speed;
        int currentNote, lastNoteValue;
        int time;
        float rate;
        juce::SortedSet<int> notes;

        notes.clear();                          // [1]
        currentNote = 0;                        // [2]
        lastNoteValue = -1;                     // [3]
        time = 0;                               // [4]
        rate = static_cast<float> (newSampleRate); // [5]
    }

    void releaseResources() override
    {
        // When playback stops, you can use this as an opportunity to free up any
        // spare memory, etc.
        keyboardState.reset();
    }

    void reset() override
    {
        // Use this method as the place to clear any delay lines, buffers, etc, as it
        // means there's been a break in the audio's continuity.
        delayBufferFloat .clear();
        delayBufferDouble.clear();
    }
    
    //==============================================================================
    void processBlock (AudioBuffer<float>& buffer, MidiBuffer& midiMessages) override
    {
        
        jassert (! isUsingDoublePrecision());
        

        
        AudioPlayHead::CurrentPositionInfo pos = lastPosInfo.get();//Ä¶ˆÊ’u
        buffer.clear(); //ƒI[ƒfƒBƒIƒoƒbƒtƒ@‰Šú‰»
        midiMessages.clear();
        MidiBuffer processedMidi; 
        MidiMessage message;
        //MidiMessage message;
        auto beats = (fmod(pos.ppqPosition, pos.timeSigNumerator) / pos.timeSigNumerator) * pos.timeSigDenominator;

        auto bar = ((int)pos.ppqPosition) / pos.timeSigNumerator + 1;
        auto beat = ((int)beats) + 1;
        auto ticks = ((int)(fmod(beats, 1.0) * 960.0 + 0.5));
        





        /*
        if(pos.isPlaying){

            jassert(buffer.getNumChannels() == 0);                                                         // [6]

            // however we use the buffer to get timing information
            auto numSamples = buffer.getNumSamples();                                                       // [7]

            // get note duration
            auto noteDuration = static_cast<int> (std::ceil(rate * 0.25f * (0.1f + (1.0f - (*speed)))));   // [8]
            auto offset = juce::jmax(0, juce::jmin((int)(noteDuration - time), numSamples - 1));     // [12]
            
            if (beat == 3 && ticks == 0) {

                //processedMidi.addEvent(juce::MidiMessage::noteOff(1, 50), offset);
                //lastNoteValue = -1;
            
            }

            if (beat == 1 && ticks == 0) {
                currentNote = (currentNote + 1) % notes.size();
                lastNoteValue = notes[currentNote];
                processedMidi.addEvent(MidiMessage::noteOn(1, 50, (uint8)127), offset);

            }
            
            
        }
        */

        for (MidiBuffer::Iterator itr(midiMessages); itr.getNextEvent(message, time);) {
            if (message.isNoteOn()) {
            
            }
            processedMidi.addEvent(message, time);

            if (beats == 0) {

            }
        }


        midiMessages.swapWith(processedMidi);


            


        /*
        AudioPlayHead::CurrentPositionInfo pos = lastPosInfo.get();//Ä¶ˆÊ’u
        int noteNumber = 50;
        auto message = juce::MidiMessage::noteOn(1, noteNumber, (uint8)127);
        auto timestamp = message.getTimeStamp();
        double sampleRate = 44100.0;
        auto sampleNumber = (int)(timestamp * sampleRate);

       

        midiMessages.addEvent(message, sampleNumber);
        */

        
        process (buffer, midiMessages, delayBufferFloat);
        
    }



    //==============================================================================
    bool hasEditor() const override                                   { return true; }

    AudioProcessorEditor* createEditor() override
    {
        return new JuceDemoPluginAudioProcessorEditor (*this);
    }

    //==============================================================================
    const String getName() const override                             { return "AudioPluginDemo"; }
    bool acceptsMidi() const override                                 { return true; }
    bool producesMidi() const override                                { return true; }
    double getTailLengthSeconds() const override                      { return 0.0; }

    //==============================================================================
    int getNumPrograms() override                                     { return 0; }
    int getCurrentProgram() override                                  { return 0; }
    void setCurrentProgram (int) override                             {}
    const String getProgramName (int) override                        { return {}; }
    void changeProgramName (int, const String&) override              {}

    //==============================================================================
    void getStateInformation (MemoryBlock& destData) override
    {
        // Store an xml representation of our state.
        if (auto xmlState = state.copyState().createXml())
            copyXmlToBinary (*xmlState, destData);
    }

    void setStateInformation (const void* data, int sizeInBytes) override
    {
        // Restore our plug-in's state from the xml representation stored in the above
        // method.
        if (auto xmlState = getXmlFromBinary (data, sizeInBytes))
            state.replaceState (ValueTree::fromXml (*xmlState));
    }

    //==============================================================================
    void updateTrackProperties (const TrackProperties& properties) override
    {
        {
            const ScopedLock sl (trackPropertiesLock);
            trackProperties = properties;
        }

        MessageManager::callAsync ([this]
        {
            if (auto* editor = dynamic_cast<JuceDemoPluginAudioProcessorEditor*> (getActiveEditor()))
                 editor->updateTrackProperties();
        });
    }

    TrackProperties getTrackProperties() const
    {
        const ScopedLock sl (trackPropertiesLock);
        return trackProperties;
    }

    class SpinLockedPosInfo
    {
    public:
        SpinLockedPosInfo() { info.resetToDefault(); }

        // Wait-free, but setting new info may fail if the main thread is currently
        // calling `get`. This is unlikely to matter in practice because
        // we'll be calling `set` much more frequently than `get`.
        void set (const AudioPlayHead::CurrentPositionInfo& newInfo)
        {
            const juce::SpinLock::ScopedTryLockType lock (mutex);

            if (lock.isLocked())
                info = newInfo;
        }

        AudioPlayHead::CurrentPositionInfo get() const noexcept
        {
            const juce::SpinLock::ScopedLockType lock (mutex);
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

    // this is kept up to date with the midi messages that arrive, and the UI component
    // registers with it so it can represent the incoming messages
    MidiKeyboardState keyboardState;

    // this keeps a copy of the last set of time info that was acquired during an audio
    // callback - the UI component will read this and display it.
    SpinLockedPosInfo lastPosInfo;

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

        int Chord_Value[8][2] = { {0,0},{7,0},{9,1},{4,1},{3,0},{0,0},{4,1},{7,0} };
        int Page = 0;
        const String Chord_Name[12] = { "C","C#","D" ,"D#" ,"E" ,"F" ,"F#" ,"G" ,"G#" ,"A" ,"A#" ,"B" };
        const String Chord_Type[4] = { "","m","M7","m7" };

        const int Chord_M[3] = { 0,4,7};
        const int Chord_m[3] = { 0,3,7};
        const int Chord_M7[4] = { 0,4,7,11 };
        const int Chord_m7[4] = { 0,3,7,10 };

        JuceDemoPluginAudioProcessorEditor (JuceDemoPluginAudioProcessor& owner)
            : AudioProcessorEditor (owner),
              midiKeyboard         (owner.keyboardState, MidiKeyboardComponent::horizontalKeyboard),
              gainAttachment       (owner.state, "gain",  gainSlider),
              delayAttachment      (owner.state, "delay", delaySlider)
        {

            //Using Button Attach
            addAndMakeVisible(Button_c1);
            Button_c1.setButtonText(Chord_Name[Chord_Value[0+Page][0]]  + Chord_Type[Chord_Value[0+Page][1]]);
            Button_c1.onClick = [this] { setNoteNumber(36); };

            addAndMakeVisible(Button_c2);
            Button_c2.setButtonText(Chord_Name[Chord_Value[1 + Page][0]] + Chord_Type[Chord_Value[1 + Page][1]]);
            Button_c2.onClick = [this] { setNoteNumber(36); };

            addAndMakeVisible(Button_c3);
            Button_c3.setButtonText(Chord_Name[Chord_Value[2 + Page][0]] + Chord_Type[Chord_Value[2 + Page][1]]);
            Button_c3.onClick = [this] { setNoteNumber(36); };

            addAndMakeVisible(Button_c4);
            Button_c4.setButtonText(Chord_Name[Chord_Value[3 + Page][0]] + Chord_Type[Chord_Value[3 + Page][1]]);
            Button_c4.onClick = [this] { setNoteNumber(36); };

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

            addAndMakeVisible(Button_g2);
            Button_g2.setButtonText("Rock");

            addAndMakeVisible(Button_g3);
            Button_g3.setButtonText("Jazz");

            addAndMakeVisible(Button_g4);
            Button_g4.setButtonText("EDM");

            addAndMakeVisible(Button_g5);
            Button_g5.setButtonText("Idol");

            addAndMakeVisible(Button_g6);
            Button_g6.setButtonText("Ballade");

            addAndMakeVisible(Button_g7);
            Button_g7.setButtonText("Anime");

            addAndMakeVisible(Button_g8);
            Button_g8.setButtonText("Game");

            addAndMakeVisible(Button_L);
            Button_L.setButtonText("<");

            addAndMakeVisible(Button_R);
            Button_R.setButtonText(">");

            addAndMakeVisible(tempoDisplayLabel);
            TempoLabel.setFont(Font(Font::getDefaultMonospacedFontName(), 15.0f, Font::plain));

            // add some sliders..
            addAndMakeVisible (gainSlider);
            gainSlider.setSliderStyle (Slider::Rotary);

            addAndMakeVisible (delaySlider);
            delaySlider.setSliderStyle (Slider::Rotary);

            // add the midi keyboard component..
            addAndMakeVisible (midiKeyboard);

            // add a label that will display the current timecode and status..
            addAndMakeVisible (timecodeDisplayLabel);
            timecodeDisplayLabel.setFont (Font (Font::getDefaultMonospacedFontName(), 15.0f, Font::plain));

            // set resize limits for this plug-in
            setResizeLimits (800, 600, 800, 600);

            lastUIWidth .referTo (owner.state.state.getChildWithName ("uiState").getPropertyAsValue ("width",  nullptr));
            lastUIHeight.referTo (owner.state.state.getChildWithName ("uiState").getPropertyAsValue ("height", nullptr));

            // set our component's initial size to be the last one that was stored in the filter's settings
            
            //setSize (lastUIWidth.getValue(), lastUIHeight.getValue());

            lastUIWidth. addListener (this);
            lastUIHeight.addListener (this);

            updateTrackProperties();

            // start a timer which will keep our timecode display updated
            startTimerHz (30);
        }

        ~JuceDemoPluginAudioProcessorEditor() override {}

        //==============================================================================
       //背景の描画
        void paint (Graphics& g) override
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
            
            

            auto r = getLocalBounds().reduced (8);
            
            //ヘッダ部分
            auto headerArea = r.removeFromTop(80);

            //楽譜部分
            auto scoreArea = r.removeFromTop(160);


            //¶‰E‚Ìƒ{ƒ^ƒ“
            auto sideWidth = 25;
            Button_L.setBounds(r.removeFromLeft(sideWidth));
            Button_R.setBounds(r.removeFromRight(sideWidth));


            auto margin1 = r.removeFromTop(30);

            //ƒR[ƒhƒ{ƒ^ƒ“
            auto chordArea = r.removeFromTop(70);
            Button_c1.setBounds(chordArea.removeFromLeft(chordArea.getWidth()/4));
            Button_c2.setBounds(chordArea.removeFromLeft(chordArea.getWidth() / 3));
            Button_c3.setBounds(chordArea.removeFromLeft(chordArea.getWidth() / 2));
            Button_c4.setBounds(chordArea.removeFromLeft(chordArea.getWidth() / 1));

            auto margin2 = r.removeFromTop(10);


            //ƒŠƒYƒ€ƒ{ƒ^ƒ“
            auto rythmArea = r.removeFromTop(40);
            Button_r1.setBounds(rythmArea.removeFromLeft(rythmArea.getWidth()/4));
            Button_r2.setBounds(rythmArea.removeFromLeft(rythmArea.getWidth()/3));
            Button_r3.setBounds(rythmArea.removeFromLeft(rythmArea.getWidth()/2));
            Button_r4.setBounds(rythmArea.removeFromLeft(rythmArea.getWidth()/1));


            auto margin3 = r.removeFromTop(10);

            //ƒWƒƒƒ“ƒ‹•”•ª
            auto genreArea = r.removeFromLeft(r.getWidth() / 2);
            auto genrerow1 = genreArea.removeFromTop(genreArea.getHeight() / 2);
            auto genrerow2 = genreArea.removeFromTop(genreArea.getHeight() / 1);
            Button_g1.setBounds(genrerow1.removeFromLeft(genrerow1.getWidth()/4));
            Button_g2.setBounds(genrerow1.removeFromLeft(genrerow1.getWidth()/3));
            Button_g3.setBounds(genrerow1.removeFromLeft(genrerow1.getWidth()/2));
            Button_g4.setBounds(genrerow1.removeFromLeft(genrerow1.getWidth()/1));
            Button_g5.setBounds(genrerow2.removeFromLeft(genrerow2.getWidth()/4));
            Button_g6.setBounds(genrerow2.removeFromLeft(genrerow2.getWidth()/3));
            Button_g7.setBounds(genrerow2.removeFromLeft(genrerow2.getWidth()/2));
            Button_g8.setBounds(genrerow2.removeFromLeft(genrerow2.getWidth()/1));

            
            //ƒeƒ“ƒ|‚È‚Ç•\Ž¦
            auto stateArea = r.removeFromLeft(r.getWidth() / 1);
            auto staterow1 = stateArea.removeFromTop(stateArea.getHeight() / 2);
            auto staterow2 = stateArea.removeFromTop(stateArea.getHeight());
            
            timecodeDisplayLabel.setBounds(staterow1.removeFromLeft(staterow1.getWidth()));  
            tempoDisplayLabel.setBounds(staterow2.removeFromLeft(staterow2.getWidth()));

            auto sliderArea = r.removeFromTop (60);
            //gainSlider.setBounds  (sliderArea.removeFromLeft (jmin (180, sliderArea.getWidth() / 2)));
            //delaySlider.setBounds (sliderArea.removeFromLeft (jmin (180, sliderArea.getWidth())));
            //midiKeyboard.setBounds(r.removeFromBottom(70));


            //ƒ{ƒgƒ€•”•ª


            lastUIWidth  = getWidth();
            lastUIHeight = getHeight();
            
        }

        void timerCallback() override
        {
            updateTimecodeDisplay (getProcessor().lastPosInfo.get());
        }

        void hostMIDIControllerIsAvailable (bool controllerIsAvailable) override
        {
            midiKeyboard.setVisible (! controllerIsAvailable);
        }

        int getControlParameterIndex (Component& control) override
        {
            if (&control == &gainSlider)
                return 0;

            if (&control == &delaySlider)
                return 1;

            return -1;
        }

        void updateTrackProperties()
        {
            auto trackColour = getProcessor().getTrackProperties().colour;
            auto& lf = getLookAndFeel();

            backgroundColour = (trackColour == Colour() ? lf.findColour (ResizableWindow::backgroundColourId)
                                                        : trackColour.withAlpha (1.0f).withBrightness (0.266f));
            repaint();
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

        void setNoteNumber(int noteNumber)
        {
            auto message = juce::MidiMessage::noteOn(midiChannel, noteNumber, (juce::uint8) 100);
            message.setTimeStamp(juce::Time::getMillisecondCounterHiRes() * 0.001 - startTime);
        }



        void buttonClicked(Button* clickedButton) {





        }








        MidiKeyboardComponent midiKeyboard;

        Label timecodeDisplayLabel, tempoDisplayLabel,
              gainLabel  { {}, "Throughput level:" },
              delayLabel { {}, "Delay:" };

        //Žg—p‚·‚éƒRƒ“ƒ|[ƒlƒ“ƒg‚ÌéŒ¾
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
        static String timeToTimecodeString (double seconds)
        {
            auto millisecs = roundToInt (seconds * 1000.0);
            auto absMillisecs = std::abs (millisecs);

            return String::formatted ("%02d:%02d:%02d.%03d",
                                      millisecs / 3600000,
                                      (absMillisecs / 60000) % 60,
                                      (absMillisecs / 1000)  % 60,
                                      absMillisecs % 1000);
        }

        // quick-and-dirty function to format a bars/beats string
        static String quarterNotePositionToBarsBeatsString (double quarterNotes, int numerator, int denominator)
        {
            if (numerator == 0 || denominator == 0)
                return "1|1|000";

            auto quarterNotesPerBar = (numerator * 4 / denominator);
            auto beats  = (fmod (quarterNotes, quarterNotesPerBar) / quarterNotesPerBar) * numerator;

            auto bar    = ((int) quarterNotes) / quarterNotesPerBar + 1;
            auto beat   = ((int) beats) + 1;
            auto ticks  = ((int) (fmod (beats, 1.0) * 960.0 + 0.5));

            return String::formatted ("%d|%d|%03d", bar, beat, ticks);
        }

        // Updates the text in our position label.
        void updateTimecodeDisplay (AudioPlayHead::CurrentPositionInfo pos)
        {
            MemoryOutputStream displayText;
            MemoryOutputStream displayText2; //ƒeƒ“ƒ|

            displayText  << pos.timeSigNumerator << '/' << pos.timeSigDenominator
            << "  -  " << timeToTimecodeString (pos.timeInSeconds)
            << "  -  " << quarterNotePositionToBarsBeatsString (pos.ppqPosition,
                                                                pos.timeSigNumerator,
                                                                pos.timeSigDenominator);
            displayText2 << " Tempo: " << String(pos.bpm, 2);

            if (pos.isRecording)
                displayText << "  (recording)";
            else if (pos.isPlaying)
                displayText << "  (playing)";

            timecodeDisplayLabel.setText (displayText.toString(), dontSendNotification);
            tempoDisplayLabel.setText(displayText2.toString(), dontSendNotification);
        }

        // called when the stored window size changes
        void valueChanged (Value&) override
        {
            setSize (800,600);
        }
    };

    //==============================================================================
    template <typename FloatType>
    void process (AudioBuffer<FloatType>& buffer, MidiBuffer& midiMessages, AudioBuffer<FloatType>& delayBuffer)
    {
        /*
        auto gainParamValue  = state.getParameter ("gain") ->getValue();
        auto delayParamValue = state.getParameter ("delay")->getValue();
        auto numSamples = buffer.getNumSamples();

        // In case we have more outputs than inputs, we'll clear any output
        // channels that didn't contain input data, (because these aren't
        // guaranteed to be empty - they may contain garbage).
        for (auto i = getTotalNumInputChannels(); i < getTotalNumOutputChannels(); ++i)
            buffer.clear (i, 0, numSamples);

        // Now pass any incoming midi messages to our keyboard state object, and let it
        // add messages to the buffer if the user is clicking on the on-screen keys
        keyboardState.processNextMidiBuffer (midiMessages, 0, numSamples, true);

        // and now get our synth to process these midi events and generate its output.
        synth.renderNextBlock (buffer, midiMessages, 0, numSamples);

        // Apply our delay effect to the new output..
        applyDelay (buffer, delayBuffer, delayParamValue);

        // Apply our gain change to the outgoing data..
        applyGain (buffer, delayBuffer, gainParamValue);
        */

        // Now ask the host for the current time so we can store it to be displayed later...
        updateCurrentTimeInfoFromHost();
    }

    template <typename FloatType>
    void applyGain (AudioBuffer<FloatType>& buffer, AudioBuffer<FloatType>& delayBuffer, float gainLevel)
    {
        ignoreUnused (delayBuffer);

        for (auto channel = 0; channel < getTotalNumOutputChannels(); ++channel)
            buffer.applyGain (channel, 0, buffer.getNumSamples(), gainLevel);
    }

    template <typename FloatType>
    void applyDelay (AudioBuffer<FloatType>& buffer, AudioBuffer<FloatType>& delayBuffer, float delayLevel)
    {
        auto numSamples = buffer.getNumSamples();

        auto delayPos = 0;

        for (auto channel = 0; channel < getTotalNumOutputChannels(); ++channel)
        {
            auto channelData = buffer.getWritePointer (channel);
            auto delayData = delayBuffer.getWritePointer (jmin (channel, delayBuffer.getNumChannels() - 1));
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

    void initialiseSynth()
    {
        auto numVoices = 8;

        // Add some voices...
        for (auto i = 0; i < numVoices; ++i)
            synth.addVoice (new SineWaveVoice());

        // ..and give the synth a sound to play
        synth.addSound (new SineWaveSound());
    }

    void updateCurrentTimeInfoFromHost()
    {
        const auto newInfo = [&]
        {
            if (auto* ph = getPlayHead())
            {
                AudioPlayHead::CurrentPositionInfo result;

                if (ph->getCurrentPosition (result))
                    return result;
            }

            // If the host fails to provide the current time, we'll just use default values
            AudioPlayHead::CurrentPositionInfo result;
            result.resetToDefault();
            return result;
        }();

        lastPosInfo.set (newInfo);
    }

    static BusesProperties getBusesProperties()
    {
        return BusesProperties().withInput  ("Input",  AudioChannelSet::stereo(), false)
                                .withOutput ("Output", AudioChannelSet::stereo(), true);
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(JuceDemoPluginAudioProcessor);
};
