.SILENT:

WITH_DEBUG ?= yes
WITH_SANITIZERS ?= yes
WITH_TLS ?= yes
STATIC ?= no

CC = clang
CXX = clang++

CLANG_WARNINGS = -Weverything \
				 -Wno-c++98-compat \
				 -Wno-c++98-compat-pedantic \
				 -Wno-old-style-cast \
				 -Wno-shadow-field-in-constructor \
				 -Wno-padded \
				 -Wno-exit-time-destructors \
				 -Wno-double-promotion \
				 -Wno-implicit-int-float-conversion \
				 -Wno-unsafe-buffer-usage

COMPILE_FLAGS = -DOCPP_LOG_LEVEL=4 \
			    -DOCPP_STATE_CALLBACKS \
			    -gdwarf-4 \
			    -fPIC \
			    -O0 \
			    ${CLANG_WARNINGS} \
			    -fdiagnostics-color=always \
			    -Ilib/ArduinoJson \
			    -Ilib/mongoose \
			    -Isrc \
			    -fno-exceptions \
			    -fno-rtti \
			    -DOCPP_METER_VALUES_ALIGNED_DATA_MAX_LENGTH=20 \
			    -DOCPP_DEFAULT_CLOCK_ALIGNED_DATA_INTERVAL=60 \
			    -DOCPP_DEFAULT_METER_VALUES_ALIGNED_DATA="\"Energy.Active.Import.Register\"" \
			    -DOCPP_METER_VALUES_SAMPLED_DATA_MAX_LENGTH=20 \
			    -DOCPP_DEFAULT_METER_VALUE_SAMPLE_INTERVAL=60 \
			    -DOCPP_DEFAULT_METER_VALUES_SAMPLED_DATA="\"Energy.Active.Import.Register\""

ifeq ($(WITH_SANITIZERS),yes)
	COMPILE_FLAGS += -fsanitize=address,undefined,leak
	LDFLAGS += -fsanitize=address,undefined,leak
	LIB_LD_FLAGS = -shared-libasan
endif

ifeq ($(WITH_DEBUG),yes)
	COMPILE_FLAGS += -g -ggdb
endif

ifeq ($(WITH_TLS),yes)
	LIBS += -lssl -lcrypto
	COMPILE_FLAGS += -DMG_ENABLE_OPENSSL=1
endif

CFLAGS += -std=c99 ${COMPILE_FLAGS}
CXXFLAGS += -std=c++11 -stdlib=libc++ ${COMPILE_FLAGS}
LDFLAGS += -pthread

STATIC_FLAG =

ifeq ($(STATIC),yes)
	LIBS += ./libwebsockets.a
	STATIC_FLAG += -static
else
	LIBS += -lwebsockets -lc++
endif

SOURCES :=	$(wildcard src/ocpp/*.cpp) \
		    lib/mongoose/mongoose.cpp \

CFILES := src/lib/musl_libc_timegm.c \
		  $(wildcard src/lib/libiso8601/*.c)

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
	$(CXX) $(OBJECTS_EXEC) $(LIBS) $(LDFLAGS) -o $@ $(STATIC_FLAG)

.PHONY: all clean

clean: Makefile
	$(E)$(RM) $(OBJECTS_LIB) $(OBJECTS_EXEC) libocpp.so
