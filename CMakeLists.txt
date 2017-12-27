cmake_minimum_required(VERSION 3.0)
project(LostTech)

add_subdirectory(dependencies/vstsdk2.4/public.sdk)
add_subdirectory(dependencies/vstsdk2.4/vstgui.sf)

set(SOURCE_FILES
        Analyzer.cpp
        Analyzer.hpp
        BufferManager.cpp
        BufferManager.hpp
        Synthesizer.cpp
        Synthesizer.hpp
        WavePlug.hpp
        WavePlugEditor.cpp
        WavePlugEditor.hpp
        waveplugparams.h
        WavePlugResource.rc
        wpfunc.cpp
        wpfunc.hpp
        wpmodulators.cpp
        wpmodulators.hpp
        wpstdinclude.h
        )

set(GUI_SOURCE_FILES
        WavePlug.cpp
        WavePlugMain.cpp
        )

set(NOGUI_SOURCE_FILES
        WavePlugMainNoGUI.cpp
        WavePlugNoGUI.cpp
        )

add_library(LostTech SHARED ${SOURCE_FILES} ${GUI_SOURCE_FILES})
add_library(LostTechNoGUI SHARED ${SOURCE_FILES} ${NOGUI_SOURCE_FILES})

target_compile_definitions(LostTech PUBLIC _CRT_SECURE_NO_WARNINGS NOMINMAX)
target_compile_definitions(LostTechNoGUI PUBLIC _CRT_SECURE_NO_WARNINGS NOMINMAX)

#target_include_directories(VSTSDK2_4 PUBLIC source/vst2.x)

target_link_libraries(LostTech PUBLIC VSTSDK2_4 vstgui -static-libgcc -static-libstdc++)
target_link_libraries(LostTechNoGUI PUBLIC VSTSDK2_4 vstgui -static-libgcc -static-libstdc++)

SET_TARGET_PROPERTIES(LostTech PROPERTIES CXX_VISIBILITY_PRESET hidden)
SET_TARGET_PROPERTIES(LostTechNoGUI PROPERTIES CXX_VISIBILITY_PRESET hidden)
