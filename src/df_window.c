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

#include <direct/clock.h>
#include <directfb.h>
#include <directfb_util.h>
#include <math.h>

#include "util.h"

#ifdef USE_FONT_HEADERS
#include "decker.h"
#endif

#ifdef USE_IMAGE_HEADERS
#include "cursor_red.h"
#include "cursor_yellow.h"
#include "dfblogo.h"
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
static IDirectFB             *dfb             = NULL;
static IDirectFBDisplayLayer *layer           = NULL;
static IDirectFBSurface      *cursor_surface  = NULL;
static IDirectFBWindow       *window1         = NULL;
static IDirectFBSurface      *window_surface1 = NULL;
static IDirectFBSurface      *cursor_surface1 = NULL;
static IDirectFBWindow       *window2         = NULL;
static IDirectFBSurface      *window_surface2 = NULL;
static IDirectFBSurface      *cursor_surface2 = NULL;
static IDirectFBEventBuffer  *event_buffer    = NULL;
static IDirectFBFont         *font            = NULL;

/* default stacking class */
static int stacking_id = DWSC_MIDDLE;

static void dfb_shutdown()
{
     if (font)            font->Release( font );
     if (event_buffer)    event_buffer->Release( event_buffer );
     if (cursor_surface2) cursor_surface2->Release( cursor_surface2 );
     if (window_surface2) window_surface2->Release( window_surface2 );
     if (window2)         window2->Release( window2 );
     if (cursor_surface1) cursor_surface1->Release( cursor_surface1 );
     if (window_surface1) window_surface1->Release( window_surface1 );
     if (window1)         window1->Release( window1 );
     if (layer) {
          layer->SetCooperativeLevel( layer, DLSCL_ADMINISTRATIVE );
          layer->SetCursorOpacity( layer, 0xFF );
          if (cursor_surface) {
               layer->SetCursorShape( layer, NULL, 0, 0 );
               cursor_surface->Release( cursor_surface );
          }
          layer->SetCooperativeLevel( layer, DLSCL_SHARED );
          layer->Release( layer );
     }
     if (dfb)             dfb->Release( dfb );
}

static void print_usage()
{
     printf( "DirectFB Window Demo\n\n" );
     printf( "Usage: df_window <stacking class>\n\n" );
}

