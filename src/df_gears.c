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
#ifdef OPENGL_H
#include <directfbgl.h>
#include OPENGL_H
#include <math.h>

#include "util.h"

#ifdef USE_FONT_HEADERS
#include "decker.h"
#endif

/* DirectFB interfaces */
static IDirectFB            *dfb          = NULL;
static IDirectFBEventBuffer *event_buffer = NULL;
static IDirectFBSurface     *primary      = NULL;
static IDirectFBFont        *font         = NULL;

/* screen width and height */
static int screen_width, screen_height;

/**********************************************************************************************************************/

static void identity( float *a )
{
     float m[16] = {
          1, 0, 0, 0,
          0, 1, 0, 0,
          0, 0, 1, 0,
          0, 0, 0, 1,
     };

     memcpy( a, m, sizeof(m) );
}

static void multiply( float *a, const float *b )
{
     float m[16];
     int   i, j;
     div_t d;

     for (i = 0; i < 16; i++) {
          m[i] = 0;
          d = div( i, 4 );
          for (j = 0; j < 4; j++)
               m[i] += (a + d.rem)[j*4] * (b + d.quot * 4)[j];
     }

     memcpy( a, m, sizeof(m) );
}

static void translate( float *a, float tx, float ty, float tz )
{
     float m[16] = {
           1,  0,  0, 0,
           0,  1,  0, 0,
           0,  0,  1, 0,
          tx, ty, tz, 1
     };

     multiply( a, m );
}

static void rotate( float *a, float r, float ux, float uy, float uz )
{
     float s, c;

     s = sinf( r * M_PI / 180 );
     c = cosf( r * M_PI / 180 );

     float m[16] = {
               ux * ux * (1 - c) + c, uy * ux * (1 - c) + uz * s, ux * uz * (1 - c) - uy * s, 0,
          ux * uy * (1 - c) - uz * s,      uy * uy * (1 - c) + c, uy * uz * (1 - c) + ux * s, 0,
          ux * uz * (1 - c) + uy * s, uy * uz * (1 - c) - ux * s,      uz * uz * (1 - c) + c, 0,
                                   0,                          0,                          0, 1
     };

     multiply( a, m );
}

static void transpose( float *a )
{
     float m[16] = {
          a[0], a[4], a[8],  a[12],
          a[1], a[5], a[9],  a[13],
          a[2], a[6], a[10], a[14],
          a[3], a[7], a[11], a[15]
     };

     memcpy( a, m, sizeof(m) );
}

static void invert( float *a )
{
     float m[16] = {
               1,      0,      0, 0,
               0,      1,      0, 0,
               0,      0,      1, 0,
          -a[12], -a[13], -a[14], 1,
     };

     a[12] = a[13] = a[14] = 0;
     transpose( a );

     multiply( a, m );
}

/**********************************************************************************************************************/

typedef float Vertex[6];

typedef struct {
     int begin;
     int count;
} Strip;

typedef struct {
     int     nvertices;
     Vertex *vertices;
     int     nstrips;
     Strip  *strips;
     GLuint  vbo;
} Gear;

typedef enum {
     GEAR_RED,
     GEAR_GREEN,
     GEAR_BLUE,
     NUM_GEARS
} GearIndex;

typedef struct {
     GLuint  program;
     Gear   *gear[3];
     float   projection[16];
     float   view[16];
} Gears;

Gears gears;

