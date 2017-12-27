/*
Copyright (c) 2006-2007 Johan Sarge

Permission is hereby granted, free of charge, to any person obtaining a copy of this
software and associated documentation files (the "Software"), to deal in the Software
without restriction, including without limitation the rights to use, copy, modify, merge,
publish, distribute, sublicense, and/or sell copies of the Software, and to permit
persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or
substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE
FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.
*/

#include "WavePlug.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <new>
#include <stdexcept>
#include "wpfunc.hpp"
#ifndef WP_NO_GUI
#include "WavePlugEditor.hpp"
#endif

// Parameter transform macros.
#define BUFFER_SIZE_T(v) (1 << (int) (7.0f*(v) + 0.5f))

#define INC_LAG_T(v) (std::pow(std::log10(9.0f*(v) + 1.0f), 0.2f))
#define DEC_LAG_T(v) (std::pow(std::log10(9.0f*(v) + 1.0f), 0.01f))
#define A_GATE_LVL_T(v) ((std::pow(2.0f, 12.0f*(v)) - 1.0f)/4095.0f)
#define S_GATE_LVL_T(v) A_GATE_LVL_T(v)
#define TRIG_LVL_T(v) (3.0f*((v)-0.5f))
#define F_MIN_T(v) ((globalMaxFrequency - 1.0f)*(std::pow(2.0f, 9.0f*(v)) - 1.0f)/511.0f + 1.0f)
#define F_MAX_T(v) F_MIN_T(v)
#define F_LAG_T(v) (std::pow(std::log10(9.0f*(v) + 1.0f), 0.4f))
#define W_LAG_T(v) F_LAG_T(v)
#define BOOL_T(v) ((v) > 0.5f)
#define U_MOD_TYPE_T(v) ((ModulationTypeU) (unsigned int) (0.999f*((v)*kNModTypesU)))
#define F_MOD_TYPE_T(v) ((ModulationTypeF) (unsigned int) (0.999f*((v)*kNModTypesF)))
#define A_OFFSET_T(v) (2.0f*((v)-0.5f))
#define A_GAIN_T(v) (((v) > 0.5f) \
                     ? ((((v) > 0.9999f)) ? 5000.0f : 1.0f / (2.0f*(1.0f - (v)))) \
                     : 2.0f*(v))
#define F_OFFSET_T(v) (((v) > 0.5f) \
                       ? (std::pow(2.0f, 32.0f*((v)-0.5f)) - 1.0f)/65535.0f \
                       : -(std::pow(2.0f, 32.0f*(0.5f-(v))) - 1.0f)/65535.0f)
#define F_GAIN_T(v) (((v) > 0.5f) \
                     ? ((((v) > 0.9999f)) ? 5000.0f : 1.0f / (2.0f*(1.0f - (v)))) \
                     : 2.0f*(v))
#define OVER_T(v) (1 + (int) (15.0f*(v)))
#define WINDOW_T(v) ((int) (250.0f*(v)))
#define S_MOD_TYPE_T(v) ((ModulationTypeS) (unsigned int) (0.999f*((v)*kNModTypesS)))

#define M_LN2 0.69314718055994530942

// Macro for standard processing method.
#define PROC_METHOD(ioStatements) \
(float *in0, float *in1, float *out0, float *out1, int sampleFrames) { \
	while (--sampleFrames >= 0) { \
		{ioStatements} \
		syn1.tick(); syn2.tick(); \
	} \
}

// Private static data.
const char *const WavePlug::paramNames[kNumParams] = {
	"AIncLag", "ADecLag", "GatLvlA", "GatLvlS", "HiTrig", "LowTrig",
	"FMin", "FMax", "FLag", "WLag", "InvTrig", "Interp",
	"AModTyp", "AModMix", "FModTyp", "FModMix", "WModTyp", "WModMix",
	"AOffset", "AGain", "FOffset", "FGain", "Oversmp", "SmooWin",
	"OModTyp", "OModMix"
};

const char *const WavePlug::paramLabels[kNumParams] = {
	"%", "%", "%", "%", "%", "%",
	"Hz", "Hz", "%", "%", "", "",
	"type", "%", "type", "%", "type", "%",
	"%", "dB", "Hz", "octaves", "", "samples",
	"type", "%"
};

const char *const WavePlug::initParamHelpTexts[kNumParams] = {
	"How quickly output amplitude reacts to an increase in AIL.", // 0
	"How quickly output amplitude reacts to a decrease in AIL.",
	"Minimum output amplitude for F&W updates.",
	"Minimum input signal level for F&W updates.",
	"RIL must go above this value to trigger a F&W update.",
	"RIL must go below this value to trigger a F&W update.",
	"Lowest frequency detected by F&W update trigger.",
	"Highest frequency detected by F&W update trigger.",
	"How quickly the output frequency reacts to changes in detected frequency.",
	"How quickly the output waveform reacts to changes in detected waveform.",
	"Toggles between standard and inverted F&W update trigger sequence.",
	"Turns waveform output interpolation off/on.",
	
	"", // 12
	"",
	
	"", // 14
	"",
	
	"", // 16
	"",
	
	"Constant term added to input amplitude.", // 18
	"Amount of gain applied to input amplitude.",
	"Constant term added to input frequency.",
	"Amount of gain applied to input frequency.",
	"Input waveform oversampling rate.",
	"Size of output smoothing window.",
	
	"", // 24
	""
};

