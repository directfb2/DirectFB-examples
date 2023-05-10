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

#ifdef USE_IMAGE_HEADERS
#include "grid.h"
#include "laden_bike.h"
#include "mask.h"
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

/* DirectFB surfaces */
static IDirectFBSurface *background = NULL;
static IDirectFBSurface *testimage  = NULL;
static IDirectFBSurface *testimage2 = NULL;

/* background */
typedef enum {
     GRID_BG,
     MASK_BG
} Background;

#ifdef USE_IMAGE_HEADERS
static const void *bg_data[] = {
     grid_data, mask_data
};
static unsigned int bg_length[] = {
     sizeof(grid_data), sizeof(mask_data)
};
#else
static const char *bg_names[] = {
     "grid.dfiff", "mask.dfiff"
};
#endif

/*
 * deinitializes test resources
 */
static void deinit_resources()
{
     if (testimage2) {
          testimage2->Release( testimage2 );
          testimage2 = NULL;
     }

     if (testimage) {
          testimage->Release( testimage );
          testimage = NULL;
     }

     if (background) {
          background->Release( background );
          background = NULL;
     }
}

/*
 * initializes test resources
 */
static void init_resources( Background bg, int w, int h, int w2, int h2 )
{
     DFBSurfaceDescription     sdsc;
#ifdef USE_IMAGE_HEADERS
     DFBDataBufferDescription  ddsc;
     IDirectFBDataBuffer      *buffer;
#else
     int                       path_length;
     char                     *imagefile;
#endif
     IDirectFBImageProvider   *provider;

     /* load the background image */
#ifdef USE_IMAGE_HEADERS
     ddsc.flags         = DBDESC_MEMORY;
     ddsc.memory.data   = bg_data[bg];
     ddsc.memory.length = bg_length[bg];
     DFBCHECK(dfb->CreateDataBuffer( dfb, &ddsc, &buffer ));
     DFBCHECK(buffer->CreateImageProvider( buffer, &provider ));
#else
     path_length = strlen( DATADIR ) + 1 + strlen( bg_names[bg] ) + 1;
     imagefile   = alloca( path_length );
     snprintf( imagefile, path_length, DATADIR"/%s", bg_names[bg] );
     DFBCHECK(dfb->CreateImageProvider( dfb, imagefile, &provider ));
#endif
     provider->GetSurfaceDescription( provider, &sdsc );
     DFBCHECK(dfb->CreateSurface( dfb, &sdsc, &background ));
     provider->RenderTo( provider, background, NULL );
     provider->Release( provider );

     /* create a surface and render an image to it */
#ifdef USE_IMAGE_HEADERS
     ddsc.flags         = DBDESC_MEMORY;
     ddsc.memory.data   = laden_bike_data;
     ddsc.memory.length = sizeof(laden_bike_data);
     DFBCHECK(dfb->CreateDataBuffer( dfb, &ddsc, &buffer ));
     DFBCHECK(buffer->CreateImageProvider( buffer, &provider ));
#else
     imagefile = DATADIR"/laden_bike.dfiff";
     DFBCHECK(dfb->CreateImageProvider( dfb, imagefile, &provider ));
#endif
     provider->GetSurfaceDescription( provider, &sdsc );
     sdsc.width  = w;
     sdsc.height = h;
     DFBCHECK(dfb->CreateSurface( dfb, &sdsc, &testimage ));
     provider->RenderTo( provider, testimage, NULL );
     sdsc.width  = w2;
     sdsc.height = h2;
     DFBCHECK(dfb->CreateSurface( dfb, &sdsc, &testimage2 ));
     provider->RenderTo( provider, testimage2, NULL );
     provider->Release( provider );
}

static void cleanup()
{
     deinit_resources();

     if (primary)
          primary->Release( primary );

     if (event_buffer)
          event_buffer->Release( event_buffer );

     if (dfb)
          dfb->Release( dfb );
}

