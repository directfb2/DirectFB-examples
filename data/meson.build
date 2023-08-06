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

examplesdata = []

dsdesc_names = [
  'star1',
  'star2',
  'star3',
  'star4'
]

rawdata_names = [
  'apple-red',
  'background',
  'biglogo',
  'card',
  'colorkeyed',
  'cursor_red',
  'cursor_yellow',
  'destination_mask',
  'dfblogo',
  'fish',
  'gnome-applets',
  'gnome-calendar',
  'gnome-foot',
  'gnome-gimp',
  'gnome-gmush',
  'gnome-gsame',
  'gnu-keys',
  'grid',
  'intro',
  'joystick',
  'keys',
  'laden_bike',
  'mask',
  'melted',
  'meter',
  'mouse',
  'panel',
  'rose',
  'sacred_heart',
  'smokey_light',
  'swirl',
  'texture',
  'tux',
  'tux_alpha',
  'wood_andi'
]

dsdesc_hdrs = []
rawdata_hdrs = []

if font_headers == 'disabled'
    examplesdata += ['decker.dgiff', 'decker_argb.dgiff']
    examplesdata += 'decker.ttf'
elif font_headers == 'dgiff'
  decker_a8_data_hdr = custom_target('decker_a8',
                                     input: 'decker.dgiff',
                                     output: 'decker_a8.h',
                                     capture: true,
                                     command: [directfb_csource, '--raw', '--name=decker', '@INPUT@'])

  decker_argb_data_hdr = custom_target('decker_argb',
                                       input: 'decker_argb.dgiff',
                                       output: 'decker_argb.h',
                                       capture: true,
                                       command: [directfb_csource, '--raw', '--name=decker_argb', '@INPUT@'])

  rawdata_hdrs += custom_target('decker',
                                input: [decker_a8_data_hdr, decker_argb_data_hdr],
                                output: 'decker.h',
                                capture: true,
                                command: [cat, '@INPUT0@', '@INPUT1@'])
else
  rawdata_hdrs += custom_target('decker',
                                input: 'decker' + '.' + font_headers,
                                output: 'decker.h',
                                capture: true,
                                command: [directfb_csource, '--raw', '--name=decker', '@INPUT@'])
endif

if image_headers == 'disabled'
  foreach name : dsdesc_names
    examplesdata += name + '.dfiff'
    examplesdata += name + '.png'
  endforeach

  foreach name : rawdata_names
    examplesdata += name + '.dfiff'
    examplesdata += name + '.png'
  endforeach
else
  foreach dsdesc_name : dsdesc_names
    dsdesc_hdrs += custom_target(dsdesc_name,
                                 input: dsdesc_name + '.' + image_headers,
                                 output: dsdesc_name + '.h',
                                 capture: true,
                                 command: [directfb_csource, '--name=@0@'.format(dsdesc_name), '@INPUT@'])
  endforeach

  foreach rawdata_name : rawdata_names
    rawdata_hdrs += custom_target(rawdata_name,
                                  input: rawdata_name + '.' + image_headers,
                                  output: rawdata_name + '.h',
                                  capture: true,
                                  command: [directfb_csource, '--raw', '--name=@0@'.format(rawdata_name), '@INPUT@'])
  endforeach
endif

if video_headers == 'disabled'
  examplesdata += 'bbb.dfvff'
  examplesdata += 'bbb.mpg'
  examplesdata += 'bbb.m2v'
  examplesdata += 'bbb.264'
else
  rawdata_hdrs += custom_target('bbb',
                                input: 'bbb' + '.' + video_headers,
                                output: 'bbb.h',
                                capture: true,
                                command: [directfb_csource, '--raw', '--name=bbb', '@INPUT@'])
endif

if fusionsound_dep.found()
  examplesdata += 'test.wav'
endif

install_data(examplesdata, install_dir: examplesdatadir)
