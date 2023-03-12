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
IFusionSound              *sound    = NULL;
IFusionSoundMusicProvider *provider = NULL;
IFusionSoundBuffer        *buffer   = NULL;
IFusionSoundPlayback      *playback = NULL;

#define BEGIN_TEST(name)                                   \
     FSCHECK(buffer->CreatePlayback( buffer, &playback )); \
     sleep( 2 );                                           \
     fprintf( stderr, "Testing %-30s", name );

#define END_TEST()                                         \
     fprintf( stderr, "OK\n" );                            \
     playback->Release( playback );                        \
     playback = NULL

static void
test_simple_playback( IFusionSoundBuffer *buffer )
{
     BEGIN_TEST( "Simple Playback" );

     FSCHECK(playback->Start( playback, 0, 0 ));

     FSCHECK(playback->Wait( playback ));

     END_TEST();
}

static void
test_positioned_playback( IFusionSoundBuffer *buffer )
{
     FSBufferDescription desc;

     BEGIN_TEST( "Positioned Playback" );

     FSCHECK(buffer->GetDescription( buffer, &desc ));

     FSCHECK(playback->Start( playback, desc.length * 1 / 6, desc.length * 2 / 3 ));

     FSCHECK(playback->Wait( playback ));

     sleep( 1 );

     FSCHECK(playback->Start( playback, desc.length * 3 / 4, desc.length * 1 / 3 ));

     FSCHECK(playback->Wait( playback ));

     END_TEST();
}

static void
test_looping_playback( IFusionSoundBuffer *buffer )
{
     BEGIN_TEST( "Looping Playback" );

     FSCHECK(playback->Start( playback, 0, -1 ));

     sleep( 8 );

     FSCHECK(playback->Stop( playback ));

     END_TEST();
}

static void
test_stop_continue_playback( IFusionSoundBuffer *buffer )
{
     int i;

     BEGIN_TEST( "Stop/Continue Playback" );

     FSCHECK(playback->Start( playback, 0, -1 ));

     for (i = 0; i < 8; i++) {
          usleep( 500000 );

          FSCHECK(playback->Stop( playback ));

          usleep( 200000 );

          FSCHECK(playback->Continue( playback ));
     }

     END_TEST();
}

static void
test_volume_level( IFusionSoundBuffer *buffer )
{
     int i;

     BEGIN_TEST( "Volume Level" );

     FSCHECK(playback->Start( playback, 0, -1 ));

     for (i = 0; i < 60; i++) {
          FSCHECK(playback->SetVolume( playback, sin( i / 3.0) / 3.0 + 0.6 ));

          usleep( 100000 );
     }

     END_TEST();
}

static void
test_pan_value( IFusionSoundBuffer *buffer )
{
     int i;

     BEGIN_TEST( "Pan Value" );

     FSCHECK(playback->Start( playback, 0, -1 ));

     for (i = 0; i < 30; i++) {
          FSCHECK(playback->SetPan( playback, sin( i / 3.0 ) ));

          usleep( 200000 );
     }

     END_TEST();
}

static void
test_pitch_value( IFusionSoundBuffer *buffer )
{
     int i;

     BEGIN_TEST( "Pitch Value" );

     FSCHECK(playback->Start( playback, 0, -1 ));

     for (i = 500; i < 1500; i++) {
          FSCHECK(playback->SetPitch( playback, i / 1000.0f ));

          usleep( 10000 );
     }

     END_TEST();
}

static void cleanup()
{
     /* Release the advanced playback. */
     if (playback)
          playback->Release( playback );

     /* Release the sound buffer. */
     if (buffer)
          buffer->Release( buffer );

     /* Release the music provider. */
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

     /* Running tests. */
     test_simple_playback( buffer );
     test_positioned_playback( buffer );
     test_looping_playback( buffer );
     test_stop_continue_playback( buffer );
     test_volume_level( buffer );
     test_pan_value( buffer );
     test_pitch_value( buffer );

     return 0;
}
