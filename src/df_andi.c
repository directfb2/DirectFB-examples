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

#include "util.h"

#ifdef USE_FONT_HEADERS
#include "decker.h"
#endif

#ifdef USE_IMAGE_HEADERS
#include "destination_mask.h"
#include "tux.h"
#include "tux_alpha.h"
#include "wood_andi.h"
#endif

/* DirectFB interfaces */
static IDirectFB             *dfb          = NULL;
static IDirectFBEventBuffer  *event_buffer = NULL;
static IDirectFBScreen       *screen       = NULL;
static IDirectFBDisplayLayer *layer        = NULL;
static IDirectFBSurface      *primary      = NULL;
static IDirectFBFont         *font         = NULL;

/* DirectFB surfaces */
static IDirectFBSurface *tuximage         = NULL;
static IDirectFBSurface *background       = NULL;
static IDirectFBSurface *destination_mask = NULL;

/* screen width and height */
static int xres, yres;

/* font information */
static int fontheight;
static int population_stringwidth;
static int fps_stringwidth;
static int idle_stringwidth;

/* resolution of the destination mask */
static int DESTINATION_MASK_WIDTH;
static int DESTINATION_MASK_HEIGHT;

/* width of the penguin surface (more that one penguin on it) */
#define XTUXSIZE 400

/* width and height of one sprite */
#define XSPRITESIZE 40
#define YSPRITESIZE 60

/* triple buffering */
static bool triple = false;

/* command line options */
static bool alpha        = false;
static bool do_print_fps = false;
static int  num_penguins = 200;

/* random function */
static unsigned int rand_pool = 0x12345678;
static unsigned int rand_add  = 0x87654321;

static inline unsigned int myrand( void )
{
     rand_pool ^= ((rand_pool << 7) | (rand_pool >> 25));
     rand_pool += rand_add;
     rand_add  += rand_pool;

     return rand_pool;
}

/* animation frame */
typedef struct _PenguinFrame {
     DFBRectangle          rect;
     struct _PenguinFrame *next;
} PenguinFrame;

/* linked lists of animation frames */
static PenguinFrame *left_frames;
static PenguinFrame *right_frames;
static PenguinFrame *up_frames;
static PenguinFrame *down_frames;

/* penguin struct, needed for each penguin on the screen */
typedef struct _Penguin {
     int               x;
     int               y;
     int               x_togo;
     int               y_togo;
     int               moving;
     int               delay;
     PenguinFrame     *left_frame;
     PenguinFrame     *right_frame;
     PenguinFrame     *up_frame;
     PenguinFrame     *down_frame;
     PenguinFrame    **frameset;
     struct _Penguin  *next;
} Penguin;

/* penguin linked list */
static Penguin *penguins = NULL;
static Penguin *last_penguin;

/* number of penguins currently on screen */
static int population = 0;

/* number of destination coordinates in coords array */
static int nr_coords = 0;

/* coords array has hardcoded maximum possible length */
static int *coords = NULL;

/*
 * adds one penguin to the list, and sets initial state
 */
static void spawn_penguin( void )
{
     Penguin *new_penguin = calloc( 1, sizeof(Penguin) );

     new_penguin->x           = xres / 2;
     new_penguin->y           = yres / 2;
     new_penguin->moving      = 1;
     new_penguin->delay       = 5;
     new_penguin->left_frame  = left_frames;
     new_penguin->right_frame = right_frames;
     new_penguin->up_frame    = up_frames;
     new_penguin->down_frame  = down_frames;
     new_penguin->frameset    = &new_penguin->down_frame;

     if (!penguins) {
          penguins = new_penguin;
          last_penguin = new_penguin;
     }
     else {
          last_penguin->next = new_penguin;
          last_penguin = new_penguin;
     }

     population++;
}

/*
 * adds a given number of penguins
 */
static void spawn_penguins( int number )
{
     while (number--)
          spawn_penguin();
}

/*
 * removes one penguin (the first) from the list
 */
static void destroy_penguin( void )
{
     Penguin *first_penguin = penguins;

     if (penguins) {
          penguins = penguins->next;
          free( first_penguin );
          population--;
     }
}

/*
 * removes a given number of penguins
 */
static void destroy_penguins( int number )
{
     while (number--)
          destroy_penguin();
}

/*
 * blits all penguins to the screen
 */
