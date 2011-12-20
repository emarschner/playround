#include <math.h>
#include <stdlib.h>
#include <iostream>
#include <string.h>
#include <time.h>
#include <algorithm>
using namespace std;

#ifdef __MACOSX_CORE__
#include <GLUT/glut.h>
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#include <ApplicationServices/ApplicationServices.h>  
#else
#ifdef _WIN32
#include <windows.h>
#endif
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>
#endif

#include "stk/Stk.h"

#include "Common.h"

#include "MyAudio.h"
#include "Engine.h"
#include "Network.h"

//-----------------------------------------------------------------------------
// function prototypes
//-----------------------------------------------------------------------------
void initializeGfx();
void initializeAudio();
void initializeNetwork();

// OpenGL callback functions
void idleFunc(int);
void displayFunc();
void reshapeFunc( GLsizei width, GLsizei height );
void keyboardFunc( unsigned char, int, int );
void mouseFunc( int button, int state, int x, int y );
void motionFunc(int x, int y);

int audioCallback(void * outputBuffer, void * inputBuffer,
                  unsigned int bufferSize, double streamTime,
                  RtAudioStreamStatus status, void * userData);


//-----------------------------------------------------------------------------
// global variables
//-----------------------------------------------------------------------------

int g_startTime;
int g_prevTime;

// Audio settings
int g_numChannels = 2;
MyAudio *g_pAudio;

// width and height
GLsizei g_width = 512;
GLsizei g_height = 512;

// OpenGL global vars
Point2D g_previousMousePos;

// If left button os down
bool g_fLeftButton = false;

// Camera control and scene rotation global variables
Point3D g_look_from(0, -0.05, 1);
Point3D g_look_to(0, -0.05, 0);
Point3D g_head_up(0, 1, 0 );
Point3D g_rotation, g_previousRotation;

// Program objects
Engine *g_pEngine = NULL;
Network *g_pNetwork = NULL;

// Network ports
int g_port = DEFAULT_PORT,
    g_peerPort = DEFAULT_PORT;
std::string g_peerHost;

void usage( int argc, char ** argv )
{
  std::cerr << "Usage: " << argv[0]
            << " <peer-hostname>[ :<peer-port = " << DEFAULT_PORT << "> ]"
            << " [ <listen-port = " << DEFAULT_PORT << "> ]" << std::endl;
  exit(1);
}

void parseCommandLine( int argc, char ** argv )
{
  if (argc < 2) {
    std::cerr << "not enough arguments! need at least hostname of a peer" << std::endl;
    usage(argc, argv);
  } else {
    g_peerHost = argv[1];
    int sepPos = g_peerHost.find(':');
    if (sepPos != string::npos) {
      g_peerPort = atoi(g_peerHost.substr(sepPos + 1).c_str());
      if (g_peerPort <= 0 || g_peerPort == INT_MAX) {
        std::cerr << "invalid peer port -- " << g_peerPort << std::endl;
        usage(argc, argv);
      }
      g_peerHost.resize(sepPos);
    }
  }

  if (argc > 2) {
    g_port = atoi(argv[2]);
    if (g_port <= 0 || g_port == INT_MAX) {
      std::cerr << "invalid listen port -- " << g_port << std::endl;
      usage(argc, argv);
    }
  }
}

