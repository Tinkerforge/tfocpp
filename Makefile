.SILENT:

CC = clang
CXX = clang++
CLANG_WARNINGS = -Weverything -Wno-c++98-compat -Wno-c++98-compat-pedantic -Wno-old-style-cast -Wno-shadow-field-in-constructor -Wno-padded -Wno-exit-time-destructors

COMPILE_FLAGS = -DOCPP_LOG_LEVEL=3 -gdwarf-4 -fPIC -O0 ${CLANG_WARNINGS} -fdiagnostics-color=always -fsanitize=address,undefined,leak -Ilib/TFJson -Ilib/ArduinoJson -Ilib/libiso8601 -I.
CFLAGS += -std=c99 ${COMPILE_FLAGS}
CXXFLAGS += -std=c++11 ${COMPILE_FLAGS}
LDFLAGS += -pthread -fsanitize=address,undefined,leak
LIB_LD_FLAGS = -shared-libasan
LIBS += -lwebsockets -lstdc++ $(wildcard lib/libiso8601/*.c.o)

WITH_DEBUG ?= yes

ifeq ($(WITH_DEBUG),yes)
	COMPILE_FLAGS += -g -ggdb
endif

SOURCES :=	$(wildcard ocpp/*.cpp) \
		    lib/mongoose/mongoose.cpp \
			lib/TFJson/TFJson.cpp

SOURCES_LIB := $(SOURCES) platforms/TestPlatform.cpp

SOURCES_EXEC := $(SOURCES) platforms/LinuxPlatform.cpp

OBJECTS_LIB := ${SOURCES_LIB:.cpp=.o}
OBJECTS_EXEC := ${SOURCES_EXEC:.cpp=.o}

all: libocpp.so ocpp_linux

libocpp.so: $(OBJECTS_LIB)
	$(CXX) $(OBJECTS_LIB) $(LIBS) $(LDFLAGS) $(LIB_LD_FLAGS) -shared -o $@

ocpp_linux: $(OBJECTS_EXEC)
	$(CXX) $(OBJECTS_EXEC) $(LIBS) $(LDFLAGS) -o $@

.PHONY: all clean

clean: Makefile
	$(E)$(RM) $(OBJECTS_LIB) $(OBJECTS_EXEC) libocpp.so
