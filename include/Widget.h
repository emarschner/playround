#ifndef __WIDGET_H_
#define __WIDGET_H_

#include <iostream>
#include <vector>
#include <map>
#include <string>

#include "stk/Mutex.h"

// Forward declarations
class Network;
class Widget;
class Track;
class SoundSource;
class RoundPad;
class SpiralTrack;
class LineTrack;
class String;

#include "Shape.h"
#include "include/Common.h"
#include "osc/OscOutboundPacketStream.h"
#include "stk/Plucked.h"

typedef std::map<std::string, Widget*> WidgetMap;
typedef std::pair<std::string, Widget*> WidgetData;

typedef std::map<std::string, SoundSource*> SoundSourceMap;
typedef std::pair<std::string, SoundSource*> SoundSourceData;

#define LEFT_SIDE -1
#define RIGHT_SIDE 1
typedef int Side;
typedef std::map<std::string, Side> StringStateMap;
typedef std::pair<std::string, Side> StringStateData;

/**
* The base class of all components who want to deal with all user interaction
*/
class Widget : public Shape
{
public:
  Widget();
  virtual ~Widget();

  static WidgetMap* getAll();
  static Widget* getSelected();
  /**
  * Select the specific widget
  */
  static void setSelected(Widget*);

  // Mouse interaction
  virtual bool onMouseDown(int button, float x, float y);
  virtual bool onMouseUp(int button, float x, float y);
  virtual bool onMouseMove(float x, float y);
  virtual bool onMouseOver(float x, float y);

  virtual void draw() = 0;
  virtual Widget *hitTest(float x, float y) { return NULL; }

  virtual void setParent(Widget *parent);
  virtual void setNetwork(Network *network);

  virtual void addChild(Widget *child);
  virtual Widget* removeChild(Widget *child);

  virtual void setUuid(const char*);
  virtual const char* getUuid() const;

  virtual void toOutboundPacketStream(osc::OutboundPacketStream&) const;

  virtual std::vector<Widget *>* getChildren() { return &m_children; }
  virtual Widget* getParent() { return m_parent; }
  virtual std::string toString() { return ""; }

  virtual Widget* getEngine();

protected:
  /**
  * The draw event called when the user drags from this widget
  */
  virtual bool handleDraw(float x, float y) { return false; }
  /**
  * The last draw event called when the user finishes dragging
  */
  virtual bool handleDrawEnd(float x, float y) { return false; }
  /**
  * Called when the user click on the widget
  */
  virtual bool handleSelect(float x, float y, bool = false) { return false; }
  /**
  * Called when the user's mouse is hovering on the widget
  */
  virtual bool handleHover(float x, float y) { return false; }

  virtual void drawChildren();

  std::vector<Widget *> m_children;
  Widget *m_parent, *m_engine;
  Widget *m_mouseDownOn;
  bool m_fLeftButtonDown, m_fRightButtonDown;
  Point2D m_mouseDownPos, m_drawStartPos;
  std::string m_uuid;

  static Network *s_network;
};

/**
* The connection between tracks that relays the plucker from one path to another
*/
class Joint : public Widget
{
public:
  Joint(Point2D center, float radius);
  ~Joint();

  void addTrack(Track *track);
  Point2D getCenter() { return m_center; }
  void setCenter(Point2D center);
  void setColor(Color color);
  std::vector<Track *> *getTracks() { return &m_tracks; }

  virtual void draw();
  virtual Widget *hitTest(float x, float y);
  virtual std::string toString();
  RoundPad *getParentRoundPad() { return m_parentRoundPad; }
  void setParentRoundPad(RoundPad *pad) { m_parentRoundPad = pad; }

protected:
  virtual bool handleDraw(float x, float y);
  virtual bool handleDrawEnd(float x, float y);
  virtual bool handleSelect(float x, float y, bool = false);

  std::vector<Track *> m_tracks;
  Point2D m_center;
  float m_radius;
  Spiral *m_circle;

  SpiralTrack* m_newSpiralTrack;
  LineTrack* m_newLineTrack;
  Line m_dragLine;
  bool m_drawing;

  RoundPad *m_parentRoundPad;
};

/**
* The path that a plucker can travel on
*/
class Track : public Widget
{
public:
  Track();
  virtual ~Track();

  virtual void draw() = 0;

  virtual Point2D getEndPoint1() = 0;
  virtual Point2D getEndPoint2() = 0;
  Joint *getJoint1() { return m_joint1; }
  Joint *getJoint2() { return m_joint2; }
  /**
  * Returns the next position on the track that is distance far away from the passed in position
  */
  virtual Point2D getNextPos(bool fReverse, float distance, Point2D pos) = 0;
  void addPlucker();

  void setEnabled(bool fEnabled) { m_fEnabled = fEnabled; }
  void setActive(bool fActive) { m_fActive = fActive; }
  void setDirected(bool fDirected) { m_fDirected = fDirected; }
  void setImmediate(bool fImmediate) { m_fImmediate = fImmediate; }
  void setJoints(Joint *j1, Joint *j2) { m_joint1 = j1; m_joint2 = j2; }
  bool isEnabled() { return m_fEnabled; }
  bool isActive() { return m_fActive; }
  bool isDirected() { return m_fDirected;}
  bool isImmediate() { return  m_fImmediate; }

protected:
  virtual void drawChildren();
  virtual void detachJoint(Joint *joint);
  void setupJoints(Joint *joint1, Joint *joint2); 

  Joint *m_joint1, *m_joint2; // Start joint and end joint
  bool m_fEnabled, m_fActive, m_fDirected, m_fImmediate;
};

