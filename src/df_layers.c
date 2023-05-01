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
#include <directfb_strings.h>
#include <directfb_util.h>

#ifdef USE_FONT_HEADERS
#include "decker.h"
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
static IDirectFBInputDevice *mouse        = NULL;
static IDirectFBEventBuffer *event_buffer = NULL;
static IDirectFBScreen      *screen       = NULL;
static IDirectFBFont        *font         = NULL;

/* screen width and height */
static int screen_width, screen_height;

/* plane color */
static DFBColor colors[] = {
     { 0xff, 0x80, 0x80, 0x80 },
     { 0xff, 0xff, 0x00, 0x00 },
     { 0xff, 0x00, 0xff, 0x00 },
     { 0xff, 0x00, 0x00, 0xff },
     { 0xff, 0xff, 0x00, 0xff },
     { 0xff, 0xff, 0xff, 0x00 },
     { 0xff, 0x00, 0xff, 0xff },
     { 0xff, 0xff, 0xff, 0xff }
};

/* command line options */
static int                   iterations   = 1000;
static int                   plane_width  = 320;
static int                   plane_height = 240;
static DFBSurfacePixelFormat pixelformat  = DSPF_ARGB;
static int                   max_planes   = D_ARRAY_SIZE(colors);

/* plane struct */
typedef struct {
     IDirectFBDisplayLayer *layer;
     IDirectFBSurface      *surface;
     int                    dx;
     int                    dy;
     int                    x;
     int                    y;
} Plane;

/* planes array */
static Plane planes[D_ARRAY_SIZE(colors)];

/* number of planes currently on screen */
static int plane_count = 0;

/**************************************************************************************************/

static const DirectFBPixelFormatNames(format_names)

static DFBSurfacePixelFormat parse_pixelformat( const char *format )
{
     int i;

     for (i = 0; i < D_ARRAY_SIZE(format_names); i++) {
          if (!strcmp( format, format_names[i].name ))
               return format_names[i].format;
     }

     return DSPF_UNKNOWN;
}

static DFBEnumerationResult
display_layer_callback( DFBDisplayLayerID layer_id, DFBDisplayLayerDescription desc, void *data )
{
     int                    width, height;
     DFBDisplayLayerConfig  config;
     char                   buf[32];
     IDirectFBDisplayLayer *layer;
     IDirectFBSurface      *surface;
     Plane                 *plane;

     if (plane_count == max_planes)
          return DFENUM_CANCEL;

     DFBCHECK(dfb->GetDisplayLayer( dfb, layer_id, &layer ));

     layer->SetCooperativeLevel( layer, DLSCL_EXCLUSIVE );

     plane = &planes[plane_count];

     /* set plane configuration */
     config.flags       = DLCONF_PIXELFORMAT;
     config.pixelformat = pixelformat;

     if (layer_id != DLID_PRIMARY) {
          config.flags  |= DLCONF_WIDTH | DLCONF_HEIGHT;
          config.width   = plane_width;
          config.height  = plane_height;

          plane->x = (layer_id * 50) % (screen_width - plane_width);
          plane->y = (layer_id * 50) % (screen_height - plane_height);
     }

     layer->SetConfiguration( layer, &config );

     /* set plane opacity */
     layer->SetOpacity( layer, 0xbb );

     /* get the layer surface */
     DFBCHECK(layer->GetSurface( layer, &surface ));

     surface->GetSize( surface, &width, &height );

     surface->SetDrawingFlags( surface, DSDRAW_SRC_PREMULTIPLY );

     surface->SetColor( surface, colors[layer_id].r, colors[layer_id].g, colors[layer_id].b, 0xbb );
     surface->FillRectangle( surface, 0, 0, width, height );

     surface->SetColor( surface, colors[layer_id].r, colors[layer_id].g, colors[layer_id].b, 0x99 );
     surface->FillRectangle( surface, width >> 2, height >> 2, width >> 1, height >> 1 );

     snprintf( buf, sizeof(buf), "Plane %u: %dx%d %s", layer_id, width, height, dfb_pixelformat_name( pixelformat ) );

     surface->SetFont( surface, font );
     surface->SetColor( surface, 0xff, 0xff, 0xff, 0xff );
     surface->DrawString( surface, buf, -1, 20, 20, DSTF_TOPLEFT );

     plane->layer   = layer;
     plane->surface = surface;
     plane->dx      = (layer_id & 1) ? 1 : -1;
     plane->dy      = (layer_id & 2) ? 1 : -1;

     plane_count++;

     return DFENUM_OK;
}

static void dfb_shutdown()
{
     int n;

     if (font) font->Release( font );

     for (n = 0; n < plane_count; n++) {
          Plane *plane = &planes[n];

          if (plane->surface) plane->surface->Release( plane->surface );
          if (plane->layer)   plane->layer->Release( plane->layer );
     }

     if (event_buffer) event_buffer->Release( event_buffer );
     if (mouse)        mouse->Release( mouse );
     if (dfb)          dfb->Release( dfb );
}

