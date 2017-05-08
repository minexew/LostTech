/*
Copyright (c) 2006 Johan Sarge

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

#include "WavePlugEditor.hpp"

#include <cstdio>
#include "wpfunc.hpp"

// Private static constant data.
enum { // Resource ID:s.
	kBack = 128,
	kLogo,
	kButton,
	kLockButton,
	kHelpButton,
	kKnob,
	kKnobHandle,
	kKnobHandle10,
	kKnobHandle6 = kKnobHandle10 + 9,
	kKnobHandle8 = kKnobHandle6 + 5,
	
	kEndResources = kKnobHandle8 + 7
};

const int WavePlugEditor::controlPositions[kNParamControls][2] = {
	{0 * 64, 20 + 0 * 64}, // 1 kAIncLag, // 0
	{1 * 64, 20 + 0 * 64}, // 1 kADecLag,
	{2 * 64, 20 + 0 * 64}, // 1 kGateLvlA,
	{3 * 64, 20 + 0 * 64}, // 1 kGateLvlS,
	{0 * 64, 20 + 1 * 64}, // 1 kHighTrig,
	{1 * 64, 20 + 1 * 64}, // 1 kLowTrig,
	{2 * 64, 20 + 1 * 64}, // 1 kFMin,
	{3 * 64, 20 + 1 * 64}, // 1 kFMax,
	{0 * 64, 20 + 2 * 64}, // 1 kFLag,
	{1 * 64, 20 + 2 * 64}, // 1 kWLag,
	{2 * 64, 20 + 2 * 64}, // 1 kInvTrig,
	{2 * 64, 20 + 2 * 64 + 32}, // 1 kInterp,
	
	{1 * 64, 20 + 3 * 64}, // 1 kAModType, // 11
	{1 * 64, 20 + 4 * 64}, // 1 kAModMix,
	
	{2 * 64, 20 + 3 * 64}, // 1 kFModType, // 13
	{2 * 64, 20 + 4 * 64}, // 1 kFModMix,
	
	{3 * 64, 20 + 3 * 64}, // 1 kWModType, // 15
	{3 * 64, 20 + 4 * 64}, // 1 kWModMix,
	
	{0 * 64, 20 + 5 * 64}, // 1 kAOffset, // 17
	{1 * 64, 20 + 5 * 64}, // 1 kAGain,
	{0 * 64, 20 + 6 * 64}, // 1 kFOffset,
	{1 * 64, 20 + 6 * 64}, // 1 kFGain,
	{2 * 64, 20 + 5 * 64}, // 1 kOversmpl,
	{2 * 64, 20 + 6 * 64}, // 1 kSmooWin,
	
	{3 * 64, 20 + 5 * 64}, // 1 kOModType, // 23
	{3 * 64, 20 + 6 * 64}, // 1 kOModMix
	
	{4 * 64, 20 + 0 * 64}, // 2 kAIncLag, // 0
	{5 * 64, 20 + 0 * 64}, // 2 kADecLag,
	{6 * 64, 20 + 0 * 64}, // 2 kGateLvlA,
	{7 * 64, 20 + 0 * 64}, // 2 kGateLvlS,
	{4 * 64, 20 + 1 * 64}, // 2 kHighTrig,
	{5 * 64, 20 + 1 * 64}, // 2 kLowTrig,
	{6 * 64, 20 + 1 * 64}, // 2 kFMin,
	{7 * 64, 20 + 1 * 64}, // 2 kFMax,
	{5 * 64, 20 + 2 * 64}, // 2 kFLag,
	{6 * 64, 20 + 2 * 64}, // 2 kWLag,
	{7 * 64, 20 + 2 * 64}, // 2 kInvTrig,
	{7 * 64, 20 + 2 * 64 + 32}, // 2 kInterp,
	
	{6 * 64, 20 + 3 * 64}, // 2 kAModType, // 11
	{6 * 64, 20 + 4 * 64}, // 2 kAModMix,
	
	{5 * 64, 20 + 3 * 64}, // 2 kFModType, // 13
	{5 * 64, 20 + 4 * 64}, // 2 kFModMix,
	
	{4 * 64, 20 + 3 * 64}, // 2 kWModType, // 15
	{4 * 64, 20 + 4 * 64}, // 2 kWModMix,
	
	{5 * 64, 20 + 5 * 64}, // 2 kAOffset, // 17
	{6 * 64, 20 + 5 * 64}, // 2 kAGain,
	{5 * 64, 20 + 6 * 64}, // 2 kFOffset,
	{6 * 64, 20 + 6 * 64}, // 2 kFGain,
	{7 * 64, 20 + 5 * 64}, // 2 kOversmpl,
	{7 * 64, 20 + 6 * 64}, // 2 kSmooWin,
	
	{4 * 64, 20 + 5 * 64}, // 2 kOModType, // 23
	{4 * 64, 20 + 6 * 64} // 2 kOModMix
};

const int WavePlugEditor::componentPositions[kNMultiButtons][2] = {
	{3 * 64, 20 + 0 * 64}, // Analyzer 1, 20 + // 0
	{1 * 64, 20 + 3 * 64}, // AMod 1,
	{2 * 64, 20 + 3 * 64}, // FMod 1,
	{3 * 64, 20 + 3 * 64}, // WMod 1,
	{2 * 64, 20 + 5 * 64}, // Synthesizer 1,
	{3 * 64, 20 + 5 * 64}, // OMod 1,
	
	{0 * 64, 20 + 3 * 64}, // Global 1,
	
	{7 * 64, 20 + 0 * 64}, // Analyzer 2, 20 + // 6
	{6 * 64, 20 + 3 * 64}, // AMod 2,
	{5 * 64, 20 + 3 * 64}, // FMod 2,
	{4 * 64, 20 + 3 * 64}, // WMod 2,
	{7 * 64, 20 + 5 * 64}, // Synthesizer 2,
	{4 * 64, 20 + 5 * 64}, // OMod 2,
	
	{7 * 64, 20 + 3 * 64} // Global 2
};

const int WavePlugEditor::controlTypes[kNumParams] = {
	kKnob, // kAIncLag, // 0
	kKnob, // kADecLag,
	kKnob, // kGateLvlA,
	kKnob, // kGateLvlS,
	kKnob, // kHighTrig,
	kKnob, // kLowTrig,
	kKnob, // kFMin,
	kKnob, // kFMax,
	kKnob, // kFLag,
	kKnob, // kWLag,
	kButton, // kInvTrig,
	kButton, // kInterp,
	
	kKnob, // kAModType, // 11
	kKnob, // kAModMix,
	
	kKnob, // kFModType, // 13
	kKnob, // kFModMix,
	
	kKnob, // kWModType, // 15
	kKnob, // kWModMix,
	
	kKnob, // kAOffset, // 17
	kKnob, // kAGain,
	kKnob, // kFOffset,
	kKnob, // kFGain,
	kKnob, // kOversmpl,
	kKnob, // kSmooWin,
	
	kKnob, // kOModType, // 23
	kKnob // kOModMix
};

const int WavePlugEditor::handleBitmapIds[kNumParams] = {
	kKnobHandle, // kAIncLag, // 0
	kKnobHandle10, // kADecLag,
	kKnobHandle10 + 1, // kGateLvlA,
	kKnobHandle10 + 2, // kGateLvlS,
	kKnobHandle10 + 3, // kHighTrig,
	kKnobHandle10 + 4, // kLowTrig,
	kKnobHandle10 + 5, // kFMin,
	kKnobHandle10 + 6, // kFMax,
	kKnobHandle10 + 7, // kFLag,
	kKnobHandle10 + 8, // kWLag,
	0, // kInvTrig,
	0, // kInterp,
	
	kKnobHandle, // kAModType, // 11
	kKnobHandle8, // kAModMix,
	
	kKnobHandle8 + 1, // kFModType, // 13
	kKnobHandle8 + 2, // kFModMix,
	
	kKnobHandle8 + 3, // kWModType, // 15
	kKnobHandle8 + 4, // kWModMix,
	
	kKnobHandle, // kAOffset, // 17
	kKnobHandle6, // kAGain,
	kKnobHandle6 + 1, // kFOffset,
	kKnobHandle6 + 2, // kFGain,
	kKnobHandle6 + 3, // kOversmpl,
	kKnobHandle6 + 4, // kSmooWin,
	
	kKnobHandle8 + 5, // kOModType, // 23
	kKnobHandle8 + 6 // kOModMix
};

const int WavePlugEditor::paramComponentNumber[kNumParams] = {
	0,0,0,0, 0,0,0,0, 0,0,0,0, // Analyzer,
	1,1, // AMod,
	2,2, // FMod,
	3,3, // WMod,
	4,4,4,4, 4,4, // Synthesizer,
	5,5 // OMod
};

void WavePlugEditor::ampToString(float a, char *text) {
	std::sprintf(text, "%1.2f", 100.0f*a);
}

void WavePlugEditor::freqToString(float f, char *text) {
	std::sprintf(text, "%1.2f", unsignedToHz(f));
}


// Constructor.
WavePlugEditor::WavePlugEditor(AudioEffect *effect) :
AEffGUIEditor(effect), wavePlug((WavePlug *) effect),
componentLocks(paramLocks + kNumParams), globalLock(paramLocks + kNParamLocks - 1),
lastTouchedParamIndex(-1), showHelp(false), paramTouched(false) {
	rect.left = 0; rect.top = 0; rect.right = 512; rect.bottom = 488;
	displayBackColor(235, 235, 235, 0);
	displayFontColor(0, 0, 0, 0);
	
	// Clear parameter locks.
	for (int i = 0; i < kNParamLocks; i++)
		paramLocks[i] = false;
}


// Inherited virtual public methods.
long WavePlugEditor::open(void *ptr) {
	AEffGUIEditor::open(ptr);
	
	CRect size;
	CPoint point;
	
	// Load some bitmaps.
	CBitmap *backBitmap = new CBitmap(kBack),
	        *logoBitmap = new CBitmap(kLogo),
	        *buttonBitmap = new CBitmap(kButton),
					*lockBitmap = new CBitmap(kLockButton),
					*helpBitmap = new CBitmap(kHelpButton),
	        *knobBitmap = new CBitmap(kKnob),
	        *handleBitmaps[kEndResources - kKnobHandle];
	for (int handleResourceId = kKnobHandle; handleResourceId < kEndResources; handleResourceId++)
		handleBitmaps[handleResourceId - kKnobHandle] = new CBitmap(handleResourceId);
	
	// Init background frame.
	size(0, 0, backBitmap->getWidth(), backBitmap->getHeight());
	frame = new CFrame(size, ptr, this);
	frame->setBackground(backBitmap);
	
	// Init logo display.
	size(3*64, 20 + 2*64, 3*64 + logoBitmap->getWidth(), 20 + 2*64 + logoBitmap->getHeight());
	point(0, 0);
	logo = new CMovieBitmap(size, NULL, -1, 1, 64, logoBitmap, point);
	frame->addView(logo);
	
	// Init controls and param displays.
	for (int index = 0; index < kNParamControls; index++) {
		if (controlTypes[index % kNumParams] == kKnob) {
			initKnob(
				index, knobBitmap,
				handleBitmaps[handleBitmapIds[index % kNumParams] - kKnobHandle],
				lockBitmap);
			
			size(
				controlPositions[index][0] + 2,  controlPositions[index][1] + 39,
				controlPositions[index][0] + 61, controlPositions[index][1] + 49);
		}
		else if (controlTypes[index % kNumParams] == kButton) {
			initButton(index, buttonBitmap, lockBitmap);
			
			size(
				controlPositions[index][0] + 31,  controlPositions[index][1] + 3,
				controlPositions[index][0] + 61, controlPositions[index][1] + 13);
		}
		
		displays[index] = initMonitor(
			size,
			WavePlug::getParamDisplayer(index),
			wavePlug->getParameter(kNumMonoParams + index));
	}
	
	// Init component-wide and global parameter locks.
	for (int lockNumber = 0; lockNumber < kNMultiLocks; lockNumber++) {
		initLockButton(
			kNumParams + lockNumber,
			componentPositions[lockNumber],
			lockBitmap, -3, 3);
		
		initLockButton(
			kNParamLocks + kNumParams + lockNumber,
			componentPositions[kNMultiLocks + lockNumber],
			lockBitmap, -3, 3);
	}
	
	// Init buffer size chooser and displayers.
	size(5, 5, 5 + 28, 5 + 10);
	bufrMenu = new COptionMenu(size, this, kBufferSize);
	bufrMenu->addEntry("1");
	bufrMenu->addEntry("2");
	bufrMenu->addEntry("4");
	bufrMenu->addEntry("8");
	bufrMenu->addEntry("16");
	bufrMenu->addEntry("32");
	bufrMenu->addEntry("64");
	bufrMenu->addEntry("128");
	bufrMenu->setFontColor(kWhiteCColor);
	bufrMenu->setBackColor(kBlackCColor);
	bufrMenu->setCurrent(2);
	frame->addView(bufrMenu);
	
	size(109, 5, 109 + 32, 5 + 10);
	bufrDispKBytes = initMonitor(size, WavePlug::bufferSizeKByteDisplayer, 0.0f,
	                             &kWhiteCColor, &kBlackCColor, &kBlackCColor, kRightText);
	
	size(164, 5, 164 + 32, 5 + 10);
	bufrDispHz = initMonitor(size, WavePlug::bufferSizeHzDisplayer, 0.0f,
	                         &kWhiteCColor, &kBlackCColor, &kBlackCColor, kRightText);
	
	size(219, 5, 219 + 32, 5 + 10);
	bufrDispMillis = initMonitor(size, WavePlug::bufferSizeMillisDisplayer, 0.0f,
	                             &kWhiteCColor, &kBlackCColor, &kBlackCColor, kRightText);
	
	// Init version display.
	size(407, 5, 407 + 100, 5 + 10);
	CTextLabel *versionDisplay = new CTextLabel(size);
	versionDisplay->setBackColor(kBlackCColor);
	versionDisplay->setFrameColor(kBlackCColor);
	versionDisplay->setFontColor(kWhiteCColor);
	versionDisplay->setFont(kNormalFontSmall);
	versionDisplay->setHoriAlign(kRightText);
	versionDisplay->setText(wavePlug->getVersionString());
	frame->addView(versionDisplay);
	
	// Init amplitude and frequency displays.
	// Pre-modulation.
	size(0*64 + 2, 20 + 3*64 + 13, 0*64 + 61, 20 + 3*64 + 23);
	aDisp0Pre = initMonitor(size, ampToString);
	
	size(7*64 + 2, 20 + 3*64 + 13, 7*64 + 61, 20 + 3*64 + 23);
	aDisp1Pre = initMonitor(size, ampToString);
	
	size(0*64 + 2, 20 + 3*64 + 39, 0*64 + 61, 20 + 3*64 + 49);
	fDisp0Pre = initMonitor(size, freqToString);
	
	size(7*64 + 2, 20 + 3*64 + 39, 7*64 + 61, 20 + 3*64 + 49);
	fDisp1Pre = initMonitor(size, freqToString);
	
	// Post-modulation.
	size(0*64 + 2, 20 + 4*64 + 13, 0*64 + 61, 20 + 4*64 + 23);
	aDisp0Post = initMonitor(size, ampToString);
	
	size(7*64 + 2, 20 + 4*64 + 13, 7*64 + 61, 20 + 4*64 + 23);
	aDisp1Post = initMonitor(size, ampToString);
	
	size(0*64 + 2, 20 + 4*64 + 39, 0*64 + 61, 20 + 4*64 + 49);
	fDisp0Post = initMonitor(size, freqToString);
	
	size(7*64 + 2, 20 + 4*64 + 39, 7*64 + 61, 20 + 4*64 + 49);
	fDisp1Post = initMonitor(size, freqToString);
	
	// Init help display.
	size(5, 20 + 7*64 + 2, 5 + 465, 20 + 7*64 + 2 + 13);
	helpDisplay = new CTextLabel(size);
	helpDisplay->setBackColor(kBlackCColor);
	helpDisplay->setFrameColor(kBlackCColor);
	helpDisplay->setFontColor(kWhiteCColor);
	helpDisplay->setFont(kNormalFontSmall);
	helpDisplay->setHoriAlign(kLeftText);
	frame->addView(helpDisplay);
	
	size(5 + 465 + 5, 20 + 7*64 + 4, 5 + 465 + 5 + 30, 20 + 7*64 + 4 + 10);
	helpButton = new COnOffButton(size, this, kHelpButtonTag, helpBitmap);
	helpButton->setValue((showHelp) ? 1.0f : 0.0f);
	frame->addView(helpButton);
	
	// Forget bitmaps.
	backBitmap->forget();
	logoBitmap->forget();
	buttonBitmap->forget();
	lockBitmap->forget();
	helpBitmap->forget();
	knobBitmap->forget();
	for (int i = kKnobHandle; i < kEndResources; i++)
		handleBitmaps[i - kKnobHandle]->forget();
	
	return true;
}

void WavePlugEditor::close() {
	// NOTE: GUI components and their bitmaps are deleted by ~CFrame.
	if (frame != NULL) {
		delete frame;
		frame = NULL;
	}
}

void WavePlugEditor::idle() {
	AEffGUIEditor::idle();
	
	// Update amplitude and frequency displays.
	aDisp0Pre->setValue(wavePlug->getAmplitude(0));
	aDisp1Pre->setValue(wavePlug->getAmplitude(1));
	
	aDisp0Post->setValue(wavePlug->getAmplitude(0, true));
	aDisp1Post->setValue(wavePlug->getAmplitude(1, true));
	
	fDisp0Pre->setValue(wavePlug->getFrequency(0));
	fDisp1Pre->setValue(wavePlug->getFrequency(1));
	
	fDisp0Post->setValue(wavePlug->getFrequency(0, true));
	fDisp1Post->setValue(wavePlug->getFrequency(1, true));
	
	// Update buffer size displays.
	float bufferSizeParamValue = wavePlug->getParameter(kBufferSize);
	bufrDispKBytes->setValue(bufferSizeParamValue);
	bufrDispHz->setValue(bufferSizeParamValue);
	bufrDispMillis->setValue(bufferSizeParamValue);
	
	// Update help display.
	if (showHelp & paramTouched) {
		helpDisplay->setText(wavePlug->getParamHelpText(lastTouchedParamIndex));
		helpDisplay->setDirty();
		paramTouched = false;
	}
}

void WavePlugEditor::setParameter(long index, float value) {
	if (!frame)
		return;
	
	if (index == kBufferSize) {
		bufrMenu->setValue(7.0f*value + 0.5f);
		
		postUpdate();
	}
	else if (index >= kNumMonoParams) {
		int stereoIndex = index - kNumMonoParams;
		
		controls[stereoIndex]->setValue(value);
		displays[stereoIndex]->setValue(value);
		
		postUpdate();
	}
}

void WavePlugEditor::valueChanged(CDrawContext* context, CControl* control) {
	long tag = control->getTag();
	float value = control->getValue();
	
	if (tag == kHelpButtonTag) { // Help button status changed.
		bool buttonOn = value >= 0.5f;
		
		if (!buttonOn & showHelp) {
			helpDisplay->setText("");
			helpDisplay->setDirty();
		}
		
		showHelp = buttonOn;
	}
	else if (tag >= kNumAllParams) { // Lock button status changed.
		long lockIndex = tag - kNumAllParams;
		
		paramLocks[lockIndex % kNParamLocks] = value >= 0.5f;
		lockButtons[(kNParamLocks + lockIndex) % kNLockButtons]->setValue(value);
	}
	else if (tag >= kNumMonoParams) { // Parameter control value changed.
		long stereoIndex = tag - kNumMonoParams;
		int paramTypeIndex = stereoIndex % kNumParams;
		
		if (*globalLock ||
		    componentLocks[paramComponentNumber[paramTypeIndex]] ||
		    paramLocks[paramTypeIndex]) {
			wavePlug->setParameterAutomated(kNumMonoParams + paramTypeIndex, value);
			wavePlug->setParameterAutomated(kNumMonoParams + kNumParams + paramTypeIndex, value);
		}
		else
			wavePlug->setParameterAutomated(tag, value);
		
		// Show help text for this param in help display.
		lastTouchedParamIndex = tag;
		paramTouched = true;
	}
	else { // Mono param changed.
		// Ignore attempts to set the "PlugVersion" dummy parameter.
		
		if (tag == kBufferSize)
			wavePlug->setParameterAutomated(tag, value / 7.0f);
		
		lastTouchedParamIndex = tag;
		paramTouched = true;
	}
}


// Private methods.
void WavePlugEditor::initKnob(int index, CBitmap *knobBitmap, CBitmap *handleBitmap,
                              CBitmap *lockBitmap) {
	
	int knobW = knobBitmap->getWidth(), knobH = knobBitmap->getHeight();
	int padW = (64 - knobW)/2, padH = 3;
	CRect size;
	size(controlPositions[index][0] + padW,         controlPositions[index][1] + padH,
	     controlPositions[index][0] + padW + knobW, controlPositions[index][1] + padH + knobH);
	CPoint knobOffset;
	knobOffset(0, 0);
	
	CKnob *knob = new CKnob(size, this, kNumMonoParams + index, knobBitmap, handleBitmap, knobOffset);
	knob->setInsetValue(7);
	knob->setValue(wavePlug->getParameter(kNumMonoParams + index));
	frame->addView(knob);
	controls[index] = knob;
	
	initLockButton(
		(index < kNumParams) ? index : kNMultiLocks + index,
		controlPositions[index], lockBitmap);
}

void WavePlugEditor::initButton(int index, CBitmap *buttonBitmap, CBitmap *lockBitmap) {
	
	// On and off states stacked in bitmap.
	int buttonW = buttonBitmap->getWidth(), buttonH = buttonBitmap->getHeight() / 2;
	int padW = 12, padH = 3;
	CRect size;
	size(controlPositions[index][0] + padW,           controlPositions[index][1] + padH,
	     controlPositions[index][0] + padW + buttonW, controlPositions[index][1] + padH + buttonH);
	
	COnOffButton *button = new COnOffButton(size, this, kNumMonoParams + index, buttonBitmap);
	button->setValue(wavePlug->getParameter(kNumMonoParams + index));
	frame->addView(button);
	controls[index] = button;
	
	initLockButton(
		(index < kNumParams) ? index : kNMultiLocks + index,
		controlPositions[index], lockBitmap);
}

void WavePlugEditor::initLockButton(int index, const int *pos, CBitmap *lockBitmap,
                                    int padW, int padH) {
	
	// On and off states stacked in bitmap.
	int buttonW = lockBitmap->getWidth(), buttonH = lockBitmap->getHeight() / 2;
	if (padW < 0)
		padW = 64 - (buttonW - padW);
	if (padH < 0)
		padH = 64 - (buttonH - padH);
	
	CRect size;
	size(pos[0] + padW,           pos[1] + padH,
	     pos[0] + padW + buttonW, pos[1] + padH + buttonH);
	CPoint point;
	
	COnOffButton *lockButton = new COnOffButton(size, this, kNumAllParams + index, lockBitmap);
	lockButton->setValue((paramLocks[index % kNParamLocks]) ? 1.0f : 0.0f);
	frame->addView(lockButton);
	lockButtons[index] = lockButton;
}

CParamDisplay *WavePlugEditor::initMonitor(CRect &size, void (*converter)(float, char *), float value,
                                           CColor *tColorP, CColor *fColorP, CColor *bColorP,
                                           CHoriTxtAlign hAlign) {
	
	CColor tColor = (fColorP) ? *tColorP : displayFontColor,
	       fColor = (fColorP) ? *fColorP : displayBackColor,
	       bColor = (bColorP) ? *bColorP : displayBackColor;
	
	CParamDisplay *mon = new CParamDisplay(size);
	
	mon->setValue(value);
	mon->setBackColor(bColor);
	mon->setFrameColor(fColor);
	mon->setFontColor(tColor);
	mon->setFont(kNormalFontSmall);
	mon->setHoriAlign(hAlign);
	mon->setStringConvert(converter);
	frame->addView(mon);
	
	return mon;
}
