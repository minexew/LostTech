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

#ifndef WP_WPMODULATORS_HPP
#define WP_WPMODULATORS_HPP

#include "wpfunc.hpp"

// TODO: Move these enums and arrays inside the Modulator subclasses.
enum ModulationTypeU {
	kModTypeUAdd,
	kModTypeUMinDiff,
	kModTypeUMaxDiff,
	kModTypeUFine,
	kModTypeUMult,
	kModTypeUDist,
	kModTypeUTop,
	kModTypeUPong,
	
	kNModTypesU
};

extern const char *const modTypeUNames[kNModTypesU];

enum ModulationTypeS {
	kModTypeSAdd,
	kModTypeSDiff,
	kModTypeSMult,
	kModTypeSDist,
	kModTypeSTop,
	kModTypeSPong,
	
	kNModTypesS
};

extern const char *const modTypeSNames[kNModTypesS];

enum ModulationTypeF {
	kModTypeFAdd,
	kModTypeFDiff,
	kModTypeFMult,
	kModTypeFDist,
	kModTypeFTop,
	kModTypeFComp,
	kModTypeFConv,
	kModTypeFPong,
	
	kNModTypesF
};

extern const char *const modTypeFNames[kNModTypesF];

class Modulator {
protected:
	float mix1, mix2, mixDist, saw, tri, lfoIncrement, lfoDivisor;
	bool rateDependentLFO;
	
public:
	void initialize(float lfoDiv, bool lfoRateDependent);
	
	float getMix() {return mix2;}
	void setMix(float mx);
	
	float getLFODivisor() {return lfoDivisor;}
	void setLFODivisor(float divisor) {lfoDivisor = divisor; setMix(mix2);}
	
protected:
	inline void updateLFO();
};

class UnsignedModulator : public Modulator {
private:
	typedef float (UnsignedModulator::*fmethod)();
	static const fmethod modFunctions[kNModTypesU];
	
	RealFunction *in1, *in2;
	ModulationTypeU modType;
	
	float fineScale, fineBase, invLogFineBase;
	
	fmethod computeValue;
	
public:
	void initialize(
		RealFunction *input1 = NULL, RealFunction *input2 = NULL,
		float scale = 0.5f, float base = 2.0f, float lfoDiv = 12.0f);
	
	void setInput1(RealFunction *input) {in1 = input;}
	void setInput2(RealFunction *input) {in2 = input;}
	
	ModulationTypeU getModulation() {return modType;}
	void setModulation(ModulationTypeU mt);
	
	float getValue() {return (this->*computeValue)();}
	
private:
	float computeAdd();
	float computeMaxDiff();
	float computeMinDiff();
	float computeFine();
	float computeMult();
	float computeDist();
	float computeTop();
	float computePong();
};

class SignedModulator : public Modulator {
private:
	typedef float (SignedModulator::*fmethod)();
	static const fmethod modFunctions[kNModTypesS];
	
	RealFunction *in1, *in2;
	ModulationTypeS modType;
	
	fmethod computeValue;
	
public:
	void initialize(
		RealFunction *input1 = NULL, RealFunction *input2 = NULL,
		float lfoDiv = 5000.0f);
	
	void setInput1(RealFunction *input) {in1 = input;}
	void setInput2(RealFunction *input) {in2 = input;}
	
	ModulationTypeS getModulation() {return modType;}
	void setModulation(ModulationTypeS mt);
	
	void setMix(float mx);
	
	float getValue() {return (this->*computeValue)();}
	
private:
	float computeIdentity();
	float computeAdd();
	float computeDiff();
	float computeMult();
	float computeDist();
	float computeTop();
	float computePong();
};

class FunctionModulator : public Modulator {
private:
	typedef float (FunctionModulator::*fmethodf)(float);
	static const fmethodf modFunctions[kNModTypesF];
	
	FunctionFunction *in1, *in2;
	ModulationTypeF modType;
	int hSizeI;
	float cycleDelta, halfIRWidth, hSizeF, hDelta;
	
	fmethodf computeValue;
	
public:
	void initialize(
		FunctionFunction *input1 = NULL, FunctionFunction *input2 = NULL,
		float lfoDiv = 5000.0f);
	
	void setInput1(FunctionFunction *input) {in1 = input;}
	void setInput2(FunctionFunction *input) {in2 = input;}
	
	ModulationTypeF getModulation() {return modType;}
	void setModulation(ModulationTypeF mt);
	
	void setMix(float mx);
	
	void setCycleSize(float nSamples) {
		cycleDelta = 2.0f/nSamples;
		halfIRWidth = std::fmod(0.5f*(hSizeF-1.0f)*cycleDelta, 2.0f);
	}
	
	float getValue(float x) {return (this->*computeValue)(x);}
	
private:
	float computeIdentity(float x);
	float computeAdd(float x);
	float computeDiff(float x);
	float computeMult(float x);
	float computeDist(float x);
	float computeTop(float x);
	float computeComp(float x);
	float computeConv(float x);
	float computePong(float x);
};

#endif
