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
#include <math.h>

#include "util.h"

#ifdef USE_FONT_HEADERS
#include "decker.h"
#endif

#ifdef USE_IMAGE_HEADERS
#include "texture.h"
#endif

/* main interface */
static IDirectFB *dfb = NULL;

/* event buffer interface */
static IDirectFBEventBuffer *event_buffer = NULL;

/* primary surface */
static IDirectFBSurface *primary = NULL;

/* font */
static IDirectFBFont *font = NULL;

/* video provider */
static IDirectFBVideoProvider *videoprovider = NULL;

/* texture surface */
static IDirectFBSurface *texture = NULL;

/* screen width and height */
static int screen_width, screen_height;

/* triple buffering */
static bool triple = false;

/* absolute coordinates */
static int axisabs_x[2] = { -1, -1 };
static int axisabs_y[2] = { -1, -1 };

#ifdef HAVE_MT
#define DFB_EVENT_SLOT_ID(e) e.slot_id
#else
#define DFB_EVENT_SLOT_ID(e) 0
#endif

/**********************************************************************************************************************/

typedef struct {
     float v[4];
} VeVector;

typedef struct {
     float m[16];
} VeMatrix;

typedef enum {
     VE_CLIP_NONE   = 0x00,

     VE_CLIP_RIGHT  = 0x01,
     VE_CLIP_LEFT   = 0x02,
     VE_CLIP_TOP    = 0x04,
     VE_CLIP_BOTTOM = 0x08,
     VE_CLIP_FAR    = 0x10,
     VE_CLIP_NEAR   = 0x20,

     VE_CLIP_ALL    = 0x3f
} VeClipMask;

typedef struct {
     /*
      * Object coordinates
      */
     VeVector   obj;

     /*
      * Texture coordinates
      */
     float      s;
     float      t;

     /*
      * Index of transformed version in output buffer
      */
     int        index;

     /*
      * Clip coordinates are object coordinates after transformation using modelview
      * and projection matrix. The clip mask has a flag for each of the six clipping
      * planes indicating that the vertex lies behind the plane (outside the view volume).
      */
     VeVector   clip;
     VeClipMask clipMask;
} VeVertex;

typedef struct {
     int              size;
     VeVertex        *data;

     int              count;

     int              max_vertices;
     int              max_indices;

     DFBVertex       *vertices;
     int              num_vertices;

     int             *indices;
     int              num_indices;
} VeVertexBuffer;

/**********************************************************************************************************************/

static const VeMatrix identity = { { 1, 0, 0, 0,
                                     0, 1, 0, 0,
                                     0, 0, 1, 0,
                                     0, 0, 0, 1 } };

static VeMatrix windowmap, projection, modelview, composite;

/* vertex buffer */
static VeVertexBuffer *buffer = NULL;

/* composite matrix update */
static bool update = false;

/**********************************************************************************************************************/

#define A(row,col) a[(col<<2)+row]
#define B(row,col) b[(col<<2)+row]
#define P(row,col) product[(col<<2)+row]

static void matmul4( float *product, const float *a, const float *b )
{
     int i;

     for (i = 0; i < 4; i++) {
          float ai0 = A(i,0), ai1 = A(i,1), ai2 = A(i,2), ai3 = A(i,3);
          P(i,0) = ai0 * B(0,0) + ai1 * B(1,0) + ai2 * B(2,0) + ai3 * B(3,0);
          P(i,1) = ai0 * B(0,1) + ai1 * B(1,1) + ai2 * B(2,1) + ai3 * B(3,1);
          P(i,2) = ai0 * B(0,2) + ai1 * B(1,2) + ai2 * B(2,2) + ai3 * B(3,2);
          P(i,3) = ai0 * B(0,3) + ai1 * B(1,3) + ai2 * B(2,3) + ai3 * B(3,3);
     }
}

#undef A
#undef B
#undef P

/**********************************************************************************************************************/

