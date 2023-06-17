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
#include <math.h>

#include "util.h"

/* DirectFB interfaces */
static IDirectFB            *dfb          = NULL;
static IDirectFBEventBuffer *event_buffer = NULL;
static IDirectFBSurface     *primary      = NULL;

/* screen width and height */
static int width, height;

/**********************************************************************************************************************/

typedef struct {
     double xx; double yx;
     double xy; double yy;
     double x0; double y0;
} matrix_t;

static void matrix_init( matrix_t *matrix, double xx, double yx, double xy, double yy, double x0, double y0 )
{
     matrix->xx = xx; matrix->yx = yx;
     matrix->xy = xy; matrix->yy = yy;
     matrix->x0 = x0; matrix->y0 = y0;
}

static void matrix_multiply( matrix_t *result, const matrix_t *a, const matrix_t *b )
{
     matrix_t r;

     r.xx = a->xx * b->xx + a->yx * b->xy;
     r.yx = a->xx * b->yx + a->yx * b->yy;

     r.xy = a->xy * b->xx + a->yy * b->xy;
     r.yy = a->xy * b->yx + a->yy * b->yy;

     r.x0 = a->x0 * b->xx + a->y0 * b->xy + b->x0;
     r.y0 = a->x0 * b->yx + a->y0 * b->yy + b->y0;

     *result = r;
}

static void matrix_init_scale( matrix_t *matrix, double sx, double sy )
{
     matrix_init( matrix, sx,  0, 0, sy, 0, 0 );
}

static void matrix_scale( matrix_t *matrix, double sx, double sy )
{
     matrix_t tmp;

     matrix_init_scale( &tmp, sx, sy );

     matrix_multiply( matrix, &tmp, matrix );
}

static void matrix_init_rotate( matrix_t *matrix, double radians )
{
     double c, s;

     c = cos( radians );
     s = sin( radians );

     matrix_init( matrix, c, s, -s, c, 0, 0 );
}

static void matrix_rotate( matrix_t *matrix, double radians )
{
     matrix_t tmp;

     matrix_init_rotate( &tmp, radians );

     matrix_multiply( matrix, &tmp, matrix );
}

/**********************************************************************************************************************/

static void set_matrix( const matrix_t *matrix )
{
     s32 m[9];

     m[0] = matrix->xx * 0x10000;
     m[1] = matrix->xy * 0x10000;
     m[2] = matrix->x0 * 0x10000;
     m[3] = matrix->yx * 0x10000;
     m[4] = matrix->yy * 0x10000;
     m[5] = matrix->y0 * 0x10000;
     m[6] = 0x00000;
     m[7] = 0x00000;
     m[8] = 0x10000;

     primary->SetMatrix( primary, m );
}

/**********************************************************************************************************************/

static void exit_application( int status )
{
     /* Release the primary surface. */
     if (primary)
          primary->Release( primary );

     /* Release the event buffer. */
     if (event_buffer)
          event_buffer->Release( event_buffer );

     /* Release the main interface. */
     if (dfb)
          dfb->Release( dfb );

     /* Terminate application. */
     exit( status );
}

static void init_application( int argc, char *argv[] )
{
     DFBResult             ret;
     DFBSurfaceDescription desc;

     /* Initialize DirectFB including command line parsing. */
     ret = DirectFBInit( &argc, &argv );
     if (ret) {
          DirectFBError( "DirectFBInit() failed", ret );
          exit_application( 1 );
     }

     /* Create the main interface. */
     ret = DirectFBCreate( &dfb );
     if (ret) {
          DirectFBError( "DirectFBCreate() failed", ret );
          exit_application( 2 );
     }

     /* Set the cooperative level to DFSCL_FULLSCREEN for exclusive access to the primary layer. */
     dfb->SetCooperativeLevel( dfb, DFSCL_FULLSCREEN );

     /* Create an event buffer for key events. */
     ret = dfb->CreateInputEventBuffer( dfb, DICAPS_BUTTONS | DICAPS_KEYS, DFB_FALSE, &event_buffer );
     if (ret) {
          DirectFBError( "CreateInputEventBuffer() failed", ret );
          exit_application( 3 );
     }

     /* Fill the surface description. */
     desc.flags = DSDESC_CAPS;
     desc.caps  = DSCAPS_PRIMARY | DSCAPS_DOUBLE;

     /* Get the primary surface, i.e. the surface of the primary layer. */
     ret = dfb->CreateSurface( dfb, &desc, &primary );
     if (ret) {
          DirectFBError( "CreateSurface() failed", ret );
          exit_application( 4 );
     }

     /* Query the size of the primary surface. */
     primary->GetSize( primary, &width, &height );
}

