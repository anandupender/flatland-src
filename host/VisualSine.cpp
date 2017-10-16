//-----------------------------------------------------------------------------
// name: VisualSine.cpp
// desc: hello sine wave, real-time
//
// author: Ge Wang (ge@ccrma.stanford.edu)
//   date: fall 2014
//   uses: RtAudio by Gary Scavone
//-----------------------------------------------------------------------------
#include "RtAudio/RtAudio.h"
#include "chuck.h"
#include <math.h>
#include <stdlib.h>
#include <iostream>
#include <cmath>
#include "chuck_fft.h"
#include <unistd.h> //timer

#include <algorithm>
using namespace std;

#ifdef __MACOSX_CORE__
#include <GLUT/glut.h>
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#else
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>
#endif


//-----------------------------------------------------------------------------
// function prototypes
//-----------------------------------------------------------------------------
void initGfx();
void idleFunc();
void displayFunc();
void reshapeFunc( GLsizei width, GLsizei height );
void keyboardFunc( unsigned char, int, int );
void mouseFunc( int button, int state, int x, int y );

// our datetype
#define SAMPLE float
// corresponding format for RtAudio
#define MY_FORMAT RTAUDIO_FLOAT32
// sample rate
#define MY_SRATE 44100
// number of channels
#define MY_CHANNELS 1
// for convenience
#define MY_PIE 3.14159265358979

// width and height
long g_width = 1024;
long g_height = 720;
// global buffer
SAMPLE * g_buffer = NULL;
long g_bufferSize;
SAMPLE * g_buffer2N = NULL;
std::list< vector<complex> > g_buffer_history;
float g_buffer_counter = 0;
#define g_path "host/computerMusic.ck"

float xCam = 0;
float yCam = 0;
float zCam = 10;

//keyboard triggers
bool g_time_domain = false;
bool g_fft;
bool g_line = true;
bool g_square = false;
bool g_cube;
bool g_zoom_out = false;
bool g_zoom_in = false;

vector< int > g_all_maxes;

float g_y_rotation = 0;
float g_y_rotation_large = 0;

int g_frequency_max = 100;

// global variables
bool g_draw_dB = false;
ChucK * the_chuck;
//-----------------------------------------------------------------------------
// name: callme()
// desc: audio callback
//-----------------------------------------------------------------------------
int callme( void * outputBuffer, void * inputBuffer, unsigned int numFrames,
            double streamTime, RtAudioStreamStatus status, void * data )
{
    // cast!
    SAMPLE * input = (SAMPLE *)inputBuffer;
    SAMPLE * output = (SAMPLE *)outputBuffer;
    the_chuck -> run(input, output, numFrames);

    // fill
    for( int i = 0; i < numFrames; i++ )
    {
        // assume mono
        g_buffer[i] = input[i];
    } 
    return 0;
}

//-----------------------------------------------------------------------------
// name: initChucK()
// desc: initialize ChucK
//-----------------------------------------------------------------------------
bool initChucK()
{
    the_chuck = new ChucK;
    
    the_chuck -> setParam(CHUCK_PARAM_SAMPLE_RATE, (t_CKINT)MY_SRATE);
    the_chuck -> setParam(CHUCK_PARAM_OUTPUT_CHANNELS,(t_CKINT)MY_CHANNELS );

    the_chuck -> init();

    return true;
}


