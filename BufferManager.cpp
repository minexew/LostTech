/*
Copyright (c) 2007 Johan Sarge

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

#include "BufferManager.hpp"

#include <algorithm>
#include <new>
#include <stdexcept>

BufferManager::BufferManager() {
#ifdef WP_OLD_WINDOWS
	InitializeCriticalSection(&myCriticalSection);
#else
	if (!InitializeCriticalSectionAndSpinCount(&myCriticalSection, 0x80000400))
		throw std::runtime_error(
			"BufferManager::BufferManager - Failed to initialize critical section.");
#endif
	
	EnterCriticalSection(&myCriticalSection);
	
	intBuffers = NULL;
	floatBuffers = NULL;
	
	try {
		intBuffers = new std::vector<int *>();
		floatBuffers = new std::vector<float *>();
	}
	catch (std::bad_alloc e) { // Editor allocation failed.
		delete intBuffers;
		delete floatBuffers;
		intBuffers = NULL;
		floatBuffers = NULL;
	}
	
	LeaveCriticalSection(&myCriticalSection);
	
	if (intBuffers == NULL) {
		DeleteCriticalSection(&myCriticalSection);
		
		throw std::runtime_error(
			"BufferManager::BufferManager - Failed to create buffer vectors.");
	}
}

BufferManager::~BufferManager() {
	EnterCriticalSection(&myCriticalSection);
	
	std::for_each(intBuffers->begin(), intBuffers->end(), &deleteBuffer<int>);
	std::for_each(floatBuffers->begin(), floatBuffers->end(), &deleteBuffer<float>);
	
	delete intBuffers;
	delete floatBuffers;
	
	LeaveCriticalSection(&myCriticalSection);
	
	DeleteCriticalSection(&myCriticalSection);
}

int *BufferManager::newIntBuffer(int size) {
	int *buffer = NULL;
	
	try {
		buffer = new int[size];
	}
	catch (std::bad_alloc e) {
		return NULL;
	}
	
	EnterCriticalSection(&myCriticalSection);
	
	intBuffers->push_back(buffer);
	
	LeaveCriticalSection(&myCriticalSection);
	
	return buffer;
}

float *BufferManager::newFloatBuffer(int size) {
	float *buffer = NULL;
	
	try {
		buffer = new float[size];
	}
	catch (std::bad_alloc e) {
		return NULL;
	}
	
	EnterCriticalSection(&myCriticalSection);
	
	floatBuffers->push_back(buffer);
	
	LeaveCriticalSection(&myCriticalSection);
	
	return buffer;
}

bool BufferManager::deleteIntBuffer(int *ptr) {
	bool found = false;
	
	EnterCriticalSection(&myCriticalSection);
	
	std::vector<int *>::iterator result =
		std::find(intBuffers->begin(), intBuffers->end(), ptr);
	
	if (result != intBuffers->end()) {
		found = true;
		intBuffers->erase(result);
	}
	
	LeaveCriticalSection(&myCriticalSection);
	
	if (found)
		delete[] ptr;
	
	return found;
}

bool BufferManager::deleteFloatBuffer(float *ptr) {
	bool found = false;
	
	EnterCriticalSection(&myCriticalSection);
	
	std::vector<float *>::iterator result =
		std::find(floatBuffers->begin(), floatBuffers->end(), ptr);
	
	if (result != floatBuffers->end()) {
		found = true;
		floatBuffers->erase(result);
	}
	
	LeaveCriticalSection(&myCriticalSection);
	
	if (found)
		delete[] ptr;
	
	return found;
}