static void create_gear( GearIndex id, float inner, float outer, float width, int teeth, float tooth_depth )
{
     Gear  *gear;
     float  r0, r1, r2, da, a1, ai, s[5], c[5];
     int    i, j;
     float  n[3];
     int    k = 0;

     gear = D_CALLOC( 1, sizeof(Gear) );

     gear->nvertices = 0;
     gear->vertices = D_CALLOC( 34 * teeth, sizeof(Vertex) );

     gear->nstrips = 7 * teeth;
     gear->strips = D_CALLOC( gear->nstrips, sizeof(Strip) );

     r0 = inner;
     r1 = outer - tooth_depth / 2;
     r2 = outer + tooth_depth / 2;
     a1 = 2 * M_PI / teeth;
     da = a1 / 4;

     #define normal(nx, ny, nz) \
          n[0] = nx; \
          n[1] = ny; \
          n[2] = nz;

     #define vertex(x, y, z) \
          gear->vertices[gear->nvertices][0] = x; \
          gear->vertices[gear->nvertices][1] = y; \
          gear->vertices[gear->nvertices][2] = z; \
          gear->vertices[gear->nvertices][3] = n[0]; \
          gear->vertices[gear->nvertices][4] = n[1]; \
          gear->vertices[gear->nvertices][5] = n[2]; \
          gear->nvertices++;

     for (i = 0; i < teeth; i++) {
          ai = i * a1;
          for (j = 0; j < 5; j++) {
               s[j] = sinf( ai + j * da );
               c[j] = cosf( ai + j * da );
          }

          /* front face begin */
          gear->strips[k].begin = gear->nvertices;
          /* front face normal */
          normal(0, 0, 1);
          /* front face vertices */
          vertex(r2 * c[1], r2 * s[1], width / 2);
          vertex(r2 * c[2], r2 * s[2], width / 2);
          vertex(r1 * c[0], r1 * s[0], width / 2);
          vertex(r1 * c[3], r1 * s[3], width / 2);
          vertex(r0 * c[0], r0 * s[0], width / 2);
          vertex(r1 * c[4], r1 * s[4], width / 2);
          vertex(r0 * c[4], r0 * s[4], width / 2);
          /* front face end */
          gear->strips[k].count = 7;
          k++;

          /* back face begin */
          gear->strips[k].begin = gear->nvertices;
          /* back face normal */
          normal(0, 0, -1);
          /* back face vertices */
          vertex(r2 * c[1], r2 * s[1], -width / 2);
          vertex(r2 * c[2], r2 * s[2], -width / 2);
          vertex(r1 * c[0], r1 * s[0], -width / 2);
          vertex(r1 * c[3], r1 * s[3], -width / 2);
          vertex(r0 * c[0], r0 * s[0], -width / 2);
          vertex(r1 * c[4], r1 * s[4], -width / 2);
          vertex(r0 * c[4], r0 * s[4], -width / 2);
          /* back face end */
          gear->strips[k].count = 7;
          k++;

          /* first outward face begin */
          gear->strips[k].begin = gear->nvertices;
          /* first outward face normal */
          normal(r2 * s[1] - r1 * s[0], r1 * c[0] - r2 * c[1], 0);
          /* first outward face vertices */
          vertex(r1 * c[0], r1 * s[0],  width / 2);
          vertex(r1 * c[0], r1 * s[0], -width / 2);
          vertex(r2 * c[1], r2 * s[1],  width / 2);
          vertex(r2 * c[1], r2 * s[1], -width / 2);
          /* first outward face end */
          gear->strips[k].count = 4;
          k++;

          /* second outward face begin */
          gear->strips[k].begin = gear->nvertices;
          /* second outward face normal */
          normal(s[2] - s[1], c[1] - c[2], 0);
          /* second outward face vertices */
          vertex(r2 * c[1], r2 * s[1],  width / 2);
          vertex(r2 * c[1], r2 * s[1], -width / 2);
          vertex(r2 * c[2], r2 * s[2],  width / 2);
          vertex(r2 * c[2], r2 * s[2], -width / 2);
          /* second outward face end */
          gear->strips[k].count = 4;
          k++;

          /* third outward face begin */
          gear->strips[k].begin = gear->nvertices;
          /* third outward face normal */
          normal(r1 * s[3] - r2 * s[2], r2 * c[2] - r1 * c[3], 0);
          /* third outward face vertices */
          vertex(r2 * c[2], r2 * s[2],  width / 2);
          vertex(r2 * c[2], r2 * s[2], -width / 2);
          vertex(r1 * c[3], r1 * s[3],  width / 2);
          vertex(r1 * c[3], r1 * s[3], -width / 2);
          /* third outward face end */
          gear->strips[k].count = 4;
          k++;

          /* fourth outward face begin */
          gear->strips[k].begin = gear->nvertices;
          /* fourth outward face normal */
          normal(s[4] - s[3], c[3] - c[4], 0);
          /* fourth outward face vertices */
          vertex(r1 * c[3], r1 * s[3],  width / 2);
          vertex(r1 * c[3], r1 * s[3], -width / 2);
          vertex(r1 * c[4], r1 * s[4],  width / 2);
          vertex(r1 * c[4], r1 * s[4], -width / 2);
          /* fourth outward face end */
          gear->strips[k].count = 4;
          k++;

          /* inside face begin */
          gear->strips[k].begin = gear->nvertices;
          /* inside face normal */
          normal(s[0] - s[4], c[4] - c[0], 0);
          /* inside face vertices */
          vertex(r0 * c[0], r0 * s[0],  width / 2);
          vertex(r0 * c[0], r0 * s[0], -width / 2);
          vertex(r0 * c[4], r0 * s[4],  width / 2);
          vertex(r0 * c[4], r0 * s[4], -width / 2);
          /* inside face end */
          gear->strips[k].count = 4;
          k++;
     }

     /* vertex buffer object */
     glGenBuffers( 1, &gear->vbo );
     glBindBuffer( GL_ARRAY_BUFFER, gear->vbo );
     glBufferData( GL_ARRAY_BUFFER, gear->nvertices * sizeof(Vertex), gear->vertices, GL_STATIC_DRAW );

     gears.gear[id] = gear;
}

