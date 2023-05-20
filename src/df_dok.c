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
#include <directfb_strings.h>
#include <directfb_util.h>

#include "util.h"

#ifdef USE_FONT_HEADERS
#include "decker.h"
#endif

#ifdef USE_IMAGE_HEADERS
#include "biglogo.h"
#include "card.h"
#include "colorkeyed.h"
#include "fish.h"
#include "intro.h"
#include "laden_bike.h"
#include "melted.h"
#include "meter.h"
#include "rose.h"
#include "sacred_heart.h"
#include "swirl.h"
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

/* main interface */
static IDirectFB *dfb = NULL;

/* event buffer interface */
static IDirectFBEventBuffer *event_buffer = NULL;

/* primary surface */
static IDirectFBSurface *primary = NULL;

/* fonts */
static IDirectFBFont *bench_font = NULL;
static IDirectFBFont *ui_font    = NULL;

/* logo for start screen */
static IDirectFBSurface *logo = NULL;

/* card icon for acceleration indicator */
static IDirectFBSurface *cardicon = NULL;

/* test images */
static IDirectFBSurface *swirl      = NULL;
static IDirectFBSurface *rose       = NULL;
static IDirectFBSurface *rose_pre   = NULL;
static IDirectFBSurface *simple     = NULL;
static IDirectFBSurface *colorkeyed = NULL;
static IDirectFBSurface *image32    = NULL;
static IDirectFBSurface *image32a   = NULL;
static IDirectFBSurface *image8a    = NULL;

/* "Press any key to proceed..." intro screen */
static IDirectFBSurface *intro = NULL;

/* primary subsurface */
static IDirectFBSurface *dest = NULL;

/* screen width and height (possibly less the height of the status bar) */
static int SW, SH;

/* font information */
static int bench_stringwidth;
static int bench_fontheight;
static int ui_fontheight;

/* command line options */
static int                    DEMOTIME       = 3000; /* milliseconds */
static int                    ITERATIONS     = 1;
static int                    SX             = 256;
static int                    SY             = 256;
static DFBSurfacePixelFormat  pixelformat    = DSPF_UNKNOWN;
static int                    do_system      = 0;
static int                    do_dump        = 0;
static int                    do_wait        = 0;
static int                    do_noaccel     = 0;
static int                    accel_only     = 0;
static int                    do_smooth      = 0;
static int                    do_aa          = 0;
static int                    do_xor         = 0;
static int                    do_matrix      = 0;
static int                    show_results   = 1;
static int                    do_all_demos   = 0;
static int                    output_csv     = 0;
static int                    run_fullscreen = 0;
static int                    with_intro     = 0;
static const char            *filename       = NULL;

/* benchmarks */
static unsigned long long draw_string            ( long long t );
static unsigned long long draw_string_blend      ( long long t );
static unsigned long long fill_rect              ( long long t );
static unsigned long long fill_rect_blend        ( long long t );
static unsigned long long fill_rects             ( long long t );
static unsigned long long fill_rects_blend       ( long long t );
static unsigned long long fill_triangle          ( long long t );
static unsigned long long fill_triangle_blend    ( long long t );
static unsigned long long draw_rect              ( long long t );
static unsigned long long draw_rect_blend        ( long long t );
static unsigned long long draw_lines             ( long long t );
static unsigned long long draw_lines_blend       ( long long t );
static unsigned long long fill_spans             ( long long t );
static unsigned long long fill_spans_blend       ( long long t );
static unsigned long long fill_traps             ( long long t );
static unsigned long long blit                   ( long long t );
static unsigned long long blit180                ( long long t );
static unsigned long long blit_colorkeyed        ( long long t );
static unsigned long long blit_dst_colorkeyed    ( long long t );
static unsigned long long blit_convert           ( long long t );
static unsigned long long blit_colorize          ( long long t );
static unsigned long long blit_mask              ( long long t );
static unsigned long long blit_blend             ( long long t );
static unsigned long long blit_blend_colorize    ( long long t );
static unsigned long long blit_srcover           ( long long t );
static unsigned long long blit_srcover_pre       ( long long t );
static unsigned long long stretch_blit           ( long long t );
static unsigned long long stretch_blit_colorkeyed( long long t );
static unsigned long long load_image             ( long long t );

typedef struct {
     char                 desc[128];
     char                *message;
     char                *status;
     char                *option;
     bool                 default_on;
     int                  requested;
     long                 result;
     DFBBoolean           accelerated;
     char                *unit;
     unsigned long long (*func)( long long );
     int                  load;
     long                 duration;
} Demo;

static Demo demos[] = {
     { "Anti-aliased Text",
       "This is the DirectFB benchmarking tool, let's start with some text!",
       "Anti-aliased Text", "draw-string", true,
       0, 0, 0, "KChars/sec",  draw_string },
     { "Anti-aliased Text (blend)",
       "Alpha blending based on color alpha",
       "Alpha Blended Anti-aliased Text", "draw-string-blend", true,
       0, 0, 0, "KChars/sec",  draw_string_blend },
     { "Fill Rectangle",
       "Ok, we'll go on with some opaque filled rectangles!",
       "Rectangle Filling", "fill-rect", true,
       0, 0, 0, "MPixel/sec", fill_rect },
     { "Fill Rectangle (blend)",
       "What about alpha blended rectangles?",
       "Alpha Blended Rectangle Filling", "fill-rect-blend", true,
       0, 0, 0, "MPixel/sec", fill_rect_blend },
     { "Fill Rectangles [10]",
       "Ok, we'll go on with some opaque filled rectangles!",
       "Rectangle Filling", "fill-rects", true,
       0, 0, 0, "MPixel/sec", fill_rects },
     { "Fill Rectangles [10] (blend)",
       "What about alpha blended rectangles?",
       "Alpha Blended Rectangle Filling", "fill-rects-blend", true,
       0, 0, 0, "MPixel/sec", fill_rects_blend },
     { "Fill Triangles",
       "Ok, we'll go on with some opaque filled triangles!",
       "Triangle Filling", "fill-triangle", true,
       0, 0, 0, "MPixel/sec", fill_triangle },
     { "Fill Triangles (blend)",
       "What about alpha blended triangles?",
       "Alpha Blended Triangle Filling", "fill-triangle-blend", true,
       0, 0, 0, "MPixel/sec", fill_triangle_blend },
     { "Draw Rectangle",
       "Now pass over to non filled rectangles!",
       "Rectangle Outlines", "draw-rect", true,
       0, 0, 0, "KRects/sec", draw_rect },
     { "Draw Rectangle (blend)",
       "Again, we want it with alpha blending!",
       "Alpha Blended Rectangle Outlines", "draw-rect-blend", true,
       0, 0, 0, "KRects/sec", draw_rect_blend },
     { "Draw Lines [10]",
       "Can we have some opaque lines, please?",
       "Line Drawing", "draw-line", true,
       0, 0, 0, "KLines/sec", draw_lines },
     { "Draw Lines [10] (blend)",
       "So what? Where's the blending?",
       "Alpha Blended Line Drawing", "draw-line-blend", true,
       0, 0, 0, "KLines/sec", draw_lines_blend },
     { "Fill Spans",
       "Can we have some spans, please?",
       "Span Filling", "fill-span", true,
       0, 0, 0, "MPixel/sec", fill_spans },
     { "Fill Spans (blend)",
       "So what? Where's the blending?",
       "Alpha Blended Span Filling", "fill-span-blend", true,
       0, 0, 0, "MPixel/sec", fill_spans_blend },
     { "Fill Trapezoids [10]",
       "Can we have some Trapezoids, please?",
       "Trapezoid Filling", "fill-traps", true,
       0, 0, 0, "MPixel/sec", fill_traps },
     { "Blit",
       "Now lead to some blitting demos! The simplest one comes first...",
       "Simple BitBlt", "blit", true,
       0, 0, 0, "MPixel/sec", blit },
     { "Blit 180",
       "Rotation?",
       "Rotated BitBlt", "blit180", true,
       0, 0, 0, "MPixel/sec", blit180 },
     { "Blit colorkeyed",
       "Color keying would be nice...",
       "BitBlt with Color Keying", "blit-colorkeyed", true,
       0, 0, 0, "MPixel/sec", blit_colorkeyed },
     { "Blit destination colorkeyed",
       "Destination color keying is also possible...",
       "BitBlt with Destination Color Keying", "blit-dst-colorkeyed", false,
       0, 0, 0, "MPixel/sec", blit_dst_colorkeyed },
     { "Blit with format conversion",
       "What if the source surface has another format?",
       "BitBlt with on-the-fly format conversion", "blit-convert", true,
       0, 0, 0, "MPixel/sec", blit_convert },
     { "Blit with colorizing",
       "How does colorizing look like?",
       "BitBlt with colorizing", "blit-colorize", true,
       0, 0, 0, "MPixel/sec", blit_colorize },
     { "Blit with mask",
       "How do masks look like?",
       "BitBlt with mask", "blit-mask", false,
       0, 0, 0, "MPixel/sec", blit_mask },
     { "Blit from 32bit (blend)",
       "Here we go with alpha again!",
       "BitBlt with Alpha Channel", "blit-blend", true,
       0, 0, 0, "MPixel/sec", blit_blend },
     { "Blit from 32bit (blend) with colorizing",
       "Here we go with colorized alpha!",
       "BitBlt with Alpha Channel & Colorizing", "blit-blend-colorize", true,
       0, 0, 0, "MPixel/sec", blit_blend_colorize },
     { "Blit SrcOver (premultiplied source)",
       "With alpha blending based on alpha entries",
       "BitBlt SrcOver", "blit-srcover", true,
       0, 0, 0, "MPixel/sec", blit_srcover },
     { "Blit SrcOver (premultiply source)",
       "With alpha blending based on alpha entries",
       "BitBlt SrcOver premultiply", "blit-srcover-pre", true,
       0, 0, 0, "MPixel/sec", blit_srcover_pre },
     { "Stretch Blit",
       "Stretching!",
       "Stretch Blit", "stretch-blit", true,
       0, 0, 0, "MPixel/sec", stretch_blit },
     { "Stretch Blit colorkeyed",
       "Stretching with color keying!",
       "Stretch Blit with color keying", "stretch-blit-colorkeyed", true,
       0, 0, 0, "MPixel/sec", stretch_blit_colorkeyed },
     { "Load Image",
       "Loading image files!",
       "Loading image files", "load-image <filename>", false,
       0, 0, 0, "MPixel/sec", load_image },
};

