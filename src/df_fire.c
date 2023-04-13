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

/******************************************************************************/

/* DirectFB interfaces */
static IDirectFB            *dfb          = NULL;
static IDirectFBEventBuffer *event_buffer = NULL;
static IDirectFBSurface     *surface      = NULL;

/* screen width and fire height (max 256) */
static int width, height;

/* how much of the screen height to skip */
static int skip;

/* fire data */
static u8 *data = NULL;

/******************************************************************************/

static void init_application( int argc, char *argv[] );
static void exit_application( int status );

/******************************************************************************/

static void render_fire();

/******************************************************************************/

static unsigned int rand_pool = 0x12345678;
static unsigned int rand_add  = 0x87654321;

static inline unsigned int myrand()
{
     rand_pool ^= ((rand_pool << 7) | (rand_pool >> 25));
     rand_pool += rand_add;
     rand_add  += rand_pool;

     return rand_pool;
}

/******************************************************************************/

int main( int argc, char *argv[] )
{
     /* Initialize application. */
     init_application( argc, argv );

     /* Main loop. */
     while (1) {
          DFBInputEvent evt;

          /* Render and display the next frame. */
          render_fire();

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

/******************************************************************************/

static void generate_palette()
{
     DFBResult         ret;
     int               i;
     DFBColor          colors[256];
     IDirectFBPalette *palette;

     /* Get access to the palette. */
     ret = surface->GetPalette( surface, &palette );
     if (ret) {
          DirectFBError( "GetPalette() failed", ret );
          exit_application( 8 );
     }

     /* Calculate RGB values. */
     for (i = 0; i < 48; i++) {
          colors[47-i].r = ((48 * 48 * 48 - 1) - (i * i * i)) / (48 * 48 / 4);
          colors[i].g = 0;
          colors[i].b = 0;
     }

     for (i = 0; i < 104; i++) {
          colors[i+48].r = 192;
          colors[i+48].g = i * 24 / 13;
          colors[i+48].b = 0;
     }

     for (i = 0; i < 104; i++) {
          colors[i+152].r = 192;
          colors[i+152].g = 192;
          colors[i+152].b = i * 24 / 13;
     }

     /* Calculate alpha values. */
     for (i = 0; i < 256; i++)
          colors[255-i].a = ~(i * i * i * i) >> 24;

     /* Set new palette data. */
     ret = palette->SetEntries( palette, colors, 256, 0 );
     if (ret) {
          palette->Release( palette );
          DirectFBError( "SetEntries() failed", ret );
          exit_application( 9 );
     }

     /* Release the palette. */
     palette->Release( palette );
}

static void fade_out_palette()
{
     DFBResult         ret;
     DFBColor          colors[256];
     IDirectFBPalette *palette;
     int               i, fade;

     /* Get access to the palette. */
     ret = surface->GetPalette( surface, &palette );
     if (ret) {
          DirectFBError( "GetPalette() failed", ret );
          return;
     }

     /* Get palette data. */
     ret = palette->GetEntries( palette, colors, 256, 0 );
     if (ret) {
          palette->Release( palette );
          DirectFBError( "SetEntries() failed", ret );
          return;
     }

     /* Fade out. */
     do {
          fade = 0;

          /* Calculate new palette entries. */
          for (i = 0; i < 256; i++) {
               if (colors[i].r || colors[i].g || colors[i].b)
                    fade = 1;

               if (colors[i].r)
                    colors[i].r -= (colors[i].r >> 4) + 1;

               if (colors[i].g)
                    colors[i].g -= (colors[i].g >> 4) + 1;

               if (colors[i].b)
                    colors[i].b -= (colors[i].b >> 4) + 1;
          }

          /* Set new palette data. */
          ret = palette->SetEntries( palette, colors, 256, 0 );
          if (ret) {
               palette->Release( palette );
               DirectFBError( "SetEntries() failed", ret );
               return;
          }

          /* Wait for vertical retrace. */
          dfb->WaitForSync( dfb );

          /* Flip the surface to display the new frame. */
          surface->Flip( surface, NULL, DSFLIP_NONE );
     } while (fade);

     /* Release the palette. */
     palette->Release( palette );
}

static void render_fire()
{
     DFBResult  ret;
     int        i;
     int        surface_pitch;
     void      *surface_data;
     u8        *fire_data   = data;
     int        fire_height = height;

     /* Loop through all lines. */
     while (fire_height--) {
          u8 *d = fire_data + 1;
          u8 *s = fire_data + width;

          /* Loop through all the columns except the first and last. */
          for (i = 0; i < width - 2; i++) {
               int val;

               /* Calculate the average of the current pixel and three below. */
               val = (d[i] + s[i] + s[i+1] + s[i+2]) >> 2;

               /* Add some randomness. */
               if (val)
                    val += (myrand() % 3) - 1;

               /* Write back with overflow checking. */
               d[i] = (val > 0xff) ? 0xff : val;
          }

          /* Increase fire data pointer to the next line. */
          fire_data += width;
     }

     /* Put some flammable stuff into the additional line. */
     memset( fire_data, 0x20, width );
     for (i = 0; i < width / 2; i++)
          fire_data[myrand()%width] = 0xff;

     /* Lock the surface's data for direct write access. */
     ret = surface->Lock( surface, DSLF_WRITE, &surface_data, &surface_pitch );
     if (ret) {
          DirectFBError( "Lock() failed", ret );
          exit_application( 6 );
     }

     /* Add skip offset. */
     surface_data += surface_pitch * skip;

     /* Write fire data to the surface. */
     for (i = 0; i < height; i++) {
          /* Copy one line to the surface. */
          memcpy( surface_data, data + i * width, width );

          /* Increase surface data pointer to the next line. */
          surface_data += surface_pitch;
     }

     /* Unlock the surface's data. */
     ret = surface->Unlock( surface );
     if (ret) {
          DirectFBError( "Unlock() failed", ret );
          exit_application( 7 );
     }

     /* Flip the surface to display the new frame. */
     surface->Flip( surface, NULL, DSFLIP_NONE );
}

/******************************************************************************/

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

     /* Create an event buffer for key events. */
     ret = dfb->CreateInputEventBuffer( dfb, DICAPS_BUTTONS | DICAPS_KEYS, DFB_FALSE, &event_buffer );
     if (ret) {
          DirectFBError( "CreateInputEventBuffer() failed", ret );
          exit_application( 3 );
     }

     /* Fill the surface description. */
     desc.flags       = DSDESC_CAPS | DSDESC_PIXELFORMAT;
     desc.caps        = DSCAPS_PRIMARY | DSCAPS_FLIPPING;
     desc.pixelformat = DSPF_LUT8;

     /* Get the primary surface, i.e. the surface of the primary layer. */
     ret = dfb->CreateSurface( dfb, &desc, &surface );
     if (ret) {
          DirectFBError( "CreateSurface() failed", ret );
          exit_application( 4 );
     }

     /* Query the size of the primary surface. */
     surface->GetSize( surface, &width, &height );

     /* Calculate skip. */
     skip = height - 256;
     if (skip < 0)
          skip = 0;

     /* Calculate fire height. */
     if (height > 256)
          height = 256;

     /* Allocate fire data including an additional line. */
     data = calloc( height + 1, width );

     /* Generate the fire palette. */
     generate_palette();

     /* Clear with black. */
     surface->Clear( surface, 0x00, 0x00, 0x00, 0xff );
     surface->Flip( surface, NULL, DSFLIP_NONE );
     surface->Clear( surface, 0x00, 0x00, 0x00, 0xff );
     surface->Flip( surface, NULL, DSFLIP_NONE );
     surface->Clear( surface, 0x00, 0x00, 0x00, 0xff );
     surface->Flip( surface, NULL, DSFLIP_NONE );
}

static void exit_application( int status )
{
     /* Fade screen to black. */
     if (surface)
          fade_out_palette( surface );

     /* Deallocate fire data. */
     if (data)
          free( data );

     /* Release the primary surface. */
     if (surface)
          surface->Release( surface );

     /* Release the event buffer. */
     if (event_buffer)
          event_buffer->Release( event_buffer );

     /* Release the main interface. */
     if (dfb)
          dfb->Release( dfb );

     /* Terminate application. */
     exit( status );
}
