#  This file is part of DirectFB-examples.
#
#  Permission is hereby granted, free of charge, to any person obtaining a copy
#  of this software and associated documentation files (the "Software"), to deal
#  in the Software without restriction, including without limitation the rights
#  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
#  copies of the Software, and to permit persons to whom the Software is
#  furnished to do so, subject to the following conditions:
#
#  The above copyright notice and this permission notice shall be included in
#  all copies or substantial portions of the Software.
#
#  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
#  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
#  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
#  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
#  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
#  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
#  THE SOFTWARE.

include $(APPDIR)/Make.defs

PROGNAME  = $(CONFIG_EXAMPLES_DIRECTFB_PROGNAME)
PRIORITY  = $(CONFIG_EXAMPLES_DIRECTFB_PRIORITY)
STACKSIZE = $(CONFIG_EXAMPLES_DIRECTFB_STACKSIZE)
MODULE    = $(CONFIG_EXAMPLES_DIRECTFB)

ifeq ($(CONFIG_DF_ANDI),y)
MAINSRC = src/df_andi.c
endif
ifeq ($(CONFIG_DF_DOK),y)
MAINSRC = src/df_dok.c
endif
ifeq ($(CONFIG_DF_FIRE),y)
MAINSRC = src/df_fire.c
endif
ifeq ($(CONFIG_DF_INPUT),y)
MAINSRC = src/df_input.c
endif
ifeq ($(CONFIG_DF_KNUCKLES),y)
MAINSRC = src/df_knuckles.c
endif
ifeq ($(CONFIG_DF_MATRIX),y)
MAINSRC = src/df_matrix.c
endif
ifeq ($(CONFIG_DF_NEO),y)
MAINSRC = src/df_neo.c
endif
ifeq ($(CONFIG_DF_PARTICLE),y)
MAINSRC = src/df_particle.c
endif
ifeq ($(CONFIG_DF_TEXTURE),y)
MAINSRC = src/df_texture.c
endif
ifeq ($(CONFIG_DF_VIDEO),y)
MAINSRC = src/df_video.c
endif
ifeq ($(CONFIG_DF_WINDOW),y)
MAINSRC = src/df_window.c
endif

CFLAGS += -Idata
CFLAGS += -DDIRECTFB_MAIN_ENTRYPOINT
CFLAGS += -DDFB_CORE_SYSTEM=nuttxfb
CFLAGS += -DDFB_INPUT_DRIVER=nuttx_input
CFLAGS += -DDFB_FONT_PROVIDER=DGIFF
CFLAGS += -DDFB_IMAGE_PROVIDER=DFIFF
CFLAGS += -DDFB_VIDEO_PROVIDER=DFVFF
CFLAGS += -DDFB_WINDOW_MANAGER=default
CFLAGS += -DUSE_FONT_HEADERS
CFLAGS += -DUSE_IMAGE_HEADERS
CFLAGS += -DUSE_VIDEO_HEADERS

RAWDATA_HDRS  = data/apple-red.h
RAWDATA_HDRS += data/bbb.h
RAWDATA_HDRS += data/background.h
RAWDATA_HDRS += data/biglogo.h
RAWDATA_HDRS += data/card.h
RAWDATA_HDRS += data/colorkeyed.h
RAWDATA_HDRS += data/cursor_red.h
RAWDATA_HDRS += data/cursor_yellow.h
RAWDATA_HDRS += data/decker.h
RAWDATA_HDRS += data/destination_mask.h
RAWDATA_HDRS += data/dfblogo.h
RAWDATA_HDRS += data/fish.h
RAWDATA_HDRS += data/gnome-applets.h
RAWDATA_HDRS += data/gnome-calendar.h
RAWDATA_HDRS += data/gnome-foot.h
RAWDATA_HDRS += data/gnome-gimp.h
RAWDATA_HDRS += data/gnome-gmush.h
RAWDATA_HDRS += data/gnome-gsame.h
RAWDATA_HDRS += data/gnu-keys.h
RAWDATA_HDRS += data/intro.h
RAWDATA_HDRS += data/joystick.h
RAWDATA_HDRS += data/keys.h
RAWDATA_HDRS += data/laden_bike.h
RAWDATA_HDRS += data/melted.h
RAWDATA_HDRS += data/meter.h
RAWDATA_HDRS += data/mouse.h
RAWDATA_HDRS += data/panel.h
RAWDATA_HDRS += data/rose.h
RAWDATA_HDRS += data/sacred_heart.h
RAWDATA_HDRS += data/swirl.h
RAWDATA_HDRS += data/texture.h
RAWDATA_HDRS += data/tux.h
RAWDATA_HDRS += data/tux_alpha.h
RAWDATA_HDRS += data/wood_andi.h

DIRECTFB_CSOURCE ?= directfb-csource

data/%.h: data/%.dfiff
	$(DIRECTFB_CSOURCE) --raw $^ --name=$* > $@

data/%.h: data/%.dfvff
	$(DIRECTFB_CSOURCE) --raw $^ --name=$* > $@

data/%.h: data/%.dgiff
	$(DIRECTFB_CSOURCE) --raw $^ --name=$* > $@

context:: $(RAWDATA_HDRS)

distclean::
	$(call DELFILE, $(RAWDATA_HDRS))

include $(APPDIR)/Application.mk