//-----------------------------------------------------------------------------
// name: main()
// desc: entry point
//-----------------------------------------------------------------------------
int main( int argc, char ** argv )
{
    // instantiate RtAudio object
    RtAudio audio;
    // variables
    unsigned int bufferBytes = 0;
    // frame size
    unsigned int bufferFrames = 1024;
    
    // check for audio devices
    if( audio.getDeviceCount() < 1 )
    {
        // nopes
        cout << "no audio devices found!" << endl;
        exit( 1 );
    }
    
    // initialize GLUT
    glutInit( &argc, argv );
    // init gfx
    initGfx();
    
    // let RtAudio print messages to stderr.
    audio.showWarnings( true );
    
    // set input and output parameters
    RtAudio::StreamParameters iParams, oParams;
    iParams.deviceId = audio.getDefaultInputDevice();
    iParams.nChannels = MY_CHANNELS;
    iParams.firstChannel = 0;
    oParams.deviceId = audio.getDefaultOutputDevice();
    oParams.nChannels = MY_CHANNELS;
    oParams.firstChannel = 0;
    
    // create stream options
    RtAudio::StreamOptions options;
    
    // go for it
    try {
        // open a stream
        audio.openStream( &oParams, &iParams, MY_FORMAT, MY_SRATE, &bufferFrames, &callme, (void *)&bufferBytes, &options );
    }
    catch( RtError& e )
    {
        // error!
        cout << e.getMessage() << endl;
        exit( 1 );
    }

    // NOTE: init ChucK (see function above)
    if( !initChucK() )

        exit( 1 );

    the_chuck -> compileFile(g_path, "");

    
    // compute
    bufferBytes = bufferFrames * MY_CHANNELS * sizeof(SAMPLE);
    // allocate global buffer
    g_bufferSize = bufferFrames;
    g_buffer = new SAMPLE[g_bufferSize];
    g_buffer2N = new SAMPLE[g_bufferSize*2];

    memset( g_buffer, 0, sizeof(SAMPLE)*g_bufferSize );
    memset( g_buffer2N, 0, sizeof(SAMPLE)*(g_bufferSize*2) );

    cout << "Welcome to FLATLAND" << endl;

    cout << "l -- restart 1D World (line)" << endl;
    cout << "s -- restart 2D World (square)" << endl;
    cout << "c -- restart 3D World (cube)" << endl;
    cout << "z -- zoom out effect" << endl;
    cout << "Z -- zoom in effect" << endl;
    cout << "t -- display time domain wave" << endl;
    cout << "f -- display fft" << endl;
    cout << "" << endl;
    cout << "q/Q -- quit program" << endl;
    
    // go for it
    try {
        the_chuck -> start();
        // start stream
        audio.startStream();
        
        // let GLUT handle the current thread from here
        glutMainLoop();
        
        // stop the stream.
        audio.stopStream();
    }
    catch( RtError& e )
    {
        // print error message
        cout << e.getMessage() << endl;
        goto cleanup;
    }
    
cleanup:
    // close if open
    if( audio.isStreamOpen() )
        audio.closeStream();
    
    // done
    return 0;
}




//-----------------------------------------------------------------------------
// Name: reshapeFunc( )
// Desc: called when window size changes
//-----------------------------------------------------------------------------
void initGfx()
{
    // double buffer, use rgb color, enable depth buffer
    glutInitDisplayMode( GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH );
    // initialize the window size
    glutInitWindowSize( g_width, g_height );
    // set the window postion
    glutInitWindowPosition( 100, 100 );
    // create the window
    glutCreateWindow( "VisualSine" );
    
    // set the idle function - called when idle
    glutIdleFunc( idleFunc );
    // set the display function - called when redrawing
    glutDisplayFunc( displayFunc );
    // set the reshape function - called when client area changes
    glutReshapeFunc( reshapeFunc );
    // set the keyboard function - called on keyboard events
    glutKeyboardFunc( keyboardFunc );
    // set the mouse function - called on mouse stuff
    glutMouseFunc( mouseFunc );
    
    // set clear color
    glClearColor( 0, 0, 0, 1 );
    // enable color material
    glEnable( GL_COLOR_MATERIAL );
    // enable depth test
    glEnable( GL_DEPTH_TEST );

    glEnable(GL_BLEND);

    glEnable(GL_LINE_SMOOTH);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glHint(GL_LINE_SMOOTH_HINT,GL_NICEST);
    
    glEnable(GL_LINE_SMOOTH);
}


//-----------------------------------------------------------------------------
// Name: reshapeFunc( )
// Desc: called when window size changes
//-----------------------------------------------------------------------------
void reshapeFunc( GLsizei w, GLsizei h )
{
    // save the new window size
    g_width = w; g_height = h;
    // map the view port to the client area
    glViewport( 0, 0, w, h );
    // set the matrix mode to project
    glMatrixMode( GL_PROJECTION );
    // load the identity matrix
    glLoadIdentity( );
    // create the viewing frustum
    gluPerspective( 45.0, (GLfloat) w / (GLfloat) h, 1.0, 300.0 );
    // set the matrix mode to modelview
    glMatrixMode( GL_MODELVIEW );
    // load the identity matrix
    glLoadIdentity( );
    // position the view point
    gluLookAt( 0.0f, 0.0f, 10.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f );
}




