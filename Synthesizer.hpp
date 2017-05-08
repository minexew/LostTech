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

#ifndef WP_SYNTHESIZER_HPP
#define WP_SYNTHESIZER_HPP

#include "wpfunc.hpp"
#include "BufferManager.hpp"
#include "wpmodulators.hpp"

#define SAMPLEINC(x) ((x) % samplesSize)

class Synthesizer {
private:
	BufferManager *bufferManager;
	
	RealFunction audioFunc;
	
	UnsignedModulator *inA, *inF;
	FunctionModulator *inW;
	
	float aOffset, aGain, fOffset, fGain;
	int oversamplingMultiplier, smoothingWindow;
	
	float *samples;
	int *windowPositions;
	float *fadeBuffer;
	
	int samplesSize, windowPositionsSize, windowSize,
	    start, last, end, startWin, endWin, nWindows;
	float aValue, fValue, oversamplingMultiplierF, sampleFraction;
	
public:
	void initialize(
		BufferManager *bMan,
		UnsignedModulator *inputA = NULL, UnsignedModulator *inputF = NULL,
		FunctionModulator *inputW = NULL, int bufferSize = 0);
	void reset();
	
	void setAmpInput(UnsignedModulator *input) {inA = input;}
	void setFreqInput(UnsignedModulator *input) {inF = input;}
	void setWaveInput(FunctionModulator *input) {inW = input;}
	
	RealFunction *getAudioFunction() {return &audioFunc;}
	
	int getBufferSize() {return samplesSize;}
	bool setBufferSize(int bufferSize);
	
	float getAOffset() {return aOffset;}
	float getAGain() {return aGain;}
	float getFOffset() {return fOffset;}
	float getFGain() {return fGain;}
	int getOversamplingMultiplier() {return oversamplingMultiplier;}
	int getSmoothingWindow() {return smoothingWindow;}
	
	void setAOffset(float offset) {aOffset = offset;}
	void setAGain(float gain) {aGain = gain;}
	void setFOffset(float offset) {fOffset = offset;}
	void setFGain(float gain) {fGain = gain;}
	void setOversamplingMultiplier(int multiplier);
	void setSmoothingWindow(int smooWin);
	
	void tick() {
		if (start == last)
			fillBuffer();
		start = SAMPLEINC(start+1);
		audioFunc.setValue(samples + start);
	}
	
	float getAmplitude() {return aValue;}
	float getFrequency() {return fValue;}
	
private:
	void fillBuffer();
	int loadCycle(float a, float f);
	void applySmoothing(int windowPos);
};

#endif