int main( int argc, char *argv[] )
{
     int                       fontheight, winx, winy, winwidth, winheight;
     DFBWindowID               id1;
     DFBFontDescription        fdsc;
     DFBSurfaceDescription     sdsc;
     DFBWindowDescription      wdsc;
     DFBDataBufferDescription  ddsc;
     IDirectFBDataBuffer      *buffer;
     IDirectFBImageProvider   *provider;
     IDirectFBWindow          *upper;
     IDirectFBWindow          *active            = NULL;
     bool                      invisible_cursor1 = false;
     bool                      invisible_cursor2 = false;
     int                       cursor_enabled    = 1;
     int                       rotation          = 0;
     int                       grabbed           = 0;
     int                       startx            = 0;
     int                       starty            = 0;
     int                       endx              = 0;
     int                       endy              = 0;
     int                       winupdate         = 0;
     int                       quit              = 0;

     /* initialize DirectFB including command line parsing */
     DFBCHECK(DirectFBInit( &argc, &argv ));

     /* parse command line */
     if (argv[1] && !strcmp( argv[1], "--help" )) {
          print_usage();
          return 0;
     }

     if (argc > 1) {
          if (strcmp( argv[1], "upper" ) == 0)
               stacking_id = DWSC_UPPER;
          else if (strcmp( argv[1], "lower" ) == 0)
               stacking_id = DWSC_LOWER;
          else {
               print_usage();
               return 1;
          }
     }

     /* create the main interface */
     DFBCHECK(DirectFBCreate( &dfb ));

     /* register termination function */
     atexit( dfb_shutdown );

     /* get the primary display layer */
     DFBCHECK(dfb->GetDisplayLayer( dfb, DLID_PRIMARY, &layer ));

     /* set cursor shape for the primary display layer */
     if (getenv( "DEFAULT_CURSOR" )) {
          DFBCHECK(dfb->CreateImageProvider( dfb, getenv( "DEFAULT_CURSOR" ), &provider ));
          provider->GetSurfaceDescription( provider, &sdsc );
          DFBCHECK(dfb->CreateSurface( dfb, &sdsc, &cursor_surface ));
          provider->RenderTo( provider, cursor_surface, NULL );
          layer->SetCooperativeLevel( layer, DLSCL_ADMINISTRATIVE );
          DFBCHECK(layer->SetCursorShape( layer, cursor_surface, 0, 0 ));
          layer->SetCooperativeLevel( layer, DLSCL_SHARED );
          provider->Release( provider );
     }

     /* fill the window description. */
     wdsc.flags        = DWDESC_CAPS | DWDESC_POSX | DWDESC_POSY | DWDESC_WIDTH | DWDESC_HEIGHT | DWDESC_SURFACE_CAPS;
     wdsc.caps         = DWCAPS_ALPHACHANNEL;
     wdsc.surface_caps = DSCAPS_PREMULTIPLIED;

     if (stacking_id) {
          wdsc.flags    |= DWDESC_STACKING;
          wdsc.stacking  = stacking_id;
     }

     /* create window1 */
     wdsc.posx   = 200;
     wdsc.posy   = 225;
     wdsc.width  = 512;
     wdsc.height = 145;

     DFBCHECK(layer->CreateWindow( layer, &wdsc, &window1 ));

#ifdef USE_IMAGE_HEADERS
     ddsc.flags         = DBDESC_MEMORY;
     ddsc.memory.data   = GET_IMAGEDATA( dfblogo );
     ddsc.memory.length = GET_IMAGESIZE( dfblogo );
#else
     ddsc.flags         = DBDESC_FILE;
     ddsc.file          = GET_IMAGEFILE( dfblogo );
#endif
     DFBCHECK(dfb->CreateDataBuffer( dfb, &ddsc, &buffer ));
     DFBCHECK(buffer->CreateImageProvider( buffer, &provider ));
     DFBCHECK(window1->GetSurface( window1, &window_surface1 ));
     provider->RenderTo( provider, window_surface1, NULL );
     window_surface1->SetDrawingFlags( window_surface1, DSDRAW_SRC_PREMULTIPLY );
     window_surface1->SetColor( window_surface1, 0xFF, 0x20, 0x20, 0x90 );
     window_surface1->DrawRectangle( window_surface1, 0, 0, wdsc.width, wdsc.height );
     provider->Release( provider );

#ifdef USE_IMAGE_HEADERS
     ddsc.flags         = DBDESC_MEMORY;
     ddsc.memory.data   = GET_IMAGEDATA( cursor_red );
     ddsc.memory.length = GET_IMAGESIZE( cursor_red );
#else
     ddsc.flags         = DBDESC_FILE;
     ddsc.file          = GET_IMAGEFILE( cursor_red );
#endif
     DFBCHECK(dfb->CreateDataBuffer( dfb, &ddsc, &buffer ));
     DFBCHECK(buffer->CreateImageProvider( buffer, &provider ));
     provider->GetSurfaceDescription( provider, &sdsc );
     DFBCHECK(dfb->CreateSurface( dfb, &sdsc, &cursor_surface1 ));
     provider->RenderTo( provider, cursor_surface1, NULL );
     DFBCHECK(window1->SetCursorFlags( window1, DWCF_NONE ));
     DFBCHECK(window1->SetCursorShape( window1, cursor_surface1, 0, 0 ));
     provider->Release( provider );

     /* create window2. */
     wdsc.posx   = 20;
     wdsc.posy   = 120;
     wdsc.width  = 400;
     wdsc.height = 200;

     DFBCHECK(layer->CreateWindow( layer, &wdsc, &window2 ));

     DFBCHECK(window2->GetSurface( window2, &window_surface2 ));
     window_surface2->SetDrawingFlags( window_surface2, DSDRAW_SRC_PREMULTIPLY );
     window_surface2->SetColor( window_surface2, 0x00, 0x30, 0x10, 0xC0 );
     window_surface2->DrawRectangle( window_surface2, 0, 0, wdsc.width, wdsc.height );
     window_surface2->SetColor( window_surface2, 0x80, 0xA0, 0x00, 0x90 );
     window_surface2->FillRectangle( window_surface2, 1, 1, wdsc.width - 2, wdsc.height - 2 );

#ifdef USE_IMAGE_HEADERS
     ddsc.flags         = DBDESC_MEMORY;
     ddsc.memory.data   = GET_IMAGEDATA( cursor_yellow );
     ddsc.memory.length = GET_IMAGESIZE( cursor_yellow );
#else
     ddsc.flags         = DBDESC_FILE;
     ddsc.file          = GET_IMAGEFILE( cursor_yellow );
#endif
     DFBCHECK(dfb->CreateDataBuffer( dfb, &ddsc, &buffer ));
     DFBCHECK(buffer->CreateImageProvider( buffer, &provider ));
     provider->GetSurfaceDescription( provider, &sdsc );
     DFBCHECK(dfb->CreateSurface( dfb, &sdsc, &cursor_surface2 ));
     provider->RenderTo( provider, cursor_surface2, NULL );
     DFBCHECK(window2->SetCursorFlags( window2, DWCF_NONE ));
     DFBCHECK(window2->SetCursorShape( window2, cursor_surface2, 0, 0 ));
     provider->Release( provider );

     /* create an event buffer */
     DFBCHECK(window1->CreateEventBuffer( window1, &event_buffer ));
     DFBCHECK(window2->AttachEventBuffer( window2, event_buffer ));

     /* load font */
     fdsc.flags  = DFDESC_HEIGHT;
     fdsc.height = 16;

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
     DFBCHECK(font->GetHeight( font, &fontheight ));

     window_surface1->SetFont( window_surface1, font );

     window_surface2->SetFont( window_surface2, font );

     /* move the cursor to the center of the window1 */
     DFBCHECK(window1->GetPosition( window1, &winx, &winy ));
     DFBCHECK(window1->GetSize( window1, &winwidth, &winheight ));
     layer->SetCooperativeLevel( layer, DLSCL_ADMINISTRATIVE );
     DFBCHECK(layer->WarpCursor( layer, winx + (winwidth >> 1), winy + (winheight >> 1) ));
     layer->SetCooperativeLevel( layer, DLSCL_SHARED );

     /* window1 settings */
     window1->GetID( window1, &id1 );

     window1->RaiseToTop( window1 );

     upper = window1;

     window1->SetOpacity( window1, 0xFF );

     window1->RequestFocus( window1 );

     /* window2 settings */
     window_surface2->SetColor( window_surface2, 0xCF, 0xBF, 0xFF, 0xFF );
     window_surface2->DrawString( window_surface2,
                                  "Move the mouse over a window to activate it.",
                                  -1, 0, 0, DSTF_TOPLEFT );

     window_surface2->SetColor( window_surface2, 0xCF, 0xCF, 0xCF, 0xFF );
     window_surface2->DrawString( window_surface2,
                                  "Press left mouse button and drag to move the window.",
                                  -1, 0, fontheight, DSTF_TOPLEFT );

     window_surface2->SetColor( window_surface2, 0xCF, 0xDF, 0x9F, 0xFF );
     window_surface2->DrawString( window_surface2,
                                  "Press middle mouse button to raise/lower the window.",
                                  -1, 0, fontheight * 2, DSTF_TOPLEFT );

     window_surface2->SetColor( window_surface2, 0xCF, 0xEF, 0x6F, 0xFF );
     window_surface2->DrawString( window_surface2,
                                  "Hold right mouse button to fade in/out the window.",
                                  -1, 0, fontheight * 3, DSTF_TOPLEFT );

     window_surface2->SetColor( window_surface2, 0xCF, 0xFF, 0x3F, 0xFF );
     window_surface2->DrawString( window_surface2,
                                  "Press r key to rotate the window.",
                                  -1, 0, fontheight * 4, DSTF_TOPLEFT );

     window2->SetOpacity( window2, 0xFF );

     /* main loop */
     while (!quit) {
          DFBWindowEvent evt;

          event_buffer->WaitForEventWithTimeout( event_buffer, 0, 10 );

          /* process event buffer */
          while (event_buffer->GetEvent( event_buffer, DFB_EVENT(&evt) ) == DFB_OK) {
               IDirectFBWindow *window;

               if (evt.window_id == id1)
                    window = window1;
               else
                    window = window2;

               if (evt.type == DWET_GOTFOCUS) {
                    active = window;
               }
               else if (active) {
                    switch (evt.type) {
                         case DWET_BUTTONDOWN:
                              if (!grabbed) {
                                   grabbed = evt.buttons;
                                   startx  = evt.cx;
                                   starty  = evt.cy;
                                   window->GrabPointer( window );
                              }
                              break;

                         case DWET_BUTTONUP:
                              switch (evt.button) {
                                   case DIBI_LEFT:
                                   case DIBI_RIGHT:
                                        if (grabbed && !evt.buttons) {
                                             window->UngrabPointer( window );
                                             grabbed = 0;
                                        }
                                        break;
                                   case DIBI_MIDDLE:
                                        upper->LowerToBottom( upper );
                                        upper = (upper == window1) ? window2 : window1;
                                        break;
                                   default:
                                        break;
                              }
                              break;

                         case DWET_KEYDOWN:
                              if (grabbed)
                                   break;
                              switch (evt.key_id) {
                                   case DIKI_RIGHT:
                                        active->Move( active, 1, 0 );
                                        break;
                                   case DIKI_LEFT:
                                        active->Move( active, -1, 0 );
                                        break;
                                   case DIKI_UP:
                                        active->Move( active, 0, -1 );
                                        break;
                                   case DIKI_DOWN:
                                        active->Move( active, 0, 1 );
                                        break;
                                   default:
                                        break;
                              }
                              break;

                         case DWET_LOSTFOCUS:
                              if (!grabbed && active == window) {
                                   active = NULL;
                              }
                              break;

                         default:
                              break;
                    }
               }

               switch (evt.type) {
                    case DWET_MOTION:
                    case DWET_ENTER:
                    case DWET_LEAVE:
                         endx = evt.cx;
                         endy = evt.cy;
                         winx = evt.x;
                         winy = evt.y;
                         winupdate = 1;
                         break;

                    case DWET_KEYDOWN:
                         switch (evt.key_symbol) {
                              case DIKS_ESCAPE:
                              case DIKS_SMALL_Q:
                              case DIKS_CAPITAL_Q:
                              case DIKS_BACK:
                              case DIKS_STOP:
                              case DIKS_EXIT:
                                   /* quit main loop */
                                   quit = 1;
                                   break;

                              case DIKS_SMALL_I:
                                   if (active == window1) {
                                        invisible_cursor1 = !invisible_cursor1;
                                        window1->SetCursorFlags( window1,
                                                                 invisible_cursor1 ? DWCF_INVISIBLE : DWCF_NONE );
                                   }
                                   else {
                                        invisible_cursor2 = !invisible_cursor2;
                                        window2->SetCursorFlags( window2,
                                                                 invisible_cursor2 ? DWCF_INVISIBLE : DWCF_NONE );
                                   }
                                   break;

                              case DIKS_SMALL_O:
                                   layer->SetCooperativeLevel( layer, DLSCL_ADMINISTRATIVE );
                                   layer->SetCursorOpacity( layer, sin( direct_clock_get_millis() / 300.0 ) * 85 + 170 );
                                   layer->SetCooperativeLevel( layer, DLSCL_SHARED );
                                   break;

                              case DIKS_SMALL_P:
                                   cursor_enabled = !cursor_enabled;
                                   layer->SetCooperativeLevel( layer, DLSCL_ADMINISTRATIVE );
                                   layer->EnableCursor( layer, cursor_enabled );
                                   layer->SetCooperativeLevel( layer, DLSCL_SHARED );
                                   break;

                              case DIKS_SMALL_R:
                                   if (active) {
                                        rotation = (rotation + 90) % 360;
                                        active->SetRotation( active, rotation );
                                   }
                                   break;

                              default:
                                   break;
                         }
                         break;

                    default:
                         break;
               }
          }

          if (active) {
               if (grabbed == DIBM_LEFT) {
                    if (startx == endx && starty == endy) {
                         if (event_buffer->WaitForEventWithTimeout( event_buffer, 2, 0 ) == DFB_TIMEOUT) {
                              /* quit main loop */
                              quit = 1;
                         }
                    }
                    else {
                         active->Move( active, endx - startx, endy - starty );
                         startx = endx;
                         starty = endy;
                    }
               }
               else if (grabbed == DIBM_RIGHT) {
                    active->SetOpacity( active, sin( direct_clock_get_millis() / 300.0 ) * 85 + 170 );
               }
               else if (winupdate) {
                    DFBRectangle rect;
                    DFBRegion    region;
                    char         buf[32];

                    snprintf( buf, sizeof(buf), "x/y: %4d,%4d", winx, winy );

                    DFBCHECK(font->GetStringExtents( font, buf, -1, &rect, NULL ));

                    rect.x  = 1;
                    rect.y  = 1;
                    rect.w += rect.w / 3;
                    rect.h += 10;

                    window_surface1->SetColor( window_surface1, 0x10, 0x10, 0x10, 0x77 );
                    window_surface1->FillRectangles( window_surface1, &rect, 1 );

                    window_surface1->SetColor( window_surface1, 0x88, 0xCC, 0xFF, 0xAA );
                    window_surface1->DrawString( window_surface1, buf, -1, rect.h / 4, 5, DSTF_TOPLEFT );

                    region = DFB_REGION_INIT_FROM_RECTANGLE( &rect );

                    window_surface1->Flip( window_surface1, &region, DSFLIP_NONE );

                    winupdate = 0;
               }
          }
     }

     return 42;
}
