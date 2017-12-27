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

#ifndef WP_WAVEPLUGEDITOR_HPP
#define WP_WAVEPLUGEDITOR_HPP

#include "wpstdinclude.h"

#include "aeffguieditor.h"
#include "waveplugparams.h"
#include "WavePlug.hpp"

class WavePlug;

class WavePlugEditor : public AEffGUIEditor, public CControlListener {
private:
	enum {
		kNComponents = 6,
		kNMultiLocks = kNComponents + 1,
		kNParamLocks = kNumParams + kNMultiLocks,
		kNParamControls = kNumStereoParams,
		kNMultiButtons = 2 * kNMultiLocks,
		kNLockButtons = 2 * kNParamLocks,
		kHelpButtonTag = kNumAllParams + kNLockButtons
	};
	
	static const int
		controlPositions[kNParamControls][2],
		componentPositions[kNMultiButtons][2],
		controlTypes[kNumParams],
		handleBitmapIds[kNumParams],
		paramComponentNumber[kNumParams];
	
	static void ampToString(float a, char *text);
	static void freqToString(float f, char *text);
	
	WavePlug *wavePlug;
	
	CColor displayBackColor, displayFontColor;
	
	CMovieBitmap *logo;
	
	bool paramLocks[kNParamLocks], *const componentLocks, *const globalLock;
	COnOffButton *lockButtons[kNLockButtons];
	
	CControl *controls[kNParamControls];
	CParamDisplay *displays[kNParamControls];
	
	COptionMenu *bufrMenu;
	CParamDisplay *bufrDispKBytes, *bufrDispHz, *bufrDispMillis;
	
	CParamDisplay *aDisp0Pre, *fDisp0Pre, *aDisp1Pre, *fDisp1Pre,
	              *aDisp0Post, *fDisp0Post, *aDisp1Post, *fDisp1Post;
	
	CTextLabel *helpDisplay;
	COnOffButton *helpButton;
	int lastTouchedParamIndex;
	bool showHelp, paramTouched;
	
public:
	WavePlugEditor(AudioEffect *effect);
	
	virtual bool open(void *ptr) override;
	virtual void close() override;
	virtual void idle() override;
	
	virtual void setParameter(VstInt32 index, float value) override;
	virtual void valueChanged(CDrawContext* context, CControl* control) override;
	
private:
	void initKnob(int tag, CBitmap *knobBitmap, CBitmap *handleBitmap, CBitmap *lockBitmap);
	void initButton(int tag, CBitmap *buttonBitmap, CBitmap *lockBitmap);
	void initLockButton(int tag, const int *pos, CBitmap *lockBitmap, int padW = 3, int padH = 3);
	CParamDisplay *initMonitor(
		CRect &size, void (*converter)(float, char *), float value = 0.0f,
		CColor *tColorP = NULL, CColor *fColorP = NULL, CColor *bColorP = NULL,
		CHoriTxtAlign hAlign = kCenterText);
};

#endif