static void veInit( float fovy, float aspect, float nearval, float farval )
{
     float left, right, bottom, top;

     windowmap = identity;
     windowmap.m[0] = windowmap.m[12] = (float) screen_width / 2;
     windowmap.m[5] = windowmap.m[13] = (float) screen_height / 2;

     top = nearval * tan( fovy * M_PI / 360.0 );
     bottom = -top;
     left = bottom * aspect;
     right = top * aspect;

     projection = identity;
     projection.m[0]  = (2 * nearval) / (right - left);
     projection.m[5]  = (2 * nearval) / (top - bottom);
     projection.m[8]  = (right + left) / (right  - left);
     projection.m[9]  = (top + bottom) / (top - bottom);
     projection.m[10] = -(farval + nearval) / (farval - nearval);
     projection.m[11] = -1;
     projection.m[14] = -(2 * farval * nearval) / (farval - nearval);

     modelview = identity;

     matmul4( composite.m, projection.m, modelview.m );
}

static void veRotate( float angle, float x, float y, float z )
{
     float m[16];
     float mag, s, c, one_c;

     if (!angle)
          return;

     mag = sqrt( x * x + y * y + z * z );
     if (!mag)
          return;

     x /= mag;
     y /= mag;
     z /= mag;

     s = sin( angle );
     c = cos( angle );
     one_c = 1 - c;

     m[0] = (one_c * x * x) + c;     m[4] = (one_c * x * y) - z * s; m[8]  = (one_c * z * x) + y * s; m[12] = 0;
     m[1] = (one_c * x * y) + z * s; m[5] = (one_c * y * y) + c;     m[9]  = (one_c * y * z) - x * s; m[13] = 0;
     m[2] = (one_c * z * x) - y * s; m[6] = (one_c * y * z) + x * s; m[10] = (one_c * z * z) + c;     m[14] = 0;
     m[3] = 0;                       m[7] = 0;                       m[11] = 0;                       m[15] = 1;

     matmul4( modelview.m, modelview.m, m );

     update = true;
}

static void veScale( float x, float y, float z )
{
     modelview.m[0] *= x; modelview.m[4] *= y; modelview.m[8]  *= z;
     modelview.m[1] *= x; modelview.m[5] *= y; modelview.m[9]  *= z;
     modelview.m[2] *= x; modelview.m[6] *= y; modelview.m[10] *= z;
     modelview.m[3] *= x; modelview.m[7] *= y; modelview.m[11] *= z;

     update = true;
}

static void veTranslate( float x, float y, float z )
{
     modelview.m[12] = modelview.m[0] * x + modelview.m[4] * y + modelview.m[8]  * z + modelview.m[12];
     modelview.m[13] = modelview.m[1] * x + modelview.m[5] * y + modelview.m[9]  * z + modelview.m[13];
     modelview.m[14] = modelview.m[2] * x + modelview.m[6] * y + modelview.m[10] * z + modelview.m[14];
     modelview.m[15] = modelview.m[3] * x + modelview.m[7] * y + modelview.m[11] * z + modelview.m[15];

     update = true;
}

/**********************************************************************************************************************/

static void vbNew( int num )
{
     buffer = calloc( 1, sizeof(VeVertexBuffer) );

     buffer->size         = num;
     buffer->data         = malloc( buffer->size * sizeof(VeVertex) );
     buffer->max_vertices = buffer->size << 2;
     buffer->max_indices  = (buffer->size - 2) * 9;
     buffer->vertices     = malloc( buffer->max_vertices * sizeof(DFBVertex) );
     buffer->indices      = malloc( buffer->max_indices * sizeof(int) );
}

static void vbAdd( float x, float y, float z, float s, float t )
{
     VeVertex *vtx;

     if (buffer->count == buffer->size) {
          return;
     }

     vtx = &buffer->data[buffer->count++];

     vtx->obj.v[0] = x;
     vtx->obj.v[1] = y;
     vtx->obj.v[2] = z;
     vtx->obj.v[3] = 1;

     vtx->s = s;
     vtx->t = t;

     vtx->index = -1;
}

