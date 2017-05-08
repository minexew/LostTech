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

#include "Analyzer.hpp"

#include <algorithm>
#include <cmath>

// NOTE: This method does not attempt to delete existing sample buffers.
// If re-initialization is desired, implement and call a "dispose"
// method first.
void Analyzer::initialize(BufferManager *bMan, int bufferSize) {
	bufferManager = bMan;
	
	ampFunc.setValue(&amplitude);
	freqFunc.setValue(&frequency);
	
	aWeightModifier = 1.0f;
	aIncWNew = 0.05f;
	aDecWNew = 0.002f;
	ampGateLevel = 0.0001;
	sampleGateLevel = 0.05;
	fHighTrig = 0.8f;
	fLowTrig = -0.8f;
	fMin = 1.0f;
	fMax = 22000.0f;
	fW = 0.0f;
	fWNew = 1.0f;
	minWaveSize = 1;
	maxWaveSize = 10000000;
	wW = 0.0f;
	wWNew = 1.0f;
	trigInverted = false;
	endOfCycle = kPosNeg;
	waveBufferSize = 0;
	oldWave = NULL;
	newWave = NULL;
	
	setBufferSize(bufferSize);
}

void Analyzer::reset(float a, float fHz) {
	if (oldWave == NULL)
		return;
	
	aWeightModifier = WP_STD_SAMPLE_RATE / globalSampleRate;
	maxWaveSize = (int) (globalSampleRate/fMin + 0.5f) + 1 <? waveBufferSize;
	
	amplitude = a;
	frequency = hzToUnsigned(fHz);
	
	trigDisabled = true;
	trigCount = 0;
	detectPeak = trigInverted;
	oldWaveSize = 2;
	newWaveSize = 0;
	maxSample = oldWave[0] = oldWave[1] = 0.0f;
	
	waveFunc.setFunction(oldWaveSize, oldWave);
}

bool Analyzer::setBufferSize(int bufferSize) {
	if (bufferSize == waveBufferSize)
		return true;
	
	bufferManager->deleteFloatBuffer(oldWave);
	bufferManager->deleteFloatBuffer(newWave);
	
	waveBufferSize = bufferSize;
	
	if (waveBufferSize > 0) {
		if (!(oldWave = bufferManager->newFloatBuffer(waveBufferSize)))
			goto alloc_failed;
		
		if (!(newWave = bufferManager->newFloatBuffer(waveBufferSize))) {
			bufferManager->deleteFloatBuffer(oldWave);
			goto alloc_failed;
		}
		
		reset();
	}
	else {
		oldWave = NULL;
		newWave = NULL;
	}
	
	return true;
	
	alloc_failed:
	waveBufferSize = 0;
	oldWave = NULL;
	newWave = NULL;
	return false;
}

void Analyzer::setAIncWeight(float weight) {
	aIncW = weight;
	aIncWNew = 1.0f - std::pow(aIncW, aWeightModifier);
}

void Analyzer::setADecWeight(float weight) {
	aDecW = weight;
	aDecWNew = 1.0f - std::pow(aDecW, aWeightModifier);
}

void Analyzer::setFMin(float hz) {
	fMin = hz <? globalMaxFrequency;
	maxWaveSize = (int) (globalSampleRate/fMin + 0.5f) + 1 <? waveBufferSize;
}

void Analyzer::setFMax(float hz) {
	fMax = hz <? globalMaxFrequency;
	minWaveSize = (int) (globalSampleRate/fMax + 0.5f) + 1;
}

void Analyzer::setTrigInverted(bool onOff) {
	trigInverted = onOff;
	endOfCycle = (trigInverted) ? kNegPos : kPosNeg;
	trigDisabled = true;
}

