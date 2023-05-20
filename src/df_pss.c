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

#include <direct/thread.h>
#include <directfb.h>
#include <math.h>

#include "util.h"

#ifdef USE_FONT_HEADERS
#include "decker.h"
#endif

#ifdef USE_IMAGE_HEADERS
#include "smokey_light.h"
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
static IDirectFBInputDevice *keyboard     = NULL;
static IDirectFBEventBuffer *event_buffer = NULL;
static IDirectFBSurface     *primary      = NULL;
static IDirectFBFont        *font         = NULL;
static IDirectFBSurface     *smokey_light = NULL;

/* screen width and height */
static int xres, yres;

/* quit main loop */
static DFBInputDeviceKeyState quit = DIKS_UP;

static int intro( DirectThread *thread )
{
     int   jitter1 = yres / 100 + 1;
     int   jitter2 = (jitter1 - 1) / 2;
     char *lines[] = { "3", "2", "1", NULL };
     int   l       = 0;

     primary->SetDrawingFlags( primary, DSDRAW_NOFX );
     primary->SetBlittingFlags( primary, DSBLIT_NOFX );

     while (lines[l]) {
          int frames = 200;

          while (frames--) {
               primary->SetColor( primary, 0, 0, 0, 0 );
               primary->FillRectangle( primary, 0, 0, xres, yres );

               primary->SetColor( primary, 0x40 + rand() % 0xc0, 0x80 + rand() % 0x80, 0x80 + rand() % 0x80, 0xff );
               primary->DrawString( primary, lines[l], -1,
                                    xres / 2 + rand() % jitter1 - jitter2, yres / 2 + rand() % jitter1 - jitter2,
                                    DSTF_CENTER );

               primary->Flip( primary, NULL, DSFLIP_WAITFORSYNC );

               direct_thread_testcancel( thread );
          }

          ++l;
     }

     return 0;
}

static int demo1( DirectThread *thread )
{
     int    frames = 400;
     double b      = 0;

     primary->SetDrawingFlags( primary, DSDRAW_NOFX );
     primary->SetBlittingFlags( primary, DSBLIT_NOFX );

     primary->SetColor( primary, 0, 0, 0, 0 );

     while (frames--) {
          DFBRectangle rect;
          double       f;

          primary->FillRectangle( primary, 0, 0, xres, yres );

          f = cos( b ) * 30 + sin( b + 0.5 ) * 40;

          rect.w = (sin( f * cos( f / 10.0 ) ) / 2 + 1.2) * 800;
          rect.h = (sin( f * sin( f / 10.0 ) )     + 1.2) * 300;

          rect.x = (xres - rect.w) / 2;
          rect.y = (yres - rect.h) / 2;

          primary->StretchBlit( primary, smokey_light, NULL, &rect );

          b += 0.001;

          primary->Flip( primary, NULL, DSFLIP_WAITFORSYNC );

          direct_thread_testcancel( thread );
     }

     return 0;
}

static int demo2( DirectThread *thread )
{
     int    frames = 400;
     double b      = 0;

     primary->SetDrawingFlags( primary, DSDRAW_BLEND );
     primary->SetBlittingFlags( primary, DSBLIT_NOFX );

     while (frames--) {
          double w;

          primary->SetColor( primary, 0, 0, 0, 0x10 );
          primary->FillRectangle( primary, 0, 0, xres, yres );

          for (w = b; w <= b + 6.29; w += 0.05) {
               primary->SetColor( primary,
                                  sin( 1 * w + b ) * 127 + 127,
                                  sin( 2 * w - b ) * 127 + 127,
                                  sin( 3 * w + b ) * 127 + 127,
                                  sin( 4 * w - b ) * 127 + 127 );
               primary->DrawLine( primary,
                                  xres / 2, yres / 2,
                                  xres / 2 + cos( w ) * xres / 2, yres / 2 + sin( w ) * yres / 2 );
          }

          b += 0.02;

          primary->Flip( primary, NULL, DSFLIP_WAITFORSYNC );

          direct_thread_testcancel( thread );
     }

     primary->SetColor( primary, 0, 0, 0, 0x10 );

     for (frames = 0; frames < 75; frames++) {
          primary->FillRectangle( primary, 0, 0, xres, yres );

          primary->Flip( primary, NULL, DSFLIP_WAITFORSYNC );

          direct_thread_testcancel( thread );
     }

     return 0;
}

static int (*demos[])( DirectThread *thread ) = { intro, demo1, demo2, NULL };
static int current_demo = 0;

static void *demos_loop( DirectThread *thread, void *arg )
{
     while (demos[current_demo]) {
          demos[current_demo]( thread );
          ++current_demo;
     }

     quit = DIKS_DOWN;

     return NULL;
}