//-----------------------------------------------------------------------------
// Name: keyboardFunc( )
// Desc: key event
//-----------------------------------------------------------------------------
void keyboardFunc( unsigned char key, int x, int y )
{
    switch( key )
    {
        case 'Q':
        case 'q':
            exit(1);
            break;
            
        case 't':
            g_time_domain = !g_time_domain;
            break;
        case 'f':
            g_fft = !g_fft;
            break;
        case 'z':
            g_zoom_out = !g_zoom_out;
            xCam = -3;
            zCam = 1;
            break;
        case 'Z':
            g_zoom_in = !g_zoom_in;
            zCam = 10;
            xCam = 0;
            break;
        case 'l':
            g_line = !g_line;
            g_square = false;
            g_cube = false;
            break;
        case 's':
            g_all_maxes.clear(); //empty current FFT buffer history
            g_square = !g_square;
            g_line = false;
            g_cube = false;
            break;
        case 'c':
            g_all_maxes.clear(); //empty current FFT buffer history
            g_line = false;
            g_square = false;
            g_cube = !g_cube;
            zCam = 10;
            xCam = 0;
            yCam = 0;
            break;
    }
    glutPostRedisplay( );
}


//-----------------------------------------------------------------------------
// Name: mouseFunc( )
// Desc: handles mouse stuff
//-----------------------------------------------------------------------------
void mouseFunc( int button, int state, int x, int y )
{
    if( button == GLUT_LEFT_BUTTON )
    {
        // when left mouse button is down
        if( state == GLUT_DOWN )
        {
        }
        else
        {
        }
    }
    else if ( button == GLUT_RIGHT_BUTTON )
    {
        // when right mouse button down
        if( state == GLUT_DOWN )
        {
        }
        else
        {
        }
    }
    else
    {
    }
    
    glutPostRedisplay( );
}


//-----------------------------------------------------------------------------
// Name: idleFunc( )
// Desc: callback from GLUT
//-----------------------------------------------------------------------------
void idleFunc( )
{
    // render the scene
    glutPostRedisplay( );
}

void gl_zoom_out(){
    glLoadIdentity();
    gluLookAt(xCam, yCam, zCam, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0);
    zCam += .005;
    while(xCam < 0){
        xCam += .01;
    }
    if(zCam >= 20){
        //zCam = 2;
    }
}

bool final_trigger = false;

void gl_final_zoom_out(){
    glLoadIdentity();
    gluLookAt(xCam, yCam, zCam, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0);
    zCam += .1;
    g_fft = true;
    if(zCam >= 100){
        final_trigger = true;
    }
    if(final_trigger){
        zCam -= 1;
        if(zCam <= -380){
            exit(EXIT_FAILURE); //end whole program
        }
    }
}

bool gl_zoom_in(){
    glLoadIdentity();
    gluLookAt(xCam, yCam, zCam, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0);
    if(zCam >= 1){
        zCam -= .01;
        xCam -= .001;
        yCam -= .002;
    }
    else{
        glClearColor(rand()%255/255.0,rand()%255/255.0,rand()%255/255.0,1);
        return true;
    }
    return false;
}

bool g_x_rotate_flip;
bool g_y_rotate_flip;

void gl_cube_rotate(){
    //start rotating screen!
    glLoadIdentity();
    gluLookAt(xCam, yCam, zCam, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0);
    int trig = 10;
    if(xCam <= -trig || xCam >= trig){
        g_x_rotate_flip = !g_x_rotate_flip;
    }
    if(yCam <= -trig || yCam >= trig){
        g_y_rotate_flip = !g_y_rotate_flip;
    }
    if(g_x_rotate_flip){
        xCam -=.01;
    }else{
        xCam += .01;
    }
    if(g_y_rotate_flip){
        yCam -=.03;
    }else{
        yCam += .03;
    }
}

bool gl_line_to_plane(){
    glLoadIdentity();
    gluLookAt(xCam, yCam, zCam, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0);
    yCam -= .002;
    zCam -= .02;
    if(zCam <= -10){
        return true;
    }
    return false;
}

vector< vector<float> > cubes;

void cube_world(){
    glColor3f( 1, 1, 1);
    float size = rand()%100/100.0;
    int sizeToFill = 150;
    float xRand = rand()%sizeToFill - sizeToFill/2;
    float yRand = rand()%sizeToFill - sizeToFill/2;
    float zRand = rand()%sizeToFill - sizeToFill/2;
    float arr[] = {size, xRand,yRand,zRand};
    vector<float> tempV (arr, arr + sizeof(arr) / sizeof(arr[0]) );
    cubes.push_back(tempV);
    for(int i = 0; i < cubes.size(); i++){
        glPushMatrix();
        glTranslatef( cubes[i][1], cubes[i][2], cubes[i][3]);
        glutWireCube((GLdouble) cubes[i][0]);
        glPopMatrix();
    }

}