const char *const WavePlug::modFuncUHelpTexts[kNModTypesU][2] = {
	{
		"Mixes input 1 with input 2.",
		"Amount of input 2 to mix in."
	},
	{
		"Mixes the lowest input with the absolute difference of the inputs.",
		"Amount of absolute difference of inputs to mix in."
	},
	{
		"Mixes the highest input with the absolute difference of the inputs.",
		"Amount of absolute difference of inputs to mix in."
	},
	{
		"Adds a mix of the fine tuning of the inputs to the order of magnitude of input 1.",
		"Amount of the fine tuning of input 2 to mix in."
	},
	{
		"Mixes input 1 with the product of the inputs.",
		"Amount of input product to mix in."
	},
	{
		"Distorts input 1. The amount of distortion is controlled by input 2.",
		"How quickly the distortion of input 1 increases with input 2."
	},
	{
		"Mixes in input 2 in the range between input 1 and the max level.",
		"Fraction of the range between input 1 and the max level to fill with input 2."
	},
	{
		"Bounces between input 1 and input 2.",
		"Bounce rate."
	}
};

const char *const WavePlug::modFuncSHelpTexts[kNModTypesS][2] = {
	{
		"Mixes input 1 with input 2.",
		"Amount of input 2 to mix in."
	},
	{
		"Mixes input 1 with input 2 inverted.",
		"Amount of inverted input 2 to mix in."
	},
	{
		"Mixes input 1 with the product of the inputs.",
		"Amount of input product to mix in."
	},
	{
		"Distorts input 1. The amount of distortion is controlled by input 2.",
		"How quickly the distortion of input 1 increases with input 2."
	},
	{
		"Mixes in input 2 in the range between input 1 and the max level.",
		"Fraction of the range between input 1 and the max level to fill with input 2."
	},
	{
		"Bounces between input 1 and input 2.",
		"Bounce rate."
	}
};

const char *const WavePlug::modFuncFHelpTexts[kNModTypesF][2] = {
	{
		"Mixes input 1 with input 2.",
		"Amount of input 2 to mix in."
	},
	{
		"Mixes input 1 with input 2 inverted.",
		"Amount of inverted input 2 to mix in."
	},
	{
		"Mixes input 1 with the product of the inputs.",
		"Amount of input product to mix in."
	},
	{
		"Distorts input 1. The amount of distortion is controlled by input 2.",
		"How quickly the distortion of input 1 increases with input 2."
	},
	{
		"Mixes in input 2 in the range between input 1 and the max level.",
		"Fraction of the range between input 1 and the max level to fill with input 2."
	},
	{
		"Mixes input 1 with the functional composition of input1 with input2.",
		"Amount of input functional composition to mix in."
	},
	{
		"Filters input 1. Uses input2 as a finite filter impulse response.",
		"Length of filter impulse response."
	},
	{
		"Bounces between input 1 and input 2.",
		"Bounce rate."
	}
};

const float WavePlug::initParamValues[kNumParams] = {
	0.1f, 0.8f, 0.05f, 0.75f, 0.783f, 0.217f, 0.00055f, 0.73f, 0.0f, 0.0f, 0.0f, 1.0f,
	0.0f, 0.0f,
	0.0f, 0.0f,
	0.0f, 0.0f,
	0.5f, 0.5f, 0.5f, 0.5f, 0.0f, 0.0f,
	0.0f, 0.0f
};

const WavePlug::funcfcp WavePlug::paramDisplayers[kNumParams] = {
	&WavePlug::aIncLagDisplayer,
	&WavePlug::aDecLagDisplayer,
	&WavePlug::gateLevelDisplayer,
	&WavePlug::gateLevelDisplayer,
	&WavePlug::trigLevelDisplayer,
	&WavePlug::trigLevelDisplayer,
	&WavePlug::fMinMaxDisplayer,
	&WavePlug::fMinMaxDisplayer,
	&WavePlug::fwLagDisplayer,
	&WavePlug::fwLagDisplayer,
	&WavePlug::onOffDisplayer,
	&WavePlug::onOffDisplayer,
	
	&WavePlug::usignModTypeDisplayer,
	&WavePlug::pcntDisplayer,
	
	&WavePlug::usignModTypeDisplayer,
	&WavePlug::pcntDisplayer,
	
	&WavePlug::funcModTypeDisplayer,
	&WavePlug::pcntDisplayer,
	
	&WavePlug::aOffsetDisplayer,
	&WavePlug::aGainDisplayer,
	&WavePlug::fOffsetDisplayer,
	&WavePlug::fGainDisplayer,
	&WavePlug::oversamplingDisplayer,
	&WavePlug::smoothingWinDisplayer,
	
	&WavePlug::signModTypeDisplayer,
	&WavePlug::pcntDisplayer
};

const WavePlug::methodf WavePlug::paramSetters[kNumParams] = {
	&WavePlug::aAIWSetter,
	&WavePlug::aADWSetter,
	&WavePlug::aAGLSetter,
	&WavePlug::aSGLSetter,
	&WavePlug::aFHTSetter,
	&WavePlug::aFLTSetter,
	&WavePlug::aFMinSetter,
	&WavePlug::aFMaxSetter,
	&WavePlug::aFWSetter,
	&WavePlug::aWWSetter,
	&WavePlug::aITSetter,
	&WavePlug::aWISetter,
	
	&WavePlug::mAModSetter,
	&WavePlug::mAMixSetter,
	
	&WavePlug::mFModSetter,
	&WavePlug::mFMixSetter,
	
	&WavePlug::mWModSetter,
	&WavePlug::mWMixSetter,
	
	&WavePlug::sAOffSetter,
	&WavePlug::sAGainSetter,
	&WavePlug::sFOffSetter,
	&WavePlug::sFGainSetter,
	&WavePlug::sOverSetter,
	&WavePlug::sWindSetter,
	
	&WavePlug::mOModSetter,
	&WavePlug::mOMixSetter
};