static Demo *current_demo;

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

/**********************************************************************************************************************/

static const DirectFBPixelFormatNames(format_names)

static DFBSurfacePixelFormat parse_pixelformat( const char *format )
{
     int i;

     for (i = 0; i < D_ARRAY_SIZE(format_names); i++) {
          if (!strcmp( format, format_names[i].name ))
               return format_names[i].format;
     }

     return DSPF_UNKNOWN;
}

static void print_usage( void )
{
     int i;

     printf( "DirectFB Benchmarking Demo\n\n" );
     printf( "Usage: df_dok [options]\n\n" );
     printf( "Options:\n\n" );
     printf( "  --duration <milliseconds>    Duration of each benchmark.\n" );
     printf( "  --iterations <num>           Number of iterations for each benchmark.\n" );
     printf( "  --size <width>x<height>      Set benchmark size.\n" );
     printf( "  --pixelformat <pixelformat>  Set benchmark pixelformat.\n" );
     printf( "  --system                     Do benchmarks in system memory.\n" );
     printf( "  --dump                       Dump output of each benchmark to a file.\n" );
     printf( "  --wait <seconds>             Wait a few seconds after each benchmark.\n" );
     printf( "  --noaccel                    Don't use hardware acceleration.\n" );
     printf( "  --accelonly                  Only show accelerated benchmarks.\n" );
     printf( "  --smooth                     Enable smooth up/down scaling option.\n" );
     printf( "  --aa                         Turn on anti-aliasing for all benchmarks.\n" );
     printf( "  --matrix                     Set a matrix transformation on all benchmarks.\n" );
     printf( "  --xor                        Use XOR raster operation in benchmarks.\n" );
     printf( "  --noresults                  Don't show results screen.\n" );
     printf( "  --all-demos                  Run all benchmarks.\n" );
     printf( "  --csv                        Output comma separated values.\n" );
     printf( "  --fullscreen                 Run fullscreen (without status bar).\n" );
     printf( "  --intro                      Display intro screen before each benchmark.\n" );
     printf( "  --help                       Print usage information.\n" );
     printf( "  --dfb-help                   Output DirectFB usage information.\n\n" );
     printf( "The following options allow to specify which benchmarks to run.\n" );
     printf( "If none of these are given, all benchmarks requested by default are run.\n\n" );
     for (i = 0; i < D_ARRAY_SIZE(demos); i++) {
          printf( "  --%-26s %s\n", demos[i].option, demos[i].desc );
     }
     printf( "\n" );
}

static void dfb_shutdown( void )
{
     if (dest)                dest->Release( dest );
     if (with_intro && intro) intro->Release( intro );
     if (image8a)             image8a->Release( image8a );
     if (image32a)            image32a->Release( image32a );
     if (image32)             image32->Release( image32 );
     if (colorkeyed)          colorkeyed->Release( colorkeyed );
     if (simple)              simple->Release( simple );
     if (rose_pre)            rose_pre->Release( rose_pre );
     if (rose)                rose->Release( rose );
     if (swirl)               swirl->Release( swirl );
     if (cardicon)            cardicon->Release( cardicon );
     if (logo)                logo->Release( logo );
     if (ui_font)             ui_font->Release( ui_font );
     if (bench_font)          bench_font->Release( bench_font );
     if (primary)             primary->Release( primary );
     if (event_buffer)        event_buffer->Release( event_buffer );
     if (dfb)                 dfb->Release( dfb );
}

static void showMessage( const char *msg )
{
     DFBInputEvent evt;

     /* process event buffer */
     while (event_buffer->GetEvent( event_buffer, DFB_EVENT(&evt) ) == DFB_OK) {
          if (evt.buttons & DIBM_LEFT) {
               if (event_buffer->WaitForEventWithTimeout( event_buffer, 2, 0 ) == DFB_TIMEOUT) {
                    exit( 42 );
               }
          }
          else if (evt.type == DIET_KEYPRESS) {
               switch (evt.key_symbol) {
                    case DIKS_ESCAPE:
                    case DIKS_SMALL_Q:
                    case DIKS_CAPITAL_Q:
                    case DIKS_BACK:
                    case DIKS_STOP:
                    case DIKS_EXIT:
                         exit( 42 );
                         break;
                    default:
                         break;
               }
          }
     }

     if (with_intro) {
          primary->SetBlittingFlags( primary, DSBLIT_NOFX );
          primary->Blit( primary, intro, NULL, 0, 0 );

          primary->SetDrawingFlags( primary, DSDRAW_NOFX );
          primary->SetColor( primary, 0xFF, 0xFF, 0xFF, 0xFF );
          primary->DrawString( primary, msg, -1, SW / 2, SH / 2, DSTF_CENTER );

          event_buffer->Reset( event_buffer );
          event_buffer->WaitForEvent( event_buffer );
     }

     primary->Clear( primary, 0, 0, 0, 0x80 );
}

