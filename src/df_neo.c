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

#include "util.h"

#ifdef USE_IMAGE_HEADERS
#include "apple-red.h"
#include "background.h"
#include "gnome-applets.h"
#include "gnome-calendar.h"
#include "gnome-foot.h"
#include "gnome-gimp.h"
#include "gnome-gmush.h"
#include "gnome-gsame.h"
#include "gnu-keys.h"
#endif

/* macro for a safe call to DirectFB functions */
#define DFBCHECK(x)                                                   \
     do {                                                             \
          DFBResult ret = x;                                          \
          if (ret != DFB_OK) {                                        \
               fprintf( stderr, "%s <%d>:\n\t", __FILE__, __LINE__ ); \
               DirectFBErrorFatal( #x, ret );                         \
          }                                                           \
     } while (0)

/* DirectFB interfaces */
static IDirectFB            *dfb          = NULL;
static IDirectFBEventBuffer *event_buffer = NULL;
static IDirectFBSurface     *primary      = NULL;

/* background image */
static IDirectFBSurface *background = NULL;
static int               back_width;
static int               back_height;

/* test images */
typedef enum {
     APPLE_RED,
     GNOME_APPLETS,
     GNOME_CALENDAR,
     GNOME_FOOT,
     GNOME_GMUSH,
     GNOME_GIMP,
     GNOME_GSAME,
     GNU_KEYS,
     NUM_IMAGES
} Image;

#ifdef USE_IMAGE_HEADERS
static const void *image_data[] = {
     GET_IMAGEDATA( apple_red ),
     GET_IMAGEDATA( gnome_applets ),
     GET_IMAGEDATA( gnome_calendar ),
     GET_IMAGEDATA( gnome_foot ),
     GET_IMAGEDATA( gnome_gmush ),
     GET_IMAGEDATA( gnome_gimp ),
     GET_IMAGEDATA( gnome_gsame ),
     GET_IMAGEDATA( gnu_keys )
};
static unsigned int image_size[] = {
     GET_IMAGESIZE( apple_red ),
     GET_IMAGESIZE( gnome_applets ),
     GET_IMAGESIZE( gnome_calendar ),
     GET_IMAGESIZE( gnome_foot ),
     GET_IMAGESIZE( gnome_gmush ),
     GET_IMAGESIZE( gnome_gimp ),
     GET_IMAGESIZE( gnome_gsame ),
     GET_IMAGESIZE( gnu_keys )
};
#else
static const char *apple_red()      { return GET_IMAGEFILE( apple-red );      }
static const char *gnome_applets()  { return GET_IMAGEFILE( gnome-applets );  }
static const char *gnome_calendar() { return GET_IMAGEFILE( gnome-calendar ); }
static const char *gnome_foot()     { return GET_IMAGEFILE( gnome-foot );     }
static const char *gnome_gmush()    { return GET_IMAGEFILE( gnome-gmush );    }
static const char *gnome_gimp()     { return GET_IMAGEFILE( gnome-gimp );     }
static const char *gnome_gsame()    { return GET_IMAGEFILE( gnome-gsame );    }
static const char *gnu_keys()       { return GET_IMAGEFILE( gnu-keys );       }

static const char *(*image_file[])() = {
     apple_red, gnome_applets, gnome_calendar, gnome_foot, gnome_gmush, gnome_gimp, gnome_gsame, gnu_keys
};
#endif

static IDirectFBSurface *images[NUM_IMAGES] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };
static int               image_widths[NUM_IMAGES];
static int               image_heights[NUM_IMAGES];

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
     int                     n;
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

     for (n = 0; n < NUM_IMAGES; n++) {
          float        ang;
          int          iw, ih;
          float        r;
          float        k;
          int          xpos, ypos;
          DFBRectangle dest;

          ang = 2.0f * M_PI * n / NUM_IMAGES - f * 2.0f * M_PI;

          iw = image_widths[n];
          ih = image_heights[n];

          r = radius + (radius / 3.0f) * sinf( f * 2.0f * M_PI );

          xpos = xmid + r * cosf( ang ) - iw / 2.0f;
          ypos = ymid + r * sinf( ang ) - ih / 2.0f;

          k = (n & 1) ? sinf( f * 2.0f * M_PI ) : cosf( f * 2.0f * M_PI );
          k = 2.0f * k * k;
          k = MAX( 0.25f, k );

          dest.x = xpos;
          dest.y = ypos;
          dest.w = iw * k;
          dest.h = ih * k;

          sub->SetColor( sub, xpos, ypos, 255 - xpos,
                         (n & 1) ? fabs( 255 * sinf( f * 2.0f * M_PI ) ) : fabs( 255 * cosf( f * 2.0f * M_PI ) ) );

          sub->StretchBlit( sub, images[n], NULL, &dest );
     }

     primary->Flip( primary, NULL, DSFLIP_ONSYNC );

     frame_num++;
}