static void dfb_shutdown( void )
{
     if (smokey_light) smokey_light->Release( smokey_light );
     if (font)         font->Release( font );
     if (primary)      primary->Release( primary );
     if (event_buffer) event_buffer->Release( event_buffer );
     if (keyboard)     keyboard->Release( keyboard );
     if (dfb)          dfb->Release( dfb );
}

int main( int argc, char *argv[] )
{
     DFBResult                 ret;
     DFBFontDescription        fdsc;
     DFBSurfaceDescription     sdsc;
     DFBDataBufferDescription  ddsc;
     IDirectFBDataBuffer      *buffer;
     IDirectFBImageProvider   *provider;
     DirectThread             *demos_loop_thread;

     srand( time( NULL ) );

     /* initialize DirectFB including command line parsing */
     DFBCHECK(DirectFBInit( &argc, &argv ));

     /* create the main interface */
     DFBCHECK(DirectFBCreate( &dfb ));

     /* register termination function */
     atexit( dfb_shutdown );

     /* set the cooperative level to DFSCL_FULLSCREEN for exclusive access to the primary layer */
     dfb->SetCooperativeLevel( dfb, DFSCL_FULLSCREEN );

     /* create an event buffer for keyboard events */
     ret = dfb->GetInputDevice( dfb, DIDID_KEYBOARD, &keyboard );
     if (ret == DFB_OK)
          DFBCHECK(keyboard->CreateEventBuffer( keyboard, &event_buffer ));

     /* get the primary surface, i.e. the surface of the primary layer */
     sdsc.flags = DSDESC_CAPS;
     sdsc.caps  = DSCAPS_PRIMARY | DSCAPS_DOUBLE;

     DFBCHECK(dfb->CreateSurface( dfb, &sdsc, &primary ));

     DFBCHECK(primary->GetSize( primary, &xres, &yres ));

     /* load font */
     fdsc.flags  = DFDESC_HEIGHT;
     fdsc.height = CLAMP( (int) (yres / 10.0 / 8) * 8, 8, 96 );

#ifdef USE_FONT_HEADERS
     ddsc.flags         = DBDESC_MEMORY;
     ddsc.memory.data   = GET_FONTDATA( decker );
     ddsc.memory.length = GET_FONTSIZE( decker );
#else
     ddsc.flags         = DBDESC_FILE;
     ddsc.file          = GET_FONTFILE( decker );
#endif
     DFBCHECK(dfb->CreateDataBuffer( dfb, &ddsc, &buffer ));
     DFBCHECK(buffer->CreateFont( buffer, &fdsc, &font ));

     primary->SetFont( primary, font );

     /* create a surface and render an image to it */
#ifdef USE_IMAGE_HEADERS
     ddsc.flags         = DBDESC_MEMORY;
     ddsc.memory.data   = GET_IMAGEDATA( smokey_light );
     ddsc.memory.length = GET_IMAGESIZE( smokey_light );
#else
     ddsc.flags         = DBDESC_FILE;
     ddsc.file          = GET_IMAGEFILE( smokey_light );
#endif
     DFBCHECK(dfb->CreateDataBuffer( dfb, &ddsc, &buffer ));
     DFBCHECK(buffer->CreateImageProvider( buffer, &provider ));
     provider->GetSurfaceDescription( provider, &sdsc );
     DFBCHECK(primary->GetPixelFormat( primary, &sdsc.pixelformat ));
     DFBCHECK(dfb->CreateSurface( dfb, &sdsc, &smokey_light ));
     provider->RenderTo( provider, smokey_light, NULL );
     provider->Release( provider );

     demos_loop_thread = direct_thread_create( DTT_DEFAULT, demos_loop, NULL, "Animation Demos" );

     /* main loop */
     while (!quit) {
          if (event_buffer) {
               event_buffer->WaitForEventWithTimeout( event_buffer, 2, 0 );

               if (!quit)
                    keyboard->GetKeyState( keyboard, DIKI_ESCAPE, &quit );
          }

          if (quit) {
               if (current_demo == D_ARRAY_SIZE(demos) - 1) {
                    /* clear with black */
                    primary->SetColor( primary, 0, 0, 0, 0 );
                    primary->FillRectangle( primary, 0, 0, xres, yres );

                    /* ending message */
                    primary->SetColor( primary, 0xff, 0xff, 0xff, 0xff );
                    primary->DrawString( primary, "The End", -1, xres / 2, yres / 2, DSTF_CENTER );

                    /* flip display */
                    primary->Flip( primary, NULL, DSFLIP_WAITFORSYNC );

                    sleep( 2 );
               }
               else {
                    direct_thread_cancel( demos_loop_thread );
                    direct_thread_join( demos_loop_thread );
                    direct_thread_destroy( demos_loop_thread );
               }
               break;
          }
     }

     return 42;
}