void createInitialWidgets()
{
  WidgetMap* widgets = Widget::getAll();
  SoundSourceMap* soundSources = SoundSource::getAllForEngine();

  // New pad!
  RoundPad* pad = new RoundPad(Point2D(g_width / 4.0, g_height / 4.0), 100);
  pad->setCommentText("Hi there!\nSo glad to see ya!\n\nWelcome to Play 'Round.\n\nThe pad above is yours.\nNo one else will ever\nsee it. Use it to\ntry things out.\n\nIf you need more room,\njust expand the window.");
  widgets->insert(WidgetData(pad->getUuid(), pad));
  g_pEngine->addChild(pad);

  // New tracks!

  // #1
  SpiralTrack* track = new SpiralTrack(pad->getCenter(), 30, 60, 270, 60, NULL, NULL);

  Joint *joint1 = track->getJoint1(),
        *joint2 = track->getJoint2(),
        *origJoint = joint1;
  joint1->setParentRoundPad(pad);
  joint2->setParentRoundPad(pad);

  track->getSpiral()->setLineWidth(3);
  track->setEnabled(true);

  widgets->insert(WidgetData(track->getUuid(), track));
  pad->addChild(track);

  track->addPlucker();

  // #2
  track = new SpiralTrack(pad->getCenter(), 270, 60, 150, 60, joint2, NULL);

  joint1 = track->getJoint1();
  joint2 = track->getJoint2(),
  joint1->setParentRoundPad(pad);
  joint2->setParentRoundPad(pad);

  track->getSpiral()->setLineWidth(3);
  track->setEnabled(true);

  widgets->insert(WidgetData(track->getUuid(), track));
  pad->addChild(track);

  // #3
  track = new SpiralTrack(pad->getCenter(), 150, 60, 30, 60, joint2, origJoint);

  joint1 = track->getJoint1();
  joint2 = track->getJoint2(),
  joint1->setParentRoundPad(pad);
  joint2->setParentRoundPad(pad);

  track->getSpiral()->setLineWidth(3);
  track->setEnabled(true);

  widgets->insert(WidgetData(track->getUuid(), track));
  pad->addChild(track);

  // New strings!

  // #1
  Point2D p1 = Spiral::getPointFromRadius(pad->getCenter(), 30, 330),
          p2 = Spiral::getPointFromRadius(pad->getCenter(), 90, 330);
  String* string = new String(p1, p2, pad->getRadius());
  soundSources->insert(SoundSourceData(string->getUuid(), string));
  widgets->insert(WidgetData(string->getUuid(), string));
  pad->addChild(string);

  // #2
  p1 = Spiral::getPointFromRadius(pad->getCenter(), 30, 210);
  p2 = Spiral::getPointFromRadius(pad->getCenter(), 90, 210);
  string = new String(p1, p2, pad->getRadius());
  soundSources->insert(SoundSourceData(string->getUuid(), string));
  widgets->insert(WidgetData(string->getUuid(), string));
  pad->addChild(string);

  // #3
  p1 = Spiral::getPointFromRadius(pad->getCenter(), 30, 90);
  p2 = Spiral::getPointFromRadius(pad->getCenter(), 90, 90);
  string = new String(p1, p2, pad->getRadius());
  soundSources->insert(SoundSourceData(string->getUuid(), string));
  widgets->insert(WidgetData(string->getUuid(), string));
  pad->addChild(string);
}

//-----------------------------------------------------------------------------
// name: main()
// desc: entry point
//-----------------------------------------------------------------------------
int main( int argc, char ** argv )
{
  parseCommandLine(argc, argv);

  g_pNetwork = new Network();
  g_pEngine = new Engine();
  g_pEngine->setSize(g_width, g_height);

  g_pEngine->setNetwork(g_pNetwork);
  g_pNetwork->setEngine(g_pEngine);
  g_pNetwork->addPeer(*(new Peer(g_peerHost.c_str(), g_peerPort)));

  // initialize GLUT
  glutInit( &argc, argv );

  // init audio
  initializeAudio();

  // init network
  initializeNetwork();

  // init gfx
  initializeGfx();

  createInitialWidgets();

  glutMainLoop();

  // cleanup
  delete g_pEngine;
  delete g_pNetwork;

  return 0;
}

