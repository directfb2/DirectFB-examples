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

#include <fusionsound.h>

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
IFusionSound              *sound    = NULL;
IFusionSoundMusicProvider *provider = NULL;
IFusionSoundBuffer        *buffer   = NULL;

static void cleanup()
{
     /* Release the sound buffer. */
     if (buffer)
          buffer->Release( buffer );

     /* Release the sound provider. */
     if (provider)
          provider->Release( provider );

     /* Release the main interface. */
     if (sound)
          sound->Release( sound );
}

int main( int argc, char *argv[] )
{
     FSBufferDescription dsc;

     /* Initialize FusionSound including command line parsing. */
     FSCHECK(FusionSoundInit( &argc, &argv ));

     /* Create the main interface. */
     FSCHECK(FusionSoundCreate( &sound ));

     /* Register termination function. */
     atexit( cleanup );

     /* Load the sound buffer. */
     FSCHECK(sound->CreateMusicProvider( sound, DATADIR"/test.wav", &provider ));
     provider->GetBufferDescription( provider, &dsc );
     FSCHECK(sound->CreateBuffer( sound, &dsc, &buffer ));
     provider->PlayToBuffer( provider, buffer, NULL, NULL );

     /* Start playing. */
     buffer->Play( buffer, FSPLAY_NOFX );

     sleep( 4 );

     return 0;
}
