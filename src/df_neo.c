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

#include <direct/util.h>
#include <directfb.h>
#include <math.h>

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
static IDirectFB            *dfb       = NULL;
static IDirectFBEventBuffer *keybuffer = NULL;
static IDirectFBSurface     *primary   = NULL;

/* background image */
static IDirectFBSurface *background = NULL;
static int               back_width;
static int               back_height;

/* test images */
static const char *image_names[] = {
     DATADIR"/apple-red.dfiff",
     DATADIR"/gnome-applets.dfiff",
     DATADIR"/gnome-calendar.dfiff",
     DATADIR"/gnome-foot.dfiff",
     DATADIR"/gnome-gmush.dfiff",
     DATADIR"/gnome-gimp.dfiff",
     DATADIR"/gnome-gsame.dfiff",
     DATADIR"/gnu-keys.dfiff"
};

static IDirectFBSurface *images[D_ARRAY_SIZE(image_names)];
static int               image_widths[D_ARRAY_SIZE(image_names)];
static int               image_heights[D_ARRAY_SIZE(image_names)];

/* primary subsurface */
static IDirectFBSurface *sub = NULL;

/* screen width and height */
static int width, height;

/* default animation speed settings */
#define CYCLE_LEN   60
#define FRAME_DELAY 50

/* color modulation */
static bool colorize = true;

/* number of frames displayed */
static long frame_num = 0;

static void render( unsigned int cycle_len )
{
     int                     i;
     float                   f;
     float                   xmid, ymid;
     float                   radius;
     DFBSurfaceBlittingFlags blit_flags;

     sub->SetBlittingFlags( sub, DSBLIT_NOFX );

     sub->Blit( sub, background, NULL, 0, 0 );

     blit_flags = DSBLIT_BLEND_ALPHACHANNEL;
     if (colorize)
       blit_flags |= DSBLIT_COLORIZE;

     sub->SetBlittingFlags( sub, blit_flags );

     f = (float) (frame_num % cycle_len) / cycle_len;

     xmid = back_width / 2.0f;
     ymid = back_height / 2.0f;

     radius = MIN( xmid, ymid ) / 2.0f;

     for (i = 0; i < D_ARRAY_SIZE(image_names); i++) {
          float        ang;
          int          iw, ih;
          float        r;
          float        k;
          int          xpos, ypos;
          DFBRectangle dest;

          ang = 2.0f * M_PI * i / D_ARRAY_SIZE(image_names) - f * 2.0f * M_PI;

          iw = image_widths[i];
          ih = image_heights[i];

          r = radius + (radius / 3.0f) * sinf( f * 2.0f * M_PI );

          xpos = xmid + r * cosf( ang ) - iw / 2.0f;
          ypos = ymid + r * sinf( ang ) - ih / 2.0f;

          k = (i & 1) ? sinf( f * 2.0f * M_PI ) : cosf( f * 2.0f * M_PI );
          k = 2.0f * k * k;
          k = MAX( 0.25f, k );

          dest.x = xpos;
          dest.y = ypos;
          dest.w = iw * k;
          dest.h = ih * k;

          sub->SetColor( sub, xpos, ypos, 255 - xpos,
                         (i & 1) ? fabs( 255 * sinf( f * 2.0f * M_PI ) ) : fabs( 255 * cosf( f * 2.0f * M_PI ) ) );

          sub->StretchBlit( sub, images[i], NULL, &dest );
     }

     primary->Flip( primary, NULL, DSFLIP_ONSYNC );

     frame_num++;
}

static void cleanup()
{
     int i;

     /* release the primary subsurface */
     if (sub)
          sub->Release( sub );

     /* release the test images */
     for (i = 0; i < D_ARRAY_SIZE(image_names); i++)
          if (images[i])
               images[i]->Release( images[i] );

     /* release the background image */
     if (background)
          background->Release( background );

     /* release the primary surface */
     if (primary)
          primary->Release( primary );

     /* release the key event buffer */
     if (keybuffer)
          keybuffer->Release( keybuffer );

     /* release the main interface */
     if (dfb)
          dfb->Release( dfb );
}

