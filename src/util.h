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
#include <direct/system.h>
#include <directfb.h>
#include <sys/times.h> /* for times() */

/**************************************************************************************************/

/* macro for a safe call to DirectFB functions */
#define DFBCHECK(x)                                                   \
     do {                                                             \
          DFBResult result = x;                                       \
          if (result != DFB_OK) {                                     \
               fprintf( stderr, "%s <%d>:\n\t", __FILE__, __LINE__ ); \
               DirectFBErrorFatal( #x, result );                      \
          }                                                           \
     } while (0)

/**************************************************************************************************/

#ifdef USE_FONT_HEADERS
#ifdef DGIFF_EXTENSION
static inline bool
check_fontformat()
{
     DFBSurfacePixelFormat fontformat = DSPF_A8;

#ifdef HAVE_GETFONTSURFACEFORMAT
     idirectfb_singleton->GetFontSurfaceFormat( idirectfb_singleton, &fontformat );
#endif

     return fontformat == DSPF_A8 ? true : false;
}

#define GET_FONTDATA(name) check_fontformat() ? name##_data : name##_argb_data
#else
#define GET_FONTDATA(name) name##_data
#endif
#define GET_FONTSIZE(name) sizeof(name##_data)
#else
static char fontfile[PATH_MAX];

static inline char *
get_fontfile( const char *name )
{
     const char *formatname;
     const char *extension = direct_getenv( "FONT_EXTENSION" ) ?: "dgiff";

     if (!strcmp( extension, "dgiff" )) {
          DFBSurfacePixelFormat fontformat = DSPF_A8;
#ifdef HAVE_GETFONTSURFACEFORMAT
          idirectfb_singleton->GetFontSurfaceFormat( idirectfb_singleton, &fontformat );
#endif
          formatname = fontformat == DSPF_A8 ? "" : "_argb";
     }
     else
          formatname = "";

     snprintf( fontfile, PATH_MAX, DATADIR"/%s%s.%s", name, formatname, extension );

     return fontfile;
}

#define GET_FONTFILE(name) get_fontfile( #name )
#endif

#ifdef USE_IMAGE_HEADERS
#define GET_IMAGEDATA(name) name##_data
#define GET_IMAGESIZE(name) sizeof(name##_data)

#define GET_IMAGEDESC(name) &name##_desc
#else
static char imagefile[PATH_MAX];

static inline char *
get_imagefile( const char *name )
{
     const char *extension = direct_getenv( "IMAGE_EXTENSION" ) ?: "dfiff";

     snprintf( imagefile, PATH_MAX, DATADIR"/%s.%s", name, extension );

     return imagefile;
}

#define GET_IMAGEFILE(name) get_imagefile( #name )
#endif

#ifdef USE_VIDEO_HEADERS
#define GET_VIDEODATA(name) name##_data
#define GET_VIDEOSIZE(name) sizeof(name##_data)
#else
static char videofile[PATH_MAX];

static inline char *
get_videofile( const char *name )
{
     const char *extension = direct_getenv( "VIDEO_EXTENSION" ) ?: "dfvff";

     snprintf( videofile, PATH_MAX, DATADIR"/%s.%s", name, extension );

     return videofile;
}

#define GET_VIDEOFILE(name) get_videofile( #name )
#endif

/**************************************************************************************************/

typedef struct {
     int       magic;

     int       frames;
     float     fps;
     long long fps_time;
     char      fps_string[20];
} FPSData;

static inline void
fps_init( FPSData *data )
{
     D_ASSERT( data != NULL );

     memset( data, 0, sizeof(FPSData) );

     data->fps_time = direct_clock_get_millis();

     D_MAGIC_SET( data, FPSData );
}

static inline void
fps_count( FPSData *data,
           int      interval )
{
     long long diff;
     long long now = direct_clock_get_millis();

     D_MAGIC_ASSERT( data, FPSData );

     data->frames++;

     diff = now - data->fps_time;
     if (diff >= interval) {
          data->fps = 1000.0f * data->frames / diff;

          snprintf( data->fps_string, sizeof(data->fps_string),
                    "%d.%d", (int) data->fps, (int) (data->fps * 10) % 10 );

          data->fps_time = now;
          data->frames   = 0;
     }
}

/**************************************************************************************************/

typedef struct {
     int          magic;

     unsigned int stat_total;
     unsigned int stat_idle;
     float        idle;
     long long    idle_time;
     char         idle_string[20];
} IdleData;

static inline void
idle_read( unsigned int *ret_total,
           unsigned int *ret_idle)
{
     FILE         *stat;
     char          cpu[4];
     unsigned int  user, nice, system, idle, iowait, irq, softirq, un1, un2;

     stat = fopen( "/proc/stat", "r" );

     if (stat) {
          if (fscanf( stat, "%s %u %u %u %u %u %u %u %u %u",
                      cpu, &user, &nice, &system, &idle, &iowait, &irq, &softirq, &un1, &un2 )) {
               fclose(stat);
               *ret_total = user + nice + system + idle + iowait + irq + softirq + un1 + un2;
               *ret_idle  = idle;
          }
     }
}

static inline void
idle_init( IdleData *data )
{

     D_ASSERT( data != NULL );

     memset( data, 0, sizeof(IdleData) );

     idle_read( &data->stat_total, &data->stat_idle );

     data->idle_time = direct_clock_get_millis();

     D_MAGIC_SET( data, IdleData );
}

static inline void
idle_count( IdleData *data,
            int       interval )
{
     long long diff;
     long long now = direct_clock_get_millis();

     D_MAGIC_ASSERT( data, IdleData );

     diff = now - data->idle_time;
     if (diff >= interval) {
         unsigned int total = 0, idle = 0;

         idle_read( &total, &idle );

         if (idle != data->stat_idle)
             data->idle = 100.0f * (idle - data->stat_idle) / (total - data->stat_total);
         else
             data->idle = 0;

          snprintf( data->idle_string, sizeof(data->idle_string),
                    "%d.%d", (int) data->idle, (int) (data->idle * 10) % 10 );

          data->idle_time  = now;
          data->stat_total = total;
          data->stat_idle  = idle;
     }
}

/**************************************************************************************************/

static inline long long
process_time( void )
{
     struct tms tms;

     times( &tms );

     return tms.tms_utime + tms.tms_stime;
}

static inline long
ticks_per_second( void )
{
     return sysconf( _SC_CLK_TCK );
}
