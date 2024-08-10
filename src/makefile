# General
EXE     = Seraphina
CXXSRC  = bitboard.cpp evaluate.cpp movegen.cpp movepick.cpp \
	      nnue.cpp search.cpp Seraphina.cpp thread.cpp tt.cpp uci.cpp
CSRC	= syzygy/tbprobe.c
SRC     = $(CSRC) $(CXXSRC)
CXX     = g++
VERSION  = 1
SERA_NNUE = S2.nnue
EVALFILE = $(SERA_NNUE)
PARAMS  = -DVERSION=\"$(VERSION)\" -DEVALFILE=\"$(EVALFILE)\"

# Flags
STD    = -std=c++17
LIBS   = -pthread
LIBS_WL = -Wl,--whole-archive -lpthread -Wl,--no-whole-archive
WARN   = -Wall -Wextra -Wshadow

FLAGS   = $(STD) $(WARN) -g -O3 -flto $(PARAMS) -static -static-libgcc -static-libstdc++
M64     = -m64 -mpopcnt
MSSE2   = $(M64) -msse -msse2
MSSSE3  = $(MSSE2) -mssse3
MAVX2   = $(MSSSE3) -msse4.1 -mbmi -mfma -mavx2
MAVX512 = $(MAVX2) -mavx512f -mavx512bw
MAVX256VNNI = $(MAVX256) -mavx256vnni -mprefer-vector-width=256
MAVX512VNNI = $(MAVX512) -mavx512vnni -mprefer-vector-width=512

# Detecting windows
ifeq ($(shell echo "test"), "test")
	FLAGS += -static
endif

# Detecting Apple Silicon (ARM64)
UNAME := $(shell uname -m)
ifeq ($(UNAME), arm64)
    ARCH = arm64
endif

# Setup arch
ifeq ($(ARCH), )
   ARCH = native
endif

ifeq ($(ARCH), native)
	CXXFLAGS = $(FLAGS) -march=native
else ifeq ($(ARCH), arm64)
	CXXFLAGS = $(FLAGS) -arch arm64
else ifeq ($(findstring x86-64, $(ARCH)), x86-64)
	CXXFLAGS = $(FLAGS) $(M64)
else ifeq ($(findstring sse2, $(ARCH)), sse2)
	CXXFLAGS = $(FLAGS) $(MSSE2)
else ifeq ($(findstring ssse3, $(ARCH)), ssse3)
	CXXFLAGS = $(FLAGS) $(MSSSE3)
else ifeq ($(findstring avx2, $(ARCH)), avx2)
	CXXFLAGS = $(FLAGS) $(MAVX2)
else ifeq ($(findstring avx512, $(ARCH)), avx512)
	CXXFLAGS = $(FLAGS) $(MAVX512)
else ifeq ($(findstring vnni256, $(ARCH)), vnni256)
	CXXFLAGS += -DVNNI256
	CXXFLAGS = $(FLAGS) $(MAVX256VNNI)
else ifeq ($(findstring vnni512, $(ARCH)), vnni512)
	CXXFLAGS += -DVNNI256
	CXXFLAGS = $(FLAGS) $(MAVX512VNNI)
endif

build:
	$(MAKE) ARCH=$(ARCH) all

all:
	$(CXX) $(CXXFLAGS) $(SRC) $(LIBS) -o $(EXE)

clean:
	rm -f $(EXE)