static void showResult( void )
{
     DFBRectangle              rect;
     DFBSurfaceDescription     sdsc;
     DFBDataBufferDescription  ddsc;
     IDirectFBDataBuffer      *buffer;
     IDirectFBImageProvider   *provider;
     IDirectFBSurface         *meter;
     int                       i, y, w, h, max_string_width = 0;
     char                      rate[32];
     long                      max_result = 0;
     double                    factor;

     for (i = 0; i < D_ARRAY_SIZE(demos); i++) {
          if (!demos[i].requested || !demos[i].result)
               continue;

          if (max_result < demos[i].result)
               max_result = demos[i].result;
     }

     factor = (double) (SW - 60) / max_result;

#ifdef USE_IMAGE_HEADERS
     ddsc.flags         = DBDESC_MEMORY;
     ddsc.memory.data   = GET_IMAGEDATA( meter );
     ddsc.memory.length = GET_IMAGESIZE( meter );
#else
     ddsc.flags         = DBDESC_FILE;
     ddsc.file          = GET_IMAGEFILE( meter );
#endif
     DFBCHECK(dfb->CreateDataBuffer( dfb, &ddsc, &buffer ));
     DFBCHECK(buffer->CreateImageProvider( buffer, &provider ));
     provider->GetSurfaceDescription( provider, &sdsc );
     sdsc.height = 8;
     DFBCHECK(dfb->CreateSurface( dfb, &sdsc, &meter ));
     provider->RenderTo( provider, meter, NULL );
     provider->Release( provider );

     primary->Clear( primary, 0, 0, 0, 0x80 );

     primary->SetBlittingFlags( primary, DSBLIT_NOFX );
     primary->SetDrawingFlags( primary, DSDRAW_NOFX );

     rect.x = 40;
     rect.y = ui_fontheight;
     rect.h = sdsc.height;

     primary->SetColor( primary, 0x66, 0x66, 0x66, 0xFF );

     for (i = 0; i < D_ARRAY_SIZE(demos); i++) {
          if (!demos[i].requested || !demos[i].result)
               continue;

          rect.w = demos[i].result * factor;
          primary->StretchBlit( primary, meter, NULL, &rect );
          if (rect.w < SW - 60)
               primary->DrawLine( primary, 40 + rect.w, rect.y + sdsc.height, SW - 20, rect.y + sdsc.height );

          rect.y += sdsc.height / 2 + ui_fontheight + 2;
     }

     meter->Release( meter );

     y = ui_fontheight + sdsc.height / 2;
     for (i = 0; i < D_ARRAY_SIZE(demos); i++) {
          if (!demos[i].requested || !demos[i].result)
               continue;

          primary->SetColor( primary, 0xCC, 0xCC, 0xCC, 0xFF );
          primary->DrawString( primary, demos[i].desc, -1, 20, y, DSTF_BOTTOMLEFT );

          snprintf( rate, sizeof(rate), "%2ld.%.3ld %s", demos[i].result / 1000, demos[i].result % 1000, demos[i].unit );

          DFBCHECK(ui_font->GetStringExtents( ui_font, rate, -1, NULL, &rect ));

          if (max_string_width < rect.w)
               max_string_width = rect.w;

          primary->SetColor( primary, 0xAA, 0xAA, 0xAA, 0xFF );
          primary->DrawString( primary, rate, -1, SW - 20, y, DSTF_BOTTOMRIGHT );

          y += sdsc.height / 2 + ui_fontheight + 2;
     }

     /* retrieve width and height */
     DFBCHECK(cardicon->GetSize( cardicon, &w, &h ));

     y = ui_fontheight + sdsc.height / 2;
     for (i = 0; i < D_ARRAY_SIZE(demos); i++) {
          if (!demos[i].requested || !demos[i].result)
               continue;

          if (demos[i].accelerated)
               primary->SetBlittingFlags( primary, DSBLIT_SRC_COLORKEY );
          else {
               primary->SetBlittingFlags( primary, DSBLIT_COLORIZE | DSBLIT_SRC_COLORKEY );
               primary->SetColor( primary, 0x20, 0x40, 0x40, 0xFF );
          }
          primary->Blit( primary, cardicon, NULL, SW - max_string_width - w - 25, y - h );

          y += sdsc.height / 2 + ui_fontheight + 2;
     }

     primary->Flip( primary, NULL, DSFLIP_NONE );

     event_buffer->Reset( event_buffer );
     event_buffer->WaitForEvent( event_buffer );
}

static void showStatus( const char *msg )
{
     if (run_fullscreen)
          return;

     primary->SetColor( primary, 0x40, 0x80, 0xFF, 0xFF );
     primary->DrawString( primary, "DirectFB Benchmarking Demo", -1, ui_fontheight * 5 / 3, SH, DSTF_TOP );

     primary->SetColor( primary, 0xFF, 0x00, 0x00, 0xFF );
     primary->DrawString( primary, msg, -1, SW - 2, SH, DSTF_TOPRIGHT );

     if (do_system) {
          primary->SetColor( primary, 0x80, 0x80, 0x80, 0xFF );
          primary->DrawString( primary, "Performing benchmark in system memory...", -1, SW / 2, SH / 2, DSTF_CENTER );
          sleep( 1 );
     }
}

static bool showAccelerated( DFBAccelerationMask func, IDirectFBSurface *source )
{
     DFBAccelerationMask mask;

     DFBCHECK(dest->GetAccelerationMask( dest, source, &mask ));

     if (mask & func)
          current_demo->accelerated = DFB_TRUE;

     if (!run_fullscreen) {
          if (mask & func) {
               primary->SetBlittingFlags( primary, DSBLIT_SRC_COLORKEY );
          }
          else {
               primary->SetBlittingFlags( primary, DSBLIT_COLORIZE | DSBLIT_SRC_COLORKEY );
               primary->SetColor( primary, 0x20, 0x40, 0x40, 0xFF );
          }

          primary->Blit( primary, cardicon, NULL, ui_fontheight / 4, SH + ui_fontheight / 10 );
     }

     return (mask & func) ? true : !accel_only;
}

/**********************************************************************************************************************/

#define SET_BLITTING_FLAGS(flags) \
     dest->SetBlittingFlags( dest, (flags) | (do_xor ? DSBLIT_XOR : 0) )

#define SET_DRAWING_FLAGS(flags) \
     dest->SetDrawingFlags( dest, (flags) | (do_xor ? DSDRAW_XOR : 0) )

static unsigned long long draw_string( long long t )
{
     long i;

     SET_DRAWING_FLAGS( DSDRAW_NOFX );

     if (!showAccelerated( DFXL_DRAWSTRING, NULL ))
          return 0;

     for (i = 0; i % 100 || direct_clock_get_millis() < (t + DEMOTIME); i++) {
          dest->SetColor( dest, myrand() & 0xFF, myrand() & 0xFF, myrand() & 0xFF, 0xFF );
          dest->DrawString( dest, "This is the DirectFB Benchmarking!!!", -1,
                            SW - bench_stringwidth > 0 ? myrand() % (SW - bench_stringwidth) : 0,
                            myrand() % (SH - bench_fontheight), DSTF_TOPLEFT );
     }

     return 1000 * 36 * (unsigned long long) i;
}

static unsigned long long draw_string_blend( long long t )
{
     long i;

     SET_DRAWING_FLAGS( DSDRAW_BLEND );

     dest->SetColor( dest, 0x80, 0x80, 0x80, 0x80 );

     if (!showAccelerated( DFXL_DRAWSTRING, NULL ))
          return 0;

     for (i = 0; i % 100 || direct_clock_get_millis() < (t + DEMOTIME); i++) {
          dest->SetColor( dest, myrand() & 0xFF, myrand() & 0xFF, myrand() & 0xFF, myrand() % 0x64 );
          dest->DrawString( dest, "This is the DirectFB Benchmarking!!!", -1,
                            SW - bench_stringwidth > 0 ? myrand() % (SW - bench_stringwidth) : 0,
                            myrand() % (SH - bench_fontheight), DSTF_TOPLEFT );
     }

     return 1000 * 36 * (unsigned long long) i;
}

static unsigned long long fill_rect( long long t )
{
     long i;

     SET_DRAWING_FLAGS( DSDRAW_NOFX );

     if (!showAccelerated( DFXL_FILLRECTANGLE, NULL ))
          return 0;

     for (i = 0; i % 100 || direct_clock_get_millis() < (t + DEMOTIME); i++) {
          dest->SetColor( dest, myrand() & 0xFF, myrand() & 0xFF, myrand() & 0xFF, 0xFF );
          dest->FillRectangle( dest, SW != SX ? myrand() % (SW - SX) : 0, SH != SY ? myrand() % (SH - SY) : 0, SX, SY );
     }

     return SX * SY * (unsigned long long) i;
}

static unsigned long long fill_rect_blend( long long t )
{
     long i;

     SET_DRAWING_FLAGS( DSDRAW_BLEND );

     if (!showAccelerated( DFXL_FILLRECTANGLE, NULL ))
          return 0;

     for (i = 0; i % 100 || direct_clock_get_millis() < (t + DEMOTIME); i++) {
          dest->SetColor( dest, myrand() & 0xFF, myrand() & 0xFF, myrand() & 0xFF, myrand() % 0x64 );
          dest->FillRectangle( dest, SW != SX ? myrand() % (SW - SX) : 0, SH != SY ? myrand() % (SH - SY) : 0, SX, SY );
     }

     return SX * SY * (unsigned long long) i;
}

static unsigned long long fill_rects( long long t )
{
     long         i, l;
     DFBRectangle rects[10];

     SET_DRAWING_FLAGS( DSDRAW_NOFX );

     if (!showAccelerated( DFXL_FILLRECTANGLE, NULL ))
          return 0;

     for (i = 0; i % 100 || direct_clock_get_millis() < (t + DEMOTIME); i++) {
          for (l = 0; l < 10; l++) {
               rects[l].x = (SW != SX) ? myrand() % (SW - SX) : 0;
               rects[l].y = (SH != SY) ? myrand() % (SH - SY) : 0;
               rects[l].w = SX;
               rects[l].h = SY;
          }

          dest->SetColor( dest, myrand() & 0xFF, myrand() & 0xFF, myrand() & 0xFF, 0xFF );
          dest->FillRectangles( dest, rects, 10 );
     }

     return SX * SY * 10 * (unsigned long long) i;
}