int main( int argc, char *argv[] )
{
     int                     quit;
     DFBSurfaceDescription   desc;
     int                     clip_enabled  = 0;
     DFBRegion               clipreg       = { 128, 128, 384 + 128 - 1, 256 + 128 - 1 };
     DFBSurfaceBlittingFlags blittingflags = DSBLIT_NOFX;

     /* initialize DirectFB including command line parsing */
     DFBCHECK(DirectFBInit( &argc, &argv ));

     /* create the main interface */
     DFBCHECK(DirectFBCreate( &dfb ));

     /* register termination function */
     atexit( cleanup );

     /* set the cooperative level to DFSCL_FULLSCREEN for exclusive access to the primary layer */
     dfb->SetCooperativeLevel( dfb, DFSCL_FULLSCREEN );

     /* create an event buffer */
     DFBCHECK(dfb->CreateInputEventBuffer( dfb, DICAPS_KEYS, DFB_FALSE, &event_buffer ));

     /* get the primary surface, i.e. the surface of the primary layer */
     desc.flags = DSDESC_CAPS;
     desc.caps  = DSCAPS_PRIMARY | DSCAPS_DOUBLE;

     DFBCHECK(dfb->CreateSurface( dfb, &desc, &primary ));

     /* test1 init */
     init_resources( MASK_BG, 128, 128, 111, 77 );

     quit = 0;
     primary->Clear( primary, 0x00, 0x00, 0x00, 0xFF );
     primary->Blit( primary, background, NULL, 0, 0 );
     primary->Flip( primary, NULL, DSFLIP_WAITFORSYNC );

     /* test1 main loop */
     while (!quit) {
          DFBInputEvent evt;

          event_buffer->WaitForEvent( event_buffer );

          /* process event buffer */
          while (event_buffer->GetEvent( event_buffer, DFB_EVENT(&evt) ) == DFB_OK) {
               if (evt.type == DIET_KEYPRESS) {
                    switch (DFB_LOWER_CASE( evt.key_symbol )) {
                         case DIKS_ESCAPE:
                         case DIKS_SMALL_Q:
                         case DIKS_BACK:
                         case DIKS_STOP:
                         case DIKS_EXIT:
                         case DIKS_9:
                              /* quit main loop */
                              quit = 1;
                              break;

                         /* test blitting */
                         case DIKS_SMALL_B:
                         case DIKS_1:
                              primary->Blit( primary, background, NULL, 0, 0 );
                              primary->Blit( primary, testimage, NULL, 20, 20 );
                              primary->Blit( primary, testimage2, NULL, 319, 70 );
                              primary->Flip( primary, NULL, DSFLIP_WAITFORSYNC );
                              break;

                         /* test stretched blitting */
                         case DIKS_SMALL_S:
                         case DIKS_2: {
                              DFBRectangle rect1 = { 319, 70, 111, 77 };
                              DFBRectangle rect2 = { 20, 20, 128, 128 };
                              primary->Blit( primary, background, NULL, 0, 0 );
                              primary->StretchBlit( primary, testimage, NULL, &rect1 );
                              primary->StretchBlit( primary, testimage2, NULL, &rect2 );
                              primary->Flip( primary, NULL, DSFLIP_WAITFORSYNC );
                              break;
                         }

                         /* test rectangle filling */
                         case DIKS_SMALL_F:
                         case DIKS_3:
                              primary->SetDrawingFlags( primary, DSDRAW_NOFX );
                              primary->Blit( primary, background, NULL, 0, 0 );
                              primary->SetColor( primary, 0xFF, 0x00, 0xFF, 0xFF );
                              primary->FillRectangle( primary, 319, 70, 111, 77 );
                              primary->FillRectangle( primary, 20, 20, 128, 128 );
                              primary->Flip( primary, NULL, DSFLIP_WAITFORSYNC );
                              break;

                         /* test rectangle drawing */
                         case DIKS_SMALL_D:
                         case DIKS_4:
                              primary->SetDrawingFlags( primary, DSDRAW_NOFX );
                              primary->Blit( primary, background, NULL, 0, 0 );
                              primary->SetColor( primary, 0xFF, 0x00, 0xFF, 0xFF );
                              primary->DrawRectangle( primary, 319, 70, 111, 77 );
                              primary->DrawRectangle( primary, 20, 20, 128, 128 );
                              primary->Flip( primary, NULL, DSFLIP_WAITFORSYNC );
                              break;

                         /* test rectangle blending */
                         case DIKS_SMALL_R:
                         case DIKS_5:
                              primary->Blit( primary, background, NULL, 0, 0 );
                              primary->SetDrawingFlags( primary, DSDRAW_BLEND );
                              primary->SetColor( primary, 0xFF, 0x00, 0xFF, 0x80 );
                              primary->FillRectangle( primary, 319, 70, 111, 77 );
                              primary->FillRectangle( primary, 20, 20, 128, 128 );
                              primary->SetDrawingFlags( primary, DSDRAW_NOFX );
                              primary->Flip( primary, NULL, DSFLIP_WAITFORSYNC );
                              break;

                         default:
                              break;
                    }
               }
          }
     }

     deinit_resources();

     /* test2 init */
     init_resources( GRID_BG, 128, 256, 192, 96 );

     quit = 0;
     primary->Clear( primary, 0x00, 0x00, 0x00, 0xFF );
     primary->Blit( primary, background, NULL, 128, 128 );
     primary->Flip( primary, NULL, DSFLIP_WAITFORSYNC );

     /* test2 main loop */
     while (!quit) {
          DFBInputEvent evt;

          event_buffer->WaitForEvent( event_buffer );

          /* process event buffer */
          while (event_buffer->GetEvent( event_buffer, DFB_EVENT(&evt) ) == DFB_OK) {
               if (evt.type == DIET_KEYPRESS) {
                    switch (DFB_LOWER_CASE( evt.key_symbol )) {
                         case DIKS_ESCAPE:
                         case DIKS_SMALL_Q:
                         case DIKS_BACK:
                         case DIKS_STOP:
                         case DIKS_EXIT:
                         case DIKS_9:
                              /* quit main loop */
                              quit = 1;
                              break;

                         /* test blitting */
                         case DIKS_SMALL_B:
                         case DIKS_1:
                              primary->SetClip( primary, NULL );

                              primary->Clear( primary, 0x00, 0x00, 0x00, 0xFF );

                              if (clip_enabled)
                                   primary->SetClip( primary, &clipreg );
                              else
                                   primary->SetClip( primary, NULL );

                              primary->SetBlittingFlags( primary, DSBLIT_NOFX );
                              primary->Blit( primary, background, NULL, 128, 128 );

                              primary->SetBlittingFlags( primary, blittingflags );
                              primary->Blit( primary, testimage2, NULL, 64, 96 );
                              primary->Blit( primary, testimage2, NULL, 384, 96 );
                              primary->Blit( primary, testimage2, NULL, 64, 320 );
                              primary->Blit( primary, testimage2, NULL, 384, 320 );

                              primary->Flip( primary, NULL, DSFLIP_WAITFORSYNC );
                              break;

                         /* test stretched blitting */
                         case DIKS_SMALL_S:
                         case DIKS_2: {
                              DFBRectangle drect;

                              primary->SetClip( primary, NULL );

                              primary->Clear( primary, 0x00, 0x00, 0x00, 0xFF );

                              if (clip_enabled)
                                   primary->SetClip( primary, &clipreg );
                              else
                                   primary->SetClip( primary, NULL );

                              primary->SetBlittingFlags( primary, DSBLIT_NOFX );
                              primary->Blit( primary, background, NULL, 128, 128 );

                              primary->SetBlittingFlags( primary, blittingflags );
                              drect.x = 96;
                              drect.y = 32;
                              drect.w = 2.5 * 64;
                              drect.h = 2.5 * 64;
                              primary->StretchBlit( primary, testimage, NULL, &drect );
                              drect.x = 384;
                              primary->StretchBlit( primary, testimage, NULL, &drect );
                              drect.y = 320;
                              primary->StretchBlit( primary, testimage, NULL, &drect );
                              drect.x = 96;
                              primary->StretchBlit( primary, testimage, NULL, &drect );

                              primary->Flip( primary, NULL, DSFLIP_WAITFORSYNC );
                              break;
                         }

                         /* toggle clipping */
                         case DIKS_SMALL_C:
                         case DIKS_3:
                              clip_enabled = !clip_enabled;
                              break;

                         /* toggle horizontal flipping */
                         case DIKS_SMALL_H:
                         case DIKS_4:
                              blittingflags ^= DSBLIT_FLIP_HORIZONTAL;
                              break;

                         /* toggle rotate 90 */
                         case DIKS_SMALL_R:
                         case DIKS_5:
                              blittingflags ^= DSBLIT_ROTATE90;
                              break;

                         /* toggle vertical clipping */
                         case DIKS_SMALL_V:
                         case DIKS_6:
                              blittingflags ^= DSBLIT_FLIP_VERTICAL;
                              break;

                         default:
                              break;
                    }
               }
          }
     }

     return 42;
}
