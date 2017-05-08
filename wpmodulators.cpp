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

#include "wpmodulators.hpp"

#include <cmath>

const char *const modTypeUNames[kNModTypesU] = {
	"Add", "MinDiff", "MaxDiff", "Fine", "Mult", "Dist", "Top", "Pong"
};

const char *const modTypeSNames[kNModTypesS] = {
	"Add", "Diff", "Mult", "Dist", "Top", "Pong"
};

const char *const modTypeFNames[kNModTypesF] = {
	"Add", "Diff", "Mult", "Dist", "Top", "Comp", "Conv", "Pong"
};


// ---<<< Modulator >>>---
void Modulator::initialize(float lfoDiv, bool lfoRateDependent) {
	lfoIncrement = 0.0f;
	lfoDivisor = lfoDiv;
	mix1 = 1.0f;
	mix2 = 0.0f;
	mixDist = 1.0f;
	saw = 0.0f;
	tri = 0.0f;
	rateDependentLFO = lfoRateDependent;
}

void Modulator::setMix(float mx) {
	mix1 = 1.0f - mx;
	mix2 = mx;
	mixDist = std::pow(2.0f, 4.0f*mx) - 1.0f;
	lfoIncrement = (rateDependentLFO)
	               ? (mx*WP_STD_SAMPLE_RATE)/(lfoDivisor*globalSampleRate)
	               : mx/lfoDivisor;
}

inline void Modulator::updateLFO() {
	saw = std::fmod(saw + lfoIncrement, 1.0f);
	tri = 2.0f*((saw > 0.5f) ? 1.0f - saw : saw);
}


// ---<<< UnsignedModulator >>>---
const UnsignedModulator::fmethod UnsignedModulator::modFunctions[kNModTypesU] = {
	&UnsignedModulator::computeAdd,
	&UnsignedModulator::computeMinDiff,
	&UnsignedModulator::computeMaxDiff,
	&UnsignedModulator::computeFine,
	&UnsignedModulator::computeMult,
	&UnsignedModulator::computeDist,
	&UnsignedModulator::computeTop,
	&UnsignedModulator::computePong
};

void UnsignedModulator::initialize(
	RealFunction *input1, RealFunction *input2, float scale, float base, float lfoDiv)
{
	Modulator::initialize(lfoDiv, false);
	
	in1 = input1;
	in2 = input2;
	fineScale = scale;
	fineBase = base;
	invLogFineBase = 1.0f / std::log(base);
	
	setModulation(kModTypeUAdd);
}

void UnsignedModulator::setModulation(ModulationTypeU mt) {
	modType = mt;
	computeValue = modFunctions[modType];
}

float UnsignedModulator::computeAdd() {
	float v1 = in1->getValue(), v2 = in2->getValue();
	return v1 + mix2*(v2 - v1);
}

float UnsignedModulator::computeMinDiff() {
	float v1 = in1->getValue(), v2 = in2->getValue();
	return (v1 < v2) ? v1 - mix2*(2.0f*v1 - v2) : v2 - mix2*(2.0f*v2 - v1);
}

float UnsignedModulator::computeMaxDiff() {
	float v1 = in1->getValue(), v2 = in2->getValue();
	return (v1 > v2) ? v1 - mix2*v2 : v2 - mix2*v1;
}

float UnsignedModulator::computeFine() {
	float v1 = in1->getValue(), v2 = in2->getValue();
	
	if (v1 < 1.0e-8f | v2 < 1.0e-8f)
		return v1;
	
	float
		exp1 = std::floor(std::log(v1 / fineScale) * invLogFineBase),
		exp2 = std::floor(std::log(v2 / fineScale) * invLogFineBase);
	
	return v1 + mix2*(v2*std::pow(fineBase, exp1 - exp2) - v1);
}

float UnsignedModulator::computeMult() {
	float v1 = in1->getValue(), v2 = in2->getValue();
	return v1 * (mix1 + mix2*v2);
}

float UnsignedModulator::computeDist() {
	float v1 = in1->getValue(), v2 = in2->getValue();
	float vX = v1 * (mixDist*v2 + 1.0f);
	return vX / std::sqrt(vX*vX + 1.0f);
}

float UnsignedModulator::computeTop() {
	float v1 = in1->getValue(), v2 = in2->getValue();
	return v1 + mix2 * v2 * std::abs(1.0f - v1);
}

float UnsignedModulator::computePong() {
	updateLFO();
	float v1 = in1->getValue(), v2 = in2->getValue();
	return v1 + tri*(v2 - v1);
}


// ---<<< SignedModulator >>>---
const SignedModulator::fmethod SignedModulator::modFunctions[kNModTypesS] = {
	&SignedModulator::computeAdd,
	&SignedModulator::computeDiff,
	&SignedModulator::computeMult,
	&SignedModulator::computeDist,
	&SignedModulator::computeTop,
	&SignedModulator::computePong
};

void SignedModulator::initialize(RealFunction *input1, RealFunction *input2, float lfoDiv) {
	Modulator::initialize(lfoDiv, true);
	
	in1 = input1;
	in2 = input2;
	
	setModulation(kModTypeSAdd);
}

void SignedModulator::setModulation(ModulationTypeS mt) {
	modType = mt;
	
	computeValue = modFunctions[modType];
	if (modType == kModTypeSAdd & mix2 == 0.0f)
		computeValue = &SignedModulator::computeIdentity;
}

