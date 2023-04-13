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

/* DirectFB surfaces */
#define NUM_STARS 4
static IDirectFBSurface *stars[NUM_STARS];

/* screen width and height */
static int xres, yres;

/******************************************************************************/

typedef struct {
     float v[4];
} Vector;

typedef struct {
     float m[16];
} Matrix;

typedef enum {
     X = 0,
     Y = 1,
     Z = 2,
     W = 3
} Vector_Elements;

typedef enum {
     X1 = 0,
     Y1 = 1,
     Z1 = 2,
     W1 = 3,
     X2 = 4,
     Y2 = 5,
     Z2 = 6,
     W2 = 7,
     X3 = 8,
     Y3 = 9,
     Z3 = 10,
     W3 = 11,
     X4 = 12,
     Y4 = 13,
     Z4 = 14,
     W4 = 15
} Matrix_Elements;

static const Matrix identity = { { 1, 0, 0, 0,
                                   0, 1, 0, 0,
                                   0, 0, 1, 0,
                                   0, 0, 0, 1 } };

static Matrix *matrix_new_identity()
{
     Matrix *matrix = malloc( sizeof(Matrix) );

     *matrix = identity;

     return matrix;
}

static Matrix *matrix_new_perspective( float d )
{
     Matrix *matrix = matrix_new_identity();

     matrix->m[Z4] = 1.0f / d;

     return matrix;
}

static void matrix_transform( Matrix *matrix, Vector *source, Vector *destination )
{
     destination->v[X] = matrix->m[X1] * source->v[X] +
                         matrix->m[Y1] * source->v[Y] +
                         matrix->m[Z1] * source->v[Z] +
                         matrix->m[W1] * source->v[W];

     destination->v[Y] = matrix->m[X2] * source->v[X] +
                         matrix->m[Y2] * source->v[Y] +
                         matrix->m[Z2] * source->v[Z] +
                         matrix->m[W2] * source->v[W];

     destination->v[Z] = matrix->m[X3] * source->v[X] +
                         matrix->m[Y3] * source->v[Y] +
                         matrix->m[Z3] * source->v[Z] +
                         matrix->m[W3] * source->v[W];

     destination->v[W] = matrix->m[X4] * source->v[X] +
                         matrix->m[Y4] * source->v[Y] +
                         matrix->m[Z4] * source->v[Z] +
                         matrix->m[W4] * source->v[W];
}