void gl_cube_fill(float finalSize, float baselineY){
    //bottom
    glVertex3f(-finalSize/2, baselineY, -finalSize/2);
    glVertex3f(finalSize/2, baselineY, -finalSize/2);
    glVertex3f(finalSize/2, baselineY, finalSize/2);
    glVertex3f(-finalSize/2,baselineY, finalSize/2);
    //top
    glVertex3f(-finalSize/2, baselineY + .1, -finalSize/2);
    glVertex3f(finalSize/2, baselineY + .1, -finalSize/2);
    glVertex3f(finalSize/2, baselineY + .1, finalSize/2);
    glVertex3f(-finalSize/2, baselineY + .1, finalSize/2);
    //back
    glVertex3f(-finalSize/2, baselineY, -finalSize/2);
    glVertex3f(finalSize/2, baselineY, -finalSize/2);
    glVertex3f(finalSize/2, baselineY + .1, -finalSize/2);
    glVertex3f(-finalSize/2, baselineY + .1, -finalSize/2);
    //front
    glVertex3f(-finalSize/2, baselineY, finalSize/2);
    glVertex3f(finalSize/2, baselineY, finalSize/2);
    glVertex3f(finalSize/2, baselineY + .1, finalSize/2);
    glVertex3f(-finalSize/2, baselineY + .1, finalSize/2);
    //left
    glVertex3f(-finalSize/2, baselineY+.1, finalSize/2);
    glVertex3f(-finalSize/2, baselineY+.1, -finalSize/2);
    glVertex3f(-finalSize/2, baselineY, -finalSize/2);
    glVertex3f(-finalSize/2, baselineY, finalSize/2);
    //right
    glVertex3f(finalSize/2, baselineY, finalSize/2);
    glVertex3f(finalSize/2, baselineY+.1, finalSize/2);
    glVertex3f(finalSize/2, baselineY + .1, -finalSize/2);
    glVertex3f(finalSize/2, baselineY, -finalSize/2);
}

int glitch_counter = 0;     //show cube 50 times
bool glitch_started = false;

void gl_glitch(){

    int glitch = rand() % 101;
    if(glitch == 100){
        glitch_started = true;
    }

    if(glitch_started && glitch_counter <=15){  //show glitch cube
        glitch_counter++;
        glColor3f(rand()%255/255.0, rand()%255/255.0,rand()%255/255.0);
        if(g_square){
            glutWireCube((GLdouble) 5);
        }
        if(g_line){
            float xLen = .2;
            float yLen = 7;
            glBegin(GL_LINE_STRIP);
            glVertex2f(xLen, yLen);
            glVertex2f(xLen, -yLen);
            glVertex2f(-xLen, -yLen);
            glVertex2f(-xLen, yLen);
            glVertex2f(xLen, yLen);
            glEnd();
        }
    }else{
        glitch_started = 0;
        glitch_counter = 0;
    }
    // glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
    //gluLookAt(2, 0, 7, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0);
    //glClearColor(rand()%255/255.0,rand()%255/255.0,rand()%255/255.0,1);
    //usleep(100000);
    //gluLookAt(0, 0, 10, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0);
}

void gl_time_domain(float max){
    glBegin( GL_LINE_STRIP );
    GLfloat x = -75;
    GLfloat xinc = ::fabs(x*2 / g_bufferSize);

    int magicScale = 70;
    glColor4f(rand()%255/255.0, rand()%255/255.0,rand()%255/255.0, max*magicScale);
    for( int i = 0; i < g_bufferSize; i++ )
    {
        glVertex3f( x, g_buffer[i],0 );
        x += xinc;
    }
    glEnd();
}

void gl_time_domain_custom(float xMin, float xMax, float yOffset, bool flip, float max, bool showIn2D){
    glBegin( GL_LINE_STRIP );
    GLfloat x = xMin;
    GLfloat xinc = ::fabs(x*2 / g_bufferSize);

    //int magicScale = 70;
    glColor3f(rand()%255/255.0, rand()%255/255.0,rand()%255/255.0);
    //glColor4f(rand()%255/255.0, rand()%255/255.0,rand()%255/255.0, max*70);

    for( int i = 0; i < g_bufferSize; i++ )
    {
        if(flip){
            glVertex3f( g_buffer[i] + yOffset, x,0 );
        }
        else{
            if(showIn2D){
                glVertex3f( x,yOffset,g_buffer[i]/10.0); //divide to make parallax OK
            }else{
                glVertex3f( x, g_buffer[i] + yOffset,0 );
            }
        }
        x += xinc;
    }
    glEnd();
    // cout << xCam << " " << yCam << " " << zCam << endl;
}