//-----------------------------------------------------------------------------
// Name: initializeAudio( )
// Desc: Initialize audio
//-----------------------------------------------------------------------------
void initializeAudio()
{
  SoundSource::initializeGlobals();

  stk::Stk::setSampleRate(MY_SRATE);

  srand(time(NULL));
  g_pAudio = new MyAudio(g_numChannels, MY_SRATE, BUFFER_SIZE, RTAUDIO_FLOAT32);
  g_pAudio->setup(&audioCallback, NULL);
  g_pAudio->start();
  
}

//-----------------------------------------------------------------------------
// Name: initializeNetwork( )
// Desc: Initialize network
//-----------------------------------------------------------------------------
void initializeNetwork()
{
  g_pNetwork->listen(g_port);
}

void setCursorVisibility(float x, float y)
{
  if (x < 0 || x > g_width || y < 0 || y > g_height) {
#ifdef __MACOSX_CORE__
    CGDisplayShowCursor(kCGDirectMainDisplay);
#else
    glutSetCursor(GLUT_CURSOR_LEFT_ARROW);
#endif
  } else {
#ifdef __MACOSX_CORE__
    CGDisplayHideCursor(kCGDirectMainDisplay);
#else
    glutSetCursor(GLUT_CURSOR_NONE);
#endif
  }
}

//-----------------------------------------------------------------------------
// Name: initializeGfx( )
// Desc: Initialize open gl
//-----------------------------------------------------------------------------
void initializeGfx()
{
  // double buffer, use rgb color, enable depth buffer
  glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH | GLUT_MULTISAMPLE);
  // initialize the window size
  glutInitWindowSize(g_width, g_height);
  // set the window postion
  glutInitWindowPosition(100, 100);
  // create the window
  glutCreateWindow("Play 'Round");
  //glutEnterGameMode();

  // set the idle function - called when idle
  //glutIdleFunc(idleFunc);
  // set the display function - called when redrawing
  glutDisplayFunc(displayFunc);
  // set the reshape function - called when client area changes
  glutReshapeFunc(reshapeFunc);
  // set the keyboard function - called on keyboard events
  glutKeyboardFunc(keyboardFunc);
  // set the mouse function - called on mouse stuff
  glutMouseFunc(mouseFunc);
  glutMotionFunc(motionFunc);
  glutPassiveMotionFunc(motionFunc);

  // set clear color
  glClearColor(1, 1, 1, 1 );
  // enable depth test
  //glEnable( GL_DEPTH_TEST );

  // anti-aliasing
  glEnable(GL_POINT_SMOOTH);
  glEnable(GL_LINE_SMOOTH);
  glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
  glHint(GL_POLYGON_SMOOTH_HINT, GL_NICEST);

  glEnable(GL_MULTISAMPLE);
  glHint(GL_MULTISAMPLE_FILTER_HINT_NV, GL_NICEST);

  // enable transparency (also needed for anti-aliasing)
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  // seed random number generator
  srand( time(NULL) );

  glutTimerFunc(TIMER_MSECS, idleFunc, 0);
  g_startTime = glutGet(GLUT_ELAPSED_TIME);
  g_prevTime = g_startTime;
}

//-----------------------------------------------------------------------------
// Name: reshapeFunc( )
// Desc: called when window size changes
//-----------------------------------------------------------------------------
void reshapeFunc( GLsizei w, GLsizei h )
{
  // save the new window size
  g_width = w;
  g_height = h;

  if (g_pEngine)
    g_pEngine->setSize(g_width, g_height);

  // map the view port to the client area
  glViewport( 0, 0, w, h);

  // set the matrix mode to project
  glMatrixMode( GL_PROJECTION );
  // load the identity matrix
  glLoadIdentity();
  gluOrtho2D(0, w, h, 0);

  // set the matrix mode to modelview
  glMatrixMode( GL_MODELVIEW );
  // load the identity matrix
  glLoadIdentity( );
}

