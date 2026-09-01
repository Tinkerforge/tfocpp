.SILENT:

MAKEFLAGS += --jobs=$(shell nproc)

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
				-I../tfjson/src \
				-I../tftools/src \
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
				-DOCPP_DEFAULT_METER_VALUES_SAMPLED_DATA="\"Energy.Active.Import.Register\"" \
				-D_POSIX_C_SOURCE # for localtime_r and gmtime_r

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
	COMPILE_FLAGS += -DMG_ENABLE_OPENSSL=1 -DMG_ENABLE_OPENSSL_NO_COMPRESSION
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

SOURCES_COMMON :=	$(wildcard src/common/*.cpp) \
					lib/mongoose/mongoose.cpp \
					src/platforms/LinuxWS.cpp

SOURCES_16 := $(wildcard src/ocpp16/*.cpp) $(SOURCES_COMMON)
SOURCES_21 := $(wildcard src/ocpp21/*.cpp) $(SOURCES_COMMON)

CFILES := src/lib/musl_libc_timegm.c \
		  $(wildcard src/lib/libiso8601/*.c)

ED448_DIR := lib/tfed448
ED448_CFILES := $(ED448_DIR)/src/tf_ed448.c \
				$(ED448_DIR)/src/libdecaf/utils.c \
				$(ED448_DIR)/src/libdecaf/shake.c \
				$(ED448_DIR)/src/libdecaf/p448/arch_32/f_impl.c \
				$(ED448_DIR)/src/libdecaf/p448/f_arithmetic.c \
				$(ED448_DIR)/src/libdecaf/ed448goldilocks/decaf_tables.c \
				$(ED448_DIR)/src/libdecaf/generated/p448/f_generic.c \
				$(ED448_DIR)/src/libdecaf/generated/ed448goldilocks/scalar.c \
				$(ED448_DIR)/src/libdecaf/generated/ed448goldilocks/decaf.c \
				$(ED448_DIR)/src/libdecaf/generated/ed448goldilocks/eddsa.c
ED448_FLAGS := -DDECAF_WORD_BITS=32 -DMBEDTLS_ED448_C \
			   -I$(ED448_DIR)/include \
			   -I$(ED448_DIR)/src \
			   -I$(ED448_DIR)/src/libdecaf/include \
			   -I$(ED448_DIR)/src/libdecaf/include/arch_32 \
			   -I$(ED448_DIR)/src/libdecaf/p448 \
			   -I$(ED448_DIR)/src/libdecaf/p448/arch_32 \
			   -I$(ED448_DIR)/src/libdecaf/generated/p448

SOURCES_LIB := $(SOURCES_16) src/platforms/TestPlatform.cpp
SOURCES_EXEC := $(SOURCES_16) src/platforms/LinuxPlatform16.cpp
SOURCES_EXEC_21 := $(SOURCES_21) src/platforms/LinuxPlatform21.cpp src/platforms/LinuxCrypto21.cpp
SOURCES_EXEC_21_MBEDTLS := $(SOURCES_21) src/platforms/LinuxPlatform21.cpp src/platforms/MbedCrypto21.cpp

MBEDTLS_DIR := lib/mbedtls
MBEDTLS_LIBS := $(MBEDTLS_DIR)/library/libmbedx509.a $(MBEDTLS_DIR)/library/libmbedcrypto.a
MBEDTLS_TLS_LIBS := $(MBEDTLS_DIR)/library/libmbedtls.a $(MBEDTLS_LIBS)
MBEDTLS_INPUTS := $(wildcard $(MBEDTLS_DIR)/include/mbedtls/*.h) \
				  $(wildcard $(MBEDTLS_DIR)/library/*.c) \
				  $(wildcard $(MBEDTLS_DIR)/library/*.h) \
				  $(MBEDTLS_DIR)/library/Makefile Makefile $(ED448_CFILES)

# Each build variant compiles with a different platform define,
# so each variant gets its own object directory below build/.
OBJECTS_LIB := $(SOURCES_LIB:%.cpp=build/test/%.o) $(CFILES:%.c=build/test/%.o)
OBJECTS_EXEC := $(SOURCES_EXEC:%.cpp=build/linux/%.o) $(CFILES:%.c=build/linux/%.o)
OBJECTS_21 := $(SOURCES_EXEC_21:%.cpp=build/linux21/%.o) $(CFILES:%.c=build/linux21/%.o)
ED448_OBJECTS := $(ED448_CFILES:%.c=build/linux21_mbedtls/%.o)
OBJECTS_21_MBEDTLS := $(SOURCES_EXEC_21_MBEDTLS:%.cpp=build/linux21_mbedtls/%.o) $(CFILES:%.c=build/linux21_mbedtls/%.o) $(ED448_OBJECTS)

build/test/%.o: %.cpp
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -MMD -MP -DOCPP_PLATFORM_TEST -c $< -o $@

build/test/%.o: %.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -MMD -MP -DOCPP_PLATFORM_TEST -c $< -o $@

build/linux/%.o: %.cpp
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -MMD -MP -DOCPP_PLATFORM_LINUX -c $< -o $@

build/linux/%.o: %.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -MMD -MP -DOCPP_PLATFORM_LINUX -c $< -o $@

build/linux21/%.o: %.cpp
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -MMD -MP -DOCPP_PLATFORM_LINUX21 -c $< -o $@

build/linux21/%.o: %.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -MMD -MP -DOCPP_PLATFORM_LINUX21 -c $< -o $@

build/linux21_mbedtls/%.o: %.cpp
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(ED448_FLAGS) -MMD -MP -DOCPP_PLATFORM_LINUX21 -DOCPP_CRYPTO_MBEDTLS -isystem $(MBEDTLS_DIR)/include -c $< -o $@

build/linux21_mbedtls/%.o: %.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(ED448_FLAGS) -MMD -MP -DOCPP_PLATFORM_LINUX21 -DOCPP_CRYPTO_MBEDTLS -c $< -o $@

$(MBEDTLS_TLS_LIBS) &: $(MBEDTLS_INPUTS)
	test -f $(MBEDTLS_DIR)/library/Makefile || { echo "lib/mbedtls is missing, run: git submodule update --init --recursive"; exit 1; }
	$(MAKE) -C $(MBEDTLS_DIR)/library clean
	$(MAKE) -C $(MBEDTLS_DIR)/library CFLAGS="-O2 -DMBEDTLS_ED448_C -I$(abspath $(ED448_DIR)/include)" libmbedcrypto.a libmbedx509.a libmbedtls.a

build/mbedtls_ed448_tls_test.o: test/mbedtls_ed448_tls_test.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(ED448_FLAGS) -I$(MBEDTLS_DIR)/include -c $< -o $@

mbedtls_ed448_tls_test: build/mbedtls_ed448_tls_test.o $(ED448_OBJECTS) $(MBEDTLS_TLS_LIBS)
	$(CC) build/mbedtls_ed448_tls_test.o $(ED448_OBJECTS) $(MBEDTLS_TLS_LIBS) $(LDFLAGS) -o $@
	./$@

build/mbedtls_ticket_expiry_test.o: test/mbedtls_ticket_expiry_test.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -DMBEDTLS_ALLOW_PRIVATE_ACCESS -I$(MBEDTLS_DIR)/include -c $< -o $@

mbedtls_ticket_expiry_test: build/mbedtls_ticket_expiry_test.o
	$(MAKE) -C $(MBEDTLS_DIR)/library CFLAGS="-O2" libmbedcrypto.a libmbedx509.a libmbedtls.a
	$(CC) build/mbedtls_ticket_expiry_test.o $(MBEDTLS_TLS_LIBS) $(LDFLAGS) -o $@
	./$@

all: libocpp.so ocpp16_linux ocpp21_linux ocpp21_linux_mbedtls

cert_store_test: test/cert_store_test.cpp src/ocpp21/CertStore21.cpp
	$(CXX) $(CXXFLAGS) -DOCPP_PLATFORM_TEST $^ -o $@
	./$@

libocpp.so: $(OBJECTS_LIB)
	$(CXX) $(OBJECTS_LIB) $(LIBS) $(LDFLAGS) $(LIB_LD_FLAGS) -shared -o $@

ocpp16_linux: $(OBJECTS_EXEC)
	$(CXX) $(OBJECTS_EXEC) $(LIBS) $(LDFLAGS) -o $@ $(STATIC_FLAG)

ocpp21_linux: $(OBJECTS_21)
	$(CXX) $(OBJECTS_21) $(LIBS) $(LDFLAGS) -o $@ $(STATIC_FLAG)

ocpp21_linux_mbedtls: $(OBJECTS_21_MBEDTLS) $(MBEDTLS_LIBS)
	$(CXX) $(OBJECTS_21_MBEDTLS) $(MBEDTLS_LIBS) $(LIBS) $(LDFLAGS) -o $@ $(STATIC_FLAG)

# Header dependency tracking, generated by -MMD.
-include $(OBJECTS_LIB:.o=.d) $(OBJECTS_EXEC:.o=.d) $(OBJECTS_21:.o=.d) $(OBJECTS_21_MBEDTLS:.o=.d)

.PHONY: all cert_store_test mbedtls_ed448_tls_test clean

clean: Makefile
	$(E)$(RM) -r build libocpp.so ocpp16_linux ocpp21_linux ocpp21_linux_mbedtls cert_store_test mbedtls_ed448_tls_test mbedtls_ticket_expiry_test