#ifdef __IDIRECTFBGL_PORTABLEGL_H__
typedef struct Uniforms {
     vec4 LightPos;
     mat4 ModelViewProjectionMatrix;
     mat4 NormalMatrix;
     vec4 Color;
} Uniforms;
#endif

static void draw_gear( GearIndex id, float model_tx, float model_ty, float model_rz, const float *color )
{
     Gear        *gear   = gears.gear[id];
     const float  pos[4] = { 5.0, 5.0, 10.0, 0.0 };
     int          k;
     float        MVP[16], N[16];

     memcpy( N, gears.view, sizeof(N) );
     translate( N, model_tx, model_ty, 0 );
     rotate( N, model_rz, 0, 0, 1 );
     memcpy( MVP, gears.projection, sizeof(MVP) );
     multiply( MVP, N );
     invert( N );
     transpose( N );

#ifdef __IDIRECTFBGL_PORTABLEGL_H__
     Uniforms uniforms;

     memcpy( &uniforms.LightPos,                  pos,   sizeof(vec4) );
     memcpy( &uniforms.ModelViewProjectionMatrix, MVP,   sizeof(mat4) );
     memcpy( &uniforms.NormalMatrix,              N,     sizeof(mat4) );
     memcpy( &uniforms.Color,                     color, sizeof(vec4) );

     pglSetUniform( &uniforms );
#else
     glUniform4fv      ( glGetUniformLocation( gears.program, "u_LightPos" ),                  1,           pos );
     glUniformMatrix4fv( glGetUniformLocation( gears.program, "u_ModelViewProjectionMatrix" ), 1, GL_FALSE, MVP );
     glUniformMatrix4fv( glGetUniformLocation( gears.program, "u_NormalMatrix" ),              1, GL_FALSE, N );
     glUniform4fv      ( glGetUniformLocation( gears.program, "u_Color" ),                     1,           color );
#endif

     glBindBuffer( GL_ARRAY_BUFFER, gear->vbo );

     glVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), NULL );
     glVertexAttribPointer( 1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const float*) NULL + 3 );

     glEnableVertexAttribArray( 0 );
     glEnableVertexAttribArray( 1 );

     for (k = 0; k < gear->nstrips; k++)
          glDrawArrays( GL_TRIANGLE_STRIP, gear->strips[k].begin, gear->strips[k].count );

     glDisableVertexAttribArray( 1 );
     glDisableVertexAttribArray( 0 );
}

static void delete_gear( GearIndex id )
{
     Gear *gear = gears.gear[id];

     glDeleteBuffers( 1, &gear->vbo );

     D_FREE( gear->strips );
     D_FREE( gear->vertices );
     D_FREE( gear );
}

/**********************************************************************************************************************/

#ifdef __IDIRECTFBGL_PORTABLEGL_H__
#define POSITION 0
#define NORMAL   1
#define COLOR    0

void vertex_shader( float *vs_output, void *vertex_attribs, Shader_Builtins *builtins, void *uniforms )
{
     vec4 *v = (vec4*) vs_output;
     vec4 *a = vertex_attribs;
     Uniforms *u = uniforms;
     builtins->gl_Position = mult_mat4_vec4( u->ModelViewProjectionMatrix, a[POSITION] );
     vec4 l = { 0.2, 0.2, 0.2, 1 };
     vec3 L = { u->LightPos.x, u->LightPos.y, u->LightPos.z };
     vec4 n = mult_mat4_vec4( u->NormalMatrix, a[NORMAL] );
     vec3 N = { n.x, n.y, n.z };
     float dot = dot_vec3s( norm_vec3( L ), norm_vec3( N ) );
     v[COLOR] = add_vec4s( mult_vec4s( u->Color, l ), scale_vec4( u->Color, MAX( dot, 0.0 ) ) );
}