static void matrix_multiply( Matrix *destination, Matrix *source )
{
     float tmp[4];

     tmp[0] = source->m[X1] * destination->m[X1] +
              source->m[Y1] * destination->m[X2] +
              source->m[Z1] * destination->m[X3] +
              source->m[W1] * destination->m[X4];
     tmp[1] = source->m[X2] * destination->m[X1] +
              source->m[Y2] * destination->m[X2] +
              source->m[Z2] * destination->m[X3] +
              source->m[W2] * destination->m[X4];
     tmp[2] = source->m[X3] * destination->m[X1] +
              source->m[Y3] * destination->m[X2] +
              source->m[Z3] * destination->m[X3] +
              source->m[W3] * destination->m[X4];
     tmp[3] = source->m[X4] * destination->m[X1] +
              source->m[Y4] * destination->m[X2] +
              source->m[Z4] * destination->m[X3] +
              source->m[W4] * destination->m[X4];

     destination->m[X1] = tmp[0];
     destination->m[X2] = tmp[1];
     destination->m[X3] = tmp[2];
     destination->m[X4] = tmp[3];

     tmp[0] = source->m[X1] * destination->m[Y1] +
              source->m[Y1] * destination->m[Y2] +
              source->m[Z1] * destination->m[Y3] +
              source->m[W1] * destination->m[Y4];
     tmp[1] = source->m[X2] * destination->m[Y1] +
              source->m[Y2] * destination->m[Y2] +
              source->m[Z2] * destination->m[Y3] +
              source->m[W2] * destination->m[Y4];
     tmp[2] = source->m[X3] * destination->m[Y1] +
              source->m[Y3] * destination->m[Y2] +
              source->m[Z3] * destination->m[Y3] +
              source->m[W3] * destination->m[Y4];
     tmp[3] = source->m[X4] * destination->m[Y1] +
              source->m[Y4] * destination->m[Y2] +
              source->m[Z4] * destination->m[Y3] +
              source->m[W4] * destination->m[Y4];

     destination->m[Y1] = tmp[0];
     destination->m[Y2] = tmp[1];
     destination->m[Y3] = tmp[2];
     destination->m[Y4] = tmp[3];

     tmp[0] = source->m[X1] * destination->m[Z1] +
              source->m[Y1] * destination->m[Z2] +
              source->m[Z1] * destination->m[Z3] +
              source->m[W1] * destination->m[Z4];
     tmp[1] = source->m[X2] * destination->m[Z1] +
              source->m[Y2] * destination->m[Z2] +
              source->m[Z2] * destination->m[Z3] +
              source->m[W2] * destination->m[Z4];
     tmp[2] = source->m[X3] * destination->m[Z1] +
              source->m[Y3] * destination->m[Z2] +
              source->m[Z3] * destination->m[Z3] +
              source->m[W3] * destination->m[Z4];
     tmp[3] = source->m[X4] * destination->m[Z1] +
              source->m[Y4] * destination->m[Z2] +
              source->m[Z4] * destination->m[Z3] +
              source->m[W4] * destination->m[Z4];

     destination->m[Z1] = tmp[0];
     destination->m[Z2] = tmp[1];
     destination->m[Z3] = tmp[2];
     destination->m[Z4] = tmp[3];

     tmp[0] = source->m[X1] * destination->m[W1] +
              source->m[Y1] * destination->m[W2] +
              source->m[Z1] * destination->m[W3] +
              source->m[W1] * destination->m[W4];
     tmp[1] = source->m[X2] * destination->m[W1] +
              source->m[Y2] * destination->m[W2] +
              source->m[Z2] * destination->m[W3] +
              source->m[W2] * destination->m[W4];
     tmp[2] = source->m[X3] * destination->m[W1] +
              source->m[Y3] * destination->m[W2] +
              source->m[Z3] * destination->m[W3] +
              source->m[W3] * destination->m[W4];
     tmp[3] = source->m[X4] * destination->m[W1] +
              source->m[Y4] * destination->m[W2] +
              source->m[Z4] * destination->m[W3] +
              source->m[W4] * destination->m[W4];

     destination->m[W1] = tmp[0];
     destination->m[W2] = tmp[1];
     destination->m[W3] = tmp[2];
     destination->m[W4] = tmp[3];
}

static void matrix_translate( Matrix *matrix, float x, float y, float z )
{
     Matrix tmp = identity;

     tmp.m[W1] = x;
     tmp.m[W2] = y;
     tmp.m[W3] = z;

     matrix_multiply( matrix, &tmp );
}

static void matrix_rotate( Matrix *matrix, Vector_Elements axis, float angle )
{
     float  c   = cos( angle );
     float  s   = sin( angle );
     Matrix tmp = identity;

     switch (axis) {
          case X:
               tmp.m[Y2] =  c;
               tmp.m[Z2] = -s;
               tmp.m[Y3] =  s;
               tmp.m[Z3] =  c;
               break;
          case Y:
               tmp.m[X1] =  c;
               tmp.m[Z1] =  s;
               tmp.m[X3] = -s;
               tmp.m[Z3] =  c;
               break;
          case Z:
               tmp.m[X1] =  c;
               tmp.m[Y1] = -s;
               tmp.m[X2] =  s;
               tmp.m[Y2] =  c;
               break;
          default:
               break;
     }

     matrix_multiply( matrix, &tmp );
}

/******************************************************************************/

static Matrix *camera     = NULL;
static Matrix *projection = NULL;

typedef struct {
     Vector pos;
} Star;

#define STARFIELD_SIZE 5000
static Star starfield[STARFIELD_SIZE];
static Star t_starfield[STARFIELD_SIZE];

