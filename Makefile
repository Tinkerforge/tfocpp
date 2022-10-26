.SILENT:

CC = clang
CXX = clang++
CLANG_WARNINGS = -Weverything -Wno-c++98-compat -Wno-c++98-compat-pedantic -Wno-old-style-cast -Wno-shadow-field-in-constructor -Wno-padded -Wno-exit-time-destructors -Wno-sign-conversion -Wno-sign-compare -Wno-double-promotion -Wno-implicit-int-float-conversion

COMPILE_FLAGS = -DMG_ENABLE_OPENSSL=1 -DOCPP_LOG_LEVEL=3 -gdwarf-4 -fPIC -O0 ${CLANG_WARNINGS} -fdiagnostics-color=always -fsanitize=address,undefined,leak -Ilib/ArduinoJson -Ilib/mongoose -Isrc
CFLAGS += -std=c99 ${COMPILE_FLAGS}
CXXFLAGS += -std=c++11 ${COMPILE_FLAGS}
LDFLAGS += -pthread -fsanitize=address,undefined,leak
LIB_LD_FLAGS = -shared-libasan
LIBS += -lssl -lcrypto -lwebsockets -lstdc++ $(wildcard src/lib/libiso8601/*.c.o)

WITH_DEBUG ?= yes

ifeq ($(WITH_DEBUG),yes)
	COMPILE_FLAGS += -g -ggdb
endif

SOURCES :=	$(wildcard src/ocpp/*.cpp) \
		    lib/mongoose/mongoose.cpp \

CFILES := src/lib/musl_libc_timegm.c

SOURCES_LIB := $(SOURCES) src/platforms/TestPlatform.cpp
SOURCES_EXEC := $(SOURCES) src/platforms/LinuxPlatform.cpp

OBJECTS_LIB := ${SOURCES_LIB:.cpp=.o} ${CFILES:.c=.o}
OBJECTS_EXEC := ${SOURCES_EXEC:.cpp=.o} ${CFILES:.c=.o}

$(OBJECTS_LIB): CXXFLAGS := $(CXXFLAGS) -DOCPP_PLATFORM_TEST
$(OBJECTS_EXEC): CXXFLAGS := $(CXXFLAGS) -DOCPP_PLATFORM_LINUX

all: libocpp.so ocpp_linux

libocpp.so: $(OBJECTS_LIB)
	$(CXX) $(OBJECTS_LIB) $(LIBS) $(LDFLAGS) $(LIB_LD_FLAGS) -shared -o $@

ocpp_linux: $(OBJECTS_EXEC)
	$(CXX) $(OBJECTS_EXEC) $(LIBS) $(LDFLAGS) -o $@

.PHONY: all clean

clean: Makefile
	$(E)$(RM) $(OBJECTS_LIB) $(OBJECTS_EXEC) libocpp.so