int main( int argc, char *argv[] )
{
     matrix_t matrix;
     int      i = 0;

     /* Initialize application. */
     init_application( argc, argv );

     /* Transform coordinates to have 0,0 in the center. */
     matrix_init( &matrix, 1, 0, 0, 1, width / 2, height / 2 );

     /* Main loop. */
     while (1) {
          DFBInputEvent evt;

          /* Convert double to DirectFB's fixed point 16.16 and call SetMatrix(). */
          set_matrix( &matrix );

          /* Clear the frame. */
          primary->Clear( primary, 0x00, 0x00, 0x00, 0x00 );

          /* Fill a red rectangle down-right without AA. */
          primary->SetRenderOptions( primary, DSRO_MATRIX );
          primary->SetColor( primary, 0xff, 0, 0, 0xff );
          primary->FillRectangle( primary, 100, 100, 100, 100 );

          /* Enable coordinate transformation and anti-aliasing. */
          primary->SetRenderOptions( primary, DSRO_MATRIX | DSRO_ANTIALIAS );

          /* Fill a small white rectangle in the middle. */
          primary->SetColor( primary, 0xff, 0xff, 0xff, 0xff );
          primary->FillRectangle( primary, -20, -20, 40, 40 );

          /* Fill a small green rectangle on the left. */
          primary->SetColor( primary, 0x00, 0xff, 0x00, 0xff );
          primary->FillRectangle( primary, -120, -20, 40, 40 );

          /* Fill a small blue rectangle at the top. */
          primary->SetColor( primary, 0x00, 0x00, 0xff, 0xff );
          primary->FillRectangle( primary, -20, -120, 40, 40 );

          /* Draw a white outline around the red rectangle. */
          primary->SetColor( primary, 0xcc, 0xcc, 0xcc, 0xff );
          primary->DrawRectangle( primary, 100, 100, 100, 100 );

          /* Draw a line across the objects. */
          primary->SetColor( primary, 0x12, 0x34, 0x56, 0xff );
          primary->DrawLine( primary, 0, 0, 300, 300 );

          primary->SetColor( primary, 0xff, 0xff, 0xff, 0xff );
          primary->DrawLine( primary, -20, -20, -300, -300 );

          /* Fill a triangle. */
          primary->SetColor( primary, 0x80, 0x90, 0x70, 0xff );
          primary->FillTriangle( primary, 0, 0, 200, -210, -200, 190 );

          /* Flip the output surface. */
          primary->Flip( primary, NULL, DSFLIP_WAITFORSYNC );

          /* Rotate and scale scene slightly. */
          matrix_rotate( &matrix, 0.1 );
          matrix_scale( &matrix, 0.99, 0.99 );

          /* Reset to initial transform after 500 frames. */
          if (++i == 500) {
               i = 0;
               matrix_init( &matrix, 1, 0, 0, 1, width / 2, height / 2 );
          }

          /* Check for new events. */
          while (event_buffer->GetEvent( event_buffer, DFB_EVENT(&evt) ) == DFB_OK) {
               if (evt.buttons & DIBM_LEFT) {
                    if (event_buffer->WaitForEventWithTimeout( event_buffer, 2, 0 ) == DFB_TIMEOUT)
                         exit_application( 42 );
               }
               else if (evt.type == DIET_KEYPRESS) {
                    switch (evt.key_symbol) {
                         case DIKS_ESCAPE:
                         case DIKS_SMALL_Q:
                         case DIKS_CAPITAL_Q:
                         case DIKS_BACK:
                         case DIKS_STOP:
                         case DIKS_EXIT:
                              exit_application( 42 );
                              break;
                         default:
                              break;
                    }
               }
          }
     }

     /* Shouldn't reach this. */
     return 0;
}
