TARGET = ptt_test
SRC = $(wildcard ../*.cpp) main.cpp
OBJ = $(SRC:%.cpp=%.o)
CXX = g++

CFLAGS += -g -std=c++11 -I.. 

ifeq ($(LOGS), 0)
CFLAGS+= -DDISABLE_LOGS
endif
LIBS = -lboost_system -lboost_thread -lboost_chrono -ludev

all: application

application: $(OBJ)
	$(CXX) $(CFLAGS) -o $(TARGET) -pthread $(OBJ) $(LIBS)

.cpp.o:
	$(CXX) $(CFLAGS) -c -o $*.o $<

clean:
	$(RM) *.o src/*.o core* $(TARGET) inc/Version.h
    