static void cleanup()
{
     int n;

     /* release the primary subsurface */
     if (sub)
          sub->Release( sub );

     /* release test images */
     for (n = 0; n < NUM_IMAGES; n++)
          if (images[n])
               images[n]->Release( images[n] );

     /* release the background image */
     if (background)
          background->Release( background );

     /* release the primary surface */
     if (primary)
          primary->Release( primary );

     /* release the event buffer */
     if (event_buffer)
          event_buffer->Release( event_buffer );

     /* release the main interface */
     if (dfb)
          dfb->Release( dfb );
}

int main( int argc, char *argv[] )
{
     int                       n;
     DFBRectangle              rect;
     DFBSurfaceDescription     sdsc;
     DFBDataBufferDescription  ddsc;
     IDirectFBDataBuffer      *buffer;
     IDirectFBImageProvider   *provider;
     unsigned int              cycle_len   = CYCLE_LEN;
     unsigned int              frame_delay = FRAME_DELAY;

     /* initialize DirectFB including command line parsing */
     DFBCHECK(DirectFBInit( &argc, &argv ));

     /* create the main interface */
     DFBCHECK(DirectFBCreate( &dfb ));

     /* register termination function */
     atexit( cleanup );

     /* set the cooperative level to DFSCL_FULLSCREEN for exclusive access to the primary layer */
     dfb->SetCooperativeLevel( dfb, DFSCL_FULLSCREEN );

     /* create an event buffer for key events */
     DFBCHECK(dfb->CreateInputEventBuffer( dfb, DICAPS_BUTTONS | DICAPS_KEYS, DFB_FALSE, &event_buffer ));

     /* get the primary surface, i.e. the surface of the primary layer */
     sdsc.flags = DSDESC_CAPS;
     sdsc.caps  = DSCAPS_PRIMARY | DSCAPS_FLIPPING;

     DFBCHECK(dfb->CreateSurface( dfb, &sdsc, &primary ));

     DFBCHECK(primary->GetSize( primary, &width, &height ));

     /* load the background image */
#ifdef USE_IMAGE_HEADERS
     ddsc.flags         = DBDESC_MEMORY;
     ddsc.memory.data   = GET_IMAGEDATA( background );
     ddsc.memory.length = GET_IMAGESIZE( background );
#else
     ddsc.flags         = DBDESC_FILE;
     ddsc.file          = GET_IMAGEFILE( background );
#endif
     DFBCHECK(dfb->CreateDataBuffer( dfb, &ddsc, &buffer ));
     DFBCHECK(buffer->CreateImageProvider( buffer, &provider ));
     provider->GetSurfaceDescription( provider, &sdsc );
     back_width  = sdsc.width;
     back_height = sdsc.height;
     DFBCHECK(dfb->CreateSurface( dfb, &sdsc, &background ));
     provider->RenderTo( provider, background, NULL );
     provider->Release( provider );

     /* fill screen and backbuffers with tiled background */
     primary->TileBlit( primary, background, NULL, rect.x, rect.y );
     primary->Flip( primary, NULL, DSFLIP_NONE );
     primary->TileBlit( primary, background, NULL, rect.x, rect.y );
     primary->Flip( primary, NULL, DSFLIP_NONE );
     primary->TileBlit( primary, background, NULL, rect.x, rect.y );

     /* load test images */
     for (n = 0; n < NUM_IMAGES; n++) {
#ifdef USE_IMAGE_HEADERS
          ddsc.flags         = DBDESC_MEMORY;
          ddsc.memory.data   = image_data[n];
          ddsc.memory.length = image_size[n];
#else
          ddsc.flags         = DBDESC_FILE;
          ddsc.file          = image_file[n]();
#endif
          DFBCHECK(dfb->CreateDataBuffer( dfb, &ddsc, &buffer ));
          DFBCHECK(buffer->CreateImageProvider( buffer, &provider ));
          provider->GetSurfaceDescription( provider, &sdsc );
          image_widths[n]  = sdsc.width;
          image_heights[n] = sdsc.height;
          DFBCHECK(dfb->CreateSurface( dfb, &sdsc, &images[n] ));
          provider->RenderTo( provider, images[n], NULL );
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
          while (event_buffer->GetEvent( event_buffer, DFB_EVENT(&evt) ) == DFB_OK) {
               if (evt.buttons & DIBM_LEFT) {
                    if (event_buffer->WaitForEventWithTimeout( event_buffer, 2, 0 ) == DFB_TIMEOUT) {
                         /* quit main loop */
                         return 42;
                    }
               }
               else if (evt.type == DIET_KEYPRESS) {
                    switch (evt.key_id) {
                         case DIKI_ESCAPE:
                         case DIKI_Q:
                              /* quit main loop */
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
