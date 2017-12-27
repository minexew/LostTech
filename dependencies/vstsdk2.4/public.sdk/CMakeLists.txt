cmake_minimum_required(VERSION 3.0)
project(VSTSDK2_4)

set(SOURCE_FILES
        source/vst2.x/aeffeditor.h
        source/vst2.x/audioeffect.cpp
        source/vst2.x/audioeffect.h
        source/vst2.x/audioeffectx.cpp
        source/vst2.x/audioeffectx.h
        )

add_library(VSTSDK2_4 STATIC ${SOURCE_FILES} ${HEADER_FILES})

target_include_directories(VSTSDK2_4 PUBLIC ..)
target_include_directories(VSTSDK2_4 PUBLIC source/vst2.x)
