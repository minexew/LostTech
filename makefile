# Copyright (c) 2006 Johan Sarge
# 
# Permission is hereby granted, free of charge, to any person obtaining a copy of this
# software and associated documentation files (the "Software"), to deal in the Software
# without restriction, including without limitation the rights to use, copy, modify, merge,
# publish, distribute, sublicense, and/or sell copies of the Software, and to permit
# persons to whom the Software is furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in all copies or
# substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
# INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
# PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE
# FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
# OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
# DEALINGS IN THE SOFTWARE.

# Version number variables.
plugversion := 0.2.5

# Directory variables.
sdkdir := /c/code/c/vstsdk2.3/source/common
guilibdir := $(sdkdir)/vstgui_3_0_beta4
installdir := "/c/Program Files/Image-Line/FL Studio 6/Plugins/VST/"

builddirs := bin/debug bin/dist dist

ifndef debug
odir := bin/dist
else
odir := bin/debug
endif


# Filename variables.
guizip := dist/LostTech-$(plugversion).zip
guitar := dist/LostTech-$(plugversion).tar.bz2
noguizip := dist/LostTechNoGUI-$(plugversion).zip
noguitar := dist/LostTechNoGUI-$(plugversion).tar.bz2
srczip := dist/LostTech-src-$(plugversion).zip
srctar := dist/LostTech-src-$(plugversion).tar.bz2

guiplug := LostTech.dll
noguiplug := LostTechNoGUI.dll

deffile := LostTech.def
docfiles := docs/*.css docs/*.html docs/*.png

guidistfiles := LICENSE $(guiplug) $(docfiles)
noguidistfiles := LICENSE $(noguiplug) $(docfiles)
srcdistfiles := LICENSE makefile $(deffile) *.cpp *.h *.hpp *.rc resources/* $(docfiles)

guiheader := WavePlug.hpp WavePlugEditor.hpp waveplugparams.h
guiobj := $(odir)/WavePlugMain.o $(odir)/WavePlug.o $(odir)/WavePlugEditor.o
guiresobj := $(odir)/WavePlugResource.o
guilibs := -lgdi32 -lole32 -lcomdlg32 -luuid

noguiheader := WavePlug.hpp waveplugparams.h
noguiobj := $(odir)/WavePlugMainNoGUI.o $(odir)/WavePlugNoGUI.o

commonheader := Analyzer.hpp wpmodulators.hpp Synthesizer.hpp BufferManager.hpp wpfunc.hpp \
                wpstdinclude.h
commonobj := $(odir)/Analyzer.o $(odir)/wpmodulators.o $(odir)/Synthesizer.o \
             $(odir)/BufferManager.o $(odir)/wpfunc.o

guisdkobj := $(odir)/aeffguieditor.o $(odir)/vstgui.o $(odir)/vstcontrols.o
commonsdkobj := $(odir)/audioeffectx.o $(odir)/AudioEffect.o


# Command option variables.
idirs := -I$(sdkdir) -I$(guilibdir)

ifndef debug
CXXFLAGS := -march=pentium2 -O3 $(idirs)
dllflags := -shared -Wl,--gc-sections -Wl,-s
else
CXXFLAGS := $(idirs)
dllflags := -shared
endif


# Phony targets.
.PHONY : all clean gui nogui install guidist noguidist srcdist

all : guidist noguidist srcdist

clean :
	$(RM) $(guiplug) $(noguiplug)
	$(RM) $(odir)/*.o

gui : $(builddirs) $(guiplug)

nogui : $(builddirs) $(noguiplug)

install : gui nogui
	cp $(guiplug) $(installdir)
	cp $(noguiplug) $(installdir)

guidist : $(builddirs) $(guizip) $(guitar)

noguidist : $(builddirs) $(noguizip) $(noguitar)

srcdist : $(builddirs) $(srczip) $(srctar)


# File targets.
$(builddirs) :
	mkdir -p $@

$(guizip) : $(guidistfiles)
	wzzip -uP $@ $^

$(guitar) : $(guidistfiles)
	tar -jcf $@ $^

$(noguizip) : $(noguidistfiles)
	wzzip -uP $@ $^

$(noguitar) : $(noguidistfiles)
	tar -jcf $@ $^

$(srczip) : $(srcdistfiles)
	wzzip -uP $@ $^

$(srctar) : $(srcdistfiles)
	tar -jcf $@ $^

# NOTE: It says at "http://gcc.gnu.org/onlinedocs/gcc-3.4.3/gcc/Link-Options.html"
# that compile flags should be used in the link step too when using -shared.
$(guiplug) : $(deffile) $(guiobj) $(guiresobj) $(commonobj) $(guisdkobj) $(commonsdkobj)
	$(CXX) $(CXXFLAGS) $(dllflags) -o $@ $^ $(guilibs)

$(guiobj) : $(odir)/%.o : %.cpp $(guiheader) $(commonheader)
	$(CXX) -c $(CXXFLAGS) -o $@ $<

$(guiresobj) : $(odir)/%.o : %.rc resources/*
	windres -o $@ $<

$(noguiplug) : $(deffile) $(noguiobj) $(commonobj) $(commonsdkobj)
	$(CXX) $(CXXFLAGS) $(dllflags) -o $@ $^

$(noguiobj) : $(odir)/%NoGUI.o : %NoGUI.cpp %.cpp $(noguiheader) $(commonheader)
	$(CXX) -c $(CXXFLAGS) -o $@ $<

$(commonobj) : $(odir)/%.o : %.cpp $(commonheader)
	$(CXX) -c $(CXXFLAGS) -o $@ $<

$(guisdkobj) : $(odir)/%.o : $(guilibdir)/%.cpp
	$(CXX) -c $(CXXFLAGS) -o $@ $<

$(commonsdkobj) : $(odir)/%.o : $(sdkdir)/%.cpp
	$(CXX) -c $(CXXFLAGS) -o $@ $<
