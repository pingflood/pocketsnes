#
# PocketSNES for the RetroFW
#
# by pingflood; 2019
#

# Define the applications properties here:

TARGET = pocketsnes/pocketsnes.dge

CHAINPREFIX := /opt/mipsel-linux-uclibc
CROSS_COMPILE := $(CHAINPREFIX)/usr/bin/mipsel-linux-

CC  := $(CROSS_COMPILE)gcc
CXX := $(CROSS_COMPILE)g++
STRIP := $(CROSS_COMPILE)strip

SYSROOT := $(shell $(CC) --print-sysroot)
SDL_CFLAGS := $(shell $(SYSROOT)/usr/bin/sdl-config --cflags)
SDL_LIBS := $(shell $(SYSROOT)/usr/bin/sdl-config --libs)

INCLUDE = -I src \
		-I sal/linux/include -I sal/include \
		-I src/include \
		-I menu -I src/linux -I src/snes9x

CFLAGS =  -std=gnu++03 $(INCLUDE) -DRC_OPTIMIZED -DGCW_ZERO -D__LINUX__ -D__DINGUX__ -DFOREVER_16_BIT  $(SDL_CFLAGS)
# CFLAGS =  -std=gnu++03 $(INCLUDE) -DRC_OPTIMIZED -D__LINUX__ -D__DINGUX__ $(SDL_CFLAGS)
CFLAGS += -O3 -fdata-sections -ffunction-sections -mips32 -march=mips32 -mno-mips16 -fomit-frame-pointer -fno-builtin
CFLAGS += -fno-common -Wno-write-strings -Wno-sign-compare -ffast-math -ftree-vectorize
CFLAGS += -funswitch-loops -fno-strict-aliasing
CFLAGS += -DMIPS_XBURST -DFAST_LSB_WORD_ACCESS
# CFLAGS += -flto
# CFLAGS += -fprofile-generate -fprofile-dir=/home/retrofw/profile/pocketsnes
CFLAGS += -fprofile-use -fprofile-dir=./profile

CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti -fno-math-errno -fno-threadsafe-statics

LDFLAGS = $(CXXFLAGS) -lpthread -lz -lpng  $(SDL_LIBS) -Wl,--as-needed -Wl,--gc-sections -s

# Find all source files
SOURCE = src/snes9x menu sal/linux sal
SRC_CPP = $(foreach dir, $(SOURCE), $(wildcard $(dir)/*.cpp))
SRC_C   = $(foreach dir, $(SOURCE), $(wildcard $(dir)/*.c))
OBJ_CPP = $(patsubst %.cpp, %.o, $(SRC_CPP))
OBJ_C   = $(patsubst %.c, %.o, $(SRC_C))
OBJS    = $(OBJ_CPP) $(OBJ_C)

.PHONY : all
all : $(TARGET)

$(TARGET) : $(OBJS)
	$(CMD)$(CXX) $(CXXFLAGS) $^ $(LDFLAGS) -o $@

ipk: all
	@rm -rf /tmp/.pocketsnes-ipk/ && mkdir -p /tmp/.pocketsnes-ipk/root/home/retrofw/emus/pocketsnes /tmp/.pocketsnes-ipk/root/home/retrofw/apps/gmenu2x/sections/emulators /tmp/.pocketsnes-ipk/root/home/retrofw/apps/gmenu2x/sections/emulators.systems
	@cp dist/PocketSNES.dge dist/PocketSNES.man.txt dist/PocketSNES.png dist/backdrop.png /tmp/.pocketsnes-ipk/root/home/retrofw/emus/pocketsnes
	@cp dist/pocketsnes.lnk /tmp/.pocketsnes-ipk/root/home/retrofw/apps/gmenu2x/sections/emulators
	@cp dist/snes.pocketsnes.lnk /tmp/.pocketsnes-ipk/root/home/retrofw/apps/gmenu2x/sections/emulators.systems
	# @sed "s/^Version:.*/Version: $$(date +%Y%m%d)/" dist/control > /tmp/.pocketsnes-ipk/control
	@sed "s/^Version:.*/Version: 20190304/" dist/control > /tmp/.pocketsnes-ipk/control
	@cp dist/conffiles /tmp/.pocketsnes-ipk/
	# echo -e "#!/bin/sh\nmkdir -p /home/retrofw/profile/pocketsnes; exit 0" > /tmp/.pocketsnes-ipk/preinst
	# chmod +x /tmp/.gmenu-ipk/postinst /tmp/.pocketsnes-ipk/preinst
	@tar --owner=0 --group=0 -czvf /tmp/.pocketsnes-ipk/control.tar.gz -C /tmp/.pocketsnes-ipk/ control conffiles #preinst
	@tar --owner=0 --group=0 -czvf /tmp/.pocketsnes-ipk/data.tar.gz -C /tmp/.pocketsnes-ipk/root/ .
	@echo 2.0 > /tmp/.pocketsnes-ipk/debian-binary
	@ar r dist/pocketsnes.ipk /tmp/.pocketsnes-ipk/control.tar.gz /tmp/.pocketsnes-ipk/data.tar.gz /tmp/.pocketsnes-ipk/debian-binary

%.o: %.c
	$(CMD)$(CC) $(CFLAGS) -c $< -o $@

%.o: %.cpp
	$(CMD)$(CXX) $(CFLAGS) -c $< -o $@

.PHONY : clean

opk: all
	@mksquashfs \
	pocketsnes/default.retrofw.desktop \
	pocketsnes/snes.retrofw.desktop \
	pocketsnes/pocketsnes.dge \
	pocketsnes/pocketsnes.man.txt \
	pocketsnes/pocketsnes.png \
	pocketsnes/backdrop.png \
	pocketsnes/pocketsnes.opk \
	-all-root -noappend -no-exports -no-xattrs

clean :
	$(CMD)rm -f $(OBJS) $(TARGET) pocketsnes/pocketsnes.ipk
