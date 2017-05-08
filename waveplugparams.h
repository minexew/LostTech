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

#ifndef WP_WAVEPLUGPARAMS_H
#define WP_WAVEPLUGPARAMS_H

enum {
	kPlugVersion,
	kBufferSize,
	
	kNumMonoParams
};

enum { // Parameter indexes.
	kAIncLag, // 0
	kADecLag,
	kGateLvlA,
	kGateLvlS,
	kHighTrig,
	kLowTrig,
	kFMin,
	kFMax,
	kFLag,
	kWLag,
	kInvTrig,
	kInterp,
	
	kAModType, // 12
	kAModMix,
	
	kFModType, // 14
	kFModMix,
	
	kWModType, // 16
	kWModMix,
	
	kAOffset, // 18
	kAGain,
	kFOffset,
	kFGain,
	kOversmpl,
	kSmooWin,
	
	kOModType, // 24
	kOModMix,
	
	kNumParams // 26
};

enum {
	kNumStereoParams = 2 * kNumParams,
	kNumAllParams = kNumMonoParams + kNumStereoParams
};

#endif