static unsigned long long fill_rects_blend( long long t )
{
     long         i, l;
     DFBRectangle rects[10];

     SET_DRAWING_FLAGS( DSDRAW_BLEND );

     if (!showAccelerated( DFXL_FILLRECTANGLE, NULL ))
          return 0;

     for (i = 0; i % 100 || direct_clock_get_millis() < (t + DEMOTIME); i++) {
          for (l = 0; l < 10; l++) {
               rects[l].x = (SW != SX) ? myrand() % (SW - SX) : 0;
               rects[l].y = (SH != SY) ? myrand() % (SH - SY) : 0;
               rects[l].w = SX;
               rects[l].h = SY;
          }

          dest->SetColor( dest, myrand() & 0xFF, myrand() & 0xFF, myrand() & 0xFF, myrand() % 0x64 );
          dest->FillRectangles( dest, rects, 10 );
     }

     return SX * SY * 10 * (unsigned long long) i;
}

static unsigned long long fill_triangle( long long t )
{
     long i, x, y;

     SET_DRAWING_FLAGS( DSDRAW_NOFX );

     if (!showAccelerated( DFXL_FILLTRIANGLE, NULL ))
          return 0;

     for (i = 0; i % 100 || direct_clock_get_millis() < (t + DEMOTIME); i++) {
          x = (SW != SX) ? myrand() % (SW - SX) : 0;
          y = (SH != SY) ? myrand() % (SH - SY) : 0;

          dest->SetColor( dest, myrand() & 0xFF, myrand() & 0xFF, myrand() & 0xFF, 0xFF );
          dest->FillTriangle( dest, x, y, x + SX - 1, y + SY / 2, x, y + SY - 1 );
     }

     return SX * SY * (unsigned long long) i / 2;
}

static unsigned long long fill_triangle_blend( long long t )
{
     long i, x, y;

     SET_DRAWING_FLAGS( DSDRAW_BLEND );

     if (!showAccelerated( DFXL_FILLTRIANGLE, NULL ))
          return 0;

     for (i = 0; i % 100 || direct_clock_get_millis() < (t + DEMOTIME); i++) {
          x = (SW != SX) ? myrand() % (SW - SX) : 0;
          y = (SH != SY) ? myrand() % (SH - SY) : 0;

          dest->SetColor( dest, myrand() & 0xFF, myrand() & 0xFF, myrand() & 0xFF, myrand() % 0x64 );
          dest->FillTriangle( dest, x, y, x + SX - 1, y + SY / 2, x, y + SY - 1 );
     }

     return SX * SY * (unsigned long long) i / 2;
}

static unsigned long long draw_rect( long long t )
{
     long i;

     SET_DRAWING_FLAGS( DSDRAW_NOFX );

     if (!showAccelerated( DFXL_DRAWRECTANGLE, NULL ))
          return 0;

     for (i = 0; i % 100 || direct_clock_get_millis() < (t + DEMOTIME); i++) {
          dest->SetColor( dest, myrand() & 0xFF, myrand() & 0xFF, myrand() & 0xFF, 0xFF );
          dest->DrawRectangle( dest, SW != SX ? myrand() % (SW - SX) : 0, SH != SY ? myrand() % (SH - SY) : 0, SX, SY );
     }

     return 1000 * (unsigned long long) i;
}

static unsigned long long draw_rect_blend( long long t )
{
     long i;

     SET_DRAWING_FLAGS( DSDRAW_BLEND );

     if (!showAccelerated( DFXL_DRAWRECTANGLE, NULL ))
          return 0;

     for (i = 0; i % 100 || direct_clock_get_millis() < (t + DEMOTIME); i++) {
          dest->SetColor( dest, myrand() & 0xFF, myrand() & 0xFF, myrand() & 0xFF, myrand() % 0x64 );
          dest->DrawRectangle( dest, SW != SX ? myrand() % (SW - SX) : 0, SH != SY ? myrand() % (SH - SY) : 0, SX, SY );
     }

     return 1000 * (unsigned long long) i;
}

static unsigned long long draw_lines( long long t )
{
     long      i, l, x, y, dx, dy;
     DFBRegion lines[10];

     SET_DRAWING_FLAGS( DSDRAW_NOFX );

     if (!showAccelerated( DFXL_DRAWLINE, NULL ))
          return 0;

     for (i = 0; i % 100 || direct_clock_get_millis() < (t + DEMOTIME); i++) {
          for (l = 0; l < 10; l++) {
               x  = myrand() % (SW - SX) + SX / 2;
               y  = myrand() % (SH - SY) + SY / 2;
               dx = myrand() % (2 * SX) - SX;
               dy = myrand() % (2 * SY) - SY;

               lines[l].x1 = x - dx / 2;
               lines[l].y1 = y - dy / 2;
               lines[l].x2 = x + dx / 2;
               lines[l].y2 = y + dy / 2;
          }

          dest->SetColor( dest, myrand() & 0xFF, myrand() & 0xFF, myrand() & 0xFF, 0xFF );
          dest->DrawLines( dest, lines, 10 );
     }

     return 1000 * 10 * (unsigned long long) i;
}

static unsigned long long draw_lines_blend( long long t )
{
     long i, l, x, y, dx, dy;
     DFBRegion lines[10];

     SET_DRAWING_FLAGS( DSDRAW_BLEND );

     if (!showAccelerated( DFXL_DRAWLINE, NULL ))
          return 0;

     for (i = 0; i % 100 || direct_clock_get_millis() < (t + DEMOTIME); i++) {
          for (l = 0; l < 10; l++) {
               x  = myrand() % (SW - SX) + SX / 2;
               y  = myrand() % (SH - SY) + SY / 2;
               dx = myrand() % (2 * SX) - SX;
               dy = myrand() % (2 * SY) - SY;

               lines[l].x1 = x - dx / 2;
               lines[l].y1 = y - dy / 2;
               lines[l].x2 = x + dx / 2;
               lines[l].y2 = y + dy / 2;
          }

          dest->SetColor( dest, myrand() & 0xFF, myrand() & 0xFF, myrand() & 0xFF, myrand() % 0x64 );
          dest->DrawLines( dest, lines, 10 );
     }

     return 1000 * 10 * (unsigned long long) i;
}

static unsigned long long fill_spans_with_flags( long long t, DFBSurfaceDrawingFlags flags )
{
     long    i, r;
     DFBSpan spans[SY];

     r = SW - SX - 8;
     if (r > 23)
          r = 23;

     SET_DRAWING_FLAGS( flags );

     if (!showAccelerated( DFXL_FILLRECTANGLE, NULL ))
          return 0;

     for (i = 0; i % 100 || direct_clock_get_millis() < (t + DEMOTIME); i++) {
          int w = myrand() % r + 2;
          int x = myrand() % (SW - SX - w * 2) + w;
          int d = 0;
          int a = 1;
          int l;

          for (l = 0; l < SY; l++) {
               spans[l].x = x + d;
               spans[l].w = SX;

               d += a;

               if (d == w)
                    a = -1;
               else if (d == -w)
                    a = 1;
          }

          dest->SetColor( dest, myrand() & 0xFF, myrand() & 0xFF, myrand() & 0xFF,
                          flags & DSDRAW_BLEND ? myrand() % 0x64 : 0xFF );
          dest->FillSpans( dest, myrand() % (SH - SY), spans, SY );
     }

     return SX * SY * (unsigned long long) i;
}

static unsigned long long fill_spans( long long t )
{
     return fill_spans_with_flags( t, DSDRAW_NOFX );
}

static unsigned long long fill_spans_blend( long long t )
{
     return fill_spans_with_flags( t, DSDRAW_BLEND );
}

