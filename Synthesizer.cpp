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

#include "Synthesizer.hpp"

#include <cmath>

#define SAMPLEINDEX(x) (((x) + samplesSize) % samplesSize)
#define WINDOWINC(x) ((x) % windowPositionsSize)
#define WINDOWINDEX(x) (((x) + windowPositionsSize) % windowPositionsSize)

void Synthesizer::initialize(
	BufferManager *bMan,
	UnsignedModulator *inputA, UnsignedModulator *inputF, FunctionModulator *inputW,
	int bufferSize)
{
	bufferManager = bMan;
	
	audioFunc.setValue(NULL);
	
	inA = inputA;
	inF = inputF;
	inW = inputW;
	aOffset = 0.0f;
	aGain = 1.0f;
	fOffset = 0.0f;
	fGain = 1.0f;
	samplesSize = 0;
	samples = NULL;
	windowPositions = NULL;
	fadeBuffer = NULL;
	aValue = 0.0f;
	fValue = 0.0f;
	
	setBufferSize(bufferSize);
	setOversamplingMultiplier(1);
	setSmoothingWindow(0);
}

void Synthesizer::reset() {
	if (samples == NULL)
		return;
	
	start = last = 0;
	end = 1;
	samples[0] = 0.0f;
	audioFunc.setValue(samples);
	
	startWin = endWin = nWindows = 0;
	
	aValue = fValue = sampleFraction = 0.0f;
	
	windowSize = (int) ((smoothingWindow * globalSampleRate)/WP_STD_SAMPLE_RATE);
	
	if (windowSize > 0) {
		// NOTE: If windowSize isn't 0 then it must be at least 2 to allow the smoothing algorithm
		// to peek two samples ahead.
		windowSize = std::max(windowSize, 2);
		
		bufferManager->deleteFloatBuffer(fadeBuffer);
		
		if (!(fadeBuffer = bufferManager->newFloatBuffer(windowSize))) { // VERY little memory left.
			smoothingWindow = windowSize = 0;
			return;
		}
		
		float x = -1.0f/windowSize - 1.0f, xDelta = 2.0f/windowSize;
		for (int i = 0; i < windowSize; i++) {
			x += xDelta;
			fadeBuffer[i] = 0.5f * (1.0f - fade(x));
		}
	}
}

bool Synthesizer::setBufferSize(int bufferSize) {
	if (bufferSize == samplesSize)
		return true;
	
	bufferManager->deleteFloatBuffer(samples);
	bufferManager->deleteIntBuffer(windowPositions);
	
	samplesSize = bufferSize;
	
	if (samplesSize > 0) {
		windowPositionsSize = samplesSize/2 + 1;
		
		if (!(samples = bufferManager->newFloatBuffer(samplesSize)))
			goto alloc_failed;
		
		if (!(windowPositions = bufferManager->newIntBuffer(windowPositionsSize))) {
			bufferManager->deleteFloatBuffer(samples);
			goto alloc_failed;
		}
		
		reset();
	}
	else {
		samples = NULL;
		windowPositions = NULL;
	}
	
	return true;
	
	alloc_failed:
	windowPositionsSize = samplesSize = 0;
	samples = NULL;
	windowPositions = NULL;
	return false;
}

void Synthesizer::setOversamplingMultiplier(int multiplier) {
	oversamplingMultiplier = multiplier;
	oversamplingMultiplierF = (float) multiplier;
}

void Synthesizer::setSmoothingWindow(int smooWin) {
	smoothingWindow = smooWin;
	reset();
}

void Synthesizer::fillBuffer() {
	aValue = clamp01(aGain*inA->getValue() + aOffset);
	fValue = clamp01(fGain*inF->getValue() + fOffset);
	
	if (nWindows == 0)
		loadCycle(aValue, fValue);
	else {
		int samplesTotal = SAMPLEINDEX(end-1 - windowPositions[startWin]);
		
		while (samplesTotal < windowSize && end != start)
			samplesTotal += loadCycle(aValue, fValue);
		
		if (samplesTotal >= windowSize)
			applySmoothing(windowPositions[startWin]);
		
		startWin = WINDOWINC(startWin+1);
		nWindows--;
	}
	
	last = SAMPLEINDEX((nWindows > 0) ? windowPositions[startWin] - windowSize : end-1);
}

// At least one new sample MUST be stored and the new end of data MUST also be stored.
// Cycles MUST be large enough to guarantee this and still avoid window buffer overflow.
// It is also REQUIRED that whenever a call to loadCycle returns 0,
// at least one sample slot and one window slot are freed before the method is called again.
int Synthesizer::loadCycle(float a, float f) {
	// Compute number of generated (nSamplesI) and fetched (nSamplesF) samples.
	float nSamplesF = globalSampleRate/unsignedToHz(f) + sampleFraction;
	sampleFraction = std::modf(nSamplesF, &nSamplesF); // Isolate and store fractional sample.
	
	int nSamplesI = (int) nSamplesF;
	if (nSamplesI > SAMPLEINDEX(start - end)) // Not enough room in output buffer.
		nSamplesF = nSamplesI = SAMPLEINDEX(start - end);
	
	nSamplesF *= oversamplingMultiplierF;
	
	// Tell the waveform modulator how many samples we'll fetch.
	inW->setCycleSize(nSamplesF);
	
	/*
		NOTE: High CPU load when inputs were silent was caused by the Analyzer's
		amplitude tracker dropping into the denormal range. When operations on denormalized
		IEEE floats are implemented in software/firmware, as is often the case, they will
		usually be much slower than pure hardware operations.
	*/
	// Fetch and generate samples.
	int lastEnd = SAMPLEINC(end + nSamplesI);
	float x = -1.0f, d = 2.0f/nSamplesF;
	a /= oversamplingMultiplierF;
	
	while (end != lastEnd) {
		float s = 0.0f;
		
		for (int j = 0; j < oversamplingMultiplier; j++) {
			s += inW->getValue(x);
			x += d;
		}
		
		samples[end] = a * s;
		end = SAMPLEINC(end+1);
	}
	
	if (windowSize > 0) {
		windowPositions[endWin] = SAMPLEINDEX(end-1);
		endWin = WINDOWINC(endWin+1);
		nWindows++;
	}
	
	return nSamplesI;
}

void Synthesizer::applySmoothing(int windowPos) {
	float s0 = samples[SAMPLEINDEX(windowPos-1)],
	      s1 = samples[windowPos],
	      s2 = samples[SAMPLEINC(windowPos+1)],
	      s3 = samples[SAMPLEINC(windowPos+2)];
	
	float lDiff = s1 - s0, eDiff = s2 - s1, rDiff = s3 - s2;
	float delta0 = 0.5f * (eDiff - 0.5f*(rDiff + lDiff)),
	      delta1 = -0.5f * (rDiff - lDiff);
	
	float mL = delta0, mR = -delta0;
	
	for (int i = 0; i < windowSize; i++) {
		samples[SAMPLEINDEX(windowPos - i)] += fadeBuffer[i]*mL;
		samples[SAMPLEINC(windowPos+1 + i)] += fadeBuffer[i]*mR;
		
		mL += delta1;
		mR += delta1;
	}
}