static void vbClear( void )
{
     buffer->count = 0;
}

static int add_vertex( const VeVertex *vtx )
{
     int        index;
     float      oow;
     DFBVertex *dst;

     /* Use the next free index. */
     index = buffer->num_vertices++;

     /* Get the pointer to the element. */
     dst = &buffer->vertices[index];

     /* Calculate one over w. */
     oow = 1 / vtx->clip.v[3];

     /* Transform to window coordinates. */
     dst->x = oow * vtx->clip.v[0] * windowmap.m[0]  + windowmap.m[12];
     dst->y = oow * vtx->clip.v[1] * windowmap.m[5]  + windowmap.m[13];
     dst->z = oow * vtx->clip.v[2] * windowmap.m[10] + windowmap.m[14];
     dst->w = oow;

     /* Copy texture coordinates. */
     dst->s = vtx->s;
     dst->t = vtx->t;

     /* Return index within the output buffer. */
     return index;
}

static int clip_polygon( const int *input, int count, VeClipMask clipOr, int *output )
{
     int       i, p;
     float     d[4], t;
     VeVertex  tmp1[count<<2];
     VeVertex  tmp2[count<<2];
     VeVertex *from = tmp1;
     VeVertex *to   = tmp2;

     for (i = 0; i < count; i++)
          from[i] = buffer->data[input[i]];

     for (p = 0; p < 6; p++) {
          int pc = p >> 1;

          if (clipOr & (1 << p)) {
               int       out   = 0;
               VeVertex *prev  = &from[count - 1];
               int       pflag = p & 1 ? prev->clip.v[pc] >= -prev->clip.v[3] : prev->clip.v[pc] <= prev->clip.v[3];

               for (i = 0; i < count; i++) {
                    VeVertex *curr = &from[i];
                    int       flag = p & 1 ? curr->clip.v[pc] >= -curr->clip.v[3] : curr->clip.v[pc] <= curr->clip.v[3];

                    if (flag ^ pflag) {
                         VeVertex *I, *O;
                         VeVertex *N = &to[out++];

                         if (flag) {
                              I = curr;
                              O = prev;
                         }
                         else {
                              I = prev;
                              O = curr;
                         }

                         d[0] = O->clip.v[0] - I->clip.v[0];
                         d[1] = O->clip.v[1] - I->clip.v[1];
                         d[2] = O->clip.v[2] - I->clip.v[2];
                         d[3] = O->clip.v[3] - I->clip.v[3];

                         if (p & 1)
                              t  = (-I->clip.v[pc] - I->clip.v[3]) / (d[3] + d[pc]);
                         else
                              t  = ( I->clip.v[pc] - I->clip.v[3]) / (d[3] - d[pc]);

                         N->clip.v[0] = I->clip.v[0] + t * d[0];
                         N->clip.v[1] = I->clip.v[1] + t * d[1];
                         N->clip.v[2] = I->clip.v[2] + t * d[2];
                         N->clip.v[3] = I->clip.v[3] + t * d[3];

                         N->s = I->s + t * (O->s - I->s);
                         N->t = I->t + t * (O->t - I->t);

                         N->index = -1;
                    }

                    if (flag)
                         to[out++] = from[i];

                    prev  = curr;
                    pflag = flag;
               }

               if (out >= 3) {
                    VeVertex *tmp;

                    tmp   = from;
                    from  = to;
                    to    = tmp;

                    count = out;
               }
               else
                    return 0;
          }
     }

     for (i = 0; i < count; i++)
          output[i] = from[i].index != -1 ? from[i].index : add_vertex( &from[i] );

     return count;
}

