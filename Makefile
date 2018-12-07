include ../../build/flags.mk
INCLUDES = -I./ -I../ -I../utils
CXXFLAGS = $(GFLAGS) -fPIC
TARGET = ipc.a

sources = $(wildcard *.cpp)
objects = $(sources:.cpp=.o)

all:	$(TARGET)

$(TARGET):	$(objects)
	$(AR) r $(TARGET) $(objects)
	
$(objects): %.o: %.cpp
	$(CXX) -c $(INCLUDES) $(CXXFLAGS) $< -o $@

clean:
	rm -f *.o *.a
