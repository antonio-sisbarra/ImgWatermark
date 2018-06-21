# 
# FF_ROOT     pointing to the FastFlow root directory (i.e.
#             the one containing the ff directory).
#
ifndef FF_ROOT 
FF_ROOT		= /usr/local/include
endif

CXX		= g++ -std=c++11 -g #-DNO_DEFAULT_MAPPING
INCLUDES	= -I $(FF_ROOT) $(CIMG_ROOT) 
CXXFLAGS  	= 

LDFLAGS 	= -L/usr/X11R6/lib -lm -lpthread -lX11 -lstdc++fs
OPTFLAGS	= -O3 -finline-functions -DNDEBUG

TARGETS		= imgWatermarkSeq

.PHONY: all clean cleanall
.SUFFIXES: .cpp 


%: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(OPTFLAGS) -o $@ $< $(LDFLAGS)

all		: $(TARGETS)
clean		: 
	rm -f $(TARGETS)
cleanall	: clean
	\rm -f *.o *~ *core