int main( int argc, char *argv[] )
{
     DFBSurfaceDescription   dsc;
     DFBRectangle            rect;
     IDirectFBImageProvider *provider;
     int                     i;
     unsigned int            cycle_len   = CYCLE_LEN;
     unsigned int            frame_delay = FRAME_DELAY;

     /* initialize the array of pointers for the test images */
     for (i = 0; i < D_ARRAY_SIZE(image_names); i++)
          images[i] = NULL;

     /* initialize DirectFB including command line parsing */
     DFBCHECK(DirectFBInit( &argc, &argv ));

     /* create the main interface */
     DFBCHECK(DirectFBCreate( &dfb ));

     /* set the cooperative level to DFSCL_FULLSCREEN for exclusive access to the primary layer */
     dfb->SetCooperativeLevel( dfb, DFSCL_FULLSCREEN );

     /* create an event buffer for key events */
     DFBCHECK(dfb->CreateInputEventBuffer( dfb, DICAPS_KEYS, DFB_FALSE, &keybuffer ));

     /* get the primary surface, i.e. the surface of the primary layer */
     dsc.flags = DSDESC_CAPS;
     dsc.caps  = DSCAPS_PRIMARY | DSCAPS_FLIPPING;

     DFBCHECK(dfb->CreateSurface( dfb, &dsc, &primary ));

     DFBCHECK(primary->GetSize( primary, &width, &height ));

     /* load the background image */
     DFBCHECK(dfb->CreateImageProvider( dfb, DATADIR"/background.dfiff", &provider ));
     provider->GetSurfaceDescription( provider, &dsc );
     back_width  = dsc.width;
     back_height = dsc.height;
     DFBCHECK(dfb->CreateSurface( dfb, &dsc, &background ));
     provider->RenderTo( provider, background, NULL );
     provider->Release( provider );

     /* fill screen and backbuffers with tiled background */
     primary->TileBlit( primary, background, NULL, rect.x, rect.y );
     primary->Flip( primary, NULL, DSFLIP_NONE );
     primary->TileBlit( primary, background, NULL, rect.x, rect.y );
     primary->Flip( primary, NULL, DSFLIP_NONE );
     primary->TileBlit( primary, background, NULL, rect.x, rect.y );

     /* load the test images */
     for (i = 0; i < D_ARRAY_SIZE(image_names); i++) {
          DFBCHECK(dfb->CreateImageProvider( dfb, image_names[i], &provider ));
          provider->GetSurfaceDescription( provider, &dsc );
          image_widths[i]  = dsc.width;
          image_heights[i] = dsc.height;
          DFBCHECK(dfb->CreateSurface( dfb, &dsc, &images[i] ));
          provider->RenderTo( provider, images[i], NULL );
          provider->Release( provider );
     }

     /* create a subsurface in the middle of the screen */
     rect.x = (width  - back_width) / 2;
     rect.y = (height - back_height) / 2;
     rect.w = back_width;
     rect.h = back_height;

     DFBCHECK(primary->GetSubSurface( primary, &rect, &sub ));

     /* main loop */
     while (1) {
          DirectClock   clock;
          DFBInputEvent evt;

          direct_clock_start( &clock );

          render( cycle_len );

          /* process event buffer */
          while (keybuffer->GetEvent( keybuffer, DFB_EVENT(&evt) ) == DFB_OK) {
               if (evt.type == DIET_KEYPRESS) {
                    switch (evt.key_id) {
                         case DIKI_ESCAPE:
                         case DIKI_Q:
                              /* quit main loop */
                              cleanup();
                              return 42;

                         case DIKI_UP:
                              cycle_len = MIN( 600, cycle_len + 6 );
                              break;

                         case DIKI_DOWN:
                              cycle_len = cycle_len > 6 ? cycle_len - 6 : 6;
                              break;

                         case DIKI_LEFT:
                              frame_delay = MIN( 500, frame_delay + 5 );
                              break;

                         case DIKI_RIGHT:
                              frame_delay = frame_delay > 5 ? frame_delay - 5 : 0;
                              break;

                         case DIKI_ENTER:
                         case DIKI_SPACE:
                              colorize = !colorize;
                              break;

                         case DIKI_HOME:
                              cycle_len   = CYCLE_LEN;
                              frame_delay = FRAME_DELAY;
                              colorize    = true;
                              break;

                         default:
                           break;
                    }
               }
          }

          if (frame_delay) {
               long delay;

               direct_clock_stop( &clock );

               delay = 1000 * frame_delay - direct_clock_diff( &clock );
               if (delay > 0)
                    usleep( delay );
          }
     }

     /* shouldn't reach this */
     return 0;
}