void Analyzer::addSample(float sample) {
	// Update the amplitude output.
	float absample = std::abs(sample);
	
	// NOTE: We make amplitude lower-bounded to stay out of cycle-sapping denormal territory.
	amplitude =
		1.0e-8f >? amplitude + ((absample > amplitude) ? aIncWNew : aDecWNew) * (absample - amplitude);
	
	// Skip frequency and waveform updates while the input signal is near-silent.
	if (trigDisabled) {
		if (amplitude > ampGateLevel | absample > sampleGateLevel) {
			trigDisabled = false;
			trigCount = 0;
			detectPeak = trigInverted;
			newWaveSize = 0;
			maxSample = absample;
		}
		else
			return;
	}
	else if (amplitude < ampGateLevel & absample < sampleGateLevel) {
		trigDisabled = true;
		return;
	}
	
	// Add the sample to the sample buffer.
	newWave[newWaveSize++] = sample;
	if (absample > maxSample)
		maxSample = absample;
	
	// Check if the trigger conditions for an F&W update are met.
	if (newWaveSize >= maxWaveSize)
		updateFreqAndWave(sample, absample);
	else if (trigCount > 1) {
		if (newWaveSize >= minWaveSize && signs(newWave[newWaveSize-2], sample) == endOfCycle)
			updateFreqAndWave(sample, absample);
	}
	else if ((detectPeak) ? sample > amplitude*fHighTrig : sample < amplitude*fLowTrig) {
		detectPeak = !detectPeak;
		trigCount++;
	}
}

void Analyzer::updateFreqAndWave(float sample, float absample) {
	// Reset update trigger.
	trigCount = 0;
	detectPeak = trigInverted;
	
	// Compute frequency, with lag if that is turned on.
	frequency += fWNew*(hzToUnsigned(globalSampleRate/(newWaveSize-1)) - frequency);
	
	// Normalize the waveform.
	// 1.0e-8f is added to values used as denominators to prevent DbZ problems.
	
	// Whichever is lower of the current input amplitude and the maximum signal level
	// in the waveform is chosen as the maximum "normal" signal level.
	float maxNormal = (amplitude <? maxSample) + 1.0e-8f;
	
	// How much higher than the maximum normal level does the waveform go?
	float diffMaxSampleMaxNormal = maxSample - maxNormal + 2.0e-8f;
	
	// How much of the [0,1] interval should be reserved for distorted samples?
	float distLevel = 1.0f - 0.125f*std::log10(8.0f*diffMaxSampleMaxNormal + 1.0f);
	
	// Normalize samples with signal level less than the maximum normal level
	// to the interval [0, distLevel]. Limit samples with signal level higher
	// than the maximum normal level to the interval [distLevel, 1].
	float normalizerGain = distLevel / maxNormal,
	      limiterGain = (1.0f - distLevel) / diffMaxSampleMaxNormal;
	
	// The limiterGain coefficient must be applied to the difference between the
	// sample's signal level and the maximum normal level. If we didn't do the
	// subtraction here, we would have to write something like
	//   newWave[i] = copysignf(limiterGain*(std::abs(newWave[i]) - maxNormal) + distLevel,
	//                          newWave[i]);
	// inside the normalization loop.
	distLevel -= limiterGain*maxNormal;
	
	for (int i = 0; i < newWaveSize; i++) { // Normalization loop.
		if (std::abs(newWave[i]) > maxNormal) {
			newWave[i] *= limiterGain;
			newWave[i] += copysignf(distLevel, newWave[i]);
		}
		else
			newWave[i] *= normalizerGain;
	}
	
	// Apply waveform lag if that is turned on.
	if (wWNew < 1.0f) {
		float d = 2.0f / newWaveSize, x = -1.0f;
		
		for (int i = 0; i < newWaveSize; i++) {
			float value = waveFunc.getValue(x);
			value += wWNew*(newWave[i] - value);
			newWave[i] = copysignf(1.0e-8f >? std::abs(value), value);
			x += d;
		}
	}
	
	// Update signal source for waveform output object.
	waveFunc.setFunction(newWaveSize, newWave);
	
	// Swap the waveform output (oldWave) and recording (newWave) buffer pointers.
	std::swap(oldWave, newWave);
	oldWaveSize = newWaveSize;
	
	// Reset the new waveform.
	newWaveSize = 1;
	newWave[0] = sample;
	maxSample = absample;
}