static unsigned long long fill_traps( long long t )
{
     long i, l;
     DFBTrapezoid traps[10];

     SET_DRAWING_FLAGS( DSDRAW_NOFX );

     if (!showAccelerated( DFXL_FILLTRAPEZOID, NULL ))
          return 0;

     for (i = 0; i % 100 || direct_clock_get_millis() < (t + DEMOTIME); i++) {
          for (l = 0; l < 10; l++) {
               traps[l].x1 = (myrand() % (SW - SX * 3 / 2)) + SX / 2;
               traps[l].y1 = (SH != SY) ? myrand() % (SH - SY) : 0;
               traps[l].x2 = traps[l].x1 - SX / 2;
               traps[l].y2 = traps[l].y1 + SY - 1;
               traps[l].w1 = SX / 2;
               traps[l].w2 = SX * 3 / 2;
          }

          dest->SetColor( dest, myrand() & 0xFF, myrand() & 0xFF, myrand() & 0xFF, 0xFF );
          dest->FillTrapezoids( dest, traps, 10 );
     }

     return SX * SY * 10 * (unsigned long long) i;
}

static unsigned long long blit( long long t )
{
     long i;

     SET_BLITTING_FLAGS( DSBLIT_NOFX );

     if (!showAccelerated( DFXL_BLIT, simple ))
          return 0;

     for (i = 0; i % 100 || direct_clock_get_millis() < (t + DEMOTIME); i++) {
          dest->Blit( dest, simple, NULL, SW != SX ? myrand() % (SW - SX) : 0, SH != SY ? myrand() % (SH - SY) : 0 );
     }

     return SX * SY * (unsigned long long) i;
}

static unsigned long long blit180( long long t )
{
     long i;

     SET_BLITTING_FLAGS( DSBLIT_ROTATE180 );

     if (!showAccelerated( DFXL_BLIT, simple ))
          return 0;

     for (i = 0; i % 100 || direct_clock_get_millis() < (t + DEMOTIME); i++) {
          dest->Blit( dest, simple, NULL, SW != SX ? myrand() % (SW - SX) : 0, SH != SY ? myrand() % (SH - SY) : 0 );
     }

     return SX * SY * (unsigned long long) i;
}

static unsigned long long blit_colorkeyed( long long t )
{
     long i;

     SET_BLITTING_FLAGS( DSBLIT_SRC_COLORKEY );

     if (!showAccelerated( DFXL_BLIT, colorkeyed ))
          return 0;

     for (i = 0; i % 100 || direct_clock_get_millis() < (t + DEMOTIME); i++) {
          dest->Blit( dest, colorkeyed, NULL, SW != SX ? myrand() % (SW - SX) : 0, SH != SY ? myrand() % (SH - SY) : 0 );
     }

     return SX * SY * (unsigned long long) i;
}

static unsigned long long blit_dst_colorkeyed( long long t )
{
     long i;
     DFBRegion clip;

     clip.x1 = 0;
     clip.x2 = SW - 1;
     clip.y1 = 0;
     clip.y2 = SH - 1;

     dest->SetClip( dest, &clip );
     SET_BLITTING_FLAGS( DSBLIT_NOFX );
     dest->TileBlit( dest, logo, NULL, 0, 0 );
     dest->SetClip( dest, NULL );

     SET_BLITTING_FLAGS( DSBLIT_DST_COLORKEY );
     dest->SetDstColorKey( dest, 0xFF, 0xFF, 0xFF );

     if (!showAccelerated( DFXL_BLIT, simple ))
          return 0;

     for (i = 0; i % 100 || direct_clock_get_millis() < (t + DEMOTIME); i++) {
          dest->Blit( dest, simple, NULL, SW != SX ? myrand() % (SW - SX) : 0, SH != SY ? myrand() % (SH - SY) : 0 );
     }

     return SX * SY * (unsigned long long) i;
}

static unsigned long long blit_convert( long long t )
{
     long i;

     SET_BLITTING_FLAGS( DSBLIT_NOFX );

     if (!showAccelerated( DFXL_BLIT, image32 ))
          return 0;

     for (i = 0; i % 100 || direct_clock_get_millis() < (t + DEMOTIME); i++) {
          dest->Blit( dest, image32, NULL, SW != SX ? myrand() % (SW - SX) : 0, SH != SY ? myrand() % (SH - SY) : 0 );
     }

     return SX * SY * (unsigned long long) i;
}

static unsigned long long blit_colorize( long long t )
{
     long i;

     SET_BLITTING_FLAGS( DSBLIT_COLORIZE );

     if (!showAccelerated( DFXL_BLIT, simple ))
          return 0;

     for (i = 0; i % 100 || direct_clock_get_millis() < (t + DEMOTIME); i++) {
          dest->SetColor( dest, myrand() & 0xFF, myrand() & 0xFF, myrand() & 0xFF, 0xFF );
          dest->Blit( dest, simple, NULL, SW != SX ? myrand() % (SW - SX) : 0, SH != SY ? myrand() % (SH - SY) : 0 );
     }
     return SX * SY * (unsigned long long) i;
}

static unsigned long long blit_mask( long long t )
{
     long i;

     SET_BLITTING_FLAGS( DSBLIT_SRC_MASK_ALPHA | DSBLIT_BLEND_ALPHACHANNEL );

     dest->SetSourceMask( dest, image8a, 0, 0, DSMF_STENCIL );

     if (!showAccelerated( DFXL_BLIT, simple ))
          return 0;

     for (i = 0; i % 100 || direct_clock_get_millis() < (t + DEMOTIME); i++) {
          DFBRectangle src = { myrand() % SX, myrand() % SY, SX, SY };

          dest->Blit( dest, swirl, &src, SW != SX ? myrand() % (SW - SX) : 0, SH != SY ? myrand() % (SH - SY) : 0 );
     }

     return SX * SY * (unsigned long long) i;
}

static unsigned long long blit_blend( long long t )
{
     long i;

     SET_BLITTING_FLAGS( DSBLIT_BLEND_ALPHACHANNEL );

     if (!showAccelerated( DFXL_BLIT, image32a ))
          return 0;

     for (i = 0; i % 100 || direct_clock_get_millis() < (t + DEMOTIME); i++) {
          dest->Blit( dest, image32a, NULL, SW != SX ? myrand() % (SW - SX) : 0, SH != SY ? myrand() % (SH - SY) : 0 );
     }

     return SX * SY * (unsigned long long) i;
}

static unsigned long long blit_blend_colorize( long long t )
{
     long i;

     SET_BLITTING_FLAGS( DSBLIT_COLORIZE | DSBLIT_BLEND_ALPHACHANNEL );

     if (!showAccelerated( DFXL_BLIT, image32a ))
          return 0;

     for (i = 0; i % 100 || direct_clock_get_millis() < (t + DEMOTIME); i++) {
          dest->SetColor( dest, myrand() & 0xFF, myrand() & 0xFF, myrand() & 0xFF, 0xFF );
          dest->Blit( dest, image32a, NULL, SW != SX ? myrand() % (SW - SX) : 0, SH != SY ? myrand() % (SH - SY) : 0 );
     }

     return SX * SY * (unsigned long long) i;
}

static unsigned long long blit_srcover( long long t )
{
     long i;

     SET_BLITTING_FLAGS( DSBLIT_BLEND_ALPHACHANNEL );

     dest->SetPorterDuff( dest, DSPD_SRC_OVER );

     if (!showAccelerated( DFXL_BLIT, rose_pre ))
          return 0;

     for (i = 0; i % 100 || direct_clock_get_millis() < (t + DEMOTIME); i++) {
          dest->Blit( dest, rose_pre, NULL, SW != SX ? myrand() % (SW - SX) : 0, SH != SY ? myrand() % (SH - SY) : 0 );
     }

     dest->SetPorterDuff( dest, DSPD_NONE );

     return SX * SY * (unsigned long long) i;
}

static unsigned long long blit_srcover_pre( long long t )
{
     long i;

     SET_BLITTING_FLAGS( DSBLIT_BLEND_ALPHACHANNEL | DSBLIT_SRC_PREMULTIPLY );

     dest->SetPorterDuff( dest, DSPD_SRC_OVER );

     if (!showAccelerated( DFXL_BLIT, rose ))
          return 0;

     for (i = 0; i % 100 || direct_clock_get_millis() < (t + DEMOTIME); i++) {
          dest->Blit( dest, rose, NULL, SW != SX ? myrand() % (SW - SX) : 0, SH != SY ? myrand() % (SH - SY) : 0 );
     }

     dest->SetPorterDuff( dest, DSPD_NONE );

     return SX * SY * (unsigned long long) i;
}