void gl_line(float max, int max_pos){
    double magicMaxPos = 5;
    double xStart = 7.2;  //ide lower frequencies
    double xEnd = 7;

    for(int i = 0; i < g_all_maxes.size(); i++){
        glBegin(GL_QUADS);
        glColor3f(rand()%255/255.0, rand()%255/255.0,rand()%255/255.0);
            // (int)(g_all_maxes[i]/10.0
            glVertex3f(g_all_maxes[i]/magicMaxPos - xStart, -.008, 0);
            glVertex3f(g_all_maxes[i]/magicMaxPos - xStart, .008, 0);
            glVertex3f(g_all_maxes[i]/magicMaxPos - xEnd, .008, 0);
            glVertex3f(g_all_maxes[i]/magicMaxPos - xEnd, -.008, 0);
        glEnd();
    }
    //TODO: decrease resolution of line (make more 1D) by drawing random pixel colors 
    //on a short line segment for each new frequency
    if(g_all_maxes.size() >= g_frequency_max){ //TODO: finalize size
        if(gl_zoom_in()){
            the_chuck->broadcastExternalEvent("lineEnd");
            g_line = false;
            g_square = true;
            xCam = 0;
            yCam = 0;
            zCam = 10;
            glClearColor(1,1,1,1);
            usleep(2000000);
            glLoadIdentity();
            gluLookAt(xCam, yCam, zCam, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0);
            g_all_maxes.clear(); //empty current FFT buffer history
        }
    }
    gl_glitch();
}

void gl_square(float max, int max_pos){
    int magicScale = 70;
    glColor4f(rand()%255/255.0, rand()%255/255.0,rand()%255/255.0, max*magicScale);

    for(int i = 0; i < g_all_maxes.size(); i++){
        // glBegin(GL_LINES);
        // glColor3f(rand()%255/255.0, rand()%255/255.0,rand()%255/255.0);
        // //glColor3f(sin(2*MY_PIE*counter), sin(2*MY_PIE*counter +2*MY_PIE/3),sin(2*MY_PIE*counter +4*MY_PIE/3));
        // counter+=.00001;
        if(g_all_maxes[i]/20.0 - 3 <= 3 && g_all_maxes[i]/20.0 - 3 >= -3){
        //     //glColor3f(i%255, i%255, i%255);
        //     glVertex3f(-3, g_all_maxes[i]/20.0 - 3, 0.0);
        //     glVertex3f(3, g_all_maxes[i]/20.0 - 3, 0);
            gl_time_domain_custom(-3, 3, g_all_maxes[i]/20.0 - 3, false, max, true);

        }
        // glEnd();
    }

    //outline square!
    glColor3f(1.0f, 1.0f, 1.0f); // Let it be yellow.
    glBegin(GL_LINE_STRIP);
    glVertex2f(3.01f, 3.01f);
    glVertex2f(3.01f, -3.01f);
    glVertex2f(-3.01f, -3.01f);
    glVertex2f(-3.01f, 3.01f);
    glVertex2f(3.01f, 3.01f);
    glEnd();

    if(g_all_maxes.size() >= g_frequency_max){
        //cool effecf
        
        if(gl_line_to_plane()){  //done with effect
            the_chuck->broadcastExternalEvent("squareEnd");
            g_square = false;
            g_cube = true;
            g_all_maxes.clear(); //empty current FFT buffer history
            xCam = 0; 
            yCam = 0;
            zCam = 0;
            glLoadIdentity();
            usleep(2000000);
            gluLookAt(xCam, yCam, zCam, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0);
        }
    }
    gl_glitch();
}

