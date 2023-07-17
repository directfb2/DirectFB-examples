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

project('DirectFB-examples', 'c',
        version: '2.0.0')

examplesdatadir = join_paths(get_option('prefix'), get_option('datadir'), 'directfb-examples')

add_global_arguments('-DDATADIR="@0@"'.format(examplesdatadir), language: 'c')

directfb_dep = dependency('directfb')

libm_dep = meson.get_compiler('c').find_library('m')

message('Checking for GetFontSurfaceFormat() support')
code = '''
          #include <directfb.h>
          int main()
          {
            IDirectFB *dfb;
            dfb->GetFontSurfaceFormat( dfb, NULL );
            return 0;
          }
       '''
if meson.get_compiler('c').compiles(code, dependencies: directfb_dep)
  message('GetFontSurfaceFormat() support found')
  add_global_arguments('-DHAVE_GETFONTSURFACEFORMAT', language: 'c')
else
  message('GetFontSurfaceFormat() support not found')
endif

message('Checking for multi-touch support')
code = '''
          #include <directfb.h>
          int main()
          {
            DFBInputEvent evt = { .slot_id = 0 };
            return 0;
          }
       '''
if meson.get_compiler('c').compiles(code, dependencies: directfb_dep)
  message('multi-touch support found')
  add_global_arguments('-DHAVE_MT', language: 'c')
else
  message('multi-touch support not found')
endif

font_headers  = get_option('font-headers')
image_headers = get_option('image-headers')
video_headers = get_option('video-headers')

if font_headers != 'disabled' or image_headers != 'disabled' or video_headers != 'disabled'
  directfb_csource = find_program('directfb-csource')
  if font_headers != 'disabled'
    add_global_arguments('-DUSE_FONT_HEADERS', language: 'c')
    if font_headers == 'dgiff'
      cat = find_program('cat')
      add_global_arguments('-DDGIFF_EXTENSION', language: 'c')
    endif
  endif
  if image_headers != 'disabled'
    add_global_arguments('-DUSE_IMAGE_HEADERS', language: 'c')
  endif
  if video_headers != 'disabled'
    add_global_arguments('-DUSE_VIDEO_HEADERS', language: 'c')
  endif
endif

gl_dep = []
if get_option('gl') != ''
  gl_dep = declare_dependency(compile_args: ['-DOPENGL_H="@0@"'.format(get_option('gl')),
                                            get_option('gl-cflags').strip().split(' ')],
                              link_args: get_option('gl-ldflags') != '' ?
                                         get_option('gl-ldflags').strip().split(' ') : [])
endif

fusionsound_dep = dependency('fusionsound', required: false)

subdir('data')
subdir('src')