const WavePlug::method4fpi WavePlug::procHandlers[9] = {
	&WavePlug::proc1In1Out,
	&WavePlug::proc1In1OutB,
	&WavePlug::proc1In2Out,
	&WavePlug::proc1In2OutB,
	&WavePlug::proc2In1Out,
	&WavePlug::proc2In1OutB,
	&WavePlug::proc2In2Out,
	&WavePlug::proc2In2OutB,
	
	&WavePlug::procDoNothing // Fallback handler for error states.
};

const WavePlug::method4fpi WavePlug::procRHandlers[9] = {
	&WavePlug::procR1In1Out,
	&WavePlug::procR1In1OutB,
	&WavePlug::procR1In2Out,
	&WavePlug::procR1In2OutB,
	&WavePlug::procR2In1Out,
	&WavePlug::procR2In1OutB,
	&WavePlug::procR2In2Out,
	&WavePlug::procR2In2OutB,
	
	&WavePlug::procDoNothing // Fallback handler for error states.
};


// Displayers.
void WavePlug::bufferSizeDisplayer(float value, char *text) {
	std::sprintf(text, "%i", BUFFER_SIZE_T(value));
}

void WavePlug::bufferSizeKByteDisplayer(float value, char *text) {
	int multiplier = BUFFER_SIZE_T(value);
	float sRateModifier = globalSampleRate / WP_STD_SAMPLE_RATE;
	int kBytes = (int) (sizeof(float) * 2.0f * multiplier * sRateModifier *
	                    (2.0f * WP_ANA_BUFFER_SIZE + 1.5f * WP_SYN_BUFFER_SIZE) / 1024.0f);
	std::sprintf(text, "%i", kBytes);
}

void WavePlug::bufferSizeHzDisplayer(float value, char *text) {
	int multiplier = BUFFER_SIZE_T(value);
	int hz = (int) (1.0f / (multiplier * (WP_ANA_BUFFER_SIZE / WP_STD_SAMPLE_RATE)));
	std::sprintf(text, "%i", hz);
}

void WavePlug::bufferSizeMillisDisplayer(float value, char *text) {
	int multiplier = BUFFER_SIZE_T(value);
	int ms = (int) (1000.0f * multiplier * (WP_ANA_BUFFER_SIZE / WP_STD_SAMPLE_RATE));
	std::sprintf(text, "%i", ms);
}

void WavePlug::onOffDisplayer(float value, char *text) {
	std::strcpy(text, BOOL_T(value) ? "On" : "Off");
}

void WavePlug::pcntDisplayer(float value, char *text) {
	std::sprintf(text, "%1.2f", 100.0f*value);
}

void WavePlug::aIncLagDisplayer(float value, char *text) {
	std::sprintf(text, "%1.3f", 100.0f*INC_LAG_T(value));
}

void WavePlug::aDecLagDisplayer(float value, char *text) {
	std::sprintf(text, "%1.3f", 100.0f*DEC_LAG_T(value));
}

void WavePlug::gateLevelDisplayer(float value, char *text) {
	std::sprintf(text, "%1.2f", 100.0f*A_GATE_LVL_T(value));
}

void WavePlug::trigLevelDisplayer(float value, char *text) {
	std::sprintf(text, "%1.2f", 100.0f*TRIG_LVL_T(value));
}

void WavePlug::fMinMaxDisplayer(float value, char *text) {
	std::sprintf(text, "%1.2f", F_MIN_T(value));
}

void WavePlug::fwLagDisplayer(float value, char *text) {
	std::sprintf(text, "%1.3f", 100.0f*F_LAG_T(value));
}

void WavePlug::usignModTypeDisplayer(float value, char *text) {
	std::strcpy(text, modTypeUNames[U_MOD_TYPE_T(value)]);
}

void WavePlug::signModTypeDisplayer(float value, char *text) {
	std::strcpy(text, modTypeSNames[S_MOD_TYPE_T(value)]);
}

void WavePlug::funcModTypeDisplayer(float value, char *text) {
	std::strcpy(text, modTypeFNames[F_MOD_TYPE_T(value)]);
}

void WavePlug::aOffsetDisplayer(float value, char *text) {
	std::sprintf(text, "%1.2f", 100.0f*A_OFFSET_T(value));
}

void WavePlug::aGainDisplayer(float value, char *text) {
	float value2 = A_GAIN_T(value);
	if (value2 == 0.0f)
		std::strcpy(text, "Quiet");
	else
		std::sprintf(text, "%1.2f", 20.0f*std::log10(value2));
}

void WavePlug::fOffsetDisplayer(float value, char *text) {
	float value2 = F_OFFSET_T(value);
	std::sprintf(text, "%1.2f", globalMaxFrequency*value2);
}

void WavePlug::fGainDisplayer(float value, char *text) {
	float value2 = F_GAIN_T(value);
	if (value2 == 0.0f)
		std::strcpy(text, "Min");
	else
		std::sprintf(text, "%1.3f", std::log(value2)/M_LN2);
}

void WavePlug::oversamplingDisplayer(float value, char *text) {
	std::sprintf(text, "%i", OVER_T(value));
}

void WavePlug::smoothingWinDisplayer(float value, char *text) {
	std::sprintf(text, "%i", WINDOW_T(value));
}


