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

#define WP_WPFUNC_CPP
#include "wpfunc.hpp"

float globalSampleRate = WP_STD_SAMPLE_RATE;

float globalMaxFrequency = sampleRateToMaxFreq(WP_STD_SAMPLE_RATE);

float globalMinPeriodLength = 1.0f / globalMaxFrequency;

// f must point to a buffer containing fSizePlus1 samples.
// The last sample in the buffer (at offset (fSizePlus1 - 1))
// must be the first sample of the next waveform.
/*void FunctionFunction::setFunction(int fSizePlus1, float *f) {
	fSize = (unsigned int) (fSizePlus1 - 1);
	fSizeCoefficient = 0.5f * fSize;
	function = f;
}

float FunctionFunction::getValue(float x) {
	float sampleIndexF;
	float sampleWeight = std::modf(fSizeCoefficient * (x + 3.0f), &sampleIndexF);
	float *sample = function + (unsigned int) sampleIndexF % fSize;
	
	return *sample + sampleWeight * (*(sample + interpolation) - *sample);
}*/