void SignedModulator::setMix(float mx) {
	Modulator::setMix(mx);
	
	computeValue = modFunctions[modType];
	if (modType == kModTypeSAdd & mix2 == 0.0f)
		computeValue = &SignedModulator::computeIdentity;
}

float SignedModulator::computeIdentity() {return in1->getValue();}

float SignedModulator::computeAdd() {
	float v1 = in1->getValue(), v2 = in2->getValue();
	return v1 + mix2*(v2 - v1);
}

float SignedModulator::computeDiff() {
	float v1 = in1->getValue(), v2 = in2->getValue();
	return v1 + mix2*(-v2 - v1);
}

float SignedModulator::computeMult() {
	float v1 = in1->getValue(), v2 = in2->getValue();
	return v1 * (mix1 + mix2*v2);
}

float SignedModulator::computeDist() {
	float v1 = in1->getValue(), v2 = in2->getValue();
	float vX = v1 * (mixDist*std::abs(v2) + 1.0f);
	return vX / std::sqrt(vX*vX + 1.0f);
}

float SignedModulator::computeTop() {
	float v1 = in1->getValue(), v2 = in2->getValue();
	return v1 + mix2 * copysignf(v2, v1) * (1.0f - std::abs(v1));
}

float SignedModulator::computePong() {
	updateLFO();
	float v1 = in1->getValue(), v2 = in2->getValue();
	return v1 + tri*(v2 - v1);
}


// ---<<< FunctionModulator >>>---
const FunctionModulator::fmethodf FunctionModulator::modFunctions[kNModTypesF] = {
	&FunctionModulator::computeAdd,
	&FunctionModulator::computeDiff,
	&FunctionModulator::computeMult,
	&FunctionModulator::computeDist,
	&FunctionModulator::computeTop,
	&FunctionModulator::computeComp,
	&FunctionModulator::computeConv,
	&FunctionModulator::computePong
};

void FunctionModulator::initialize(FunctionFunction *input1, FunctionFunction *input2, float lfoDiv) {
	Modulator::initialize(lfoDiv, true);
	
	in1 = input1;
	in2 = input2;
	cycleDelta = -1.0f;
	halfIRWidth = -1.0f;
	hSizeI = 2;
	hSizeF = 2.0f;
	hDelta = 1.0f;
	
	setModulation(kModTypeFAdd);
}

void FunctionModulator::setModulation(ModulationTypeF mt) {
	modType = mt;
	
	computeValue = modFunctions[modType];
	if (modType == kModTypeFAdd & mix2 == 0.0f)
		computeValue = &FunctionModulator::computeIdentity;
}

void FunctionModulator::setMix(float mx) {
	Modulator::setMix(mx);
	
	hSizeF = 2.0f + std::floor(28.0f*mx);
	hDelta = 2.0f / hSizeF;
	hSizeI = (int) hSizeF;
	
	computeValue = modFunctions[modType];
	if (modType == kModTypeFAdd & mix2 == 0.0f)
		computeValue = &FunctionModulator::computeIdentity;
}

float FunctionModulator::computeIdentity(float x) {return in1->getValue(x);}

float FunctionModulator::computeAdd(float x) {
	float v1 = in1->getValue(x), v2 = in2->getValue(x);
	return v1 + mix2*(v2 - v1);
}

float FunctionModulator::computeDiff(float x) {
	float v1 = in1->getValue(x), v2 = in2->getValue(x);
	return v1 + mix2*(-v2 - v1);
}

float FunctionModulator::computeMult(float x) {
	float v1 = in1->getValue(x), v2 = in2->getValue(x);
	return v1 * (mix1 + mix2*v2);
}

float FunctionModulator::computeDist(float x) {
	float v1 = in1->getValue(x), v2 = in2->getValue(x);
	float vX = v1 * (mixDist*std::abs(v2) + 1.0f);
	return vX / std::sqrt(vX*vX + 1.0f);
}

float FunctionModulator::computeTop(float x) {
	float v1 = in1->getValue(x), v2 = in2->getValue(x);
	return v1 + mix2 * copysignf(v2, v1) * (1.0f - std::abs(v1));
}

float FunctionModulator::computeComp(float x) {
	float v1 = in1->getValue(x);
	float v2 = in2->getValue(v1);
	return v1 + mix2*(v2 - v1);
}

float FunctionModulator::computeConv(float x) {
	float acc = 0.0f, cycleTau = x + halfIRWidth, hTau = 0.5f*hDelta - 1.0f;
	if (cycleTau > 1.0f) // Treat in1 waveform as a periodic signal.
		cycleTau -= 2.0f;
	
	// Convolve in1 with the IR formed by taking hSize samples from in2.
	for (int k = 0; k < hSizeI; k++) {
		acc += in1->getValue(cycleTau) * in2->getValue(hTau);
		
		cycleTau -= cycleDelta;
		if (cycleTau < -1.0f)
			cycleTau += 2.0f;
		hTau += hDelta;
	}
	
	// Make abs(output) <= output of an hSize-point averager to avoid overload.
	return acc / hSizeF;
}

float FunctionModulator::computePong(float x) {
	updateLFO();
	float v1 = in1->getValue(x), v2 = in2->getValue(x);
	return v1 + tri*(v2 - v1);
}