// Constructor.
WavePlug::WavePlug(audioMasterCallback audioMaster) :
	AudioEffectX(audioMaster, 1, kNumAllParams), BufferManager()
{
	// NOTE: This critical section visit is only meaningful on a multiprocessor.
	// It ensures that the initial values stored in the memory locations
	// containing the instance fields of the WavePlug object
	// are seen by all threads. (There might have been something in
	// that part of the address space before the constructor was called
	// and parts of the old data might still be cached by some processors.)
	EnterCriticalSection(&myCriticalSection);
	
	// Set plugin properties.
	setNumInputs(2); // Stereo in.
	setNumOutputs(2); // Stereo out.
	setUniqueID(CCONST('C','Q','C','Q')); // Identify.
	//canMono(); // Mono-to-stereo operation possible.
	canProcessReplacing(); // Supports both accumulating and replacing output.
	std::strcpy(programName, "Default");	// Default program name.
	
	// Set initial plugin info.
	paramHelpTexts[kPlugVersion] = "Plugin version number.";
	paramHelpTexts[kBufferSize] = "Sample buffer size multiplier.";
	std::memcpy(
		paramHelpTexts + kNumMonoParams,
		initParamHelpTexts, kNumParams * sizeof (const char *));
	std::memcpy(
		paramHelpTexts + kNumMonoParams + kNumParams,
		initParamHelpTexts, kNumParams * sizeof (const char *));
	
	// Set initial plugin configuration.
	sharedData.operational = true;
	sharedData.bypassedFlag = false;
	sharedData.input0Connected = sharedData.output0Connected = 1;
	sharedData.input1Connected = sharedData.output1Connected = 0;
	sharedData.bufferSizeMultiplier = 1;
	
	// Set initial parameter and signal monitor values.
	sharedData.paramValues[kPlugVersion] = 0.0f;
	sharedData.paramValues[kBufferSize] = 2.0f / 7.0f;
	std::memcpy(
		sharedData.paramValues + kNumMonoParams,
		initParamValues, kNumParams * sizeof (float));
	std::memcpy(
		sharedData.paramValues + kNumMonoParams + kNumParams,
		initParamValues, kNumParams * sizeof (float));
	
	sharedData.preA1 = sharedData.postA1 = sharedData.preF1 = sharedData.postF1 =
	sharedData.preA2 = sharedData.postA2 = sharedData.preF2 = sharedData.postF2 = 0.0f;
	
	// Tell processing thread to initialize its private data.
	sharedData.reinitFlag = true; // This is SUPER IMPORTANT!
	sharedData.resetFlag = false;
	sharedData.setProcessHandlersFlag = false;
	sharedData.newSampleRate = NAN;
	std::memcpy(
		sharedData.newParamValues, sharedData.paramValues, kNumAllParams * sizeof (float));
	
	// Create GUI editor (if GUI build).
	editor = NULL;
	
#ifndef WP_NO_GUI
	try {
		editor = new WavePlugEditor(this);
	}
	catch (std::bad_alloc e) { // Editor allocation failed.
		sharedData.operational = false;
	}
#endif
	
	// Done.
	LeaveCriticalSection(&myCriticalSection);
}

WavePlug::~WavePlug() {
#ifndef WP_NO_GUI
	EnterCriticalSection(&myCriticalSection);
	
	// NOTE: ~AudioEffect will delete the editor if it exists.
	// I do it myself here for thread safety.
	delete editor;
	editor = NULL;
	
	LeaveCriticalSection(&myCriticalSection);
#endif
}


// Inherited virtual public methods.
bool WavePlug::getEffectName(char *text) {
	std::strcpy(text, "Lost Technology");
	return true;
}

bool WavePlug::getProductString(char *text) {
	std::strcpy(text, "Lost Recombinant Audio Technology");
	return true;
}

bool WavePlug::getVendorString(char *text) {
	std::strcpy(text, "Transvaal Audio");
	return true;
}

VstInt32 WavePlug::getVendorVersion() {return WP_VENDOR_VERSION;}

VstPlugCategory WavePlug::getPlugCategory() {return kPlugCategEffect;}

VstInt32 WavePlug::canDo(char *text) {
	if (!std::strcmp(text, "sendVstEvents") ||
	    !std::strcmp(text, "sendVstMidiEvent") ||
	    !std::strcmp(text, "sendVstTimeInfo"))
		return -1l; // No event send.
	if (!std::strcmp(text, "receiveVstEvents") ||
	    !std::strcmp(text, "receiveVstMidiEvent") ||
	    !std::strcmp(text, "receiveVstTimeInfo"))
		return -1l; // No event receive.
	else if (!std::strcmp(text, "offline") || !std::strcmp(text, "noRealTime"))
		return -1l; // Realtime interface only.
	else if (!std::strcmp(text, "1in1out") || !std::strcmp(text, "1in2out") ||
		       !std::strcmp(text, "2in1out") || !std::strcmp(text, "2in2out"))
		return 1l; // Supported IO configurations.
	else if (!std::strcmp(text, "2in4out") || !std::strcmp(text, "4in2out") ||
		       !std::strcmp(text, "4in4out") || !std::strcmp(text, "4in8out") ||
	         !std::strcmp(text, "8in4out") || !std::strcmp(text, "8in8out"))
		return -1l; // Unsupported IO configurations.
	else if (!std::strcmp(text, "bypass"))
		return 1l; // Soft/listening bypass supported.
	return 0l; // Dunno.
}

void WavePlug::setProgram(VstInt32 program) {} // No program changes allowed.

void WavePlug::setProgramName(char *name) { // SYNCHRONIZED
	EnterCriticalSection(&myCriticalSection);
	
	std::strcpy(programName, name);
	
	LeaveCriticalSection(&myCriticalSection);
}

