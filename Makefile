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

PRIORITY  = SCHED_PRIORITY_DEFAULT
STACKSIZE = 16384
MODULE    = $(CONFIG_EXAMPLES_DIRECTFB)

CFLAGS += -Idata
CFLAGS += -DDFB_CORE_SYSTEM=nuttxfb
CFLAGS += -DDFB_INPUT_DRIVER=nuttx_input
CFLAGS += -DDFB_FONT_PROVIDER=$(CONFIG_EXAMPLES_DIRECTFB_FONT_PROVIDER)
CFLAGS += -DDFB_IMAGE_PROVIDER=$(CONFIG_EXAMPLES_DIRECTFB_IMAGE_PROVIDER)
CFLAGS += -DDFB_VIDEO_PROVIDER=$(CONFIG_EXAMPLES_DIRECTFB_VIDEO_PROVIDER)
CFLAGS += -DDFB_WINDOW_MANAGER=default
CFLAGS += -DUSE_FONT_HEADERS
CFLAGS += -DUSE_IMAGE_HEADERS
CFLAGS += -DUSE_VIDEO_HEADERS

ifeq ($(CONFIG_DF_ANDI),y)
ifeq ($(CONFIG_NSH_BUILTIN_APPS),y)
PROGNAME = df_andi
else
PROGNAME = directfb
endif
MAINSRC = src/df_andi.c
endif

ifeq ($(CONFIG_DF_DOK),y)
ifeq ($(CONFIG_NSH_BUILTIN_APPS),y)
PROGNAME = df_dok
else
PROGNAME = directfb
endif
MAINSRC = src/df_dok.c
endif

ifeq ($(CONFIG_DF_FIRE),y)
ifeq ($(CONFIG_NSH_BUILTIN_APPS),y)
PROGNAME = df_fire
else
PROGNAME = directfb
endif
MAINSRC = src/df_fire.c
endif

ifeq ($(CONFIG_DF_GEARS),y)
CFLAGS += -DDFB_OPENGL_IMPLEMENTATION=$(CONFIG_EXAMPLES_DIRECTFB_OPENGL_IMPLEMENTATION)
CFLAGS += -DOPENGL_H=\"idirectfbgl_portablegl.h\"
ifeq ($(CONFIG_NSH_BUILTIN_APPS),y)
PROGNAME = df_gears
else
PROGNAME = directfb
endif
MAINSRC = src/df_gears.c
endif

ifeq ($(CONFIG_DF_INPUT),y)
ifeq ($(CONFIG_NSH_BUILTIN_APPS),y)
PROGNAME = df_input
else
PROGNAME = directfb
endif
MAINSRC = src/df_input.c
endif

ifeq ($(CONFIG_DF_KNUCKLES),y)
ifeq ($(CONFIG_NSH_BUILTIN_APPS),y)
PROGNAME = df_knuckles
else
PROGNAME = directfb
endif
MAINSRC = src/df_knuckles.c
endif

ifeq ($(CONFIG_DF_MATRIX),y)
ifeq ($(CONFIG_NSH_BUILTIN_APPS),y)
PROGNAME = df_matrix
else
PROGNAME = directfb
endif
MAINSRC = src/df_matrix.c
endif

ifeq ($(CONFIG_DF_NEO),y)
ifeq ($(CONFIG_NSH_BUILTIN_APPS),y)
PROGNAME = df_neo
else
PROGNAME = directfb
endif
MAINSRC = src/df_neo.c
endif

ifeq ($(CONFIG_DF_PARTICLE),y)
ifeq ($(CONFIG_NSH_BUILTIN_APPS),y)
PROGNAME = df_particle
else
PROGNAME = directfb
endif
MAINSRC = src/df_particle.c
endif

ifeq ($(CONFIG_DF_TEXTURE),y)
ifeq ($(CONFIG_NSH_BUILTIN_APPS),y)
PROGNAME = df_texture
else
PROGNAME = directfb
endif
MAINSRC = src/df_texture.c
endif

ifeq ($(CONFIG_DF_VIDEO),y)
ifeq ($(CONFIG_NSH_BUILTIN_APPS),y)
PROGNAME = df_video
else
PROGNAME = directfb
endif
MAINSRC = src/df_video.c
endif

ifeq ($(CONFIG_DF_WINDOW),y)
ifeq ($(CONFIG_NSH_BUILTIN_APPS),y)
PROGNAME = df_window
else
PROGNAME = directfb
endif
MAINSRC = src/df_window.c
endif

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

data/%.h: data/%.$(shell echo $(CONFIG_EXAMPLES_DIRECTFB_IMAGE_EXTENSION))
	$(DIRECTFB_CSOURCE) --raw $^ --name=$* > $@

data/%.h: data/%.$(shell echo $(CONFIG_EXAMPLES_DIRECTFB_VIDEO_EXTENSION))
	$(DIRECTFB_CSOURCE) --raw $^ --name=$* > $@

data/%.h: data/%.$(shell echo $(CONFIG_EXAMPLES_DIRECTFB_FONT_EXTENSION))
	$(DIRECTFB_CSOURCE) --raw $^ --name=$* > $@

src/df_andi.c: data/decker.h data/destination_mask.h data/tux.h data/tux_alpha.h data/wood_andi.h
src/df_dok.c: data/decker.h data/biglogo.h data/card.h data/colorkeyed.h data/fish.h data/intro.h data/laden_bike.h data/melted.h data/meter.h data/rose.h data/sacred_heart.h data/swirl.h
src/df_input.c: data/decker.h data/joystick.h data/keys.h data/mouse.h
src/df_neo.c: data/apple-red.h data/background.h data/gnome-applets.h data/gnome-calendar.h data/gnome-foot.h data/gnome-gimp.h data/gnome-gmush.h data/gnome-gsame.h data/gnu-keys.h
src/df_texture.c: data/decker.h data/texture.h
src/df_video.c: data/panel.h data/bbb.h
src/df_window.c: data/decker.h data/cursor_red.h data/cursor_yellow.h data/dfblogo.h

context:: $(RAWDATA_HDRS)

distclean::
	$(call DELFILE, $(RAWDATA_HDRS))

include $(APPDIR)/Application.mk
