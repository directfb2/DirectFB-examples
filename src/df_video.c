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
#define DFBCHECK(x)                                                   \
     do {                                                             \
          DFBResult ret = x;                                          \
          if (ret != DFB_OK) {                                        \
               fprintf( stderr, "%s <%d>:\n\t", __FILE__, __LINE__ ); \
               DirectFBErrorFatal( #x, ret );                         \
          }                                                           \
     } while (0)

/* DirectFB interfaces */
static IDirectFB              *dfb           = NULL;
static IDirectFBEventBuffer   *event_buffer  = NULL;
static IDirectFBDisplayLayer  *layer         = NULL;
static IDirectFBVideoProvider *videoprovider = NULL;
static IDirectFBWindow        *videowindow   = NULL;
static IDirectFBSurface       *videosurface  = NULL;
static IDirectFBWindow        *window        = NULL;
static IDirectFBSurface       *surface       = NULL;

/* screen width and height */
static int screen_width, screen_height;

/* video width and height */
static int video_width, video_height;

/* values on which placement of panel depends */
static int movx, movy;

/* default animation speed settings */
unsigned int frame_delay = 50;

/* number of frames displayed */
static long frame_num = 0;

static void render()
{
     if (frame_num % frame_delay) {
          static int wx   = 0;
          static int wy   = 0;
          static int dirx = 4;
          static int diry = 2;

          wx += dirx;
          wy += diry;

          if (wx >= screen_width - video_width || wx <= 0)
               dirx *= -1;
          if (wy >= screen_height - video_height || wy <= 0)
               diry *= -1;

          videowindow->Move( videowindow, dirx, diry );
     }
     else {
          static float  w = 0;
          unsigned char r = sin( w )        * 128 + 127;
          unsigned char g = sin( w * 0.3f ) * 128 + 127;
          unsigned char b = sin( w * 0.5f ) * 128 + 127;

          layer->SetBackgroundColor( layer, r, g, b, 0 );

          w += 0.1f * frame_delay;
     }

     if (movx || movy)
          window->Move( window, movx, movy );

     frame_num++;
}

static void dfb_shutdown()
{
     if (surface)       surface->Release( surface );
     if (window)        window->Release( window );
     if (videosurface)  videosurface->Release( videosurface );
     if (videowindow)   videowindow->Release( videowindow );
     if (videoprovider) videoprovider->Release( videoprovider );
     if (layer)         layer->Release( layer );
     if (event_buffer)  event_buffer->Release( event_buffer );
     if (dfb)           dfb->Release( dfb );
}

static void print_usage()
{
     printf( "DirectFB Video Demo\n\n" );
     printf( "Usage: df_video <videofile>\n\n" );
}

int main( int argc, char *argv[] )
{
     DFBDisplayLayerConfig   config;
     DFBSurfaceDescription   sdsc;
     DFBWindowDescription    wdsc;
     IDirectFBImageProvider *provider;

     /* initialize DirectFB including command line parsing */
     DFBCHECK(DirectFBInit( &argc, &argv ));

     /* parse command line */
     if (argv[1] && !strcmp( argv[1], "--help" )) {
          print_usage();
          return 0;
     }

     /* create the main interface */
     DFBCHECK(DirectFBCreate( &dfb ));

     /* register termination function */
     atexit( dfb_shutdown );

     /* create an event buffer for all devices */
     DFBCHECK(dfb->CreateInputEventBuffer( dfb, DICAPS_ALL, DFB_TRUE, &event_buffer ));

     /* get the primary display layer */
     DFBCHECK(dfb->GetDisplayLayer( dfb, DLID_PRIMARY, &layer ));

     layer->SetCooperativeLevel( layer, DLSCL_ADMINISTRATIVE );

     DFBCHECK(layer->GetConfiguration( layer, &config ));
     screen_width  = config.width;
     screen_height = config.height;

     /* video window */
     DFBCHECK(dfb->CreateVideoProvider( dfb, argc > 1 ? argv[1] : DATADIR"/bbb.dfvff", &videoprovider ));
     videoprovider->GetSurfaceDescription( videoprovider, &sdsc );
     wdsc.flags  = DWDESC_POSX | DWDESC_POSY | DWDESC_WIDTH | DWDESC_HEIGHT;
     wdsc.posx   = 0;
     wdsc.posy   = 0;
     wdsc.width  = video_width = sdsc.width;
     wdsc.height = video_height = sdsc.height;
     DFBCHECK(layer->CreateWindow( layer, &wdsc, &videowindow ));
     DFBCHECK(videowindow->GetSurface( videowindow, &videosurface ));
     videowindow->SetOpacity( videowindow, 0xFF );
     videoprovider->SetPlaybackFlags( videoprovider, DVPLAY_LOOPING );
     videoprovider->PlayTo( videoprovider, videosurface, NULL, NULL, NULL );

     /* panel window */
     DFBCHECK(dfb->CreateImageProvider( dfb, DATADIR"/panel.dfiff", &provider ));
     provider->GetSurfaceDescription( provider, &sdsc );
     wdsc.flags  = DWDESC_CAPS | DWDESC_POSX | DWDESC_POSY | DWDESC_WIDTH | DWDESC_HEIGHT;
     wdsc.caps   = DWCAPS_ALPHACHANNEL;
     wdsc.posx   = 0;
     wdsc.posy   = 20;
     wdsc.width  = sdsc.width;
     wdsc.height = sdsc.height;
     DFBCHECK(layer->CreateWindow( layer, &wdsc, &window ));
     DFBCHECK(window->GetSurface( window, &surface ));
     provider->RenderTo( provider, surface, NULL );
     window->SetOpacity( window, 0xFF );
     provider->Release( provider );

     /* main loop */
     while (1) {
          DirectClock   clock;
          DFBInputEvent evt;

          direct_clock_start( &clock );

          render();

          movx = 0;
          movy = 0;

          /* process event buffer */
          while (event_buffer->GetEvent( event_buffer, DFB_EVENT(&evt) ) == DFB_OK) {
               if (evt.type == DIET_AXISMOTION && (evt.flags & DIEF_AXISREL)) {
                    switch (evt.axis) {
                         case DIAI_X:
                              movx += evt.axisrel;
                              break;
                         case DIAI_Y:
                              movy += evt.axisrel;
                              break;
                         case DIAI_Z:
                              frame_delay = CLAMP( frame_delay + evt.axisrel, 2, 100 );
                              break;
                         default:
                              break;
                    }
               }
               else if (evt.buttons & DIBM_LEFT) {
                    if (event_buffer->WaitForEventWithTimeout( event_buffer, 2, 0 ) == DFB_TIMEOUT) {
                         /* quit main loop */
                         return 42;
                    }
               }
               else if (evt.type == DIET_KEYPRESS) {
                    switch (DFB_LOWER_CASE( evt.key_symbol )) {
                         case DIKS_ESCAPE:
                         case DIKS_SMALL_Q:
                         case DIKS_BACK:
                         case DIKS_STOP:
                         case DIKS_EXIT:
                              /* quit main loop */
                              return 42;
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