void WavePlug::getProgramName(char *name) { // SYNCHRONIZED
	EnterCriticalSection(&myCriticalSection);
	
	std::strcpy(name, programName);
	
	LeaveCriticalSection(&myCriticalSection);
}

bool WavePlug::getProgramNameIndexed(VstInt32 category, VstInt32 index, char *text) { // SYNCHRONIZED
	bool success = false;
	
	EnterCriticalSection(&myCriticalSection);
	
	if ((category == 0 || category == -1) && index == 0) {
		std::strcpy(text, programName);
		success = true;
	}
	
	LeaveCriticalSection(&myCriticalSection);
	
	return success;
}

void WavePlug::setParameter(VstInt32 index, float value) { // SYNCHRONIZED
	// Get appropriate help texts for modulator functions.
	const char *const*texts = NULL;
	
	switch (index) {
		case (kNumMonoParams + kAModType):
		case (kNumMonoParams + kNumParams + kAModType):
		case (kNumMonoParams + kFModType):
		case (kNumMonoParams + kNumParams + kFModType):
		texts = modFuncUHelpTexts[U_MOD_TYPE_T(value)];
		break;
		
		case (kNumMonoParams + kWModType):
		case (kNumMonoParams + kNumParams + kWModType):
		texts = modFuncFHelpTexts[F_MOD_TYPE_T(value)];
		break;
		
		case (kNumMonoParams + kOModType):
		case (kNumMonoParams + kNumParams + kOModType):
		texts = modFuncSHelpTexts[S_MOD_TYPE_T(value)];
		break;
	}
	
	// Set new parameter value (and help texts).
	EnterCriticalSection(&myCriticalSection);
	
	sharedData.newParamValues[index] = value;
	
	if (texts != NULL) { // Modulator function parameter updated.
		paramHelpTexts[index] = texts[0];
		paramHelpTexts[index+1] = texts[1];
	}
	
#ifndef WP_NO_GUI
	if (editor != NULL)
		((AEffGUIEditor *) editor)->setParameter(index, value);
#endif
	
	LeaveCriticalSection(&myCriticalSection);
}

float WavePlug::getParameter(VstInt32 index) { // SYNCHRONIZED
	float value = NAN;
	
	EnterCriticalSection(&myCriticalSection);
	
	value = sharedData.paramValues[index];
	
	LeaveCriticalSection(&myCriticalSection);
	
	return value;
}

void WavePlug::getParameterName(VstInt32 index, char *label) {
	if (index < kNumMonoParams) {
		if (index == kPlugVersion)
			std::strcpy(label, "Version");
		else if (index == kBufferSize)
			std::strcpy(label, "BufrSize");
	}
	else {
		index -= kNumMonoParams;
		
		std::strcpy(label, paramNames[index % kNumParams]);
		std::strcat(label, (index < kNumParams) ? "1" : "2");
	}
}

void WavePlug::getParameterDisplay(VstInt32 index, char *text) {
	float value = getParameter(index);
	
	if (index < kNumMonoParams) {
		if (index == kPlugVersion)
			std::strcpy(text, WP_VERSION_STRING(WP_MAJOR, WP_MINOR, WP_UPDATE));
		else if (index == kBufferSize)
			bufferSizeDisplayer(value, text);
	}
	else {
		index -= kNumMonoParams;
		
		paramDisplayers[index % kNumParams](value, text);
	}
}

void WavePlug::getParameterLabel(VstInt32 index, char *label) {
	if (index < kNumMonoParams)
		std::strcpy(label, "");
	else {
		index -= kNumMonoParams;
		
		std::strcpy(label, paramLabels[index % kNumParams]);
	}
}

void WavePlug::setSampleRate(float sRate) { // SYNCHRONIZED
	EnterCriticalSection(&myCriticalSection);
	
	sharedData.newSampleRate = sRate;
	
	LeaveCriticalSection(&myCriticalSection);
}

bool WavePlug::setBypass(bool onOff) { // SYNCHRONIZED
	EnterCriticalSection(&myCriticalSection);
	
	sharedData.bypassedFlag = onOff;
	sharedData.setProcessHandlersFlag = true;
	
	LeaveCriticalSection(&myCriticalSection);
	
	return true;
}

void WavePlug::suspend() { // SYNCHRONIZED
	EnterCriticalSection(&myCriticalSection);
	
	sharedData.resetFlag = true;
	
	LeaveCriticalSection(&myCriticalSection);
}

void WavePlug::resume() { // SYNCHRONIZED
	bool noInputs = false, noOutputs = false;
	
	EnterCriticalSection(&myCriticalSection);
	
	sharedData.resetFlag = true;
	sharedData.setProcessHandlersFlag = true;
	
    sharedData.input0Connected = true; // isInputConnected(0l);
	sharedData.input1Connected = true; // isInputConnected(1l);
    sharedData.output0Connected = true; // isOutputConnected(0l);
	sharedData.output1Connected = true; // isOutputConnected(1l);
	
	if (!(sharedData.input0Connected | sharedData.input1Connected)) {
		sharedData.operational = false;
		noInputs = true;
	}
	
	if (!(sharedData.output0Connected | sharedData.output1Connected)) {
		sharedData.operational = false;
		noOutputs = true;
	}
	
	LeaveCriticalSection(&myCriticalSection);
	
	if (noInputs & noOutputs)
		throw std::runtime_error("WavePlug::resume - No inputs or outputs connected.");
	else if (noInputs)
		throw std::runtime_error("WavePlug::resume - No inputs connected.");
	else if (noOutputs)
		throw std::runtime_error("WavePlug::resume - No outputs connected.");
}