static void draw_penguins( void )
{
     int           j = population;
     Penguin      *p = penguins;
     DFBRectangle  rects[population < 256 ? population : 256];
     DFBPoint      points[population < 256 ? population : 256];

     primary->SetBlittingFlags( primary, alpha ? DSBLIT_BLEND_ALPHACHANNEL : DSBLIT_SRC_COLORKEY );

     while (j > 0) {
          int n = 0;

          while (n < j && n < 256) {
               rects[n] = (*(p->frameset))->rect;

               points[n].x = p->x;
               points[n].y = p->y;

               n++;
               p = p->next;
          }

          primary->BatchBlit( primary, tuximage, rects, points, n );

          j -= n;
     }
}

/*
 *  moves and clips all penguins, penguins that are in formation shiver, penguins not in formation walk
 */
static void move_penguins( int step )
{
     Penguin *p = penguins;

     while (p) {
          if (ABS( p->x_togo ) < step)
               p->x_togo = 0;

          if (ABS( p->y_togo ) < step)
               p->y_togo = 0;

          if (!p->x_togo && !p->y_togo) {
               if (p->moving) {
                    /* walking penguins get new destination if they have reached their destination */
                    p->x_togo = (myrand() % 101) - 50;
                    p->y_togo = (myrand() % 101) - 50;
               }
               else {
                    p->frameset = &p->down_frame;

                    /* penguins that have reached their formation point jitter */
                    p->x += myrand() % 3 - 1;
                    p->y += myrand() % 3 - 1;
               }
          }

          /* increase/decrease coordinates and to go variables */
          if (p->x_togo > 0) {
               p->x -= step;
               p->x_togo -= step;

               p->frameset = &p->left_frame;
          }
          if (p->x_togo < 0) {
               p->x += step;
               p->x_togo += step;

               p->frameset = &p->right_frame;
          }
          if (p->y_togo > 0) {
               p->y -= step;
               p->y_togo -= step;

               p->frameset = &p->up_frame;
          }
          if (p->y_togo < 0) {
               p->y += step;
               p->y_togo += step;

               p->frameset = &p->down_frame;
          }

          /* clip penguin */
          if (p->x < 0)
               p->x = 0;

          if (p->y < 0)
               p->y = 0;

          if (p->x > xres - XSPRITESIZE)
               p->x = xres - XSPRITESIZE;

          if (p->y > yres - YSPRITESIZE)
               p->y = yres - YSPRITESIZE;

          if (p->delay == 0) {
               *(p->frameset) = (*(p->frameset))->next;
               p->delay = 5;
          }
          else {
               p->delay--;
          }

          p = p->next;
     }
}

/*
 * searches a destination point in the coords array for each penguin
 */
static void penguins_search_destination( void )
{
     Penguin *p = penguins;

     if (nr_coords) {
          while (p) {
               int entry = (myrand() % nr_coords) * 2;

               p->x_togo = p->x - coords[entry]   * xres / 1000.0f;
               p->y_togo = p->y - coords[entry+1] * yres / 1000.0f;
               p->moving = 0;

               p = p->next;
          }
     }
}

/*
 * removes all penguins
 */
static void destroy_all_penguins( void )
{
     Penguin *p = penguins;

     while (p) {
          penguins = p->next;
          free( p );
          p = penguins;
     }
}

/*
 * revives all penguins, penguins that are in formation move again
 */
static void revive_penguins( void )
{
     Penguin *p = penguins;

     while (p) {
          p->moving = 1;
          p = p->next;
     }
}

/*
 * interprets the destination mask from the destination_mask surface
 * all back pixels become formation points, and are stored in the coords array
 */
static void read_destination_mask( void )
{
     int   x, y;
     int   pitch;
     void *ptr;

     coords = calloc( DESTINATION_MASK_WIDTH * DESTINATION_MASK_HEIGHT, sizeof(int) );

     DFBCHECK(destination_mask->Lock( destination_mask, DSLF_READ, &ptr, &pitch ));

     for (y = 0; y < DESTINATION_MASK_HEIGHT; y++) {
          u32 *src = ptr + y * pitch;

          for (x = 0; x < DESTINATION_MASK_WIDTH; x++) {
               if ((src[x] & 0x00FFFFFF) == 0) {
                    coords[nr_coords*2  ] = x * (1000 / DESTINATION_MASK_WIDTH);
                    coords[nr_coords*2+1] = y * (1000 / DESTINATION_MASK_HEIGHT);
                    nr_coords++;
               }
          }
     }

     DFBCHECK(destination_mask->Unlock( destination_mask ));
}

/*
 * initializes the animation frames for a specified direction at yoffset
 */
