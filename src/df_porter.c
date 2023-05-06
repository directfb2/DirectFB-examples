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

#ifdef USE_FONT_HEADERS
#include "decker.h"
#endif

#ifdef USE_IMAGE_HEADERS
#include "wood_andi.h"
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

/* main interface */
static IDirectFB *dfb = NULL;

/* event buffer interface */
static IDirectFBEventBuffer *event_buffer = NULL;

/* primary surface */
static IDirectFBSurface *primary = NULL;

/* temporary surface */
static IDirectFBSurface *surface = NULL;

/* font */
static IDirectFBFont *font = NULL;

/* screen width and height */
static int screen_width, screen_height;

static char *rules[] = {
     " CLEAR",    " SRC",      " SRC OVER", " DST OVER",
     " SRC IN",   " DST IN",   " SRC OUT",  " DST OUT",
     " SRC ATOP", " DST ATOP", " ADD",      " XOR"
};

static void dfb_shutdown()
{
     if (font)         font->Release( font );
     if (surface)      surface->Release( surface );
     if (primary)      primary->Release( primary );
     if (event_buffer) event_buffer->Release( event_buffer );
     if (dfb)          dfb->Release( dfb );
}

static void print_usage()
{
     printf( "DirectFB Porter/Duff Demo\n\n" );
     printf( "Usage: df_porter <background>\n\n" );
}

int main( int argc, char *argv[] )
{
     int                       i, step;
     DFBFontDescription        fdsc;
     DFBSurfaceDescription     sdsc;
#if defined(USE_FONT_HEADERS) || defined(USE_IMAGE_HEADERS)
     DFBDataBufferDescription  ddsc;
     IDirectFBDataBuffer      *buffer;
#endif
#ifndef USE_FONT_HEADERS
     const char               *fontfile;
#endif
#ifndef USE_IMAGE_HEADERS
     const char               *imagefile;
#endif
     IDirectFBImageProvider   *provider;
     DFBSurfacePixelFormat     fontformat = DSPF_A8;

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

     /* set the cooperative level to DFSCL_FULLSCREEN for exclusive access to the primary layer */
     dfb->SetCooperativeLevel( dfb, DFSCL_FULLSCREEN );

     /* create an event buffer for key events */
     DFBCHECK(dfb->CreateInputEventBuffer( dfb, DICAPS_BUTTONS | DICAPS_KEYS, DFB_FALSE, &event_buffer ));

     /* get the primary surface, i.e. the surface of the primary layer */
     sdsc.flags = DSDESC_CAPS;
     sdsc.caps  = DSCAPS_PRIMARY;

     DFBCHECK(dfb->CreateSurface( dfb, &sdsc, &primary ));

     DFBCHECK(primary->GetSize( primary, &screen_width, &screen_height ));

     /* create the temporary surface */
     sdsc.flags       = DSDESC_CAPS | DSDESC_PIXELFORMAT | DSDESC_WIDTH | DSDESC_HEIGHT;
     sdsc.caps        = DSCAPS_PREMULTIPLIED;
     sdsc.pixelformat = DSPF_ARGB;
     sdsc.width       = screen_width;
     sdsc.height      = screen_height;

     DFBCHECK(dfb->CreateSurface( dfb, &sdsc, &surface ));

     /* create an image provider for loading the background image. */
     if (argc > 1) {
          DFBCHECK(dfb->CreateImageProvider( dfb, argv[1], &provider ));
     }
     else {
#ifdef USE_IMAGE_HEADERS
          ddsc.flags         = DBDESC_MEMORY;
          ddsc.memory.data   = wood_andi_data;
          ddsc.memory.length = sizeof(wood_andi_data);
          DFBCHECK(dfb->CreateDataBuffer( dfb, &ddsc, &buffer ));
          DFBCHECK(buffer->CreateImageProvider( buffer, &provider ));
#else
          imagefile = DATADIR"/wood_andi.dfiff";
          DFBCHECK(dfb->CreateImageProvider( dfb, imagefile, &provider ));
#endif
     }

     /* render the image to the temporary surface. */
     provider->RenderTo( provider, surface, NULL );

     /* release the provider */
     provider->Release( provider );

     /* blit background onto the primary surface (dimmed). */
     primary->SetBlittingFlags( primary, DSBLIT_COLORIZE );
     primary->SetColor( primary, 190, 200, 180, 0 );
     primary->Blit( primary, surface, NULL, 0, 0 );

     /* clear the temporary surface */
     surface->Clear( surface, 0, 0, 0, 0 );

     /* load font */
#ifdef HAVE_GETFONTSURFACEFORMAT
     DFBCHECK(dfb->GetFontSurfaceFormat( dfb, &fontformat ));
#endif
     fdsc.flags  = DFDESC_HEIGHT;
     fdsc.height = CLAMP( (int) (screen_width / 32.0 / 8) * 8, 8, 96 );

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

     surface->SetFont( surface, font );

     surface->SetColor( surface, 0xFF, 0xFF, 0xFF, 0xFF );
     surface->DrawString( surface, "Porter/Duff Demo", -1, screen_width / 2, 20, DSTF_TOPCENTER );

     /* set drawing flags */
     surface->SetDrawingFlags( surface, DSDRAW_SRC_PREMULTIPLY | DSDRAW_BLEND );

     step = screen_width / 5;

     for (i = 0; i < D_ARRAY_SIZE(rules); i++) {
          DFBAccelerationMask  mask;
          char                *str;
          int                  x = (1 + i % 4) * step;
          int                  y = (0 + i / 4) * 180;

          str = strdup( rules[i] );

          surface->SetPorterDuff( surface, DSPD_SRC );
          surface->SetColor( surface, 255, 0, 0, 140 );
          surface->FillRectangle( surface, x - 50, y + 100, 80, 70 );

          surface->SetPorterDuff( surface, i + 1 );
          surface->SetColor( surface, 0, 0, 255, 200 );
          surface->FillRectangle( surface, x - 30, y + 130, 80, 70 );

          DFBCHECK(surface->GetAccelerationMask( surface, NULL, &mask ));
          if (mask & DFXL_FILLRECTANGLE)
               str[0] = '*';

          surface->SetPorterDuff( surface, DSPD_SRC_OVER );
          surface->SetColor( surface, 6 * 0x1F, 6 * 0x10 + 0x7f, 0xFF, 0xFF );
          surface->DrawString( surface, str, -1, x, y + 210, DSTF_TOPCENTER );

          free( str );
     }

     primary->SetBlittingFlags( primary, DSBLIT_BLEND_ALPHACHANNEL );
     primary->SetPorterDuff( primary, DSPD_SRC_OVER );
     primary->Blit( primary, surface, NULL, 0, 0 );
     primary->Flip( primary, NULL, DSFLIP_NONE );

     sleep( 1 );

     event_buffer->Reset( event_buffer );

     event_buffer->WaitForEvent( event_buffer );

     return 42;
}