/*void WavePlug::process(float **inputs, float **outputs, long sampleFrames) {
	doThreadSynchronizedDataExchange();
	
	(this->*procHandler)(
		inputs[in0Index], inputs[in1Index], outputs[out0Index], outputs[out1Index], sampleFrames);
}*/

void WavePlug::processReplacing(float **inputs, float **outputs, VstInt32 sampleFrames) {
	doThreadSynchronizedDataExchange();
	
	(this->*procRHandler)(
		inputs[in0Index], inputs[in1Index], outputs[out0Index], outputs[out1Index], sampleFrames);
}


// Non-inherited non-virtual public methods.
const char *WavePlug::getParamHelpText(int index) { // SYNCHRONIZED
	const char *helpText = "";
	
	EnterCriticalSection(&myCriticalSection);
	
	helpText = paramHelpTexts[index];
	
	LeaveCriticalSection(&myCriticalSection);
	
	return helpText;
}

float WavePlug::getAmplitude(int channel, bool postmod) { // SYNCHRONIZED
	float value = NAN;
	
	EnterCriticalSection(&myCriticalSection);
	
	value = (channel == 0)
		? ((postmod) ? sharedData.postA1 : sharedData.preA1)
		: ((channel == 1)
			? ((postmod) ? sharedData.postA2 : sharedData.preA2)
	    : 0.0f);
	
	LeaveCriticalSection(&myCriticalSection);
	
	return value;
}

float WavePlug::getFrequency(int channel, bool postmod) { // SYNCHRONIZED
	float value = NAN;
	
	EnterCriticalSection(&myCriticalSection);
	
	value = (channel == 0)
		? ((postmod) ? sharedData.postF1 : sharedData.preF1)
		: ((channel == 1)
			? ((postmod) ? sharedData.postF2 : sharedData.preF2)
	    : 0.0f);
	
	LeaveCriticalSection(&myCriticalSection);
	
	return value;
}

bool WavePlug::isOperational() { // SYNCHRONIZED
	bool flag = false;
	
	EnterCriticalSection(&myCriticalSection);
	
	flag = sharedData.operational;
	
	LeaveCriticalSection(&myCriticalSection);
	
	return flag;
}

int WavePlug::getBufferSizeMultiplier() { // SYNCHRONIZED
	int multiplier = -1;
	
	EnterCriticalSection(&myCriticalSection);
	
	multiplier = sharedData.bufferSizeMultiplier;
	
	LeaveCriticalSection(&myCriticalSection);
	
	return multiplier;
}


// ---<<< PRIVATE METHODS BEGIN HERE >>>---
void WavePlug::setOperational(bool flag) { // SYNCHRONIZED
	EnterCriticalSection(&myCriticalSection);
	
	sharedData.operational = flag;
	
	LeaveCriticalSection(&myCriticalSection);
}

void WavePlug::WavePlugData::clearUpdateFields() {
	reinitFlag = false;
	resetFlag = false;
	setProcessHandlersFlag = false;
	
	newSampleRate = NAN;
	
	std::fill(newParamValues, newParamValues + kNumAllParams, NAN);
}

void WavePlug::doThreadSynchronizedDataExchange() {
	
	EnterCriticalSection(&myCriticalSection);
	
	// Update signal monitors.
	if (sharedData.operational & !sharedData.reinitFlag) {
		sharedData.preA1 = ana1.getAmplitude();
		sharedData.postA1 = syn1.getAmplitude();
		sharedData.preF1 = ana1.getFrequency();
		sharedData.postF1 = syn1.getFrequency();
		sharedData.preA2 = ana2.getAmplitude();
		sharedData.postA2 = syn2.getAmplitude();
		sharedData.preF2 = ana2.getFrequency();
		sharedData.postF2 = syn2.getFrequency();
	}
	
	// Copy shared structure to thread-private structure.
	processingData = sharedData;
	sharedData.clearUpdateFields();
	
	LeaveCriticalSection(&myCriticalSection);
	
	// Update plugin configuration.
	if (processingData.reinitFlag) {
		if (!reinitialize())
			setOperational(false);
	}
	
	if (processingData.setProcessHandlersFlag)
		setProcHandlers();
	
	if (!processingData.operational) // SYSTEM ATE SHIT.
		return;
	
	if (processingData.resetFlag) {
		ana1.reset();
		ana2.reset();
		syn1.reset();
		syn2.reset();
	}
	
	if (!std::isnan(processingData.newSampleRate)) {
		float oldSampleRate = globalSampleRate;
		
		setGlobalSampleRate(processingData.newSampleRate);
		
		if (setBufferSizeMultiplier(processingData.bufferSizeMultiplier)) {
			EnterCriticalSection(&myCriticalSection);
			
			AudioEffectX::setSampleRate(processingData.newSampleRate);
			
			LeaveCriticalSection(&myCriticalSection);
		}
		else if (processingData.operational) // Old buffer size restored.
			setGlobalSampleRate(oldSampleRate);
		else { // Sample buffers unrecoverably lost.
			setProcHandlers(); // Set emergency handlers.
			setOperational(false);
			return;
		}
	}
	
	// Perform parameter updates. Write back new param values to shared structure.
	if (doParameterUpdates() > 0) {
		EnterCriticalSection(&myCriticalSection);
		
		std::memcpy(
			sharedData.paramValues, processingData.paramValues, sizeof sharedData.paramValues);
		sharedData.bufferSizeMultiplier = processingData.bufferSizeMultiplier;
		
		LeaveCriticalSection(&myCriticalSection);
	}
	else if (!processingData.operational) { // Unrecoverable error.
		setProcHandlers(); // Set emergency handlers.
		setOperational(false);
	}
}

