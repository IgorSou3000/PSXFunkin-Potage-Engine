TARGET = funkin
TYPE = ps-exe

SRCS = src/psx/main.c \
       src/psx/mutil.c \
       src/psx/random.c \
       src/psx/archive.c \
       src/psx/font.c \
       src/psx/trans.c \
       src/psx/loadscr.c \
       src/menu/menu.c \
       src/stage/stage.c \
       src/stage/debug.c \
       src/stage/events.c \
       src/psx/save.c \
       src/psx/psx.c \
       src/psx/io.c \
       src/psx/gfx.c \
       src/psx/audio.c \
       src/psx/pad.c \
       src/psx/timer.c \
       src/psx/movie.c \
       src/psx/strplay.c \
       src/week0/dummy.c \
       src/week1/week1.c \
       src/psx/animation.c \
       src/psx/character.c \
       src/characters/bf.c \
       src/characters/speaker.c \
       src/characters/dad.c \
       src/characters/gf.c \
       src/psx/object.c \
       src/object/combo.c \
       src/object/splash.c \
       mips/common/crt0/crt0.s

CPPFLAGS += -Wall -Wextra -pedantic -Isrc/ -mno-check-zero-division
LDFLAGS += -Wl,--start-group
# TODO: remove unused libraries
LDFLAGS += -lapi
#LDFLAGS += -lc
LDFLAGS += -lc2
LDFLAGS += -lcard
LDFLAGS += -lcd
#LDFLAGS += -lcomb
LDFLAGS += -lds
LDFLAGS += -letc
LDFLAGS += -lgpu
#LDFLAGS += -lgs
#LDFLAGS += -lgte
#LDFLAGS += -lgun
#LDFLAGS += -lhmd
#LDFLAGS += -lmath
LDFLAGS += -lmcrd
#LDFLAGS += -lmcx
LDFLAGS += -lpad
LDFLAGS += -lpress
#LDFLAGS += -lsio
LDFLAGS += -lsnd
LDFLAGS += -lspu
#LDFLAGS += -ltap
LDFLAGS += -flto -Wl,--end-group

include mips/common.mk
