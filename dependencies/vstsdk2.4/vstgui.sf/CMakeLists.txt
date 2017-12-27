cmake_minimum_required(VERSION 3.0)
project(vstgui)

set(SOURCE_FILES
        vstgui/aeffguieditor.cpp
        vstgui/aeffguieditor.h
        vstgui/cfileselector.cpp
        vstgui/cfileselector.h
        vstgui/cscrollview.cpp
        vstgui/cscrollview.h
        vstgui/ctabview.cpp
        vstgui/ctabview.h
        #vstgui/plugguieditor.cpp
        vstgui/plugguieditor.h
        vstgui/vstcontrols.cpp
        vstgui/vstcontrols.h
        vstgui/vstgui.cpp
        vstgui/vstgui.h
        vstgui/vstkeycode.h
        vstgui/vstplugscarbon.h
        vstgui/vstplugsmac.h
        vstgui/vstplugsmacho.h
        vstgui/vstplugsquartz.h
        )

add_library(vstgui STATIC ${SOURCE_FILES})

target_include_directories(vstgui PUBLIC vstgui)
target_include_directories(vstgui PRIVATE ..)
target_include_directories(vstgui PRIVATE ../public.sdk/source/vst2.x)
