#####################
## Euterpe Project ##
#####################

CXX = g++
CXXFLAGS = -O3 -Wall
#CXXFLAGS = -g -O0 -Wall
RM = rm -f

EXE = euterpe 
OBJS = tempoPitch.o gui.o audioPlay.o signalProcessingLibrary.o phaseRecov.o

## Libraries
L_PTHREAD       = -lpthread
#L_BOOST_OPTIONS = -lboost_program_options-mt
L_FFTW3         = -lfftw3
L_PORTAUDIO     = -lportaudio
LIBS = $(L_PTHREAD) $(L_BOOST_OPTIONS) $(L_FFTW3) $(L_PORTAUDIO) -lm



## all
.PHONY: all
all: $(EXE) 

$(EXE): $(OBJS) euterpe.o 
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LIBS)
	cp $(EXE) ..

stand_alone: $(OBJS) stand_alone.o
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LIBS)



## Suffix Rule
.SUFFIXES: .o .cpp
.cpp.o:
	$(CXX) $(CXXFLAGS) -c $<  $(INCLUDE)

## clean
.PHONY: clean
clean: 
	$(RM) $(OBJS) $(EXE) $(ZIPFILE)
	$(RM) *.o
	$(RM) *~



