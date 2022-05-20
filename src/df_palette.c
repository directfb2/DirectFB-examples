/*
   This file is part of DirectFB-examples.

   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software and associated documentation files (the "Software"), to deal
   in the Software without restriction, including without limitation the rights
   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
   copies of the Software, and to permit persons to whom the Software is
   furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included in
   all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
   THE SOFTWARE.
*/

#include <directfb.h>

/* macro for a safe call to DirectFB functions */
#define DFBCHECK(x...)                                                \
     do {                                                             \
          DFBResult err = x;                                          \
          if (err != DFB_OK) {                                        \
               fprintf( stderr, "%s <%d>:\n\t", __FILE__, __LINE__ ); \
               DirectFBErrorFatal( #x, err );                         \
          }                                                           \
     } while (0)

/* DirectFB interfaces */
static IDirectFB            *dfb          = NULL;
static IDirectFBEventBuffer *event_buffer = NULL;
static IDirectFBSurface     *primary      = NULL;
static IDirectFBPalette     *palette      = NULL;

/* screen width and height */
static int screen_width, screen_height;

static void generate_palette()
{
     DFBColor  colors[256];
     int       i;

     for (i = 0; i < 256; i++) {
          colors[i].a = 0xff;
          colors[i].r = i + 85;
          colors[i].g = i;
          colors[i].b = i + 171;
     }

     DFBCHECK(palette->SetEntries( palette, colors, 256, 0 ));
}

static void rotate_palette()
{
     DFBColor colors[256];

     DFBCHECK(palette->GetEntries( palette, colors, 256, 0 ));

     DFBCHECK(palette->SetEntries( palette, colors + 1, 255, 0 ));

     colors[0].r += 17;
     colors[0].g += 31;
     colors[0].b += 29;

     DFBCHECK(palette->SetEntries( palette, colors, 1, 255 ));
}

static void fill_surface()
{
     int   x;
     int   y;
     void *ptr;
     int   pitch;
     u8   *dst;

     DFBCHECK(primary->Lock( primary, DSLF_WRITE, &ptr, &pitch ));

     for (y = 0; y < screen_height; y++) {
          dst = ptr + y * pitch;

          for (x = 0; x < screen_width; x++)
               dst[x] = (x * x + y) / (y + 1);
     }

     DFBCHECK(primary->Unlock( primary ));
}

static void dfb_shutdown()
{
     if (palette)      palette->Release( palette );
     if (primary)      primary->Release( primary );
     if (event_buffer) event_buffer->Release( event_buffer );
     if (dfb)          dfb->Release( dfb );
}

int main( int argc, char *argv[] )
{
     DFBSurfaceDescription desc;

     /* initialize DirectFB including command line parsing */
     DFBCHECK(DirectFBInit( &argc, &argv ));

     /* create the main interface */
     DFBCHECK(DirectFBCreate( &dfb ));

     /* create an event buffer for button events */
     DFBCHECK(dfb->CreateInputEventBuffer( dfb, DICAPS_BUTTONS | DICAPS_KEYS, DFB_FALSE, &event_buffer ));

     /* get the primary surface, i.e. the surface of the primary layer */
     desc.flags       = DSDESC_CAPS | DSDESC_PIXELFORMAT;
     desc.caps        = DSCAPS_PRIMARY;
     desc.pixelformat = DSPF_LUT8;

     DFBCHECK(dfb->CreateSurface( dfb, &desc, &primary ));

     DFBCHECK(primary->GetSize( primary, &screen_width, &screen_height ));

     /* get access to the palette */
     DFBCHECK(primary->GetPalette( primary, &palette ));

     /* generate the palette */
     generate_palette();

     /* fill with values */
     fill_surface();

     /* main loop */
     while (1) {
          DFBInputEvent evt;

          /* process event buffer */
          while (event_buffer->GetEvent( event_buffer, DFB_EVENT(&evt) ) == DFB_OK) {
               if (evt.buttons & DIBM_LEFT) {
                    if (event_buffer->WaitForEventWithTimeout( event_buffer, 2, 0 ) == DFB_TIMEOUT) {
                         /* quit main loop */
                         dfb_shutdown();
                         return 42;
                    }
               }
               else if (evt.type == DIET_KEYPRESS) {
                    switch (evt.key_id) {
                         case DIKI_ESCAPE:
                         case DIKI_Q:
                              /* quit main loop */
                              dfb_shutdown();
                              return 42;

                         default:
                              break;
                    }
               }
          }

          rotate_palette();
     }

     /* shouldn't reach this */
     return 0;
}
