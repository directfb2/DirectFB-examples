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
#include <fusionsound.h>
#include <math.h>

/* macro for a safe call to FusionSound functions */
#define FSCHECK(x)                                                    \
     do {                                                             \
          DirectResult ret = x;                                       \
          if (ret != DR_OK) {                                         \
               fprintf( stderr, "%s <%d>:\n\t", __FILE__, __LINE__ ); \
               FusionSoundErrorFatal( #x, ret );                      \
          }                                                           \
     } while (0)

/* FusionSound interfaces */
IFusionSound       *sound  = NULL;
IFusionSoundStream *stream = NULL;

static void cleanup()
{
     /* Release the sound stream. */
     if (stream)
          stream->Release( stream );

     /* Release the main interface. */
     if (sound)
          sound->Release( sound );
}

int main( int argc, char *argv[] )
{
     int                 i;
     FSStreamDescription dsc;
     s16                 buf[16384];

     /* Initialize FusionSound including command line parsing. */
     FSCHECK(FusionSoundInit( &argc, &argv ));

     /* Create the main interface. */
     FSCHECK(FusionSoundCreate( &sound ));

     /* Register termination function. */
     atexit( cleanup );

     /* Fill out the description of the sound stream and create it. */
     dsc.flags        = FSSDF_BUFFERSIZE | FSSDF_CHANNELS | FSSDF_SAMPLEFORMAT | FSSDF_SAMPLERATE;
     dsc.buffersize   = sizeof(buf);
     dsc.channels     = 1;
     dsc.sampleformat = FSSF_S16;
     dsc.samplerate   = 44100;

     FSCHECK(sound->CreateStream( sound, &dsc, &stream ));

     /* Fill the ring buffer. */
     for (i = 0; i < D_ARRAY_SIZE(buf); i++)
          buf[i] = 10000 * sinf( (float) i * i / (D_ARRAY_SIZE(buf) * D_ARRAY_SIZE(buf)) * M_PI * 200 );

     for (i = 0; i < 8; i++)
          FSCHECK(stream->Write( stream, buf, D_ARRAY_SIZE(buf) ));

     /* Wait until end of stream. */
     stream->Wait( stream, 0 );

     return 0;
}