static void initialize_direction_frames( PenguinFrame **direction_frames, int yoffset )
{
     PenguinFrame *new_frame;
     PenguinFrame *last_frame = NULL;

     if (!*direction_frames) {
          int i;

          for (i = 0; i < (XTUXSIZE / XSPRITESIZE - 1) ; i++) {
               new_frame = malloc( sizeof(PenguinFrame) );

               new_frame->rect.x = XSPRITESIZE * i;
               new_frame->rect.y = YSPRITESIZE * yoffset;
               new_frame->rect.w = XSPRITESIZE;
               new_frame->rect.h = YSPRITESIZE;

               if (!*direction_frames) {
                    *direction_frames = new_frame;
               }
               else {
                    last_frame->next = new_frame;
               }

               last_frame = new_frame;
          }

          last_frame->next = *direction_frames;
     }
}

/*
 * initializes all animation frames
 */
static void initialize_animation( void )
{
     initialize_direction_frames( &down_frames,  0 );
     initialize_direction_frames( &left_frames,  1 );
     initialize_direction_frames( &up_frames,    2 );
     initialize_direction_frames( &right_frames, 3 );
}

/*
 * print usage
 */
static void print_usage( void )
{
     printf( "DirectFB Penguin Demo\n\n" );
     printf( "Usage: df_andi [options]\n\n" );
     printf( "Options:\n\n" );
     printf( "  --alpha           Use alpha channel for penguins instead of color keying.\n" );
     printf( "  --fps             Print frame rate every second on console.\n" );
     printf( "  --penguins <num>  Number of penguins (default = 200).\n" );
     printf( "  --help            Print usage information.\n" );
     printf( "  --dfb-help        Output DirectFB usage information.\n\n" );
}

/*
 * deinitializes resources and DirectFB
 */
static void deinit_resources( void )
{
     destroy_all_penguins();

     if (coords) free( coords );

     if (destination_mask) destination_mask->Release( destination_mask );
     if (background)       background->Release( background );
     if (tuximage)         tuximage->Release( tuximage );
     if (font)             font->Release( font );
     if (primary)          primary->Release( primary );
     if (layer)            layer->Release( layer );
     if (screen)           screen->Release( screen );
     if (event_buffer)     event_buffer->Release( event_buffer );
     if (dfb)              dfb->Release( dfb );
}

/*
 * set up DirectFB and load resources
 */
