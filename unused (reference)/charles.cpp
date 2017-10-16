//-----------------------------------------------------------------------------
// name: charles.cpp
// desc: hello ChucK, boilerplate file
//
// author: Ge Wang (ge@ccrma.stanford.edu)
//   date: fall 2017
//   uses: RtAudio by Gary Scavone
//         ChucK core (http://chuck.stanford.edu/)
//-----------------------------------------------------------------------------
#include "RtAudio/RtAudio.h"
#include "chuck.h"
#include <math.h>
#include <iostream>
#include <cstdlib>
using namespace std;


// our datetype
// #define SAMPLE float
// corresponding format for RtAudio
// note: this MUST correspond to size of ChucK's SAMPLE
#define MY_FORMAT RTAUDIO_FLOAT32
// for convenience
#define MY_PIE 3.14159265358979
// sample rate
const t_CKFLOAT MY_SRATE = 44100;
// number of channels
const t_CKINT MY_CHANNELS = 2;

// global chuck instance
ChucK * the_chuck;




//-----------------------------------------------------------------------------
// name: callme()
// desc: audio callback
//-----------------------------------------------------------------------------
int callme( void * outputBuffer, void * inputBuffer, unsigned int numFrames,
            double streamTime, RtAudioStreamStatus status, void * data )
{
    // debug print something out per callback
    cerr << ".";

    // cast!
    SAMPLE * in = (SAMPLE *)inputBuffer;
    SAMPLE * out = (SAMPLE *)outputBuffer;

    // NOTE: run chuck from here for this window of audio...

    return 0;
}



//-----------------------------------------------------------------------------
// name: initChucK()
// desc: initialize ChucK
//-----------------------------------------------------------------------------
bool initChucK()
{
    // NOTE: instantiate ChucK here...
    
    // NOTE: set params here...
    
    // NOTE: initialize ChucK here...
    
    return true;
}




//-----------------------------------------------------------------------------
// name: main()
// desc: entry point
//-----------------------------------------------------------------------------
int main( int argc, char ** argv )
{
    // instantiate RtAudio object
    RtAudio adac;
    // variables
    unsigned int bufferBytes = 0;
    // frame size
    unsigned int bufferFrames = 512;
    
    // NOTE: check command line arguments
    // ...
    
    // check for audio devices
    if( adac.getDeviceCount() < 1 )
    {
        // nopes
        cout << "no audio devices found!" << endl;
        exit( 1 );
    }

    // let RtAudio print messages to stderr.
    adac.showWarnings( true );

    // set input and output parameters
    RtAudio::StreamParameters iParams, oParams;
    iParams.deviceId = adac.getDefaultInputDevice();
    iParams.nChannels = MY_CHANNELS;
    iParams.firstChannel = 0;
    oParams.deviceId = adac.getDefaultOutputDevice();
    oParams.nChannels = MY_CHANNELS;
    oParams.firstChannel = 0;
    
    // create stream options
    RtAudio::StreamOptions options;

    // go for it
    try {
        // open a stream
        adac.openStream( &oParams, &iParams, MY_FORMAT, MY_SRATE, &bufferFrames, &callme, (void *)&bufferBytes, &options );
    }
    catch( RtError& e )
    {
        // error!
        cout << e.getMessage() << endl;
        exit( 1 );
    }

    // compute
    bufferBytes = bufferFrames * MY_CHANNELS * sizeof(SAMPLE);
    
    // test RtAudio functionality for reporting latency.
    cout << "stream latency: " << adac.getStreamLatency() << " frames" << endl;
    
    // NOTE: init ChucK (see function above)
    if( !initChucK() )
        exit( 1 );
    
    // NOTE: create a ChucK program and compile it here...

    // go for it
    try {
        // NOTE: start chuck here...

        // start stream
        adac.startStream();

        // get input
        char input;
        std::cout << "running... press <enter> to quit (buffer frames: " << bufferFrames << ")" << endl;
        std::cin.get(input);
        
        // stop the stream.
        adac.stopStream();
    }
    catch( RtError& e )
    {
        // print error message
        cout << e.getMessage() << endl;
        goto cleanup;
    }
    
cleanup:
    // close if open
    if( adac.isStreamOpen() )
        adac.closeStream();
    
    // done
    return 0;
}
