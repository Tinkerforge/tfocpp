#.SILENT:

# Executable name
TARGET := ocpp

#CC = clang
#CXX = clang++

COMPILE_FLAGS = -g
CFLAGS += -std=c99 ${COMPILE_FLAGS}
CXXFLAGS += -std=c++11 ${COMPILE_FLAGS}
LDFLAGS += -pthread
LIBS += -lwebsockets -lstdc++ ./libiso8601/builddir/libiso8601.a

WITH_DEBUG ?= yes

ifeq ($(WITH_DEBUG),yes)
	COMPILE_FLAGS += -g -ggdb
endif

SOURCES :=	$(wildcard *.cpp) \
		RaccoonWSClient/WsRaccoonClient.cpp

SOURCES += ${SOURCES_LIB}

OBJECTS := ${SOURCES:.cpp=.o}
DEPENDS := ${SOURCES:.cpp=.p}

$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $(OBJECTS) $(LIBS)

.PHONY: all clean

clean: Makefile
	$(E)$(RM) $(OBJECTS) $(TARGET) $(DEPENDS)

