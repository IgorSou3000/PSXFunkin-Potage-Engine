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
       src/week2/week2.c \
       src/week3/week3.c \
       src/week4/week4.c \
       src/week5/week5.c \
       src/week6/week6.c \
       src/week7/week7.c \
       src/psx/animation.c \
       src/psx/character.c \
       src/character/bf.c \
       src/character/bfweeb.c \
       src/character/speaker.c \
       src/character/dad.c \
       src/character/spook.c \
       src/character/monster.c \
       src/character/pico.c \
       src/character/mom.c \
       src/character/xmasbf.c \
       src/character/xmasp.c \
       src/character/monsterx.c \
       src/character/senpai.c \
       src/character/senpaim.c \
       src/character/spirit.c \
       src/character/bfgf.c \
       src/character/tank.c \
       src/character/picospeaker.c \
       src/character/gf.c \
       src/character/gfweeb.c \
       src/character/clucky.c \
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