static void build_polygon( const int *input, int count )
{
     int        i;
     VeClipMask clipOr  = VE_CLIP_NONE;
     VeClipMask clipAnd = VE_CLIP_ALL;

     /* Combine clipping masks. */
     for (i = 0; i < count; i++) {
          VeClipMask mask = buffer->data[input[i]].clipMask;

          clipOr  |= mask;
          clipAnd &= mask;
     }

     if (clipAnd) {
          /* At least one plane clips them all, the polygon is outside the view volume. */
          return;
     }

     if (clipOr) {
          /* At least one vertex is clipped by at least one plane. */
          int output[count<<2];

          for (i = 2; i < clip_polygon( input, count, clipOr, output ); i++) {
               buffer->indices[buffer->num_indices++] = output[0];
               buffer->indices[buffer->num_indices++] = output[i-1];
               buffer->indices[buffer->num_indices++] = output[i];
          }
     }
     else {
          /* No vertex is clipped. */
          for (i = 2; i < count; i++) {
               buffer->indices[buffer->num_indices++] = buffer->data[input[0]].index;
               buffer->indices[buffer->num_indices++] = buffer->data[input[i-1]].index;
               buffer->indices[buffer->num_indices++] = buffer->data[input[i]].index;
          }
     }
}

#define TRANSFORM_POINT( Q, M, P )                                   \
     Q[0] = M[0] * P[0] + M[4] * P[1] + M[8]  * P[2] + M[12] * P[3]; \
     Q[1] = M[1] * P[0] + M[5] * P[1] + M[9]  * P[2] + M[13] * P[3]; \
     Q[2] = M[2] * P[0] + M[6] * P[1] + M[10] * P[2] + M[14] * P[3]; \
     Q[3] = M[3] * P[0] + M[7] * P[1] + M[11] * P[2] + M[15] * P[3];

static void vbExec( void )
{
     int i;

     /* Make sure the product of modelview and projection matrix is up to date. */
     if (update) {
          matmul4( composite.m, projection.m, modelview.m );

          update = false;
     }

     /* Reset the output buffer. */
     buffer->num_vertices = buffer->num_indices = 0;

     /* Prepare input buffer. */
     for (i = 0; i < buffer->count; i++) {
          VeVertex   *vtx  = &buffer->data[i];
          float      *clip = vtx->clip.v;
          VeClipMask  mask = VE_CLIP_NONE;

          /* Reset output buffer index. */
          vtx->index = -1;

          /* Transform object to clip coordinates using combined modelview and projection matrix. */
          TRANSFORM_POINT( vtx->clip.v, composite.m, vtx->obj.v );

          /* Check each plane of the clipping volume. */
          if (-clip[0] + clip[3] < 0) mask |= VE_CLIP_RIGHT;
          if ( clip[0] + clip[3] < 0) mask |= VE_CLIP_LEFT;
          if (-clip[1] + clip[3] < 0) mask |= VE_CLIP_TOP;
          if ( clip[1] + clip[3] < 0) mask |= VE_CLIP_BOTTOM;
          if (-clip[2] + clip[3] < 0) mask |= VE_CLIP_FAR;
          if ( clip[2] + clip[3] < 0) mask |= VE_CLIP_NEAR;

          /* Keep the clipping test result. */
          vtx->clipMask = mask;

          /* Transform to perspective window coordinates and store in output buffer if no clipping is required. */
          if (!mask)
               vtx->index = add_vertex( vtx );
     }

     /* Build list of indices, fill output buffer with original and/or extra vertices as needed. */
     for (i = 2; i < buffer->count; i += 2) {
          int list[4] = { i - 2, i - 1, i + 1, i };

          build_polygon( list, 4 );
     }

     /* Render the list of built triangles. */
     if (buffer->num_indices > 0)
          primary->TextureTriangles( primary, texture,
                                     buffer->vertices, buffer->indices, buffer->num_indices, DTTF_LIST );
}

static void vbDestroy( void )
{
     free( buffer->indices );
     free( buffer->vertices );
     free( buffer->data );
     free( buffer );
}

/**********************************************************************************************************************/

