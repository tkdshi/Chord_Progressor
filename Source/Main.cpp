/*
  ==============================================================================

    This file was auto-generated and contains the startup code for a PIP.

  ==============================================================================
*/

#include <JuceHeader.h>
#include "Chordp.h"

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{

#if JUCE_WINDOWS
    String typeFaceName = "Meiryo UI";
    Desktop::getInstance().getDefaultLookAndFeel().setDefaultSansSerifTypefaceName(typeFaceName);
#elif JUCE_MAC
    String typeFaceName = "Arial Unicode MS";
    Desktop::getInstance().getDefaultLookAndFeel().setDefaultSansSerifTypefaceName(typeFaceName);
#endif

    return new JuceDemoPluginAudioProcessor();
}