void fragment_shader( float *fs_input, Shader_Builtins *builtins, void *uniforms )
{
     vec4 *v = (vec4*) fs_input;
     builtins->gl_FragColor = v[COLOR];
}
#else
static const GLchar *vertShaderSource =
     "varying vec4 v_Color;\n"
     "attribute vec3 a_Position;\n"
     "attribute vec3 a_Normal;\n"
     "uniform vec4 u_LightPos;\n"
     "uniform mat4 u_ModelViewProjectionMatrix;\n"
     "uniform mat4 u_NormalMatrix;\n"
     "uniform vec4 u_Color;\n"
     "void main()\n"
     "{\n"
     "gl_Position = u_ModelViewProjectionMatrix * vec4( a_Position, 1 );\n"
     "vec4 l = vec4( 0.2, 0.2, 0.2, 1 );\n"
     "vec3 L = u_LightPos.xyz;\n"
     "vec4 n = u_NormalMatrix * vec4( a_Normal, 1 );\n"
     "vec3 N = n.xyz;\n"
     "float dot = dot( normalize( L ), normalize( N ) );\n"
     "v_Color = u_Color * l + u_Color * max( dot, 0.0 );\n"
     "}";

static const GLchar *fragShaderSource =
     "varying vec4 v_Color;\n"
     "void main()\n"
     "{\n"
     "gl_FragColor = v_Color;\n"
     "}\n";
#endif

void gears_init( void )
{
     const float zNear = 5;
     const float zFar  = 60;

#ifdef __IDIRECTFBGL_PORTABLEGL_H__
     GLenum interpolation[3] = { SMOOTH, SMOOTH, SMOOTH };

     gears.program = pglCreateProgram( vertex_shader, fragment_shader, 3, interpolation, GL_FALSE );
#else
     GLuint vertShader, fragShader;

     gears.program = glCreateProgram();

     /* vertex shader */
     vertShader = glCreateShader( GL_VERTEX_SHADER );
     glShaderSource( vertShader, 1, &vertShaderSource, NULL );
     glCompileShader( vertShader );
     glAttachShader( gears.program, vertShader );
     glDeleteShader( vertShader );

     /* fragment shader */
     fragShader = glCreateShader( GL_FRAGMENT_SHADER );
     glShaderSource( fragShader, 1, &fragShaderSource, NULL );
     glCompileShader( fragShader );
     glAttachShader( gears.program, fragShader );
     glDeleteShader( fragShader );

     /* vertex attribs */
     glBindAttribLocation( gears.program, 0, "a_Position" );
     glBindAttribLocation( gears.program, 1, "a_Normal" );

     /* link program */
     glLinkProgram( gears.program );
#endif

     /* enable depth testing */
     glEnable( GL_DEPTH_TEST );

     /* use program */
     glUseProgram( gears.program );

     /* set clear values */
     glClearColor( 0, 0, 0, 1 );

     /* set viewport */
     glViewport( 0, 0, screen_width, screen_height );

     /* set projection matrix */
     memset( gears.projection, 0, sizeof(gears.projection) );
     gears.projection[0]  = zNear;
     gears.projection[5]  = (float) screen_width / screen_height * zNear;
     gears.projection[10] = -(zFar + zNear) / (zFar - zNear);
     gears.projection[11] = -1;
     gears.projection[14] = -2 * zFar * zNear / (zFar - zNear);

     create_gear( GEAR_RED,   1.0, 4.0, 1.0, 20, 0.7 );
     create_gear( GEAR_GREEN, 0.5, 2.0, 2.0, 10, 0.7 );
     create_gear( GEAR_BLUE,  1.3, 2.0, 0.5, 10, 0.7 );
}

static void gears_draw( float view_tz, float view_rx, float view_ry, float model_rz )
{
     const float red[4]   = { 0.8, 0.1, 0.0, 1.0 };
     const float green[4] = { 0.0, 0.8, 0.2, 1.0 };
     const float blue[4]  = { 0.2, 0.2, 1.0, 1.0 };

     glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

     /* set view matrix */
     identity( gears.view );
     translate( gears.view, 0, 0, view_tz );
     rotate( gears.view, view_rx, 1, 0, 0 );
     rotate( gears.view, view_ry, 0, 1, 0 );

     draw_gear( GEAR_RED,   -3.0, -2.0,      model_rz     , red );
     draw_gear( GEAR_GREEN,  3.1, -2.0, -2 * model_rz - 9 , green );
     draw_gear( GEAR_BLUE,  -3.1,  4.2, -2 * model_rz - 25, blue );
}

static void gears_term( void )
{
     int n;

     for (n = 0; n < NUM_GEARS; n++)
          delete_gear( n );

     glDeleteProgram( gears.program );
}

/**********************************************************************************************************************/

