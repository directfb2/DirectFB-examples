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

#include "util.h"

/**************************************************************************************************/

/* DirectFB interfaces */
static IDirectFB             *dfb          = NULL;
static IDirectFBDisplayLayer *layer        = NULL;
static IDirectFBWindow       *window       = NULL;
static IDirectFBSurface      *surface      = NULL;
static IDirectFBEventBuffer  *event_buffer = NULL;

/**************************************************************************************************/

static void exit_application( int status )
{
     /* Release the event buffer. */
     if (event_buffer)
          event_buffer->Release( event_buffer );

     /* Release the window's surface. */
     if (surface)
          surface->Release( surface );

     /* Release the window. */
     if (window)
          window->Release( window );

     /* Release the layer. */
     if (layer)
          layer->Release( layer );

     /* Release the main interface. */
     if (dfb)
          dfb->Release( dfb );

     /* Terminate application. */
     exit( status );
}

static void init_application( int argc, char *argv[] )
{
     DFBResult             ret;
     DFBDisplayLayerConfig config;
     DFBWindowDescription  desc;

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

     /* Get the primary display layer. */
     ret = dfb->GetDisplayLayer( dfb, DLID_PRIMARY, &layer );
     if (ret) {
          DirectFBError( "GetDisplayLayer() failed", ret );
          exit_application( 3 );
     }

     /* Get the screen size. */
     layer->GetConfiguration( layer, &config );

     /* Fill the window description. */
     desc.flags  = DWDESC_CAPS | DWDESC_POSX | DWDESC_POSY | DWDESC_WIDTH | DWDESC_HEIGHT;
     desc.caps   = DWCAPS_ALPHACHANNEL | DWCAPS_NODECORATION;
     desc.posx   = config.width - 224;
     desc.posy   = 8;
     desc.width  = 64;
     desc.height = 64;

     /* Create the window. */
     ret = layer->CreateWindow( layer, &desc, &window );
     if (ret) {
          DirectFBError( "CreateWindow() failed", ret );
          exit_application( 4 );
     }

     /* Get the window's surface. */
     ret = window->GetSurface( window, &surface );
     if (ret) {
          DirectFBError( "GetSurface() failed", ret );
          exit_application( 5 );
     }

     /* Create an event buffer. */
     ret = window->CreateEventBuffer( window, &event_buffer );
     if (ret) {
          DirectFBError( "CreateEventBuffer() failed", ret );
          exit_application( 6 );
     }

     /* Add ghost option (behave like an overlay). */
     window->SetOptions( window, DWOP_ALPHACHANNEL | DWOP_GHOST );

     /* Move window to upper stacking class. */
     window->SetStackingClass( window, DWSC_UPPER );

     /* Make it the top most window. */
     window->RaiseToTop( window );
}

/**************************************************************************************************/

static int get_load( void )
{
     static int            old_load = 0;
     static unsigned long  old_u, old_n, old_s, old_i, old_wa, old_hi, old_si;
     unsigned long         new_u, new_n, new_s, new_i, new_wa, new_hi, new_si;
     int                   u = 0, n = 0, s = 0, load;
     unsigned long         ticks_past;
     char                  dummy[16];
     FILE                 *stat;

     stat = fopen( "/proc/stat", "r" );
     if (!stat)
          return 0;

     if (fscanf( stat, "%s %lu %lu %lu %lu %lu %lu %lu", dummy,
                 &new_u, &new_n, &new_s, &new_i, &new_wa, &new_hi, &new_si) < 8 ) {
          fclose( stat );
          return 0;
     }

     fclose( stat );

     ticks_past = (new_u + new_n + new_s + new_i + new_wa + new_hi + new_si) -
                  (old_u + old_n + old_s + old_i + old_wa + old_hi + old_si);

     if (ticks_past) {
          u = ((new_u - old_u) << 16) / ticks_past;
          n = ((new_n - old_n) << 16) / ticks_past;
          s = ((new_s - old_s) << 16) / ticks_past;
     }
     else {
          u = 0;
          n = 0;
          s = 0;
     }

     old_u  = new_u;
     old_n  = new_n;
     old_s  = new_s;
     old_i  = new_i;
     old_wa = new_wa;
     old_hi = new_hi;
     old_si = new_si;

     load = u + n + s;

     old_load = (load + load + load + old_load) >> 2;

     return old_load >> 10;
}

static void update( void )
{
     int load = get_load();

     surface->SetColor( surface, 0xff, 0xff, 0xff, 0x30 );
     surface->FillRectangle( surface, 63, 0, 1, 64 - load );

     surface->SetColor( surface, 0x00, 0x50, 0xD0, 0xcc );
     surface->FillRectangle( surface, 63, 64 - load, 1, load );

     surface->Blit( surface, surface, NULL, -1, 0 );

     surface->SetColor( surface, 0x00, 0x00, 0x00, 0x60 );
     surface->DrawRectangle( surface, 0, 0, 64, 64 );

     surface->Flip( surface, NULL, DSFLIP_NONE );
}

/**************************************************************************************************/

int main( int argc, char *argv[] )
{
     long long next_update = 0;

     /* Initialize application. */
     init_application( argc, argv );

     surface->Clear( surface, 0xff, 0xff, 0xff, 0x30 );

     window->SetOpacity( window, 0xff );

     /* Main loop. */
     while (1) {
          DFBWindowEvent event;
          long long      now = direct_clock_get_time( DIRECT_CLOCK_MONOTONIC ) / 1000;

          if (next_update <= now) {
               update();

               next_update = now + 100;
          }

          event_buffer->WaitForEventWithTimeout( event_buffer, 0, next_update - now );

          /* Check for new events. */
          while (event_buffer->GetEvent( event_buffer, DFB_EVENT(&event) ) == DFB_OK) {
               switch (event.type) {
                    default:
                         break;
               }
          }
     }

     /* Shouldn't reach this. */
     return 0;
}