void gl_cube(float max, int max_pos){

    float finalSize = 5;
    //draw large outline cube
    glColor3f( 1, 1, 1);
    glutWireCube((GLdouble) finalSize);

    for(int i = 0; i < g_all_maxes.size(); i++){
        glBegin(GL_QUADS);
        glColor3f(rand()%255/255.0, rand()%255/255.0,rand()%255/255.0);
        float baselineY = g_all_maxes[i]/20.0 - finalSize;
        // //glColor3f(sin(2*MY_PIE*counter), sin(2*MY_PIE*counter +2*MY_PIE/3),sin(2*MY_PIE*counter +4*MY_PIE/3));
        if(baselineY <= finalSize/2 && baselineY >= -finalSize/2){
            gl_cube_fill(finalSize, baselineY);
        }
        glEnd();
    }
    gl_cube_rotate();
    if(g_all_maxes.size() >= g_frequency_max){ //TODO: finalize size
        cube_world();
        gl_final_zoom_out();
        the_chuck->broadcastExternalEvent("squareEnd");
    }


}

//TODO; implement hypercube?
//TODO: where incorporate FFT and time domain?
//TODO: include harsh glitches where you get a glimpse and new dimension

//TODO: remove FFT visualized bu still calculate it based on the global bool
float gl_fft(float max, int max_pos){
    // define a starting point
    GLfloat x = -50;
    GLfloat yOffset = -20;
    glLineWidth( 1.0 );

    // increment
    //GLfloat xinc = ::fabs(x*2 / g_bufferSize);
    //scaled to human singing tones
    GLfloat xinc = ::fabs(x*20 / g_bufferSize);
    bool currentFFT = true;
    GLfloat colorFade = 1;
    GLfloat yOffsetSmall = 0;
    for (list< vector<complex> >::iterator it = g_buffer_history.begin(); it != g_buffer_history.end(); ++it){
        x = -75;
        glBegin(  GL_LINE_STRIP );
        for( int j = 1; j < g_bufferSize; j++ ) //igore first bin
        {
            // plot
            glColor4f(rand()%255/255.0, rand()%255/255.0,rand()%255/255.0, colorFade);
            if(g_fft){
                glVertex2f( x, 2000*cmp_abs((*it)[j]) + yOffset + yOffsetSmall); //std::abs?
            }
            if( currentFFT && cmp_abs((*it)[j]) > max){
                max = cmp_abs((*it)[j]);
                max_pos = j;
            }
            x += xinc;
        } 
        colorFade -= .1;
        yOffset += 5;
        currentFFT = false;
        glEnd();
    }
    if (!(std::find(g_all_maxes.begin(), g_all_maxes.end(), max_pos) != g_all_maxes.end()))
    {
        g_all_maxes.push_back(max_pos);
    }
    return max;
}


//-----------------------------------------------------------------------------
// Name: displayFunc( )
// Desc: callback function invoked to draw the client area
//-----------------------------------------------------------------------------
void displayFunc( )
{
    GLfloat my_window[g_bufferSize];
    hamming(my_window,g_bufferSize);
    apply_window(g_buffer, my_window, g_bufferSize);
    memcpy( g_buffer2N, g_buffer, g_bufferSize*2 ); //only makes a more granular fft

    //RFFT
    rfft( (float *)g_buffer2N, g_bufferSize, FFT_FORWARD);

    // cast to complex
    complex * cbuf = (complex *)g_buffer2N;

    std::vector<complex> temp(g_bufferSize);
    for(int i = 0; i < g_bufferSize; i++){
        temp[i] = cbuf[i];
    }
    g_buffer_history.push_front(temp);    
    
    // local state
    static GLfloat zrot = 0.0f, c = 0.0f;
    // clear the color and depth buffers
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
    glClearColor(0,0,0,1);
    
    float max = 0;
    float max_pos = 0;

    max = gl_fft(max, max_pos);

    if(g_line){
        gl_line(max, max_pos);
    }
    else if(g_cube){
        gl_cube(max, max_pos);
    }
    else if(g_square){
        gl_square(max, max_pos);
    }
    if(g_time_domain){
        gl_time_domain(max);
    }
    if(g_buffer_history.size() > 50){
        g_buffer_history.pop_back();
    }

    if(g_zoom_out){
        gl_zoom_out();
    }else if(g_zoom_in){
        gl_zoom_in();
    }

    // flush!
    glFlush();
    // swap the double buffer
    glutSwapBuffers();
    //unsigned char pixel[4];
    // glReadPixels(0, 0, 1, 1, GL_RGB, GL_UNSIGNED_BYTE, pixel);
    // cout << "R: " << pixel[0] << endl;
    // cout << "G: " << pixel[1] << endl;
    // cout << "B: " << pixel[2] << endl;
    // cout << endl;
}