static unsigned long long stretch_blit( long long t )
{
     long               i, l;
     unsigned long long pixels = 0;

     SET_BLITTING_FLAGS( DSBLIT_NOFX );

     if (!showAccelerated( DFXL_STRETCHBLIT, simple ))
          return 0;

     for (i = 1; direct_clock_get_millis() < (t + DEMOTIME); i++) {
          if (i > SH) {
               i = 10;
          }

          for (l = 10; l < SH; l += i) {
               DFBRectangle rect = { SW / 2 - l / 2, SH / 2 - l / 2, l, l };

               dest->StretchBlit( dest, simple, NULL, &rect );

               pixels += rect.w * rect.h;
          }
     }

     return pixels;
}

static unsigned long long stretch_blit_colorkeyed( long long t )
{
     long               i, l;
     unsigned long long pixels = 0;

     SET_BLITTING_FLAGS( DSBLIT_SRC_COLORKEY );

     if (!showAccelerated( DFXL_STRETCHBLIT, simple ))
          return 0;

     for (i = 1; direct_clock_get_millis() < (t + DEMOTIME); i++) {
          if (i > SH) {
               i = 10;
          }

          for (l = 10; l < SH; l += i) {
               DFBRectangle rect = { SW / 2 - l / 2, SH / 2 - l / 2, l, l };

               dest->StretchBlit( dest, colorkeyed, NULL, &rect );

               pixels += rect.w * rect.h;
          }
     }

     return pixels;
}

static unsigned long long load_image( long long t )
{
     int                     i;
     DFBSurfaceDescription   dsc;
     char                    buf[32];
     IDirectFBImageProvider *provider;
     IDirectFBSurface       *surface = NULL;

     if (!filename || accel_only)
          return 0;

     for (i = 0; direct_clock_get_millis() < (t + DEMOTIME); i++) {
          /* create an image provider for loading the file */
          DFBCHECK(dfb->CreateImageProvider( dfb, filename, &provider ));

          /* retrieve a surface description for the image */
          provider->GetSurfaceDescription( provider, &dsc );

          /* use the specified pixelformat */
          if (pixelformat != DSPF_UNKNOWN)
               dsc.pixelformat = pixelformat;

          /* create a surface using the description */
          if (!surface)
               DFBCHECK(dfb->CreateSurface( dfb, &dsc, &surface ));

          /* render the image to the created surface */
          provider->RenderTo( provider, surface, NULL );

          /* release the provider */
          provider->Release( provider );
     }

     if (surface)
          surface->Release( surface );

     snprintf( buf, sizeof(buf), " (%dx%d %s)", dsc.width, dsc.height, dfb_pixelformat_name( dsc.pixelformat ) );
     strcat( current_demo->desc, buf );

     return dsc.width * dsc.height * (unsigned long long) i;
}

/**********************************************************************************************************************/

