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

/* macro for a safe call to DirectFB functions */
#define DFBCHECK(x...)                                                \
     do {                                                             \
          DFBResult err = x;                                          \
          if (err != DFB_OK) {                                        \
               fprintf( stderr, "%s <%d>:\n\t", __FILE__, __LINE__ ); \
               DirectFBErrorFatal( #x, err );                         \
          }                                                           \
     } while (0)

/* DirectFB interfaces */
static IDirectFB            *dfb       = NULL;
static IDirectFBEventBuffer *keybuffer = NULL;
static IDirectFBSurface     *primary   = NULL;

/* screen width and height */
static int sx, sy;

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

typedef struct _Particle {
     float             w;
     int               sw, sh;
     int               size;
     int               launch;
     struct _Particle *next;
} Particle;

static Particle *particles = NULL;
static Particle *last_particle;

static float f = 0;

static void spawn_particle()
{
     Particle *new_particle = malloc( sizeof(Particle) );

     new_particle->w      = 0.05f;
     new_particle->sw     = myrand() % (int) (sx / 3.2f) + sx / 3.2f * sin( f ) + sx / 3.2f;
     new_particle->sh     = myrand() % 100 + sy - 130;
     new_particle->size   = myrand() % (sx / 160) + 2;
     new_particle->launch = myrand() % (sx / 70);
     new_particle->next   = NULL;

     if (!particles) {
          particles = new_particle;
          last_particle = new_particle;
     }
     else {
          last_particle->next = new_particle;
          last_particle = new_particle;
     }
}

static void draw_particles()
{
     Particle *p = particles;

     while (p) {
          primary->SetColor( primary, 0xA0 + myrand() % 0x50, 0xA0 + myrand() % 0x50, 0xFF, 0x25 );
          primary->FillRectangle( primary, p->launch + sin( p->w / 2 ) * p->sw, sy - sin( p->w ) * p->sh,
                                  p->w * p->size + 1, p->w * p->size + 1 );

          p->w += M_PI / 500 * sqrt( p->w ) * sx / 640.0f;

          if (p->w > M_PI) {
               particles = p->next;
               free( p );
               p = particles;
               if (!p)
                    last_particle = NULL;
          }
          else {
               p = p->next;
          }
     }
}

static void destroy_particles()
{
     Particle *p = particles;

     while (p) {
          particles = p->next;
          free( p );
          p = particles;
     }
}

/******************************************************************************/

static void init_resources( int argc, char *argv[] )
{
     DFBSurfaceDescription dsc;

     /* initialize DirectFB including command line parsing */
     DFBCHECK(DirectFBInit( &argc, &argv ));

     /* create the main interface */
     DFBCHECK(DirectFBCreate( &dfb ));

     /* set the cooperative level to DFSCL_FULLSCREEN for exclusive access to the primary layer */
     dfb->SetCooperativeLevel( dfb, DFSCL_FULLSCREEN );

     /* create an event buffer for key events */
     DFBCHECK(dfb->CreateInputEventBuffer( dfb, DICAPS_KEYS, DFB_FALSE, &keybuffer ));

     /* get the primary surface, i.e. the surface of the primary layer */
     dsc.flags = DSDESC_CAPS;
     dsc.caps  = DSCAPS_PRIMARY | DSCAPS_DOUBLE;

     DFBCHECK(dfb->CreateSurface( dfb, &dsc, &primary ));

     DFBCHECK(primary->GetSize( primary, &sx, &sy ));

     /* clear with black */
     primary->Clear( primary, 0x00, 0x00, 0x00, 0xFF );
}

static void deinit_resources()
{
     destroy_particles();

     if (primary)   primary->Release( primary );
     if (keybuffer) keybuffer->Release( keybuffer );
     if (dfb)       dfb->Release( dfb );
}

int main( int argc, char *argv[] )
{
     int spawn;
     int left  = 0;
     int right = 0;
     int quit  = 0;

     init_resources( argc, argv );

     primary->SetDrawingFlags( primary, DSDRAW_BLEND );

     /* main loop */
     while (!quit) {
          DFBInputEvent evt;

          primary->SetColor( primary, 0, 0, 0, 0x17 );
          primary->FillRectangle( primary, 0, 0, sx, sy );

          if (!(myrand() % 50))
               left = !left;

          if (left)
               f -= 0.02f;

          if (f < -M_PI / 2)
               f = -M_PI / 2;

          if (!(myrand() % 50))
               right = !right;

          if (right)
               f += 0.02f;

          if (f > M_PI / 2)
               f = M_PI / 2;

          spawn = sx >> 7;
          while (spawn--)
               spawn_particle();

          draw_particles();

          /* flip display */
          primary->Flip( primary, NULL, DSFLIP_BLIT | DSFLIP_WAITFORSYNC );

          /* process event buffer */
          while (keybuffer->GetEvent( keybuffer, DFB_EVENT(&evt) ) == DFB_OK) {
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

                         default:
                              break;
                    }
               }
          }
     }

     deinit_resources();

     return 42;
}
