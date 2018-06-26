# 
# FF_ROOT     pointing to the FastFlow root directory (i.e.
#             the one containing the ff directory).
#
ifndef FF_ROOT 
FF_ROOT		= /usr/local/include #/home/spm18-sisbarra/ImgWatermarkProject/ImgWatermark in shared machine
endif

CXX		= g++ -std=c++11 -g -Wall #-DNO_DEFAULT_MAPPING
INCLUDES	= -I $(FF_ROOT)
CXXFLAGS  	= 

LDFLAGS 	= -L/usr/X11R6/lib -lm -lpthread -lX11 -lstdc++fs
OPTFLAGS	= -O3 -finline-functions -faligned-new -DNDEBUG

TARGETS		= imgWatermarkSeq      \
		  imgWatermarkSimpleFarm	\
		  imgWatermarkFarmPipe		\
		  imgWatermarkFFSimpleFarm	\
		  imgWatermarkFFFarmPipe

.PHONY: all clean cleanall
.SUFFIXES: .cpp 

all		: $(TARGETS)
imgWatermarkSeq: imgWatermarkSeq.cpp watermarkUtil.cpp utils.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(OPTFLAGS) -o $@ $< $(LDFLAGS)
imgWatermarkSimpleFarm: imgWatermarkSimpleFarm.cpp watermarkUtil.cpp utils.cpp myqueue.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(OPTFLAGS) -o $@ $< $(LDFLAGS)
imgWatermarkFarmPipe: imgWatermarkFarmPipe.cpp watermarkUtil.cpp utils.cpp myqueue.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(OPTFLAGS) -o $@ $< $(LDFLAGS)
imgWatermarkFFSimpleFarm: imgWatermarkFFSimpleFarm.cpp watermarkUtil.cpp utils.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(OPTFLAGS) -o $@ $< $(LDFLAGS)
imgWatermarkFFFarmPipe: imgWatermarkFFFarmPipe.cpp watermarkUtil.cpp utils.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(OPTFLAGS) -o $@ $< $(LDFLAGS)
clean		: 
	rm -f $(TARGETS)
cleanall	: clean
	\rm -f *.o *~ *core