static void init_resources( int argc, char *argv[] )
{
     int                         n;
     DFBDisplayLayerConfig       config;
     DFBDisplayLayerConfigFlags  ret_failed;
     DFBFontDescription          fdsc;
     DFBSurfaceDescription       sdsc;
     DFBDataBufferDescription    ddsc;
     IDirectFBDataBuffer        *buffer;
     IDirectFBImageProvider     *provider;

     /* initialize DirectFB including command line parsing */
     DFBCHECK(DirectFBInit( &argc, &argv ));

     /* parse command line */
     for (n = 1; n < argc; n++) {
          if (strncmp( argv[n], "--", 2 ) == 0) {
               if (strcmp( argv[n] + 2, "help" ) == 0) {
                    print_usage();
                    exit( 0 );
               }
               else if (strcmp( argv[n] + 2, "alpha" ) == 0) {
                    alpha = true;
                    continue;
               }
               else if (strcmp( argv[n] + 2, "fps" ) == 0) {
                    do_print_fps = true;
                    continue;
               }
               else if (strcmp( argv[n] + 2, "penguins" ) == 0 && ++n < argc) {
                    num_penguins = atoi( argv[n] );
                    continue;
               }
          }

          print_usage();
          exit( 1 );
     }

     /* create the main interface */
     DFBCHECK(DirectFBCreate( &dfb ));

     /* register termination function */
     atexit( deinit_resources );

     /* set the cooperative level to DFSCL_FULLSCREEN for exclusive access to the primary layer */
     dfb->SetCooperativeLevel( dfb, DFSCL_FULLSCREEN );

     /* create an event buffer for key events */
     DFBCHECK(dfb->CreateInputEventBuffer( dfb, DICAPS_BUTTONS | DICAPS_KEYS, DFB_FALSE, &event_buffer ));

     /* get the primary screen */
     DFBCHECK(dfb->GetScreen( dfb, DSCID_PRIMARY, &screen ));

     /* check if triple buffering is supported */
     config.flags      = DLCONF_BUFFERMODE;
     config.buffermode = DLBM_TRIPLE;

     DFBCHECK(dfb->GetDisplayLayer( dfb, DLID_PRIMARY, &layer ));
     layer->TestConfiguration( layer, &config, &ret_failed );
     if (ret_failed == DLCONF_NONE)
          triple = true;

     /* get the primary surface, i.e. the surface of the primary layer */
     sdsc.flags = DSDESC_CAPS;
     sdsc.caps  = DSCAPS_PRIMARY | (triple ? DSCAPS_TRIPLE : DSCAPS_DOUBLE);

     DFBCHECK(dfb->CreateSurface( dfb, &sdsc, &primary ));

     DFBCHECK(primary->GetSize( primary, &xres, &yres ));

     /* load font */
     fdsc.flags  = DFDESC_HEIGHT;
     fdsc.height = CLAMP( (int) (xres / 40.0 / 8) * 8, 8, 96 );

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
     DFBCHECK(font->GetStringWidth( font, "Penguin Population: 0000  ", -1, &population_stringwidth ));
     DFBCHECK(font->GetStringWidth( font, "FPS: 0000.0  ", -1, &fps_stringwidth ));
     DFBCHECK(font->GetStringWidth( font, "CPU Idle: 00.0%  ", -1, &idle_stringwidth ));

     primary->SetFont( primary, font );

     /* load penguin animation */
#ifdef USE_IMAGE_HEADERS
     ddsc.flags         = DBDESC_MEMORY;
     ddsc.memory.data   = alpha ? GET_IMAGEDATA( tux_alpha ) : GET_IMAGEDATA( tux );
     ddsc.memory.length = alpha ? GET_IMAGESIZE( tux_alpha ) : GET_IMAGESIZE( tux );
#else
     ddsc.flags         = DBDESC_FILE;
     ddsc.file          = alpha ? GET_IMAGEFILE( tux_alpha ) : GET_IMAGEFILE( tux );
#endif
     DFBCHECK(dfb->CreateDataBuffer( dfb, &ddsc, &buffer ));
     DFBCHECK(buffer->CreateImageProvider( buffer, &provider ));
     provider->GetSurfaceDescription( provider, &sdsc );
     DFBCHECK(primary->GetPixelFormat( primary, &sdsc.pixelformat ));
     DFBCHECK(dfb->CreateSurface( dfb, &sdsc, &tuximage ));
     provider->RenderTo( provider, tuximage, NULL );
     provider->Release( provider );

     /* set the colorkey to green */
     if (!alpha) {
          DFBCHECK(tuximage->SetSrcColorKey( tuximage, 0x00, 0xFF, 0x00 ));
     }

     /* load the background image */
#ifdef USE_IMAGE_HEADERS
     ddsc.flags         = DBDESC_MEMORY;
     ddsc.memory.data   = GET_IMAGEDATA( wood_andi );
     ddsc.memory.length = GET_IMAGESIZE( wood_andi );
#else
     ddsc.flags         = DBDESC_FILE;
     ddsc.file          = GET_IMAGEFILE( wood_andi );
#endif
     DFBCHECK(dfb->CreateDataBuffer( dfb, &ddsc, &buffer ));
     DFBCHECK(buffer->CreateImageProvider( buffer, &provider ));
     provider->GetSurfaceDescription( provider, &sdsc );
     sdsc.width  = xres;
     sdsc.height = yres;
     DFBCHECK(primary->GetPixelFormat( primary, &sdsc.pixelformat ));
     DFBCHECK(dfb->CreateSurface( dfb, &sdsc, &background ));
     provider->RenderTo( provider, background, NULL );
     provider->Release( provider );

     /* load the penguin destination mask */
#ifdef USE_IMAGE_HEADERS
     ddsc.flags         = DBDESC_MEMORY;
     ddsc.memory.data   = GET_IMAGEDATA( destination_mask );
     ddsc.memory.length = GET_IMAGESIZE( destination_mask );
#else
     ddsc.flags         = DBDESC_FILE;
     ddsc.file          = GET_IMAGEFILE( destination_mask );
#endif
     DFBCHECK(dfb->CreateDataBuffer( dfb, &ddsc, &buffer ));
     DFBCHECK(buffer->CreateImageProvider( buffer, &provider ));
     provider->GetSurfaceDescription( provider, &sdsc );
     DFBCHECK(dfb->CreateSurface( dfb, &sdsc, &destination_mask ));
     provider->RenderTo( provider, destination_mask, NULL );
     provider->Release( provider );

     DESTINATION_MASK_WIDTH  = sdsc.width;
     DESTINATION_MASK_HEIGHT = sdsc.height;
     read_destination_mask();
}

