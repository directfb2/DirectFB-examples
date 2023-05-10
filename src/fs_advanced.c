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
IFusionSoundStream        *stream   = NULL;
IFusionSoundPlayback      *playback = NULL;

typedef enum {
     SOUND_BUFFER,
     SOUND_STREAM
} SoundType;

#define BEGIN_TEST(type,name)                                   \
     if (type == SOUND_BUFFER)                                  \
          FSCHECK(buffer->CreatePlayback( buffer, &playback )); \
     else if (type == SOUND_STREAM)                             \
          FSCHECK(stream->GetPlayback( stream, &playback ));    \
     sleep( 2 );                                                \
     fprintf( stderr, "Testing %-30s", name );

#define END_TEST()                                              \
     fprintf( stderr, "OK\n" );                                 \
     playback->Release( playback );                             \
     playback = NULL

static void test_start_simple_playback()
{
     BEGIN_TEST( SOUND_BUFFER, "Simple Playback" );

     FSCHECK(playback->Start( playback, 0, 0 ));

     FSCHECK(playback->Wait( playback ));

     END_TEST();
}

static void test_start_positioned_playback()
{
     FSBufferDescription desc;

     BEGIN_TEST( SOUND_BUFFER, "Positioned Playback" );

     FSCHECK(buffer->GetDescription( buffer, &desc ));

     FSCHECK(playback->Start( playback, desc.length * 1 / 6, desc.length * 2 / 3 ));

     FSCHECK(playback->Wait( playback ));

     sleep( 1 );

     FSCHECK(playback->Start( playback, desc.length * 3 / 4, desc.length * 1 / 3 ));

     FSCHECK(playback->Wait( playback ));

     END_TEST();
}

static void test_start_looping_playback()
{
     BEGIN_TEST( SOUND_BUFFER, "Looping Playback" );

     FSCHECK(playback->Start( playback, 0, -1 ));

     sleep( 8 );

     FSCHECK(playback->Stop( playback ));

     END_TEST();
}

static void test_stop_continue_playback( SoundType type )
{
     int i;

     BEGIN_TEST( type, "Stop/Continue Playback" );

     if (type == SOUND_BUFFER)
          FSCHECK(playback->Start( playback, 0, -1 ));

     for (i = 0; i < 8; i++) {
          usleep( 500000 );

          FSCHECK(playback->Stop( playback ));

          usleep( 200000 );

          FSCHECK(playback->Continue( playback ));
     }

     END_TEST();
}

static void test_volume_level( SoundType type )
{
     int i;

     BEGIN_TEST( type, "Volume Level" );

     if (type == SOUND_BUFFER)
          FSCHECK(playback->Start( playback, 0, -1 ));

     for (i = 0; i < 60; i++) {
          FSCHECK(playback->SetVolume( playback, sin( i / 3.0) / 3.0 + 0.6 ));

          usleep( 100000 );
     }

     END_TEST();
}

static void test_pan_value( SoundType type )
{
     int i;

     BEGIN_TEST( type, "Pan Value" );

     if (type == SOUND_BUFFER)
          FSCHECK(playback->Start( playback, 0, -1 ));

     for (i = 0; i < 30; i++) {
          FSCHECK(playback->SetPan( playback, sin( i / 3.0 ) ));

          usleep( 200000 );
     }

     END_TEST();
}

static void test_pitch_value( SoundType type )
{
     int i;

     BEGIN_TEST( type, "Pitch Value" );

     if (type == SOUND_BUFFER)
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

     /* Release the sound stream. */
     if (stream)
          stream->Release( stream );

     /* Release the music provider. */
     if (provider)
          provider->Release( provider );

     /* Release the main interface. */
     if (sound)
          sound->Release( sound );
}

int main( int argc, char *argv[] )
{
     FSBufferDescription bdsc;
     FSStreamDescription sdsc;

     /* Initialize FusionSound including command line parsing. */
     FSCHECK(FusionSoundInit( &argc, &argv ));

     /* Create the main interface. */
     FSCHECK(FusionSoundCreate( &sound ));

     /* Register termination function. */
     atexit( cleanup );

     /* Create a music provider for loading the file. */
     FSCHECK(sound->CreateMusicProvider( sound, DATADIR"/test.wav", &provider ));
     provider->GetBufferDescription( provider, &bdsc );
     provider->GetStreamDescription( provider, &sdsc );

     /* Running sound buffer tests. */
     fprintf( stderr, "\nRunning sound buffer tests:\n" );
     FSCHECK(sound->CreateBuffer( sound, &bdsc, &buffer ));
     provider->PlayToBuffer( provider, buffer, NULL, NULL );

     test_start_simple_playback();
     test_start_positioned_playback();
     test_start_looping_playback();
     test_stop_continue_playback( SOUND_BUFFER );
     test_volume_level( SOUND_BUFFER );
     test_pan_value( SOUND_BUFFER );
     test_pitch_value( SOUND_BUFFER );

     /* Running sound stream tests. */
     fprintf( stderr, "\nRunning sound stream tests:\n" );
     FSCHECK(sound->CreateStream( sound, &sdsc, &stream ));
     provider->PlayToStream( provider, stream );
     provider->SetPlaybackFlags( provider, FMPLAY_LOOPING );

     test_stop_continue_playback( SOUND_STREAM );
     test_volume_level( SOUND_STREAM );
     test_pan_value( SOUND_STREAM );
     test_pitch_value( SOUND_STREAM );

     return 0;
}