/**
* The spiral shaped track
*/
class SpiralTrack : public Track
{
public:
  SpiralTrack(Point2D center, float startAngle, float startRadius,
                              float endAngle, float endRadius, Joint *joint1, Joint *joint2);
  ~SpiralTrack();

  void initialize(Point2D center, float startAngle, float startRadius,
                                  float endAngle, float endRadius, Joint *joint1, Joint *joint2);

  virtual void draw();
  virtual Widget *hitTest(float x, float y);

  virtual void toOutboundPacketStream(osc::OutboundPacketStream&) const;

  virtual Point2D getEndPoint1();
  virtual Point2D getEndPoint2();
  virtual Point2D getNextPos(bool fReverse, float distance, Point2D pos);

  Point2D getCenter();
  void setCenter(const Point2D& center);
  void setEndAngle(float angle);
  void setEndRadius(float radius);

  float getStartAngle();
  float getEndAngle();
  Spiral* getSpiral() { return m_spiral; }

  virtual bool handleHover(float x, float y);

  virtual std::string toString();

protected:
  Point2D m_center;
  float m_startRadius, m_endRadius;
  float m_startAngle, m_endAngle;
  Spiral *m_spiral;
};

/**
* The line shaped track
*/
class LineTrack : public Track
{
public:
  LineTrack(Point2D p1, Point2D p2, Joint *joint1, Joint *joint2);
  ~LineTrack();

  void initialize(Point2D p1, Point2D p2, Joint *joint1, Joint *joint2);

  virtual void draw();
  virtual Widget *hitTest(float x, float y);

  virtual void toOutboundPacketStream(osc::OutboundPacketStream&) const;

  virtual Point2D getEndPoint1() { return m_p1; }
  virtual Point2D getEndPoint2() { return m_p2; }
  virtual Point2D getNextPos(bool fReverse, float distance, Point2D pos);

  virtual bool handleHover(float x, float y);

  Line* getLine() { return m_line; }

protected:
  Line *m_line;
  Point2D m_p1, m_p2;
};

/**
* The component that travels along the tracks and makes sound when it passes a string 
*/
class Plucker : public Widget
{
public:
  Plucker(Track *track, Joint *startJoint, Joint *endJoint);
  ~Plucker();

  /**
  * When the plucker is at an end joint, call this function to put more pluckers on the tracks following the current one.
  */
  std::vector<Plucker *> split();
  bool isAtEnd();

  /**
  * Proceed the plucker to the next position along the track
  */
  void tick();
  virtual void draw();

  Point2D getPos() { return m_pos; }

  Side getSideOfString(String*);
  Side updateSideOfString(String*);

private:
  Track *m_track;
  Joint *m_startJoint, *m_endJoint;
  Point2D m_pos;
  Spiral *m_circle;

  StringStateMap m_stringStates;
};

/**
* The base class for all the components that want to make sound
*/
class SoundSource : public Widget
{
public:
  static void initializeGlobals();
  static void lockGlobals();
  static void unlockGlobals();
  static void swapGlobals();
  static SoundSourceMap* getAllForAudio();
  static SoundSourceMap* getAllForEngine();
  /**
  * Returns the current sound signal and proceed to the next time unit
  */
  virtual SAMPLE tick() { return 0.0f; }

private:
  static SoundSourceMap *s_forAudio, *s_forEngine;
  static stk::Mutex s_swapMutex;
};

/**
* Represents a string that can be plucked to make sound
*/
class String : public SoundSource
{
public:
  String(Point2D p1, Point2D p2, float radius);
  ~String();

  void initialize(Point2D p1, Point2D p2, float radius);
  bool pluck(Plucker *plucker);

  // Overrides Widget
  virtual Widget *hitTest(float x, float y);
  virtual void draw();
  virtual bool handleHover(float x, float y);

  virtual void toOutboundPacketStream(osc::OutboundPacketStream&) const;

  void setPadRadius(float);

  // Overrides SoundSource
  virtual SAMPLE tick();

  Line* getLine();

  Side getMouseSide(float, float);
  Side updateMouseSide(float, float);

protected:
  Line *m_line;           // Shape on GUI
  Point2D m_p1, m_p2;     // For determining the pitch
  Spiral *m_p1Dot, *m_p2Dot;
  Plucker *m_lastPlucker;
  Side m_mouseSide;
  stk::Plucked m_plucked; // stk string
  float m_freq;           // Frequency to be plucked
};

/**
* The round shaped platform to put on strings and tracks.
*/
class RoundPad : public Widget
{
public:
  RoundPad(Point2D center, float radius);
  ~RoundPad();

  virtual Point2D getCenter() { return m_center; }
  virtual float getRadius() { return m_radius; }
  virtual void setRadius(float radius);

  virtual void draw();
  virtual Widget *hitTest(float x, float y);

  virtual void toOutboundPacketStream(osc::OutboundPacketStream&) const;

  virtual Text* getCommentText() const { return m_commentText; }
  virtual void setCommentText(const std::string comment);
  virtual void appendCommentText(const std::string comment);

protected:
  Point2D m_center;
  float m_radius;
  Track* m_newTrack;
  Line m_hoverLine, m_dragLine;
  Text* m_commentText;

  virtual bool handleDraw(float x, float y);
  virtual bool handleDrawEnd(float x, float y);
  virtual bool handleHover(float x, float y);

  SpiralTrack* m_newSpiralTrack;
  String* m_newString;
  std::vector<Spiral *> m_circles;
  std::vector<Track *> m_tracks;
};

#endif
