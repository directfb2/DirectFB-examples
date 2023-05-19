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

executable('df_andi',       ['df_andi.c',       rawdata_hdr], dependencies:  directfb_dep,               install: true)
executable('df_cpuload',    ['df_cpuload.c',    rawdata_hdr], dependencies:  directfb_dep,               install: true)
executable('df_dok',        ['df_dok.c',        rawdata_hdr], dependencies:  directfb_dep,               install: true)
executable('df_drivertest', ['df_drivertest.c', rawdata_hdr], dependencies:  directfb_dep,               install: true)
executable('df_fire',       ['df_fire.c',       rawdata_hdr], dependencies:  directfb_dep,               install: true)
executable('df_input',      ['df_input.c',      rawdata_hdr], dependencies:  directfb_dep,               install: true)
executable('df_knuckles',   ['df_knuckles.c',   rawdata_hdr], dependencies: [directfb_dep, libm_dep],    install: true)
executable('df_layers',     ['df_layers.c',     rawdata_hdr], dependencies:  directfb_dep,               install: true)
executable('df_matrix',     ['df_matrix.c',     rawdata_hdr], dependencies: [directfb_dep, libm_dep],    install: true)
executable('df_neo',        ['df_neo.c',        rawdata_hdr], dependencies: [directfb_dep, libm_dep],    install: true)
executable('df_palette',    ['df_palette.c',    rawdata_hdr], dependencies:  directfb_dep,               install: true)
executable('df_particle',   ['df_particle.c',   rawdata_hdr], dependencies: [directfb_dep, libm_dep],    install: true)
executable('df_porter',     ['df_porter.c',     rawdata_hdr], dependencies:  directfb_dep,               install: true)
executable('df_pss',        ['df_pss.c',        rawdata_hdr], dependencies: [directfb_dep, libm_dep],    install: true)
executable('df_spacedream', ['df_spacedream.c', dsdesc_hdr],  dependencies: [directfb_dep, libm_dep],    install: true)
executable('df_texture',    ['df_texture.c',    rawdata_hdr], dependencies: [directfb_dep, libm_dep],    install: true)
executable('df_video',      ['df_video.c',      rawdata_hdr], dependencies: [directfb_dep, libm_dep],    install: true)
executable('df_window',     ['df_window.c',     rawdata_hdr], dependencies: [directfb_dep, libm_dep],    install: true)

if fusionsound_dep.found()
executable('fs_advanced',    'fs_advanced.c',                 dependencies: [fusionsound_dep, libm_dep], install: true)
executable('fs_buffer',      'fs_buffer.c',                   dependencies:  fusionsound_dep,            install: true)
executable('fs_stream',      'fs_stream.c',                   dependencies: [fusionsound_dep, libm_dep], install: true)
endif