bool WavePlug::reinitialize() {
	// Processing components.
	ana1.initialize(this);
	ana2.initialize(this);
	modA1.initialize(ana1.getAmpFunction(), ana2.getAmpFunction());
	modA2.initialize(ana2.getAmpFunction(), ana1.getAmpFunction());
	modF1.initialize(ana1.getFreqFunction(), ana2.getFreqFunction(), hzToUnsigned(261.63f));
	modF2.initialize(ana2.getFreqFunction(), ana1.getFreqFunction(), hzToUnsigned(261.63f));
	modW1.initialize(ana1.getWaveFunction(), ana2.getWaveFunction());
	modW2.initialize(ana2.getWaveFunction(), ana1.getWaveFunction());
	syn1.initialize(this, &modA1, &modF1, &modW1);
	syn2.initialize(this, &modA2, &modF2, &modW2);
	modO1.initialize(syn1.getAudioFunction(), syn2.getAudioFunction());
	modO2.initialize(syn2.getAudioFunction(), syn1.getAudioFunction());
	
	// Attempt to allocate minimal sample buffers.
	if (setBufferSizeMultiplier(processingData.bufferSizeMultiplier)) {
		// Set initial parameter values in components.
		editMode = -1; // NOT 0 or 1.
		doParameterUpdates(); // MUST be called AFTER editMode initialization.
	}
	else
		processingData.operational = false;
	
	// Set initial processing handlers.
	setProcHandlers();
	
	return processingData.operational;
}


// Setters.
int WavePlug::doParameterUpdates() {
	int nUpdated = 0;
	
	for (int index = 0; index < kNumAllParams; index++) {
		if (std::isnan(processingData.newParamValues[index])) // Parameter not updated.
			continue;
		else if (index < kNumMonoParams) {
			// Ignore attempts to set the "PlugVersion" dummy parameter.
			
			if (index == kBufferSize) {
				if (setBufferSizeMultiplier(BUFFER_SIZE_T(processingData.newParamValues[index]))) {
					processingData.paramValues[index] = processingData.newParamValues[index];
					
					nUpdated++;
				}
				else if (!processingData.operational) // Sample buffers unrecoverably lost.
					return -1;
			}
		}
		else {
			long stereoIndex = index - kNumMonoParams;
			
			setEditMode((stereoIndex < kNumParams) ? 0 : 1);
			
			(this->*paramSetters[stereoIndex % kNumParams])(processingData.newParamValues[index]);
			processingData.paramValues[index] = processingData.newParamValues[index];
			
			nUpdated++;
		}
	}
	
	return nUpdated;
}

bool WavePlug::setBufferSizeMultiplier(int multiplier) {
	int anaSize = (int) (multiplier * (WP_ANA_BUFFER_SIZE * globalSampleRate) / WP_STD_SAMPLE_RATE),
			synSize = (int) (multiplier * (WP_SYN_BUFFER_SIZE * globalSampleRate) / WP_STD_SAMPLE_RATE),
			oldAnaSize = ana1.getBufferSize(),
			oldSynSize = syn1.getBufferSize();
	
	if (!(ana1.setBufferSize(anaSize) && ana2.setBufferSize(anaSize) &&
				syn1.setBufferSize(synSize) && syn2.setBufferSize(synSize))) {
		
		if (!(ana1.setBufferSize(oldAnaSize) && ana2.setBufferSize(oldAnaSize) &&
					syn1.setBufferSize(oldSynSize) && syn2.setBufferSize(oldSynSize)))
			processingData.operational = false;
		
		return false;
	}
	
	processingData.bufferSizeMultiplier = multiplier;
	return true;
}

void WavePlug::setEditMode(int mode) {
	if (mode == 0 && editMode != 0) {
		editMode = 0;
		
		anaE  = &ana1;
		modAE = &modA1;
		modFE = &modF1;
		modWE = &modW1;
		synE  = &syn1;
		modOE = &modO1;
	}
	else if (mode == 1 && editMode != 1) {
		editMode = 1;
		
		anaE  = &ana2;
		modAE = &modA2;
		modFE = &modF2;
		modWE = &modW2;
		synE  = &syn2;
		modOE = &modO2;
	}
}

void WavePlug::aAIWSetter(float value) {anaE->setAIncWeight(INC_LAG_T(value));}

void WavePlug::aADWSetter(float value) {anaE->setADecWeight(DEC_LAG_T(value));}

void WavePlug::aAGLSetter(float value) {anaE->setAmpGateLevel(A_GATE_LVL_T(value));}

void WavePlug::aSGLSetter(float value) {anaE->setSampleGateLevel(S_GATE_LVL_T(value));}

void WavePlug::aFHTSetter(float value) {anaE->setHighTrig(TRIG_LVL_T(value));}

void WavePlug::aFLTSetter(float value) {anaE->setLowTrig(TRIG_LVL_T(value));}

void WavePlug::aFMinSetter(float value) {anaE->setFMin(F_MIN_T(value));}

void WavePlug::aFMaxSetter(float value) {anaE->setFMax(F_MAX_T(value));}

void WavePlug::aFWSetter(float value) {anaE->setFWeight(F_LAG_T(value));}

void WavePlug::aWWSetter(float value) {anaE->setWWeight(W_LAG_T(value));}

void WavePlug::aITSetter(float value) {anaE->setTrigInverted(BOOL_T(value));}

void WavePlug::aWISetter(float value) {anaE->setWInterpolation(BOOL_T(value));}

void WavePlug::mAModSetter(float value) {modAE->setModulation(U_MOD_TYPE_T(value));}