static void generate_flag( int num, float cycles, float amplitude, float phase )
{
     int   i;
     int   n = num >> 1;
     float m = n - 1;
     float t = cycles * M_PI * 2.0f / m;
     float p = phase  * M_PI * 2.0f;

     vbClear();

     for (i = 0; i < n; i++) {
          float T = i * t + p;
          float R = i * amplitude / m;
          float x = -5.0f + i * 10.0f / m;
          float y = sin( T ) * R  +  sin( 0.27f * T ) * R  +  sin( 0.37f * T ) * R;
          float s = i / m;

          vbAdd( x, y, -5.0f, s, 0.0f );
          vbAdd( x, y,  5.0f, s, 1.0f );
     }
}

static void cleanup( void )
{
     /* Free vertex buffer. */
     if (buffer)
          vbDestroy();

     /* Release the texture. */
     if (texture)
          texture->Release( texture );

     /* Release the video provider. */
     if (videoprovider)
          videoprovider->Release( videoprovider );

     /* Release the font. */
     if (font)
          font->Release( font );

     /* Release the primary surface. */
     if (primary)
          primary->Release( primary );

     /* Release the event buffer. */
     if (event_buffer)
          event_buffer->Release( event_buffer );

     /* Release the main interface. */
     if (dfb)
          dfb->Release( dfb );
}

static void print_usage( void )
{
     printf( "DirectFB Texture Demo\n\n" );
     printf( "Usage: df_texture <file>\n\n" );
}

