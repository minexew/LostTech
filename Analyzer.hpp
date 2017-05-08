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

#ifndef WP_ANALYZER_HPP
#define WP_ANALYZER_HPP

#include "wpfunc.hpp"
#include "BufferManager.hpp"

class Analyzer {
private:
	BufferManager *bufferManager;
	
	RealFunction ampFunc, freqFunc;
	FunctionFunction waveFunc;
	
	float aIncW, aDecW, ampGateLevel, sampleGateLevel,
	      fHighTrig, fLowTrig, fMin, fMax, fW, wW;
	bool trigInverted;
	
	float *oldWave, *newWave;
	
	SignCode endOfCycle;
	bool trigDisabled, detectPeak;
	int waveBufferSize, minWaveSize, maxWaveSize, oldWaveSize, newWaveSize, trigCount;
	float aWeightModifier, aIncWNew, aDecWNew, fWNew, wWNew, amplitude, frequency, maxSample;
	
public:
	void initialize(BufferManager *bMan, int bufferSize = 0);
	void reset(float a = 1.0e-8f, float fHz = 440.0f); // 440Hz = concert A.
	
	RealFunction *getAmpFunction() {return &ampFunc;}
	RealFunction *getFreqFunction() {return &freqFunc;}
	FunctionFunction *getWaveFunction() {return &waveFunc;}
	
	int getBufferSize() {return waveBufferSize;}
	bool setBufferSize(int bufferSize);
	
	float getAIncWeight() {return aIncW;}
	float getADecWeight() {return aDecW;}
	float getAmpGateLevel() {return ampGateLevel;}
	float getSampleGateLevel() {return sampleGateLevel;}
	float getFMin() {return fMin;}
	float getFMax() {return fMax;}
	float getHighTrig() {return fHighTrig;}
	float getLowTrig() {return fLowTrig;}
	float getFWeight() {return fW;}
	float getWWeight() {return wW;}
	bool getTrigInverted() {return trigInverted;}
	bool getWInterpolation() {return waveFunc.getInterpolation();}
	
	void setAIncWeight(float weight);
	void setADecWeight(float weight);
	void setAmpGateLevel(float level) {ampGateLevel = level;}
	void setSampleGateLevel(float level) {sampleGateLevel = level;}
	void setFMin(float hz);
	void setFMax(float hz);
	void setHighTrig(float level) {fHighTrig = level;}
	void setLowTrig(float level) {fLowTrig = level;}
	void setFWeight(float weight) {fW = weight; fWNew = 1.0f - weight;}
	void setWWeight(float weight) {wW = weight; wWNew = 1.0f - weight;}
	void setTrigInverted(bool onOff);
	void setWInterpolation(bool onOff) {waveFunc.setInterpolation(onOff);}
	
	void addSample(float sample);
	
	float getAmplitude() {return amplitude;}
	float getFrequency() {return frequency;}
	
private:
	void updateFreqAndWave(float sample, float absample);
};

#endif
