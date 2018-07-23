# Define the applications properties here:

TARGET = ./dist/PocketSNES.dge

CHAINPREFIX := /opt/rs97-toolchain-musl
CROSS_COMPILE := $(CHAINPREFIX)/usr/bin/mipsel-linux-

CC  := $(CROSS_COMPILE)gcc
CXX := $(CROSS_COMPILE)g++
STRIP := $(CROSS_COMPILE)strip

SYSROOT := $(shell $(CC) --print-sysroot)
SDL_CFLAGS := $(shell $(SYSROOT)/usr/bin/sdl-config --cflags)
SDL_LIBS := $(shell $(SYSROOT)/usr/bin/sdl-config --libs)

INCLUDE = -I pocketsnes \
		-I sal/linux/include -I sal/include \
		-I pocketsnes/include \
		-I menu -I pocketsnes/linux -I pocketsnes/snes9x

# CFLAGS =  -std=gnu++03 $(INCLUDE) -DRC_OPTIMIZED -D__LINUX__ -D__DINGUX__ -DFOREVER_16_BIT  $(SDL_CFLAGS)
CFLAGS =  -std=gnu++03 $(INCLUDE) -DRC_OPTIMIZED -D__LINUX__ -D__DINGUX__ $(SDL_CFLAGS)
CFLAGS += -O2 -fdata-sections -ffunction-sections -mips32 -march=mips32 -mno-mips16 -fomit-frame-pointer -fno-builtin
CFLAGS += -fno-common -Wno-write-strings -Wno-sign-compare -ffast-math -ftree-vectorize
CFLAGS += -funswitch-loops -fno-strict-aliasing
CFLAGS += -DMIPS_XBURST -DFAST_LSB_WORD_ACCESS -DNO_ROM_BROWSER

CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti -fno-math-errno -fno-threadsafe-statics

LDFLAGS = $(CXXFLAGS) -lpthread -lz -lpng  $(SDL_LIBS) -flto -Wl,--as-needed -Wl,--gc-sections -s

# Find all source files
SOURCE = pocketsnes/snes9x menu sal/linux sal
SRC_CPP = $(foreach dir, $(SOURCE), $(wildcard $(dir)/*.cpp))
SRC_C   = $(foreach dir, $(SOURCE), $(wildcard $(dir)/*.c))
OBJ_CPP = $(patsubst %.cpp, %.o, $(SRC_CPP))
OBJ_C   = $(patsubst %.c, %.o, $(SRC_C))
OBJS    = $(OBJ_CPP) $(OBJ_C)

.PHONY : all
all : $(TARGET)

$(TARGET) : $(OBJS)
	$(CMD)$(CXX) $(CXXFLAGS) $^ $(LDFLAGS) -o $@

%.o: %.c
	$(CMD)$(CC) $(CFLAGS) -c $< -o $@

%.o: %.cpp
	$(CMD)$(CXX) $(CFLAGS) -c $< -o $@

.PHONY : clean
clean :
	$(CMD)rm -f $(OBJS) $(TARGET)
	$(CMD)rm -rf .opk_data $(TARGET).opk