static void dfb_shutdown( void )
{
     if (font)         font->Release( font );
     if (primary)      primary->Release( primary );
     if (event_buffer) event_buffer->Release( event_buffer );
     if (dfb)          dfb->Release( dfb );
}

int main( int argc, char *argv[] )
{
     FPSData                   fps;
     long long                 start;
     DFBFontDescription        fdsc;
     DFBSurfaceDescription     sdsc;
     DFBDataBufferDescription  ddsc;
     IDirectFBDataBuffer      *buffer;
     IDirectFBGL              *gl;
     float                     model_rz;
     float                     view_rx =  20.0;
     float                     view_ry =  30.0;
     float                     view_tz = -40.0;
     int                       quit    = 0;

     /* initialize DirectFB including command line parsing */
     DFBCHECK(DirectFBInit( &argc, &argv ));

     /* create the main interface */
     DFBCHECK(DirectFBCreate( &dfb ));

     /* register termination function */
     atexit( dfb_shutdown );

     /* set the cooperative level to DFSCL_FULLSCREEN for exclusive access to the primary layer */
     dfb->SetCooperativeLevel( dfb, DFSCL_FULLSCREEN );

     /* create an event buffer for key events */
     DFBCHECK(dfb->CreateInputEventBuffer( dfb, DICAPS_BUTTONS | DICAPS_KEYS, DFB_FALSE, &event_buffer ));

     /* fill the surface description */
     sdsc.flags = DSDESC_CAPS;
     sdsc.caps  = DSCAPS_PRIMARY | DSCAPS_DOUBLE | DSCAPS_DEPTH;

     /* get the primary surface, i.e. the surface of the primary layer */
     DFBCHECK(dfb->CreateSurface( dfb, &sdsc, &primary ));

     /* query the size of the primary surface */
     DFBCHECK(primary->GetSize( primary, &screen_width, &screen_height ));

     /* load font */
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

     /* set color for text */
     primary->SetColor( primary, 0xff, 0xff, 0xff, 0xff );

     /* get OpenGL context */
     DFBCHECK(primary->GetGL( primary, &gl ));

     /* set current OpenGL context */
     gl->Lock( gl );

     /* initialize gears */
     gears_init();

     /* initialize time base */
     start = direct_clock_get_millis();

     /* initialize fps stuff */
     fps_init( &fps );

     /* main loop */
     while (!quit) {
          DFBInputEvent evt;
          float         dt = (direct_clock_get_millis() - start) / 1000.0f;

          /* draw gears */
          model_rz = fmod( 15 * dt, 360 );
          gears_draw( view_tz, view_rx, view_ry, model_rz );

          /* draw the fps in upper right corner */
          primary->DrawString( primary, fps.fps_string, -1, screen_width - 5, 5, DSTF_TOPRIGHT );

          /* flip display */
          primary->Flip( primary, NULL, DSFLIP_NONE );

          /* fps calculations */
          fps_count( &fps, 2000 );

          /* process event buffer */
          while (event_buffer->GetEvent( event_buffer, DFB_EVENT(&evt) ) == DFB_OK) {
               if (evt.buttons & DIBM_LEFT) {
                    if (event_buffer->WaitForEventWithTimeout( event_buffer, 2, 0 ) == DFB_TIMEOUT) {
                         /* quit main loop */
                         quit = 1;
                    }
               }
               else if (evt.type == DIET_KEYPRESS) {
                    switch (evt.key_symbol) {
                         case DIKS_ESCAPE:
                         case DIKS_SMALL_Q:
                         case DIKS_STOP:
                         case DIKS_EXIT:
                              /* quit main loop */
                              quit = 1;
                              break;
                         case DIKS_CURSOR_UP:
                              view_rx += 5.0;
                              break;
                         case DIKS_CURSOR_DOWN:
                              view_rx -= 5.0;
                              break;
                         case DIKS_CURSOR_LEFT:
                              view_ry += 5.0;
                              break;
                         case DIKS_CURSOR_RIGHT:
                              view_ry -= 5.0;
                              break;
                         case DIKS_PAGE_DOWN:
                              view_tz -= -5.0;
                              break;
                         case DIKS_PAGE_UP:
                              view_tz += -5.0;
                              break;
                         default:
                              break;
                    }
               }
          }
     }

     /* terminate gears */
     gears_term();

     /* release OpenGL context */
     gl->Unlock( gl );
     gl->Release( gl );

     return 42;
}
#else
int main( int argc, char *argv[] )
{
     printf( "No OpenGL support\n" );
     exit( 1 );
}
#endif