int main( int argc, char *argv[] )
{
     int                       i, n;
     DFBInputEvent             evt;
     DFBFontDescription        fdsc;
     DFBSurfaceDescription     sdsc;
     DFBDataBufferDescription  ddsc;
     IDirectFBDataBuffer      *buffer;
     IDirectFBImageProvider   *provider;
     DFBSurfaceRenderOptions   render_options = DSRO_NONE;
     int                       demo_requested = 0;

     /* initialize DirectFB including command line parsing */
     DFBCHECK(DirectFBInit( &argc, &argv ));

     /* parse command line */
     for (n = 1; n < argc; n++) {
          if (strncmp( argv[n], "--", 2 ) == 0) {
               for (i = 0; i < D_ARRAY_SIZE(demos); i++) {
                    if (strcmp( argv[n] + 2, demos[i].option ) == 0) {
                         demo_requested = 1;
                         demos[i].requested = 1;
                         break;
                    }
               }

               if (i == D_ARRAY_SIZE(demos)) {
                    if (strcmp( argv[n] + 2, "help" ) == 0) {
                         print_usage();
                         return 0;
                    } else
                    if (strcmp( argv[n] + 2, "duration" ) == 0 && n + 1 < argc &&
                        sscanf( argv[n+1], "%d", &DEMOTIME ) == 1) {
                         n++;
                         continue;
                    } else
                    if (strcmp( argv[n] + 2, "iterations" ) == 0 && n + 1 < argc &&
                        sscanf( argv[n+1], "%d", &ITERATIONS ) == 1) {
                         n++;
                         continue;
                    } else
                    if (strcmp( argv[n] + 2, "size" ) == 0 && n + 1 < argc &&
                        sscanf( argv[n+1], "%dx%d", &SX, &SY ) == 2) {
                         n++;
                         continue;
                    } else
                    if (strcmp( argv[n] + 2, "pixelformat" ) == 0 && n + 1 < argc) {
                         pixelformat = parse_pixelformat( argv[n+1] );
                         n++;
                         continue;
                    } else
                    if (strcmp( argv[n] + 2, "system" ) == 0) {
                         do_system = 1;
                         continue;
                    } else
                    if (strcmp( argv[n] + 2, "dump" ) == 0) {
                         do_dump = 1;
                         continue;
                    } else
                    if (strcmp( argv[n] + 2, "wait" ) == 0 && n + 1 < argc &&
                        sscanf( argv[n+1], "%d", &do_wait ) == 1) {
                         n++;
                         continue;
                    } else
                    if (strcmp( argv[n] + 2, "noaccel" ) == 0) {
                         do_noaccel = 1;
                         continue;
                    } else
                    if (strcmp( argv[n] + 2, "accelonly" ) == 0) {
                         accel_only = 1;
                         continue;
                    } else
                    if (strcmp( argv[n] + 2, "smooth" ) == 0) {
                         do_smooth = 1;
                         continue;
                    } else
                    if (strcmp( argv[n] + 2, "aa" ) == 0) {
                         do_aa = 1;
                         continue;
                    } else
                    if (strcmp( argv[n] + 2, "xor" ) == 0) {
                         do_xor = 1;
                         continue;
                    } else
                    if (strcmp( argv[n] + 2, "matrix" ) == 0) {
                         do_matrix = 1;
                         continue;
                    } else
                    if (strcmp( argv[n] + 2, "noresults" ) == 0) {
                         show_results = 0;
                         continue;
                    } else
                    if (strcmp( argv[n] + 2, "all-demos" ) == 0) {
                         do_all_demos = 1;
                         continue;
                    } else
                    if (strcmp( argv[n] + 2, "csv" ) == 0) {
                         output_csv = 1;
                         continue;
                    } else
                    if (strcmp( argv[n] + 2, "fullscreen" ) == 0) {
                         run_fullscreen = 1;
                         continue;
                    } else
                    if (strcmp( argv[n] + 2, "intro" ) == 0) {
                         with_intro = 1;
                         continue;
                    } else
                    if (strcmp( argv[n] + 2, "load-image" ) == 0 && ++n < argc) {
                         filename = argv[n];
                         demo_requested = 1;
                         demos[D_ARRAY_SIZE(demos)-1].requested = 1;
                         continue;
                    }
               }
               else {
                    continue;
               }
          }

          print_usage();
          return 1;
     }

     if (!demo_requested || do_all_demos) {
          for (i = 0; i < D_ARRAY_SIZE(demos); i++)
               demos[i].requested = (demos[i].default_on || do_all_demos);
     }

     DFBCHECK(DirectFBSetOption( "bg-none", NULL ));

     /* create the main interface */
     DFBCHECK(DirectFBCreate( &dfb ));

     /* register termination function */
     atexit( dfb_shutdown );

     /* set the cooperative level to DFSCL_FULLSCREEN for exclusive access to the primary layer */
     dfb->SetCooperativeLevel( dfb, DFSCL_FULLSCREEN );

     /* create an event buffer for key events */
     DFBCHECK(dfb->CreateInputEventBuffer( dfb, DICAPS_BUTTONS | DICAPS_KEYS, DFB_FALSE, &event_buffer ));

     /* get the primary surface, i.e. the surface of the primary layer */
     sdsc.flags = DSDESC_CAPS;
     sdsc.caps  = DSCAPS_PRIMARY;

     DFBCHECK(dfb->CreateSurface( dfb, &sdsc, &primary ));

     DFBCHECK(primary->GetSize( primary, &SW, &SH ));

     /* load bench and ui fonts */
     fdsc.flags = DFDESC_HEIGHT;

#ifdef USE_FONT_HEADERS
     ddsc.flags         = DBDESC_MEMORY;
     ddsc.memory.data   = GET_FONTDATA( decker );
     ddsc.memory.length = GET_FONTSIZE( decker );
#else
     ddsc.flags         = DBDESC_FILE;
     ddsc.file          = GET_FONTFILE( decker );
#endif
     DFBCHECK(dfb->CreateDataBuffer( dfb, &ddsc, &buffer ));

     fdsc.height = 24;
     DFBCHECK(buffer->CreateFont( buffer, &fdsc, &bench_font ));
     DFBCHECK(bench_font->GetHeight( bench_font, &bench_fontheight ));
     DFBCHECK(bench_font->GetStringWidth( bench_font, "This is the DirectFB Benchmarking!!!", -1, &bench_stringwidth ));

     fdsc.height = 16;
     DFBCHECK(buffer->CreateFont( buffer, &fdsc, &ui_font ));
     DFBCHECK(ui_font->GetHeight( ui_font, &ui_fontheight ));

     /* clear with black */
     primary->Clear( primary, 0, 0, 0, 0x80 );

     /* start screen */
#ifdef USE_IMAGE_HEADERS
     ddsc.flags         = DBDESC_MEMORY;
     ddsc.memory.data   = GET_IMAGEDATA( biglogo );
     ddsc.memory.length = GET_IMAGESIZE( biglogo );
#else
     ddsc.flags         = DBDESC_FILE;
     ddsc.file          = GET_IMAGEFILE( biglogo );
#endif
     DFBCHECK(dfb->CreateDataBuffer( dfb, &ddsc, &buffer ));
     DFBCHECK(buffer->CreateImageProvider( buffer, &provider ));
     provider->GetSurfaceDescription( provider, &sdsc );
     sdsc.width  = (SH / 8) * sdsc.width / sdsc.height;
     sdsc.height = SH / 8;
     DFBCHECK(dfb->CreateSurface( dfb, &sdsc, &logo ));
     provider->RenderTo( provider, logo, NULL );
     provider->Release( provider );

     primary->SetBlittingFlags( primary, DSBLIT_BLEND_ALPHACHANNEL );
     primary->Blit( primary, logo, NULL, (SW - sdsc.width) / 2, SH / 5 );

     primary->SetFont( primary, ui_font );
     primary->SetColor( primary, 0xA0, 0xA0, 0xA0, 0xFF );
     primary->DrawString( primary, "Preparing...", -1, SW / 2, SH / 2, DSTF_CENTER );
     primary->Flip( primary, NULL, DSFLIP_NONE );

     /* status bar */
     if (!run_fullscreen)
          SH -= ui_fontheight;

     /* benchmark size */
     if (SX > SW - 10)
          SX = SW - 10;

     if (SY > SH - 10)
          SY = SH - 10;

     /* benchmark pixelformat */
     if (pixelformat == DSPF_UNKNOWN)
          DFBCHECK(primary->GetPixelFormat( primary, &pixelformat ));

     /* card icon */
#ifdef USE_IMAGE_HEADERS
     ddsc.flags         = DBDESC_MEMORY;
     ddsc.memory.data   = GET_IMAGEDATA( card );
     ddsc.memory.length = GET_IMAGESIZE( card );
#else
     ddsc.flags         = DBDESC_FILE;
     ddsc.file          = GET_IMAGEFILE( card );
#endif
     DFBCHECK(dfb->CreateDataBuffer( dfb, &ddsc, &buffer ));
     DFBCHECK(buffer->CreateImageProvider( buffer, &provider ));
     provider->GetSurfaceDescription( provider, &sdsc );
     sdsc.width  = sdsc.width * (ui_fontheight - ui_fontheight / 5) / sdsc.height;
     sdsc.height = (ui_fontheight - ui_fontheight / 5);
     DFBCHECK(primary->GetPixelFormat( primary, &sdsc.pixelformat ));
     DFBCHECK(dfb->CreateSurface( dfb, &sdsc, &cardicon ));
     provider->RenderTo( provider, cardicon, NULL );
     provider->Release( provider );

     /* create a surface and render an image to it */
#ifdef USE_IMAGE_HEADERS
     ddsc.flags         = DBDESC_MEMORY;
     ddsc.memory.data   = GET_IMAGEDATA( swirl );
     ddsc.memory.length = GET_IMAGESIZE( swirl );
#else
     ddsc.flags         = DBDESC_FILE;
     ddsc.file          = GET_IMAGEFILE( swirl );
#endif
     DFBCHECK(dfb->CreateDataBuffer( dfb, &ddsc, &buffer ));
     DFBCHECK(buffer->CreateImageProvider( buffer, &provider ));
     provider->GetSurfaceDescription( provider, &sdsc );
     sdsc.width       = SX * 2;
     sdsc.height      = SY * 2;
     sdsc.pixelformat = pixelformat;
     DFBCHECK(dfb->CreateSurface( dfb, &sdsc, &swirl ));
     provider->RenderTo( provider, swirl, NULL );
     provider->Release( provider );

     /* create a surface and render an image to it */
#ifdef USE_IMAGE_HEADERS
     ddsc.flags         = DBDESC_MEMORY;
     ddsc.memory.data   = GET_IMAGEDATA( rose );
     ddsc.memory.length = GET_IMAGESIZE( rose );
#else
     ddsc.flags         = DBDESC_FILE;
     ddsc.file          = GET_IMAGEFILE( rose );
#endif
     DFBCHECK(dfb->CreateDataBuffer( dfb, &ddsc, &buffer ));
     DFBCHECK(buffer->CreateImageProvider( buffer, &provider ));
     provider->GetSurfaceDescription( provider, &sdsc );
     sdsc.width       = SX;
     sdsc.height      = SY;
     sdsc.pixelformat = DSPF_ARGB;
     DFBCHECK(dfb->CreateSurface( dfb, &sdsc, &rose ));
     provider->RenderTo( provider, rose, NULL );
     sdsc.flags      |= DSDESC_CAPS;
     sdsc.caps        = DSCAPS_PREMULTIPLIED;
     DFBCHECK(dfb->CreateSurface( dfb, &sdsc, &rose_pre ));
     provider->RenderTo( provider, rose_pre, NULL );
     provider->Release( provider );

     /* create a surface and render an image to it */
#ifdef USE_IMAGE_HEADERS
     ddsc.flags         = DBDESC_MEMORY;
     ddsc.memory.data   = GET_IMAGEDATA( melted );
     ddsc.memory.length = GET_IMAGESIZE( melted );
#else
     ddsc.flags         = DBDESC_FILE;
     ddsc.file          = GET_IMAGEFILE( melted );
#endif
     DFBCHECK(dfb->CreateDataBuffer( dfb, &ddsc, &buffer ));
     DFBCHECK(buffer->CreateImageProvider( buffer, &provider ));
     provider->GetSurfaceDescription( provider, &sdsc );
     sdsc.width       = SX;
     sdsc.height      = SY;
     sdsc.pixelformat = pixelformat;
     DFBCHECK(dfb->CreateSurface( dfb, &sdsc, &simple ));
     provider->RenderTo( provider, simple, NULL );
     provider->Release( provider );

     /* create a surface and render an image to it */
#ifdef USE_IMAGE_HEADERS
     ddsc.flags         = DBDESC_MEMORY;
     ddsc.memory.data   = GET_IMAGEDATA( colorkeyed );
     ddsc.memory.length = GET_IMAGESIZE( colorkeyed );
#else
     ddsc.flags         = DBDESC_FILE;
     ddsc.file          = GET_IMAGEFILE( colorkeyed );
#endif
     DFBCHECK(dfb->CreateDataBuffer( dfb, &ddsc, &buffer ));
     DFBCHECK(buffer->CreateImageProvider( buffer, &provider ));
     provider->GetSurfaceDescription( provider, &sdsc );
     sdsc.width       = SX;
     sdsc.height      = SY;
     sdsc.pixelformat = pixelformat;
     DFBCHECK(dfb->CreateSurface( dfb, &sdsc, &colorkeyed ));
     provider->RenderTo( provider, colorkeyed, NULL );
     DFBCHECK(colorkeyed->SetSrcColorKey( colorkeyed, 0x06, 0x18, 0xF4 ));
     provider->Release( provider );

     /* create a surface and render an image to it */
#ifdef USE_IMAGE_HEADERS
     ddsc.flags         = DBDESC_MEMORY;
     ddsc.memory.data   = GET_IMAGEDATA( laden_bike );
     ddsc.memory.length = GET_IMAGESIZE( laden_bike );
#else
     ddsc.flags         = DBDESC_FILE;
     ddsc.file          = GET_IMAGEFILE( laden_bike );
#endif
     DFBCHECK(dfb->CreateDataBuffer( dfb, &ddsc, &buffer ));
     DFBCHECK(buffer->CreateImageProvider( buffer, &provider ));
     provider->GetSurfaceDescription( provider, &sdsc );
     sdsc.width       = SX;
     sdsc.height      = SY;
     sdsc.pixelformat = DFB_BYTES_PER_PIXEL( pixelformat ) == 2 ? DSPF_RGB32 : DSPF_RGB16;
     DFBCHECK(dfb->CreateSurface( dfb, &sdsc, &image32 ));
     provider->RenderTo( provider, image32, NULL );
     provider->Release( provider );

     /* create a surface and render an image to it */
#ifdef USE_IMAGE_HEADERS
     ddsc.flags         = DBDESC_MEMORY;
     ddsc.memory.data   = GET_IMAGEDATA( sacred_heart );
     ddsc.memory.length = GET_IMAGESIZE( sacred_heart );
#else
     ddsc.flags         = DBDESC_FILE;
     ddsc.file          = GET_IMAGEFILE( sacred_heart );
#endif
     DFBCHECK(dfb->CreateDataBuffer( dfb, &ddsc, &buffer ));
     DFBCHECK(buffer->CreateImageProvider( buffer, &provider ));
     provider->GetSurfaceDescription( provider, &sdsc );
     sdsc.width       = SX;
     sdsc.height      = SY;
     sdsc.pixelformat = DSPF_ARGB;
     DFBCHECK(dfb->CreateSurface( dfb, &sdsc, &image32a ));
     provider->RenderTo( provider, image32a, NULL );
     provider->Release( provider );

     /* create a surface and render an image to it */
#ifdef USE_IMAGE_HEADERS
     ddsc.flags         = DBDESC_MEMORY;
     ddsc.memory.data   = GET_IMAGEDATA( fish );
     ddsc.memory.length = GET_IMAGESIZE( fish );
#else
     ddsc.flags         = DBDESC_FILE;
     ddsc.file          = GET_IMAGEFILE( fish );
#endif
     DFBCHECK(dfb->CreateDataBuffer( dfb, &ddsc, &buffer ));
     DFBCHECK(buffer->CreateImageProvider( buffer, &provider ));
     provider->GetSurfaceDescription( provider, &sdsc );
     sdsc.width       = SX;
     sdsc.height      = SY;
     sdsc.pixelformat = DSPF_A8;
     DFBCHECK(dfb->CreateSurface( dfb, &sdsc, &image8a ));
     provider->RenderTo( provider, image8a, NULL );
     provider->Release( provider );

     /* intro screen */
     if (with_intro) {
#ifdef USE_IMAGE_HEADERS
          ddsc.flags         = DBDESC_MEMORY;
          ddsc.memory.data   = GET_IMAGEDATA( intro );
          ddsc.memory.length = GET_IMAGESIZE( intro );
#else
          ddsc.flags         = DBDESC_FILE;
          ddsc.file          = GET_IMAGEFILE( intro );
#endif
          DFBCHECK(dfb->CreateDataBuffer( dfb, &ddsc, &buffer ));
          DFBCHECK(buffer->CreateImageProvider( buffer, &provider ));
          provider->GetSurfaceDescription( provider, &sdsc );
          sdsc.width  = SW;
          sdsc.height = run_fullscreen ? SH : SH + ui_fontheight;
          DFBCHECK(dfb->CreateSurface( dfb, &sdsc, &intro ));
          provider->RenderTo( provider, intro, NULL );
          provider->Release( provider );
     }

     printf( "Benchmarking %dx%d on %dx%d %s (%dbit)...\n",
             SX, SY, SW, SH, dfb_pixelformat_name( pixelformat ), DFB_BYTES_PER_PIXEL( pixelformat ) * 8 );

     if (do_system) {
          sdsc.flags       = DSDESC_WIDTH | DSDESC_HEIGHT | DSDESC_PIXELFORMAT | DSDESC_CAPS;
          sdsc.width       = SW;
          sdsc.height      = SH;
          sdsc.pixelformat = pixelformat;
          sdsc.caps        = DSCAPS_SYSTEMONLY;

          DFBCHECK(dfb->CreateSurface( dfb, &sdsc, &dest ));

          dest->Clear( dest, 0, 0, 0, 0x80 );
     }
     else {
          DFBRectangle rect = { 0, 0, SW, SH };

          DFBCHECK(primary->GetSubSurface( primary, &rect, &dest ));
     }

     if (do_noaccel)
          dest->DisableAcceleration( dest, DFXL_ALL );

     dest->SetFont( dest, bench_font );

     if (do_smooth)
          render_options |= DSRO_SMOOTH_UPSCALE | DSRO_SMOOTH_DOWNSCALE;

     if (do_aa)
          render_options |= DSRO_ANTIALIAS;

     if (do_matrix) {
          const s32 matrix[9] = { 0x01000, 0x19F00, 0x00000,
                                  0x08A00, 0x01000, 0x00000,
                                  0x00000, 0x00000, 0x10000 };

          dest->SetMatrix( dest, matrix );

          render_options |= DSRO_MATRIX;
     }

     dest->SetRenderOptions( dest, render_options );

     direct_sync();

run:
     for (i = 0; i < D_ARRAY_SIZE(demos); i++) {
           int j, skip;

           if (!demos[i].requested)
                continue;

           current_demo = &demos[i];

           skip = 0;

           for (j = 0; j < ITERATIONS; j++) {
                long               perf;
                long long          t, dt, t1, t2;
                unsigned long long pixels;

                showMessage( demos[i].message );

                showStatus( demos[i].status );

                /* Get ready... */
                direct_sync();
                dfb->WaitIdle( dfb );

                /* Take start... */
                t1 = process_time();
                t = direct_clock_get_millis();

                /* Go... */
                pixels = demos[i].func( t );

                /* Wait... */
                dfb->WaitIdle( dfb );

                /* Take stop... */
                dt = direct_clock_get_millis() - t;
                t2 = process_time();

                if (!pixels || !dt) {
                     skip = 1;
                     break;
                }

                primary->Flip( primary, NULL, DSFLIP_NONE );

                perf = pixels / dt;
                if (perf > demos[i].result) {
                     demos[i].result   = perf;
                     demos[i].load     = (t2 - t1) * 1000 / (ticks_per_second() * dt / 1000);
                     demos[i].duration = dt;
                }
           }

           if (skip)
                continue;

           if (output_csv) {
                printf( "%s%s%s,%ld.%.3ld,%s,%ld.%.3ld,%s,%d.%d\n",
                        do_aa ? "AA " : "",
                        do_matrix ? "MX " : "",
                        demos[i].desc, demos[i].duration / 1000, demos[i].duration % 1000,
                        demos[i].accelerated ? "*" : " ",
                        demos[i].result / 1000, demos[i].result % 1000, demos[i].unit,
                        demos[i].load / 10, demos[i].load % 10 );
           }
           else {
                printf( "%s%s%-44s %3ld.%.3ld secs (%s%4ld.%.3ld %s) [%3d.%d%%]\n",
                        do_aa ? "AA " : "",
                        do_matrix ? "MX " : "",
                        demos[i].desc, demos[i].duration / 1000, demos[i].duration % 1000,
                        demos[i].accelerated ? "*" : " ",
                        demos[i].result / 1000, demos[i].result % 1000, demos[i].unit,
                        demos[i].load / 10, demos[i].load % 10 );
           }

           if (do_system) {
                primary->SetBlittingFlags( primary, DSBLIT_NOFX );
                primary->Blit( primary, dest, NULL, 0, 0 );
                sleep( 2 );
                dest->Clear( dest, 0, 0, 0, 0x80 );
           }

           if (do_dump) {
                int  index = 0;
                char buf[200];

                snprintf( buf, sizeof(buf), "DirectFB_%s%s%s",
                          do_aa ? "AA " : "", do_matrix ? "MX " : "", demos[i].desc );

                while (buf[index]) {
                     if (buf[index] == ' ')
                          buf[index] = '_';

                     index++;
                }

                primary->Dump( primary, ".", buf );
           }

           if (do_wait)
                sleep( do_wait );
     }

     /* results screen */
     if (show_results)
          showResult();

     event_buffer->GetEvent( event_buffer, DFB_EVENT(&evt) );

     if (evt.key_id == DIKI_HOME || evt.key_id == DIKI_ENTER) {
          for (i = 0; i < D_ARRAY_SIZE(demos); i++)
               demos[i].result = 0;

          sleep( 1 );

          goto run;
     }

     return 42;
}