static void print_usage()
{
     printf( "DirectFB Layers Demo\n\n" );
     printf( "Usage: df_layers [options]\n\n" );
     printf( "Options:\n\n" );
     printf( "  --iterations <num>           Number of iterations.\n" );
     printf( "  --size <width>x<height>      Set plane size.\n" );
     printf( "  --pixelformat <pixelformat>  Set plane pixelformat.\n" );
     printf( "  --planes <num>               Number of planes.\n" );
     printf( "  --help                       Print usage information.\n" );
     printf( "  --dfb-help                   Output DirectFB usage information.\n\n" );
}

int main( int argc, char *argv[] )
{
     DFBResult                 ret;
     int                       n;
     DFBFontDescription        fdsc;
#ifdef USE_FONT_HEADERS
     DFBDataBufferDescription  ddsc;
     IDirectFBDataBuffer      *buffer;
#else
     const char               *fontfile;
#endif
     DFBSurfacePixelFormat     fontformat = DSPF_A8;

     /* initialize planes */
     memset( planes, 0, sizeof(planes) );

     /* initialize DirectFB including command line parsing */
     DFBCHECK(DirectFBInit( &argc, &argv ));

     /* parse command line */
     for (n = 1; n < argc; n++) {
          if (strncmp( argv[n], "--", 2 ) == 0) {
               if (strcmp( argv[n] + 2, "help" ) == 0) {
                    print_usage();
                    exit( 0 );
               }
               else if (strcmp( argv[n] + 2, "iterations" ) == 0 && n + 1 < argc &&
                   sscanf( argv[n+1], "%d", &iterations ) == 1) {
                    n++;
                    continue;
               }
               else if (strcmp( argv[n] + 2, "size" ) == 0 && n + 1 < argc &&
                        sscanf( argv[n+1], "%dx%d", &plane_width, &plane_height ) == 2) {
                    n++;
                    continue;
               }
               else if (strcmp( argv[n] + 2, "pixelformat" ) == 0 && n + 1 < argc) {
                    pixelformat = parse_pixelformat( argv[n+1] );
                    n++;
                    continue;
               }
               else if (strcmp( argv[n] + 2, "planes" ) == 0 && ++n < argc) {
                    max_planes = atoi( argv[n] );
                    if (max_planes > D_ARRAY_SIZE(colors)) {
                         print_usage();
                         return 1;
                    }
                    continue;
               }
          }

          print_usage();
          return 1;
     }

     /* create the main interface */
     DFBCHECK(DirectFBCreate( &dfb ));

     /* register termination function */
     atexit( dfb_shutdown );

     /* create an event buffer for mouse events */
     ret = dfb->GetInputDevice( dfb, DIDID_MOUSE, &mouse );
     if (ret == DFB_OK)
          DFBCHECK(mouse->CreateEventBuffer( mouse, &event_buffer ));

     /* get the primary screen */
     DFBCHECK(dfb->GetScreen( dfb, DSCID_PRIMARY, &screen ));

     /* query screen size */
     DFBCHECK(screen->GetSize( screen, &screen_width, &screen_height ));

     /* load font */
#ifdef HAVE_GETFONTSURFACEFORMAT
     DFBCHECK(dfb->GetFontSurfaceFormat( dfb, &fontformat ));
#endif
     fdsc.flags  = DFDESC_HEIGHT;
     fdsc.height = 16;

#ifdef USE_FONT_HEADERS
     ddsc.flags         = DBDESC_MEMORY;
     ddsc.memory.data   = fontformat == DSPF_A8 ? decker_data : decker_argb_data;
     ddsc.memory.length = fontformat == DSPF_A8 ? sizeof(decker_data) : sizeof(decker_argb_data);
     DFBCHECK(dfb->CreateDataBuffer( dfb, &ddsc, &buffer ));
     DFBCHECK(buffer->CreateFont( buffer, &fdsc, &font ));
#else
     fontfile = fontformat == DSPF_A8 ? DATADIR"/decker.dgiff" : DATADIR"/decker_argb.dgiff";
     DFBCHECK(dfb->CreateFont( dfb, fontfile, &fdsc, &font ));
#endif

     screen->EnumDisplayLayers( screen, display_layer_callback, NULL );

     /* main loop */
     while (iterations--) {
          if (event_buffer) {
               DFBInputEvent evt;

               /* process event buffer */
               while (event_buffer->GetEvent( event_buffer, DFB_EVENT(&evt) ) == DFB_OK) {
                    if (evt.buttons & DIBM_LEFT) {
                         if (event_buffer->WaitForEventWithTimeout( event_buffer, 2, 0 ) == DFB_TIMEOUT) {
                              /* quit main loop */
                              return 42;
                         }
                    }
               }
          }

          for (n = 0; n < plane_count; n++) {
               Plane *plane = &planes[n];

               if (plane->x == screen_width - plane_width)
                    plane->dx = -1;
               else if (plane->x == 0)
                    plane->dx = 1;

               if (plane->y == screen_height - plane_height)
                    plane->dy = -1;
               else if (plane->y == 0)
                    plane->dy = 1;

               plane->x += plane->dx;
               plane->y += plane->dy;

               plane->layer->SetScreenPosition( plane->layer, plane->x, plane->y );

               plane->surface->Flip( plane->surface, NULL, DSFLIP_NONE );

               usleep( 10000 );
          }
     }

     return 42;
}