void WavePlug::mAMixSetter(float value) {modAE->setMix(value);}

void WavePlug::mFModSetter(float value) {modFE->setModulation(U_MOD_TYPE_T(value));}

void WavePlug::mFMixSetter(float value) {modFE->setMix(value);}

void WavePlug::mWModSetter(float value) {modWE->setModulation(F_MOD_TYPE_T(value));}

void WavePlug::mWMixSetter(float value) {modWE->setMix(value);}

void WavePlug::sAOffSetter(float value) {synE->setAOffset(A_OFFSET_T(value));}

void WavePlug::sAGainSetter(float value) {synE->setAGain(A_GAIN_T(value));}

void WavePlug::sFOffSetter(float value) {synE->setFOffset(F_OFFSET_T(value));}

void WavePlug::sFGainSetter(float value) {synE->setFGain(F_GAIN_T(value));}

void WavePlug::sOverSetter(float value) {synE->setOversamplingMultiplier(OVER_T(value));}

void WavePlug::sWindSetter(float value) {synE->setSmoothingWindow(WINDOW_T(value));}

void WavePlug::mOModSetter(float value) {modOE->setModulation(S_MOD_TYPE_T(value));}

void WavePlug::mOMixSetter(float value) {modOE->setMix(value);}


// Processing handlers.
void WavePlug::setProcHandlers() {
	if (!processingData.operational) { // Unrecoverable error.
		procHandler = procHandlers[8]; // Use do-nothing handlers.
		procRHandler = procRHandlers[8];
		return;
	}
	
	int handlerIndex = 0;
	
	switch (processingData.input1Connected << 1 | processingData.input0Connected) {
		case 1: // !in1 in0
		in1Index = 0; in0Index = 0;
		break;
		
		case 2: // in1 !in0
		in1Index = 1; in0Index = 1;
		break;
		
		case 3: // in1 in0
		in1Index = 1; in0Index = 0;
		handlerIndex += 4; // Two inputs.
		break;
	}
	
	switch (processingData.output1Connected << 1 | processingData.output0Connected) {
		case 1: // !out1 out0
		out1Index = 0; out0Index = 0;
		break;
		
		case 2: // out1 !out0
		out1Index = 1; out0Index = 1;
		break;
		
		case 3: // out1 out0
		out1Index = 1; out0Index = 0;
		handlerIndex += 2; // Two outputs.
		break;
	}
	
	if (processingData.bypassedFlag)
		handlerIndex += 1; // Bypass mode.
	
	procHandler = procHandlers[handlerIndex];
	procRHandler = procRHandlers[handlerIndex];
}

void WavePlug::proc1In1Out PROC_METHOD(
	ana1.addSample(*in0);
	ana2.addSample(*in0++);
	*out0++ += modO1.getValue();)

void WavePlug::proc1In1OutB PROC_METHOD(
	ana1.addSample(*in0);
	ana2.addSample(*in0);
	*out0++ += *in0++;)

void WavePlug::proc1In2Out PROC_METHOD(
	ana1.addSample(*in0);
	ana2.addSample(*in0++);
	*out0++ += modO1.getValue();
	*out1++ += modO2.getValue();)

void WavePlug::proc1In2OutB PROC_METHOD(
	ana1.addSample(*in0);
	ana2.addSample(*in0);
	*out0++ += *in0;
	*out1++ += *in0++;)

void WavePlug::proc2In1Out PROC_METHOD(
	ana1.addSample(*in0++);
	ana2.addSample(*in1++);
	*out0++ += modO1.getValue();)

void WavePlug::proc2In1OutB PROC_METHOD(
	ana1.addSample(*in0);
	ana2.addSample(*in1++);
	*out0++ += *in0++;)

void WavePlug::proc2In2Out PROC_METHOD(
	ana1.addSample(*in0++);
	ana2.addSample(*in1++);
	*out0++ += modO1.getValue();
	*out1++ += modO2.getValue();)

void WavePlug::proc2In2OutB PROC_METHOD(
	ana1.addSample(*in0);
	ana2.addSample(*in1);
	*out0++ += *in0++;
	*out1++ += *in1++;)

void WavePlug::procR1In1Out PROC_METHOD(
	ana1.addSample(*in0);
	ana2.addSample(*in0++);
	*out0++ = modO1.getValue();)

void WavePlug::procR1In1OutB PROC_METHOD(
	ana1.addSample(*in0);
	ana2.addSample(*in0);
	*out0++ = *in0++;)

void WavePlug::procR1In2Out PROC_METHOD(
	ana1.addSample(*in0);
	ana2.addSample(*in0++);
	*out0++ = modO1.getValue();
	*out1++ = modO2.getValue();)

void WavePlug::procR1In2OutB PROC_METHOD(
	ana1.addSample(*in0);
	ana2.addSample(*in0);
	*out0++ = *in0;
	*out1++ = *in0++;)

void WavePlug::procR2In1Out PROC_METHOD(
	ana1.addSample(*in0++);
	ana2.addSample(*in1++);
	*out0++ = modO1.getValue();)

void WavePlug::procR2In1OutB PROC_METHOD(
	ana1.addSample(*in0);
	ana2.addSample(*in1++);
	*out0++ = *in0++;)

void WavePlug::procR2In2Out PROC_METHOD(
	ana1.addSample(*in0++);
	ana2.addSample(*in1++);
	*out0++ = modO1.getValue();
	*out1++ = modO2.getValue();)

void WavePlug::procR2In2OutB PROC_METHOD(
	ana1.addSample(*in0);
	ana2.addSample(*in1);
	*out0++ = *in0++;
	*out1++ = *in1++;)