static DirectMutex render_start  = DIRECT_MUTEX_INITIALIZER();
static DirectMutex render_finish = DIRECT_MUTEX_INITIALIZER();

static void *render_loop( DirectThread *thread, void *arg )
{
     primary->SetBlittingFlags( primary, DSBLIT_SRC_COLORKEY | DSBLIT_COLORIZE );

     while (!direct_mutex_lock( &render_start )) {
          int i;

          direct_thread_testcancel( thread );

          primary->SetColor( primary, 0, 0, 0, 0 );
          primary->FillRectangle( primary, 0, 0, xres, yres );

          for (i = 0; i < STARFIELD_SIZE; i++) {
               int map = (int) (t_starfield[i].pos.v[Z]) >> 8;
               int light = 0xFF - ((int) (t_starfield[i].pos.v[Z] * t_starfield[i].pos.v[Z]) >> 12);

               if (map >= 0 && light > 0) {
                    if (map >= NUM_STARS)
                         map = NUM_STARS - 1;

                    primary->SetColor( primary, light, light, light, 0xFF );
                    primary->Blit( primary, stars[map], NULL, t_starfield[i].pos.v[X], t_starfield[i].pos.v[Y] );
               }
          }

          primary->Flip( primary, NULL, DSFLIP_WAITFORSYNC );

          direct_mutex_unlock( &render_finish );
     }

     return NULL;
}

static void load_stars()
{
     int                     i;
     DFBSurfaceDescription   dsc;
     char                    name[strlen( DATADIR"/star.dfiff" ) + 4];
     IDirectFBImageProvider *provider;

     for (i = 0; i < NUM_STARS; i++) {
          sprintf( name, DATADIR"/star%d.dfiff", i+1 );

          DFBCHECK(dfb->CreateImageProvider( dfb, name, &provider ));
          provider->GetSurfaceDescription( provider, &dsc );
          DFBCHECK(dfb->CreateSurface( dfb, &dsc, &stars[i] ));
          provider->RenderTo( provider, stars[i], NULL );
          provider->Release( provider );

          stars[i]->SetSrcColorKey( stars[i], 0xFF, 0x00, 0xFF );
     }
}

static void generate_starfield()
{
     int i;

     for (i = 0; i < STARFIELD_SIZE; i++) {
          starfield[i].pos.v[X] = rand() % 3001 - 1500;
          starfield[i].pos.v[Y] = rand() % 3001 - 1500;
          starfield[i].pos.v[Z] = rand() % 3001 - 1500;
          starfield[i].pos.v[W] = 1;
     }
}

static void unload_stars()
{
     int i;

     for (i = 0; i < NUM_STARS; i++)
          if (stars[i]) stars[i]->Release( stars[i] );
}

static void deinit_resources()
{
     if (projection) free( projection );
     if (camera)     free( camera );

     unload_stars();

     if (primary)      primary->Release( primary );
     if (event_buffer) event_buffer->Release( event_buffer );
     if (dfb)          dfb->Release( dfb );
}

static void init_resources( int argc, char *argv[] )
{
     DFBSurfaceDescription dsc;

     /* initialize stars */
     memset( stars, 0, sizeof(stars) );

     /* initialize DirectFB including command line parsing */
     DFBCHECK(DirectFBInit( &argc, &argv ));

     /* create the main interface */
     DFBCHECK(DirectFBCreate( &dfb ));

     /* register termination function */
     atexit( deinit_resources );

     /* set the cooperative level to DFSCL_FULLSCREEN for exclusive access to the primary layer */
     dfb->SetCooperativeLevel( dfb, DFSCL_FULLSCREEN );

     /* create an event buffer for axis and key events. */
     DFBCHECK(dfb->CreateInputEventBuffer( dfb, DICAPS_ALL, DFB_FALSE, &event_buffer ));

     /* get the primary surface, i.e. the surface of the primary layer */
     dsc.flags = DSDESC_CAPS;
     dsc.caps  = DSCAPS_PRIMARY | DSCAPS_DOUBLE;

     DFBCHECK(dfb->CreateSurface( dfb, &dsc, &primary ));

     DFBCHECK(primary->GetSize( primary, &xres, &yres ));

     /* load stars and generate starfield */
     load_stars();
     generate_starfield();

     camera     = matrix_new_identity();
     projection = matrix_new_perspective( 400 );
}

