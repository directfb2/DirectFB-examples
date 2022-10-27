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
static IDirectFBSurface *gridimage  = NULL;
static IDirectFBSurface *testimage  = NULL;
static IDirectFBSurface *testimage2 = NULL;

/*
 * deinitializes resources and DirectFB
 */
static void deinit_resources()
{
     if (testimage2)   testimage2->Release( testimage2 );
     if (testimage)    testimage->Release( testimage );
     if (gridimage)    gridimage->Release( gridimage );
     if (primary)      primary->Release( primary );
     if (event_buffer) event_buffer->Release( event_buffer );
     if (dfb)          dfb->Release( dfb );
}

/*
 * set up DirectFB and load resources
 */
static void init_resources( int argc, char *argv[] )
{
     DFBSurfaceDescription   dsc;
     IDirectFBImageProvider *provider;

     /* initialize DirectFB including command line parsing */
     DFBCHECK(DirectFBInit( &argc, &argv ));

     /* create the main interface */
     DFBCHECK(DirectFBCreate( &dfb ));

     /* register termination function */
     atexit( deinit_resources );

     /* set the cooperative level to DFSCL_FULLSCREEN for exclusive access to the primary layer */
     DFBCHECK(dfb->SetCooperativeLevel( dfb, DFSCL_FULLSCREEN ));

     /* create an event buffer */
     DFBCHECK(dfb->CreateInputEventBuffer( dfb, DICAPS_KEYS, DFB_FALSE, &event_buffer ));

     /* get the primary surface, i.e. the surface of the primary layer */
     dsc.flags = DSDESC_CAPS;
     dsc.caps  = DSCAPS_PRIMARY;

     DFBCHECK(dfb->CreateSurface( dfb, &dsc, &primary ));

     /* create a surface and render an image to it */
     DFBCHECK(dfb->CreateImageProvider( dfb, DATADIR"/grid.dfiff", &provider ));
     provider->GetSurfaceDescription( provider, &dsc );
     DFBCHECK(dfb->CreateSurface( dfb, &dsc, &gridimage ));
     provider->RenderTo( provider, gridimage, NULL );
     provider->Release( provider );

     /* create a surface and render an image to it */
     DFBCHECK(dfb->CreateImageProvider( dfb, DATADIR"/laden_bike.dfiff", &provider ));
     provider->GetSurfaceDescription( provider, &dsc );
     dsc.width  = 128;
     dsc.height = 256;
     DFBCHECK(dfb->CreateSurface( dfb, &dsc, &testimage ));
     provider->RenderTo( provider, testimage, NULL );
     dsc.width  = 192;
     dsc.height = 96;
     DFBCHECK(dfb->CreateSurface( dfb, &dsc, &testimage2 ));
     provider->RenderTo( provider, testimage2, NULL );
     provider->Release( provider );
}

int main( int argc, char *argv[] )
{
     int                     clip_enabled  = 0;
     DFBRegion               clipreg       = { 128, 128, 384 + 128 - 1, 256 + 128 - 1 };
     DFBSurfaceBlittingFlags blittingflags = DSBLIT_NOFX;
     int                     quit          = 0;

     init_resources( argc, argv );

     primary->Clear( primary, 0x00, 0x00, 0x00, 0xFF );
     primary->Blit( primary, gridimage, NULL, 128, 128 );
     primary->Flip( primary, NULL, DSFLIP_WAITFORSYNC );

     /* main loop */
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
                              /* quit main loop */
                              quit = 1;
                              break;

                         /* test blitting */
                         case DIKS_SMALL_B:
                              primary->SetClip( primary, NULL );

                              primary->Clear( primary, 0x00, 0x00, 0x00, 0xFF );

                              if (clip_enabled)
                                   primary->SetClip( primary, &clipreg );
                              else
                                   primary->SetClip( primary, NULL );

                              primary->SetBlittingFlags( primary, DSBLIT_NOFX );
                              primary->Blit( primary, gridimage, NULL, 128, 128 );

                              primary->SetBlittingFlags( primary, blittingflags );
                              primary->Blit( primary, testimage2, NULL, 64, 96 );
                              primary->Blit( primary, testimage2, NULL, 384, 96 );
                              primary->Blit( primary, testimage2, NULL, 64, 320 );
                              primary->Blit( primary, testimage2, NULL, 384, 320 );

                              primary->Flip( primary, NULL, DSFLIP_WAITFORSYNC );
                              break;

                         /* test stretched blitting */
                         case DIKS_SMALL_S: {
                              DFBRectangle drect;

                              primary->SetClip( primary, NULL );

                              primary->Clear( primary, 0x00, 0x00, 0x00, 0xFF );

                              if (clip_enabled)
                                   primary->SetClip( primary, &clipreg );
                              else
                                   primary->SetClip( primary, NULL );

                              primary->SetBlittingFlags( primary, DSBLIT_NOFX );
                              primary->Blit( primary, gridimage, NULL, 128, 128 );

                              primary->SetBlittingFlags( primary, blittingflags );
                              drect.x = 96;
                              drect.y = 32;
                              drect.w = 2.5 * 64;
                              drect.h = 2.5 * 64;
                              primary->StretchBlit( primary, testimage2, NULL, &drect );
                              drect.x = 384;
                              primary->StretchBlit( primary, testimage2, NULL, &drect );
                              drect.y = 320;
                              primary->StretchBlit( primary, testimage2, NULL, &drect );
                              drect.x = 96;
                              primary->StretchBlit( primary, testimage2, NULL, &drect );

                              primary->Flip( primary, NULL, DSFLIP_WAITFORSYNC );
                              break;
                         }

                         /* toggle clipping */
                         case DIKS_SMALL_C:
                              clip_enabled = !clip_enabled;
                              break;

                         /* toggle horizontal flipping */
                         case DIKS_SMALL_H:
                              blittingflags ^= DSBLIT_FLIP_HORIZONTAL;
                              break;

                         /* toggle vertical clipping */
                         case DIKS_SMALL_V:
                              blittingflags ^= DSBLIT_FLIP_VERTICAL;
                              break;

                         /* toggle rotate 90 */
                         case DIKS_SMALL_R:
                              blittingflags ^= DSBLIT_ROTATE90;
                              break;

                         default:
                              break;
                    }
               }
          }
     }

     return 42;
}
