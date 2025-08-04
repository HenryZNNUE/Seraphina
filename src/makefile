EXE     = Seraphina-$(VERSION)-$(ARCH)
CXXSRC  = bitboard.cpp evaluate.cpp movegen.cpp movepick.cpp nnue.cpp \
		  search.cpp selfplay/selfplay.cpp Seraphina.cpp thread.cpp tt.cpp uci.cpp
CSRC	= syzygy/tbprobe.c
SRC     = $(CSRC) $(CXXSRC)
CXX     = g++
VERSION  = $(shell grep -oP 'Version "\K[^"]+' types.h)
SERA_NNUE = $(shell grep -oP 'Seraphina_NNUE "\K[^"]+' nnue.h)
EVALFILE = $(SERA_NNUE)
PARAMS  = -DVERSION=\"$(VERSION)\" -DEVALFILE=\"$(EVALFILE)\"

STD    = -std=c++17
LIBS   = -pthread
# WARN   = -Wall -Wextra -Wshadow

OPT = -O3 -flto -fno-math-errno -funroll-loops -fno-exceptions
FLAGS   = $(STD) $(PARAMS) $(OPT) -s -pthread -static -DNDEBUG
M64     = -m64 -mpopcnt
MSSE2   = $(M64) -msse -msse2
MSSSE3  = $(MSSE2) -mssse3
MAVX2   = $(MSSSE3) -msse4.1 -mbmi -mfma -mavx2
MAVXVNNI = $(MAVX2) -avxvnni
MAVX512 = $(MAVX2) -mavx512f -mavx512bw
MAVX256VNNI = $(MAVX512) -mavx512vnni -mavx512dq -mavx512vl -mprefer-vector-width=256
MAVX512VNNI = $(MAVX512) -mavx512vnni -mavx512dq -mavx512vl -mprefer-vector-width=512

# Detecting windows
ifeq ($(shell echo "test"), "test")
	FLAGS += -static
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
else ifeq ($(findstring avxvnni, $(ARCH)), avxvnni)
	CXXFLAGS += -DVNNI
	CXXFLAGS = $(FLAGS) $(MAVXVNNI)
else ifeq ($(findstring avx512, $(ARCH)), avx512)
	CXXFLAGS = $(FLAGS) $(MAVX512)
else ifeq ($(findstring vnni256, $(ARCH)), vnni256)
	CXXFLAGS += -DVNNI
	CXXFLAGS = $(FLAGS) $(MAVX256VNNI)
else ifeq ($(findstring vnni512, $(ARCH)), vnni512)
	CXXFLAGS += -DVNNI
	CXXFLAGS = $(FLAGS) $(MAVX512VNNI)
endif

cyan='\e[1;96m'
blue='\e[1;94m'
yellow='\e[1;93m'
default='\e[0m'

download_nnue:
	@echo -e $(yellow)
	sh ./scripts/get_nnue.sh

build: download_nnue
	@echo -e $(cyan)"Building Seraphina $(VERSION) $(ARCH)..."$(blue)
	$(MAKE) ARCH=$(ARCH) all
	@echo -e $(cyan)"---Seraphina $(VERSION) $(ARCH) Build Finished---"$(default)

all:
	$(CXX) $(CXXFLAGS) $(SRC) $(LIBS) -o $(EXE)

clean:
	rm -f $(EXE)