int directfb_main( int argc, char *argv[] )
{
     DFBResult                 ret = DFB_FAILURE;
     FPSData                   fps;
     int                       num;
     long long                 start;
     DFBFontDescription        fdsc;
     DFBSurfaceDescription     sdsc;
     DFBDataBufferDescription  ddsc;
     IDirectFBDataBuffer      *buffer;
     IDirectFBImageProvider   *provider;

     /* Initialize DirectFB including command line parsing. */
     DFBCHECK(DirectFBInit( &argc, &argv ));

     /* Parse command line. */
     if (argv[1] && !strcmp( argv[1], "--help" )) {
          print_usage();
          return 0;
     }

     /* Create the main interface. */
     DFBCHECK(DirectFBCreate( &dfb ));

     /* Register termination function. */
     atexit( cleanup );

     /* Set the cooperative level to DFSCL_FULLSCREEN for exclusive access to the primary layer. */
     dfb->SetCooperativeLevel( dfb, DFSCL_FULLSCREEN );

     /* Create an event buffer for axis and key events. */
     DFBCHECK(dfb->CreateInputEventBuffer( dfb, DICAPS_ALL, DFB_FALSE, &event_buffer ));

#ifdef HAVE_MT
     /* Set multi-touch configuration. */
     IDirectFBInputDevice *mouse;
     if (dfb->GetInputDevice( dfb, DIDID_MOUSE, &mouse ) == DFB_OK) {
          DFBInputDeviceConfig config;

          config.flags     = DIDCONF_MAX_SLOTS;
          config.max_slots = 2;

          mouse->SetConfiguration( mouse, &config );

          mouse->Release( mouse );
     }
#endif

     /* Check if triple buffering is supported. */
     IDirectFBDisplayLayer *layer;
     if (dfb->GetDisplayLayer( dfb, DLID_PRIMARY, &layer ) == DFB_OK) {
          DFBDisplayLayerConfig      config;
          DFBDisplayLayerConfigFlags ret_failed;

          config.flags      = DLCONF_BUFFERMODE;
          config.buffermode = DLBM_TRIPLE;

          layer->TestConfiguration( layer, &config, &ret_failed );
          if (ret_failed == DLCONF_NONE)
               triple = true;

          layer->Release( layer );
     }

     /* Fill the surface description. */
     sdsc.flags = DSDESC_CAPS;
     sdsc.caps  = DSCAPS_PRIMARY | (triple ? DSCAPS_TRIPLE : DSCAPS_DOUBLE);

     /* Get the primary surface, i.e. the surface of the primary layer. */
     DFBCHECK(dfb->CreateSurface( dfb, &sdsc, &primary ));

     /* Query the size of the primary surface. */
     DFBCHECK(primary->GetSize( primary, &screen_width, &screen_height ));

     /* Load font. */
     fdsc.flags  = DFDESC_HEIGHT;
     fdsc.height = CLAMP( (int) (screen_width / 42.0 / 8) * 8, 8, 96 );

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

     primary->SetFont( primary, font );

     /* Set color for text. */
     primary->SetColor( primary, 0xff, 0xff, 0xff, 0xff );

     /* Load the texture. */
     if (argc > 1) {
          ret = dfb->CreateVideoProvider( dfb, argv[1], &videoprovider );
          if (ret == DFB_OK) {
               videoprovider->GetSurfaceDescription( videoprovider, &sdsc );
               DFBCHECK(primary->GetPixelFormat( primary, &sdsc.pixelformat ));
               DFBCHECK(primary->GetColorSpace( primary, &sdsc.colorspace ));
               DFBCHECK(dfb->CreateSurface( dfb, &sdsc, &texture ));
               videoprovider->SetPlaybackFlags( videoprovider, DVPLAY_LOOPING );
               videoprovider->PlayTo( videoprovider, texture, NULL, NULL, NULL );
          }
     }

     if (ret != DFB_OK) {
          if (argc > 1)
               ret = dfb->CreateImageProvider( dfb, argv[1], &provider );

          if (ret != DFB_OK) {
#ifdef USE_IMAGE_HEADERS
               ddsc.flags         = DBDESC_MEMORY;
               ddsc.memory.data   = GET_IMAGEDATA( texture );
               ddsc.memory.length = GET_IMAGESIZE( texture );
#else
               ddsc.flags         = DBDESC_FILE;
               ddsc.file          = GET_IMAGEFILE( texture );
#endif
               DFBCHECK(dfb->CreateDataBuffer( dfb, &ddsc, &buffer ));
               DFBCHECK(buffer->CreateImageProvider( buffer, &provider ));
          }

          provider->GetSurfaceDescription( provider, &sdsc );
          DFBCHECK(primary->GetPixelFormat( primary, &sdsc.pixelformat ));
          DFBCHECK(dfb->CreateSurface( dfb, &sdsc, &texture ));
          provider->RenderTo( provider, texture, NULL );
          provider->Release( provider );
     }

     /* Calculate the number of vertices used. */
     num = MAX( screen_width, screen_height ) / 16 + 16;

     /* Allocate vertex buffer. */
     vbNew( num );

     /* Setup initial transformation. */
     veInit( 70.0f, (float) screen_width / screen_height, 1.0f, 20.0f );

     /* Move model into clipping volume. */
     veTranslate( 0, 0, -10 );

     /* Rotate a bit around the X axis. */
     veRotate( -0.7f, 1.0f, 0.0f, 0.0f );

     /* Initialize time base. */
     start = direct_clock_get_millis();

     /* Initialize FPS stuff. */
     fps_init( &fps );

     /* Main loop. */
     while (1) {
          DFBInputEvent evt;
          float         dt = (direct_clock_get_millis() - start) / 1000.0f;

          /* Clear with black. */
          primary->Clear( primary, 0, 0, 0, 0 );

          /* Fill the vertex buffer. */
          generate_flag( num, 2.5f, 1.0f, -dt );

          /* Draw the FPS in upper left corner. */
          primary->DrawString( primary, fps.fps_string, -1, 50, 50, DSTF_TOPLEFT );

          /* Render vertex buffer content. */
          vbExec();

          /* Flip display. */
          primary->Flip( primary, NULL, DSFLIP_WAITFORSYNC );

          /* FPS calculations. */
          fps_count( &fps, 1000 );

          /* Check for new events. */
          while (event_buffer->GetEvent( event_buffer, DFB_EVENT(&evt) ) == DFB_OK) {
               if (evt.type == DIET_AXISMOTION && (evt.flags & DIEF_AXISREL)) {
                    switch (evt.axis) {
                         case DIAI_X:
                              if (evt.buttons & DIBM_LEFT)
                                   veTranslate( evt.axisrel * 0.01f, 0.0f, 0.0f );

                              if (evt.buttons & DIBM_MIDDLE)
                                   veRotate( evt.axisrel * 0.01f, 0.0f, 1.0f, 0.0f );

                              if (evt.buttons & DIBM_RIGHT)
                                   veScale( 1.0f + evt.axisrel * 0.01f, 1.0f, 1.0f );

                              break;

                         case DIAI_Y:
                              if (evt.buttons & DIBM_LEFT)
                                   veTranslate( 0.0f, 0.0f, evt.axisrel * 0.01f );

                              if (evt.buttons & DIBM_MIDDLE)
                                   veRotate( -evt.axisrel * 0.01f, 1.0f, 0.0f, 0.0f );

                              if (evt.buttons & DIBM_RIGHT)
                                   veScale( 1.0f, 1.0f + evt.axisrel * 0.01f, 1.0f );

                              break;

                         default:
                              break;
                    }
               }
               else if (evt.type == DIET_AXISMOTION && (evt.flags & DIEF_AXISABS)) {
                    switch (evt.axis) {
                         case DIAI_X:
                              if (evt.buttons & DIBM_LEFT) {
                                   if (axisabs_x[0] >= 0 && axisabs_x[1] >= 0)
                                        veScale( 1.0f + SIGN( ABS( evt.axisabs - axisabs_x[!DFB_EVENT_SLOT_ID(evt)] ) -
                                                              ABS( axisabs_x[1] - axisabs_x[0] ) ) * 0.01f,
                                                 1.0f, 1.0f );
                                   else if ((axisabs_x[0] >= 0 && !DFB_EVENT_SLOT_ID(evt)) ||
                                            (axisabs_x[1] >= 0 &&  DFB_EVENT_SLOT_ID(evt)))
                                        veTranslate( (evt.axisabs - axisabs_x[DFB_EVENT_SLOT_ID(evt)]) * 0.01f,
                                                     0.0f, 0.0f );

                                   axisabs_x[DFB_EVENT_SLOT_ID(evt)] = evt.axisabs;
                              }

                              break;

                         case DIAI_Y:
                              if (evt.buttons & DIBM_LEFT) {
                                   if (axisabs_y[0] >= 0 && axisabs_y[1] >= 0)
                                        veScale( 1.0f,
                                                 1.0f + SIGN( ABS( evt.axisabs - axisabs_y[!DFB_EVENT_SLOT_ID(evt)] ) -
                                                              ABS( axisabs_y[1] - axisabs_y[0] ) ) * 0.01f, 1.0f );
                                   else if ((axisabs_y[0] >= 0 && !DFB_EVENT_SLOT_ID(evt)) ||
                                            (axisabs_y[1] >= 0 &&  DFB_EVENT_SLOT_ID(evt)))
                                        veTranslate( 0.0f, 0.0f,
                                                     (evt.axisabs - axisabs_y[DFB_EVENT_SLOT_ID(evt)]) * 0.01f );

                                   axisabs_y[DFB_EVENT_SLOT_ID(evt)] = evt.axisabs;
                              }

                              break;

                         default:
                              break;
                    }
               }
               else if (evt.type == DIET_BUTTONRELEASE) {
                    axisabs_x[DFB_EVENT_SLOT_ID(evt)] = -1;
                    axisabs_y[DFB_EVENT_SLOT_ID(evt)] = -1;
               }
               else if (evt.buttons & DIBM_LEFT) {
                    if (event_buffer->WaitForEventWithTimeout( event_buffer, 2, 0 ) == DFB_TIMEOUT) {
                         return 42;
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
                              return 42;

                         default:
                              break;
                    }
               }
          }
     }

     /* Shouldn't reach this. */
     return 0;
}

DIRECTFB_MAIN()