static void transform_starfield()
{
     int    i;
     Matrix m = *camera;

     matrix_multiply( &m, projection );

     for (i = 0; i < STARFIELD_SIZE; i++) {
          matrix_transform( &m, &starfield[i].pos, &t_starfield[i].pos );

          if (t_starfield[i].pos.v[W]) {
               t_starfield[i].pos.v[X] /= t_starfield[i].pos.v[W];
               t_starfield[i].pos.v[Y] /= t_starfield[i].pos.v[W];
          }

          t_starfield[i].pos.v[X] += xres / 2;
          t_starfield[i].pos.v[Y] += yres / 2;
     }
}

int main( int argc, char *argv[] )
{
     DirectThread *render_loop_thread;
     int           quit = 0;

     srand( time( NULL ) );

     init_resources( argc, argv );

     direct_mutex_lock( &render_start );
     direct_mutex_lock( &render_finish );
     render_loop_thread = direct_thread_create( DTT_DEFAULT, render_loop, NULL, "Starfield Render" );

     /* main loop */
     while (!quit) {
          static float  translation[3] = { 0, 0, 0 };
          DFBInputEvent evt;

          /* transform world to screen coordinates */
          transform_starfield();

          /* start rendering before waiting for events */
          direct_mutex_unlock( &render_start );

          event_buffer->WaitForEvent( event_buffer );

          /* process event buffer */
          while (event_buffer->GetEvent( event_buffer, DFB_EVENT(&evt) ) == DFB_OK) {
               if (evt.buttons & DIBM_LEFT) {
                    if (event_buffer->WaitForEventWithTimeout( event_buffer, 2, 0 ) == DFB_TIMEOUT) {
                         /* quit main loop */
                         quit = 1;
                    }
               }
               else if (evt.type == DIET_KEYPRESS) {
                    switch (evt.key_id) {
                         case DIKI_ESCAPE:
                         case DIKI_Q:
                              /* quit main loop */
                              quit = 1;
                              break;

                         case DIKI_LEFT:
                              translation[0] =  10;
                              break;

                         case DIKI_RIGHT:
                              translation[0] = -10;
                              break;

                         case DIKI_UP:
                              translation[2] = -10;
                              break;

                         case DIKI_DOWN:
                              translation[2] =  10;
                              break;

                         default:
                              break;
                    }
               }
               else if (evt.type == DIET_KEYRELEASE) {
                    switch (evt.key_id) {
                         case DIKI_LEFT:
                         case DIKI_RIGHT:
                              translation[0] = 0;
                              break;

                         case DIKI_UP:
                         case DIKI_DOWN:
                              translation[2] = 0;
                              break;

                         default:
                              break;
                    }
               }
               else if (evt.type == DIET_AXISMOTION && (evt.flags & DIEF_AXISREL)) {
                    switch (evt.axis) {
                         case DIAI_X:
                              matrix_rotate( camera, Y, -evt.axisrel / 80.0f );
                              break;

                         case DIAI_Y:
                              matrix_rotate( camera, X,  evt.axisrel / 80.0f );
                              break;

                         case DIAI_Z:
                              matrix_rotate( camera, Z,  evt.axisrel / 8.0f );
                              break;

                         default:
                              break;
                    }
               }
          }

          matrix_translate( camera, translation[0], translation[1], translation[2] );

          /* finish rendering before retransforming the world */
          direct_mutex_lock( &render_finish );
     }

     direct_thread_cancel( render_loop_thread );
     direct_mutex_unlock( &render_start );
     direct_thread_join( render_loop_thread );
     direct_thread_destroy( render_loop_thread );

     return 42;
}