int main( int argc, char *argv[] )
{
     FPSData            fps;
     IdleData           idle;
     int                clipping  = 0;
     DFBScreenPowerMode powerMode = DSPM_ON;
     int                quit      = 0;

     init_resources( argc, argv );

     initialize_animation();

     spawn_penguins( num_penguins );

     primary->SetDrawingFlags( primary, DSDRAW_BLEND );

     fps_init( &fps );
     idle_init( &idle );

     /* main loop */
     while (!quit) {
          DFBInputEvent evt;
          char          buf[32];

          primary->SetBlittingFlags( primary, DSBLIT_NOFX );

          /* draw the background image */
          primary->Blit( primary, background, NULL, 0, 0 );

          /* move the penguins 3 times, it's faster ;-) */
          move_penguins( 3 );

          /* draw all penguins */
          draw_penguins();

          /* draw the population string and fps in upper left corner */
          primary->SetColor( primary, 0, 0, 60, 0xA0 );
          primary->FillRectangle( primary, 20, 20, population_stringwidth + fps_stringwidth, fontheight + 5 );

          snprintf( buf, sizeof(buf), "Penguin Population: %d", population );

          primary->SetColor( primary, 180, 200, 255, 0xFF );
          primary->DrawString( primary, buf, -1, 25, 20, DSTF_TOPLEFT );

          snprintf( buf, sizeof(buf), "FPS: %s", fps.fps_string );

          primary->SetColor( primary, 190, 210, 255, 0xFF );
          primary->DrawString( primary, buf, -1, 25 + population_stringwidth, 20, DSTF_TOPLEFT );

          /* add idle information in upper right corner */
          primary->SetColor( primary, 0, 0, 60, 0xA0 );
          primary->FillRectangle( primary, xres - idle_stringwidth - 10, 20, idle_stringwidth, fontheight + 5 );

          snprintf( buf, sizeof(buf), "CPU Idle: %s%%", idle.idle_string );

          primary->SetColor( primary, 180, 200, 255, 0xFF );
          primary->DrawString( primary, buf, -1, xres - idle_stringwidth - 5, 20, DSTF_TOPLEFT );

          /* flip display */
          primary->Flip( primary, NULL, triple ? DSFLIP_ONSYNC : DSFLIP_WAITFORSYNC );

          /* fps calculations */
          fps_count( &fps, 1000 );
          idle_count( &idle, 1000 );

          if (do_print_fps && !fps.frames)
               printf( "%s\n", fps.fps_string );

          /* process event buffer */
          while (event_buffer->GetEvent( event_buffer, DFB_EVENT(&evt) ) == DFB_OK) {
               if (evt.buttons & DIBM_LEFT) {
                    if (event_buffer->WaitForEventWithTimeout( event_buffer, 2, 0 ) == DFB_TIMEOUT) {
                         /* quit main loop */
                         quit = 1;
                    }
               }
               else if (evt.type == DIET_KEYPRESS) {
                    switch (DFB_LOWER_CASE( evt.key_symbol )) {
                         case DIKS_ESCAPE:
                         case DIKS_SMALL_Q:
                         case DIKS_BACK:
                         case DIKS_STOP:
                         case DIKS_EXIT:
                              /* quit main loop */
                              quit = 1;
                              break;

                         case DIKS_SMALL_S:
                         case DIKS_CURSOR_UP:
                              /* add another penguins */
                              spawn_penguins( 10 );
                              break;

                         case DIKS_SMALL_R:
                              /* penguins in formation will walk around again */
                              revive_penguins();
                              break;

                         case DIKS_SMALL_D:
                         case DIKS_CURSOR_DOWN:
                              /* remove penguins */
                              destroy_penguins( 10 );
                              break;

                         case DIKS_SMALL_C:
                              /* toggle clipping */
                              clipping = !clipping;
                              if (clipping) {
                                   DFBRegion clipregion = { 100, 100, xres - 100, yres - 100 };
                                   primary->SetClip( primary, &clipregion );
                              }
                              else
                                   primary->SetClip( primary, NULL );
                              break;

                         case DIKS_SPACE:
                         case DIKS_ENTER:
                         case DIKS_OK:
                              /* penguins go in formation */
                              penguins_search_destination();
                              break;

                         case DIKS_SMALL_P:
                              powerMode = (powerMode == DSPM_ON) ? DSPM_OFF : DSPM_ON;
                              screen->SetPowerMode( screen, powerMode );
                              break;

                         default:
                              break;
                    }
               }
          }
     }

     return 42;
}