//-----------------------------------------------------------------------------
// Name: idleFunc( )
// Desc: callback from GLUT
//-----------------------------------------------------------------------------
void idleFunc(int value)
{
  // Set up the next timer tick (do this first)
  glutTimerFunc(TIMER_MSECS, idleFunc, 0);

  // Measure the elapsed time
  int currTime = glutGet(GLUT_ELAPSED_TIME);
  int timeSincePrevFrame = currTime - g_prevTime;
  int elapsedTime = currTime - g_startTime;

  // TODO: put 'tick' code here

  // Force a redisplay to render the new image
  glutPostRedisplay();

  g_prevTime = currTime;
}

//-----------------------------------------------------------------------------
// Name: displayFunc( )
// Desc: callback function invoked to draw the client area
//-----------------------------------------------------------------------------
void displayFunc( )
{
  g_pEngine->draw();

  SoundSource::swapGlobals();
  glutSwapBuffers();
  glFlush();
  return;
}

//-----------------------------------------------------------------------------
// Name: keyboardFunc( )
// Desc: respond to key events
//-----------------------------------------------------------------------------
void keyboardFunc( unsigned char key, int x, int y )
{
  g_pEngine->onKeyDown(key, x, y);
}

//-----------------------------------------------------------------------------
// Name: mouseFunc( )
// Desc: Deal with mouse related stuff
//-----------------------------------------------------------------------------
void mouseFunc( int button, int state, int x, int y )
{
  if (state == GLUT_DOWN)
    g_pEngine->onMouseDown(button, x, y);
  else if (state == GLUT_UP)
    g_pEngine->onMouseUp(button, x, y);

  if( button == GLUT_LEFT_BUTTON ) {
    // when left mouse button is down
    if( state == GLUT_DOWN ) {
      g_previousMousePos = Point2D(x, y);
      g_fLeftButton = true;
    } else {
      g_previousRotation = g_rotation;
      g_fLeftButton = false;
    }
    g_pNetwork->sendMousePosition(x, y, g_fLeftButton);
  } else if ( button == GLUT_RIGHT_BUTTON ) {
    // when right mouse button down
    if( state == GLUT_DOWN ) {}
    else {}
  } else {}

  glutPostRedisplay( );
}

//-----------------------------------------------------------------------------
// Name: motionFunc( )
// Desc:
//-----------------------------------------------------------------------------
void motionFunc(int x, int y)
{
  setCursorVisibility(x, y);

  g_pNetwork->sendMousePosition(x, y, g_fLeftButton);
  g_pEngine->setMouseCursorPosition(x, y);
  g_pEngine->setTextMode(Engine::TEXT_REPLACE);
  g_pEngine->setSelectedWidget(NULL);

  if (g_fLeftButton)
    g_pEngine->onMouseMove(x, y);
  else
    g_pEngine->onMouseOver(x, y);
}

//-----------------------------------------------
// name: audioCallback
// desc: ...
//-----------------------------------------------
int audioCallback(void * outputBuffer, void * inputBuffer,
                  unsigned int bufferSize, double streamTime,
                  RtAudioStreamStatus status, void * userData) {
  SAMPLE * out = (SAMPLE *)outputBuffer;
  SAMPLE * in = (SAMPLE *)inputBuffer;

  for(size_t i = 0; i < bufferSize; ++i)
    out[i * g_numChannels] = 0; // initialize first

  SoundSource::lockGlobals();
  {
    SoundSourceMap *data = SoundSource::getAllForAudio();

    for(size_t i = 0; i < bufferSize; ++i)
      for (SoundSourceMap::iterator ptr = data->begin(); ptr != data->end(); ptr++)
        out[i * g_numChannels] += ptr->second->tick();
  }
  SoundSource::unlockGlobals();

  for(size_t i = 0; i < bufferSize; ++i) {
    out[i * g_numChannels] = std::min(1.0f, std::max(-1.0f, out[i * g_numChannels]));
    for(size_t j = 1; j < g_numChannels; ++j)
      out[i * g_numChannels + j] = out[i * g_numChannels];
  }

  return 0;
}
