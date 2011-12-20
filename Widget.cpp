#include <math.h>
#include <stdlib.h>
#include <iostream>
#include <string.h>
#include <sstream>
#include <time.h>
#include <algorithm>

#ifdef __MACOSX_CORE__
#include <GLUT/glut.h>
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#else
#ifdef _WIN32
#include <windows.h>
#endif
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>
#endif

#include "Widget.h"
#include "Engine.h"
#include "Network.h"

// Widget globals
WidgetMap g_widgets;
WidgetMap* Widget::getAll() { return &g_widgets; }

SoundSourceMap g_soundSources1, g_soundSources2;
void SoundSource::initializeGlobals() {
  s_forAudio = &g_soundSources1;
  s_forEngine = &g_soundSources2;
}
void SoundSource::lockGlobals() { s_swapMutex.lock(); }
void SoundSource::unlockGlobals() { s_swapMutex.unlock(); }
void SoundSource::swapGlobals() {
  SoundSourceMap* tmp = s_forAudio;

  s_swapMutex.lock();
  {
    s_forAudio = s_forEngine;
    s_forEngine = tmp;
  }
  s_swapMutex.unlock();

  s_forEngine->clear();
  s_forEngine->insert(s_forAudio->begin(), s_forAudio->end());
}
SoundSourceMap* SoundSource::getAllForAudio() { return s_forAudio; }
SoundSourceMap* SoundSource::getAllForEngine() { return s_forEngine; }
SoundSourceMap* SoundSource::s_forAudio, *SoundSource::s_forEngine;
stk::Mutex SoundSource::s_swapMutex;

// Widget statics
Network* Widget::s_network;

// ==========================
// Base Widget implementation
// ==========================

Widget::Widget() :
    m_fLeftButtonDown(false),
    m_fRightButtonDown(false),
    m_mouseDownOn(NULL),
    m_parent(NULL),
    m_engine(NULL),
    m_uuid("")
{
  uuid_t uuid;
  uuid_generate(uuid);
  char s[37];
  uuid_unparse(uuid, s);
  setUuid(s);
}

Widget::~Widget() {
  for (std::vector<Widget*>::iterator cit = m_children.begin();
       cit != m_children.end();
       cit++)
    delete *cit;
  m_children.clear();

  g_widgets.erase(m_uuid);
}

void Widget::setUuid(const char* uuid)
{
  m_uuid = std::string(uuid);
}

const char* Widget::getUuid() const
{
  return m_uuid.c_str();
}

void Widget::addChild(Widget* child)
{
  child->setParent(this);
  m_children.push_back(child);
}

Widget* Widget::removeChild(Widget* child)
{
  for (std::vector<Widget*>::iterator wit = m_children.begin(); wit != m_children.end(); wit++)
    if (*wit == child) {
      m_children.erase(wit);
      return child;
    }
  return NULL;
}

bool Widget::onMouseDown(int button, float x, float y)
{
  if (button == GLUT_LEFT_BUTTON) {
    m_mouseDownPos = Point2D(x, y);
    m_fLeftButtonDown = true;

    for (int i = m_children.size() - 1; i >= 0; i--)
      if (m_children[i]->hitTest(x, y)) {
        m_mouseDownOn = m_children[i];
        m_mouseDownOn->onMouseDown(button, x, y);
        break;
      }
  } else if (button == GLUT_RIGHT_BUTTON) {
    m_mouseDownPos = Point2D(x, y);
    m_fRightButtonDown = true;

    for (int i = m_children.size() - 1; i >= 0; i--)
      if (m_children[i]->hitTest(x, y)) {
        m_mouseDownOn = m_children[i];
        m_mouseDownOn->onMouseDown(button, x, y);
        break;
      }
  }

  return true;
}

bool Widget::onMouseUp(int button, float x, float y)
{
  if ((m_fLeftButtonDown && button == GLUT_LEFT_BUTTON) ||
      (m_fRightButtonDown && button == GLUT_RIGHT_BUTTON)) {
    for (int i = m_children.size() - 1; i >= 0; i--)
      if (m_children[i]->hitTest(x, y) && m_children[i] == m_mouseDownOn) {
        m_children[i]->handleSelect(x, y, m_fRightButtonDown);
        break;
      }
    this->handleDrawEnd(x, y);
    if (m_mouseDownOn) {
      m_mouseDownOn->onMouseUp(button, x, y);
      m_mouseDownOn = NULL;
    }
    m_fLeftButtonDown = false;
    m_fRightButtonDown = false;
  }

  return true;
}

bool Widget::onMouseMove(float x, float y)
{
  if (m_mouseDownOn) {
    m_mouseDownOn->onMouseMove(x, y);
  } else if (m_fLeftButtonDown) {
    this->handleDraw(x, y);
  } else {
    for (int i = m_children.size() - 1; i >= 0; i--)
      if (m_children[i]->hitTest(x, y)) {
        m_children[i]->onMouseOver(x, y);
        return true;
      }
  }

  return false;
}

bool Widget::onMouseOver(float x, float y)
{
  this->handleHover(x, y);
  for (int i = m_children.size() - 1; i >= 0; i--)
    if (m_children[i]->hitTest(x, y)) {
      m_children[i]->onMouseOver(x, y);
      return true;
    }

  return false;
}

void Widget::drawChildren()
{
  for (int i = 0; i < m_children.size(); i++)
    m_children[i]->draw();
}

void Widget::setParent(Widget *parent)
{
  m_parent = parent;
}

void Widget::setNetwork(Network *network)
{
  s_network = network;
}

void Widget::toOutboundPacketStream(osc::OutboundPacketStream& ps) const
{
}

Widget* Widget::getEngine()
{
  if (!m_engine) {
    Widget* ancestor = m_parent;
    while (ancestor && ancestor != ancestor->getParent())
      ancestor = ancestor->getParent();
    m_engine = ancestor;
  }

  return m_engine;
}

// =======================
// RoundPad implementation
// =======================

RoundPad::RoundPad(Point2D center, float radius) :
    m_center(center),
    m_newTrack(NULL),
    m_newSpiralTrack(NULL),
    m_newString(NULL),
    m_hoverLine(Point2D(0, 0), Point2D(0, 0)),
    m_dragLine(Point2D(0, 0), Point2D(0, 0)),
    m_commentText(new Text(Point2D(-100, -100), ""))
{
  this->setRadius(radius);
  m_hoverLine.setColor(Color(0, 0, 0));
  m_dragLine.setColor(Color(0, 0, 0));
}

RoundPad::~RoundPad()
{
  // Free the circles
  for (int i = 0; i < m_circles.size(); i ++)
    if (m_circles[i]) {
      delete m_circles[i];
      m_circles[i] = NULL;
    }
  // Special check for line track whose parent is not this round pad but has the end joint on it
  std::vector<LineTrack *> lineTracks;
  WidgetMap *widgets = Widget::getAll();
  for (WidgetMap::iterator it = widgets->begin(); it != widgets->end(); it ++)
  {
    LineTrack *lineTrack = dynamic_cast<LineTrack *>(it->second);
    if (lineTrack && lineTrack->getParent() != this &&
      (lineTrack->getJoint1()->getParentRoundPad() == this ||
       lineTrack->getJoint2()->getParentRoundPad() == this))
    {
      lineTracks.push_back(lineTrack);
    }
  }
  for (int i = 0; i < lineTracks.size(); i ++)
  {
    lineTracks[i]->getParent()->removeChild(lineTracks[i]);
    lineTracks[i] = NULL;
  }

}

void RoundPad::setRadius(float radius) {
  m_radius = radius;

  // Delete old circles
  for (std::vector<Spiral*>::iterator cit = m_circles.begin();
       cit != m_circles.end();
       cit++)
    delete *cit;
  m_circles.clear();

  // Generate circles for rendering
  Spiral* circle;
  for (int i = 30; i <= m_radius; i += 20) {
    circle = new Spiral(m_center, 360, i, 0, i);
    circle->setColor(Color(0, 0, 0, 0.1));
    circle->setLineWidth(1);
    m_circles.push_back(circle);
  }

  circle = new Spiral(m_center, 360, m_radius, 0, m_radius);
  m_circles.push_back(circle);

  circle = new Spiral(m_center, 360, m_radius, 0, m_radius);
  circle->setFilled(true);
  circle->setColor(Color(0.75, 0.75, 0.75, 0.1));
  m_circles.push_back(circle);

  circle = new Spiral(m_center, 360, 10, 0, 10);
  circle->setFilled(true);
  m_circles.push_back(circle);

  m_commentText->setPos(Point2D(m_center.x - m_commentText->getWidth() / 2,
                                m_center.y + m_radius + 20));
}

void RoundPad::draw()
{
  if (m_newSpiralTrack)
    m_newSpiralTrack->draw();
  if (m_newString)
    m_newString->draw();

  for (int i = 0; i < m_circles.size(); i++)
    m_circles[i]->draw();

  drawChildren();

  m_hoverLine.draw();
  m_dragLine.draw();

  m_commentText->draw();
}

Widget *RoundPad::hitTest(float x, float y)
{
  Point2D p(x, y);
  float dist = Point2D::distance(m_center, p);

  if (dist < m_radius)
    return this;

  m_hoverLine.setPoints(Point2D(0, 0), Point2D(0, 0));
  return NULL;
}

void RoundPad::toOutboundPacketStream(osc::OutboundPacketStream& ps) const
{
  ps.Clear();
  ps << osc::BeginMessage("/object/pad")
     << osc::Symbol(m_uuid.c_str()) << m_center.x << m_center.y << m_radius
     << osc::EndMessage;
}

bool RoundPad::handleDraw(float x, float y)
{
  // Draw guide-lines
  float angle = Spiral::getAngle(m_center, Point2D(x, y));
  m_dragLine.setPoints(Spiral::getPointFromRadius(m_center, -m_radius, angle),
                       Spiral::getPointFromRadius(m_center, m_radius, angle));

  Point2D pos(x, y);
  float startAngle = Spiral::getAngle(m_center, m_mouseDownPos);
  m_hoverLine.setPoints(Spiral::getPointFromRadius(m_center, -m_radius, startAngle),
                        Spiral::getPointFromRadius(m_center, m_radius, startAngle));

  // Decide between drawing a string or a track
  Line radialLine(Spiral::getPointFromRadius(m_center, 0, startAngle),
                  Spiral::getPointFromRadius(m_center, m_radius, startAngle));
  if (radialLine.getParallelPosition(pos) >= 0 && fabs(radialLine.getDistance(pos)) < 10) {
    Point2D endPoint =
      Spiral::getPointFromRadius(m_center,
                                 std::min(m_radius, Point2D::distance(m_center, pos)),
                                 startAngle);
    if (m_newString)
      m_newString->initialize(m_mouseDownPos, endPoint, m_radius);
    else {
      m_newString = new String(m_mouseDownPos, endPoint, m_radius);
      SoundSource::getAllForEngine()->insert(SoundSourceData(m_newString->getUuid(), m_newString));
    }

    if (m_newSpiralTrack) {
      delete m_newSpiralTrack;
      m_newSpiralTrack = NULL;
    }
  } else {
    // Find whether the position corresponds to other end point
    float endAngle = Spiral::getAngle(m_center, pos);
    float startRadius = Point2D::distance(m_center, m_mouseDownPos);
    float endRadius = std::min(m_radius, Point2D::distance(m_center, pos));
    Joint *endJoint = NULL;

    for (int i = m_children.size() - 1; i >= 0; i --) {
      Track *track = dynamic_cast<Track *>(m_children[i]);
      if (track) {
        Joint *joint = track->getJoint1();
        if (joint && joint->hitTest(x, y)) {
          endJoint = joint;
          break;
        }

        joint = track->getJoint2();
        if (joint && joint->hitTest(x, y)) {
          endJoint = joint;
          break;
        }
      }
    }

    if (m_newSpiralTrack)
      m_newSpiralTrack->initialize(m_center, startAngle, startRadius, endAngle, endRadius, NULL, endJoint);
    else {
      m_newSpiralTrack = new SpiralTrack(m_center, startAngle, startRadius,
                                                   endAngle, endRadius, NULL, endJoint);
      m_newSpiralTrack->getSpiral()->setLineWidth(1);
      m_newSpiralTrack->setEnabled(false);
    }
    m_newSpiralTrack->getJoint1()->setParentRoundPad(this);
    m_newSpiralTrack->getJoint2()->setParentRoundPad(this);

    if (m_newString) {
      delete m_newString;
      m_newString = NULL;
    }
  }
  return true;
}

bool RoundPad::handleDrawEnd(float x, float y)
{
  m_hoverLine.setPoints(Point2D(0, 0), Point2D(0, 0));
  m_dragLine.setPoints(Point2D(0, 0), Point2D(0, 0));

  if (m_newSpiralTrack) {
    m_newSpiralTrack->getSpiral()->setLineWidth(3);
    m_newSpiralTrack->setEnabled(true);
    g_widgets.insert(WidgetData(m_newSpiralTrack->getUuid(), m_newSpiralTrack));
    addChild(m_newSpiralTrack);
    s_network->sendObjectMessage(m_newSpiralTrack);
    m_newSpiralTrack = NULL;
  }
  if (m_newString) {
    g_widgets.insert(WidgetData(m_newString->getUuid(), m_newString));
    addChild(m_newString);
    s_network->sendObjectMessage(m_newString);
    m_newString = NULL;
  }
  return true;
}

bool RoundPad::handleHover(float x, float y)
{
  Color color = Color(0, 0, 0, 1);
  if (Point2D::distance(m_center, Point2D(x, y)) <= 10) {
    m_hoverLine.setPoints(Point2D(0, 0), Point2D(0, 0));
    bool childHovered = false;
    for (int i = m_children.size() - 1; i >= 0; i--)
      if (m_children[i]->hitTest(x, y)) {
        childHovered = true;
        break;
      }
    if (!childHovered) {
      ((Engine*)getEngine())->setSelectedWidget(this);
      color = Color(1, .5, 0, 1);
    }
  } else {
    float angle = Spiral::getAngle(m_center, Point2D(x, y));
    m_hoverLine.setPoints(Spiral::getPointFromRadius(m_center, -m_radius, angle),
                          Spiral::getPointFromRadius(m_center, m_radius, angle));
  }
  (*(m_circles.end() - 1))->setColor(color);
}

void RoundPad::setCommentText(std::string comment)
{
  m_commentText->setText(comment);
  m_commentText->setPos(Point2D(m_center.x - m_commentText->getWidth() / 2,
                                m_center.y + m_radius + 30));
}

void RoundPad::appendCommentText(std::string comment)
{
  m_commentText->setText(m_commentText->getText() + comment);
  m_commentText->setPos(Point2D(m_center.x - m_commentText->getWidth() / 2,
                                m_center.y + m_radius + 30));
}

// ====================
// Joint implementation
// ====================

Joint::Joint(Point2D center, float radius) :
    m_center(center),
    m_radius(radius),
    m_newSpiralTrack(NULL),
    m_newLineTrack(NULL),
    m_drawing(false),
    m_dragLine(Point2D(0, 0), Point2D(0, 0)),
    m_parentRoundPad(NULL)
{
  m_circle = new Spiral(center, 360, radius, 0, radius);
  m_circle->setLineWidth(3);
  m_circle->setFilled(true);
  m_dragLine.setColor(Color(0, 0, 0));
}

Joint::~Joint()
{
  delete m_circle;
}

void Joint::draw()
{
  // Prevent recursive draw
  if (m_drawing)
    return;

  m_drawing = true;
  m_circle->draw();
  m_dragLine.draw();

  if (m_newSpiralTrack)
    m_newSpiralTrack->draw();
  if (m_newLineTrack)
    m_newLineTrack->draw();

  m_drawing = false;
}

void Joint::addTrack(Track *track)
{
  if (track)
    if (std::find(m_tracks.begin(), m_tracks.end(), track) == m_tracks.end())
      m_tracks.push_back(track);
}

Widget *Joint::hitTest(float x, float y)
{
  if (Point2D::distance(m_center, Point2D(x, y)) <= m_radius)
    return this;
  return NULL;
}

void Joint::setCenter(Point2D center)
{
  //std::cerr << "Joint::setCenter" << std::endl;
  //std::cerr << m_center.x << " " << m_center.y << std::endl;
  m_center = center;
  m_circle->setCenter(m_center);
}

void Joint::setColor(Color color)
{
  m_circle->setColor(color);
}

bool Joint::handleDraw(float x, float y)
{
  Point2D center;
  float radius;
  RoundPad *pad;

  // This shouldn't happen actually
  if (m_tracks.size() <= 0)
    return false;

  pad = getParentRoundPad();
  // Can't find any parent round pad so the operation fails
  if (!pad)
    return false;

  center = pad->getCenter();
  radius = pad->getRadius();

  // Find whether the position corresponds to other end point
  Joint *endJoint = NULL;
  WidgetMap widgets = *Widget::getAll();
  for (WidgetMap::iterator ptr = widgets.begin(); ptr != widgets.end(); ptr ++) {
    Track *track = dynamic_cast<Track *>(ptr->second);
    if (track) {
      Joint *joint = track->getJoint1();
      if (joint && joint != this && joint->hitTest(x, y)) {
        endJoint = joint;
        break;
      }
      joint = track->getJoint2();
      if (joint && joint != this && joint->hitTest(x, y)) {
        endJoint = joint;
        break;
      }
    }
  }

  float angle = Spiral::getAngle(center, Point2D(x, y));
  m_dragLine.setPoints(Spiral::getPointFromRadius(center, -radius, angle),
                       Spiral::getPointFromRadius(center, radius, angle));
  Point2D pos(x, y);
  float startAngle = Spiral::getAngle(center, m_mouseDownPos),
        endAngle = Spiral::getAngle(center, pos);

  if (endJoint && pad != endJoint->getParentRoundPad()) {
    //std::cerr << "Joint::handleDraw: Connecting two joints in differnet pads" << std::endl;
    Point2D endPoint =
      Spiral::getPointFromRadius(center,
                                 std::min(radius, Point2D::distance(center, pos)),
                                 startAngle);
    if (m_newLineTrack)
      m_newLineTrack->initialize(m_mouseDownPos, endPoint, this, endJoint);
    else {
      m_newLineTrack = new LineTrack(m_mouseDownPos, endPoint, this, endJoint);
      m_newLineTrack->getLine()->setLineWidth(1);
      m_newLineTrack->setEnabled(false);
    }

    if (m_newSpiralTrack) {
      delete m_newSpiralTrack;
      m_newSpiralTrack = NULL;
    }
  } else {
    float startRadius = Point2D::distance(center, m_mouseDownPos);
    float endRadius = std::min(radius, Point2D::distance(center, pos));
    if (m_newSpiralTrack)
      m_newSpiralTrack->initialize(center, startAngle, startRadius, endAngle, endRadius, this, endJoint);
    else
    {
      m_newSpiralTrack = new SpiralTrack(center, startAngle, startRadius,
                                                   endAngle, endRadius, this, endJoint);
      m_newSpiralTrack->getSpiral()->setLineWidth(1);
      m_newSpiralTrack->setEnabled(false);
    }
    m_newSpiralTrack->getJoint2()->setParentRoundPad(pad);
    if (m_newLineTrack) {
      delete m_newLineTrack;
      m_newLineTrack = NULL;
    }
  }
  return true;
}

bool Joint::handleDrawEnd(float x, float y)
{
  RoundPad *pad = getParentRoundPad();

  if (!pad)
    return false;

  m_dragLine.setPoints(Point2D(0, 0), Point2D(0, 0));

  if (m_newSpiralTrack) {
    m_newSpiralTrack->getSpiral()->setLineWidth(3);
    m_newSpiralTrack->setEnabled(true);
    g_widgets.insert(WidgetData(m_newSpiralTrack->getUuid(), m_newSpiralTrack));
    pad->addChild(m_newSpiralTrack);
    s_network->sendObjectMessage(m_newSpiralTrack);
    m_newSpiralTrack = NULL;
  }
  if (m_newLineTrack) {
    m_newLineTrack->getLine()->setLineWidth(3);
    m_newLineTrack->setEnabled(true);
    g_widgets.insert(WidgetData(m_newLineTrack->getUuid(), m_newLineTrack));
    pad->addChild(m_newLineTrack);
    s_network->sendObjectMessage(m_newLineTrack);
    m_newLineTrack = NULL;
  }

  return true;
}

bool Joint::handleSelect(float x, float y, bool send)
{
  for (int i = 0; i < m_tracks.size(); i ++) {
    Track* track = m_tracks[i];

    // If it's the start point
    if (track->getJoint1() == this) {
      track->addPlucker();
      if (send)
        s_network->sendPlucker(track);
    }
  }
  return true;
}

std::string Joint::toString()
{
  std::ostringstream os;
  os << "Joint Center: " << m_center.x << " " << m_center.y
     <<  " Tracks: " << m_tracks.size();
  return os.str();
}

// =========================
// Base Track implementation
// =========================

Track::Track() :
  m_joint1(NULL),
  m_joint2(NULL),
  m_fEnabled(true),
  m_fActive(true),
  m_fDirected(true),
  m_fImmediate(false)
{
}

// Small hack to prevent a joint to be deleted in Widget::~Widget()
Track::~Track() {
  //std::cerr << "Track::~Track" << std::endl;

  for (std::vector<Widget*>::iterator cit = m_children.begin();
       cit != m_children.end();
       cit++)
  {
    Joint *joint = dynamic_cast<Joint *>(*cit);
    if (joint)
    {
      std::vector<Track *> *tracks = joint->getTracks();
      std::vector<Track *>::iterator ptr = std::find(tracks->begin(), tracks->end(), this);
      if (ptr != tracks->end())
        tracks->erase(ptr);
      //std::cerr << tracks->size() << std::endl;
      if (tracks->size() == 0)
        delete *cit;
    }
    else
      delete *cit;
  }
  m_children.clear();

}

void Track::detachJoint(Joint *joint)
{
  std::vector<Widget *>::iterator ptr;
  std::vector<Track *>::iterator trackPtr;
  if (joint)
  {
    ptr = std::find(m_children.begin(), m_children.end(), joint);
    if (ptr != m_children.end())
      m_children.erase(ptr);
    std::vector<Track *> tracks = *joint->getTracks();
    // remove the track from the joint
    trackPtr = std::find(tracks.begin(), tracks.end(), this);
    if (trackPtr != tracks.end())
      tracks.erase(trackPtr);
    if (tracks.size() == 0)
      delete joint;
    joint = NULL;
  }
}

void Track::setupJoints(Joint *joint1, Joint *joint2)
{
  this->detachJoint(m_joint1);
  this->detachJoint(m_joint2);
  m_joint1 = NULL;
  m_joint2 = NULL;

  if (joint1)
    m_joint1 = joint1;
  else {
    // Set to origin first
    m_joint1 = new Joint(Point2D(0, 0), 7);
    m_joint1->setColor(Color(0.2, 0.6, 0.3));
  }

  if (joint2)
    m_joint2 = joint2;
  else {
      m_joint2 = new Joint(Point2D(0, 0), 7);
      m_joint2->setColor(Color(0.6, 0.2, 0.3));
  }

  m_joint1->addTrack(this);
  m_joint2->addTrack(this);
  m_children.insert(m_children.begin(), m_joint2);
  m_children.insert(m_children.begin(), m_joint1);
}

void Track::addPlucker()
{
  Plucker *plucker = new Plucker(this, getJoint1(), getJoint2());
  this->addChild(plucker);
}

void Track::drawChildren()
{
  for (int i = 0; i < m_children.size(); i++) {
    Plucker* plucker = dynamic_cast<Plucker*>(m_children[i]);
    if (plucker) {
      plucker->tick();

      if (plucker->isAtEnd()) {
        std::vector<Plucker *> pluckers = plucker->split();
        delete plucker;
        m_children.erase(m_children.begin() + i);
        i--;

        for (int j = 0; j < pluckers.size(); j++)
          pluckers[j]->draw();
      } else
        plucker->draw();
    } else
      m_children[i]->draw();
  }
}

// =======================
// SpiralTrack implementation
// =======================

SpiralTrack::SpiralTrack(Point2D center, float startAngle, float startRadius,
                                         float endAngle, float endRadius,
                                         Joint *joint1, Joint *joint2) :
  Track(),
  m_center(center),
  m_startRadius(startRadius),
  m_endRadius(endRadius),
  m_startAngle(startAngle),
  m_endAngle(endAngle),
  m_spiral(NULL)
{
  m_fActive = true;

  initialize(center, startAngle, startRadius, endAngle, endRadius, joint1, joint2);
}

std::string SpiralTrack::toString()
{
  std::ostringstream os;
  os << "center: " << m_center.x << ", " << m_center.y << "; "
     << "start: " << m_startRadius << ", " << m_startAngle << "; "
     << "end: " << m_endRadius << ", " << m_endAngle << "; "
     << "joint1: " << m_joint1->toString() << "; "
     << "joint2: " << m_joint2->toString();
  return os.str();
}

SpiralTrack::~SpiralTrack()
{
  delete m_spiral;
}

void SpiralTrack::initialize(Point2D center, float startAngle, float startRadius,
                                             float endAngle, float endRadius, Joint *joint1, Joint *joint2) {
  setupJoints(joint1, joint2);

  // Adjust the settings based on the joints
  if (m_joint1->getTracks()->size() > 1)
  {
    startAngle = Spiral::getAngle(center, m_joint1->getCenter()),
    startRadius = Point2D::distance(center, m_joint1->getCenter());
  }

  if (m_joint2->getTracks()->size() > 1)
  {
    endAngle = Spiral::getAngle(center, m_joint2->getCenter()),
    endRadius = Point2D::distance(center, m_joint2->getCenter());
  }

  // Restrict the angle to be between 0 and 360
  m_startAngle = Spiral::clampAngle(startAngle);
  m_endAngle = Spiral::clampAngle(endAngle);

  m_startRadius = startRadius;
  m_endRadius = endRadius;

  if (m_spiral) {
    m_spiral->setRadii(m_startRadius, m_endRadius);
    m_spiral->setCenter(center);
    m_spiral->setAngles(m_startAngle, m_endAngle);
  } else
    m_spiral = new Spiral(center, m_startAngle, m_startRadius, m_endAngle, m_endRadius);

  if (!joint1)
    m_joint1->setCenter(getEndPoint1());

  if (!joint2)
    m_joint2->setCenter(getEndPoint2());

}

void SpiralTrack::draw()
{
  m_spiral->setDotted(!m_fActive);
  m_spiral->setDirected(m_fDirected);

  m_spiral->draw();
  drawChildren();

}

Point2D SpiralTrack::getEndPoint1()
{
  float x = m_center.x + m_startRadius * cos((double) m_startAngle * MY_PI / 180),
        y = m_center.y - m_startRadius * sin((double) m_startAngle * MY_PI / 180);
  return Point2D(x, y);
}

Point2D SpiralTrack::getEndPoint2()
{
  float x = m_center.x + m_endRadius * cos((double) m_endAngle * MY_PI / 180),
        y = m_center.y - m_endRadius * sin((double) m_endAngle * MY_PI / 180);
  return Point2D(x, y);
}

Widget *SpiralTrack::hitTest(float x, float y)
{
  if (m_joint1->hitTest(x, y) || m_joint2->hitTest(x, y))
    return this;

  Widget* hit = NULL;
  Color color(0, 0, 0, 1);

  Point2D p(x, y);
  float angle = Spiral::getAngle(m_center, p);
  if (angle > m_endAngle && m_endAngle > m_startAngle || angle < m_startAngle) {
    float baseAngleDiff = m_startAngle - m_endAngle;
    float angleDiff = m_startAngle - angle;
    if (m_endAngle > m_startAngle) {
      baseAngleDiff += 360;
      if (angle > m_startAngle)
        angleDiff += 360;
    }
    float radius = m_endRadius + (m_startRadius - m_endRadius) * (1 - angleDiff / baseAngleDiff);
    float dist = Point2D::distance(m_center, p);
    if (fabs(radius - dist) < 5) {
      color = Color(1, .5, 0, 1);
      hit = this;
    }
  }

  m_spiral->setColor(color);
  return hit;
}

void SpiralTrack::toOutboundPacketStream(osc::OutboundPacketStream& ps) const
{
  ps.Clear();
  ps << osc::BeginMessage("/object/track/spiral")
     << osc::Symbol(m_uuid.c_str()) << osc::Symbol(m_parent->getUuid())
     << m_startAngle << m_startRadius << m_endAngle << m_endRadius
     << osc::EndMessage;
}

Point2D SpiralTrack::getNextPos(bool fReverse, float distance, Point2D pos)
{
  float oradius = m_startRadius,
        dradius = m_endRadius,
        oangle = m_startAngle,
        dangle = m_endAngle,
        cangle = Spiral::getAngle(m_center, pos);
  /*
  if (fabs(cangle - oangle) < 1)
    cangle = oangle;

  if (fabs(cangle - dangle) < 1)
    cangle = dangle;
  */
  //std::cerr << pos.x << ", " << pos.y << std::endl;
  // adjust the value of the angle for more convenient computation
  for (; cangle > oangle; cangle -= 360);
  for (; dangle > cangle; dangle -= 360);

  float oangleR = oangle * MY_PI / 180,
        dangleR = dangle * MY_PI / 180,
        cangleR = cangle * MY_PI / 180;

  float angleDiff = oangle - dangle,
        angleDiffR = angleDiff * MY_PI / 180;

  if (angleDiff == 0)
    return pos;

  float radiusDiff = dradius - oradius,
        rate = radiusDiff / angleDiff,
        rateR = radiusDiff / angleDiffR;

  float newAngleR;
  //std::cerr << rateR << std::endl;
  if (fabs(radiusDiff) < 0.0001)
  {
    newAngleR = cangleR - distance / oradius;
    // std::cerr << distance / oradius << std::endl;
  }
  else 
  {
    float a = rateR / 2,
          b = -oradius - rateR * oangleR,
          c = oradius * cangleR + rateR * oangleR * cangleR - rateR * cangleR * cangleR / 2 - distance;
    // new angle
    float sol1 = (-b + sqrt(b * b - 4 * a * c)) / (2 * a),
          sol2 = (-b - sqrt(b * b - 4 * a * c)) / (2 * a);

    newAngleR = (sol1 <= cangleR && sol1 >= dangleR) ? sol1 :
                (sol2 <= cangleR && sol2 >= dangleR) ? sol2 : dangleR;
  }
  float newRadius = oradius + (oangleR - newAngleR) * rateR;
  Point2D result = Point2D(m_center.x + newRadius * cos(newAngleR), m_center.y - newRadius * sin(newAngleR));
  /*
  std::cerr << "SpiralTrack::getNextPos: from " << pos.x << ", " << pos.y
            << " to " << result.x << ", " << result.y
            << " distance: " << distance
            << " start angle: " << oangle << " end angle: " << dangle 
            << " current angle: " << cangle << " new angle " << (newAngleR * 180 / MY_PI)
            << " old radius" << oradius << " new radius " << newRadius << std::endl;
  */
  return result;
}

Point2D SpiralTrack::getCenter()
{
  return m_center;
}

void SpiralTrack::setCenter(const Point2D& center)
{
  m_center = center;
  m_spiral->setCenter(m_center);
  m_joint1->setCenter(getEndPoint1());
  m_joint2->setCenter(getEndPoint2());
}

void SpiralTrack::setEndAngle(float angle)
{
  m_endAngle = Spiral::clampAngle(angle);
  m_spiral->setEndAngle(m_endAngle);
  m_joint2->setCenter(getEndPoint2());
}

void SpiralTrack::setEndRadius(float radius)
{
  m_endRadius = radius;
  m_spiral->setEndRadius(m_endRadius);
  m_joint2->setCenter(getEndPoint2());
}

float SpiralTrack::getStartAngle()
{
  return m_startAngle;
}

float SpiralTrack::getEndAngle()
{
  return m_endAngle;
}

bool SpiralTrack::handleHover(float x, float y)
{
  ((Engine*)getEngine())->setSelectedWidget(this);

  return true;
}


// ========================
// LineTrack implementation
// ========================

LineTrack::LineTrack(Point2D p1, Point2D p2, Joint *joint1, Joint *joint2) :
    Track(),
    m_line(NULL)
{
  m_fActive = true;

  initialize(p1, p2, joint1, joint2);

  m_line->setLineWidth(3);
}

LineTrack::~LineTrack()
{
  delete m_line;
}

void LineTrack::initialize(Point2D p1, Point2D p2, Joint *joint1, Joint *joint2)
{
  if (joint1)
    p1 = joint1->getCenter();

  if (joint2)
    p2 = joint2->getCenter();

  m_p1 = p1;
  m_p2 = p2;

  setupJoints(joint1, joint2);

  if (!joint1)
    m_joint1->setCenter(getEndPoint1());

  if (!joint2)
    m_joint2->setCenter(getEndPoint2());

  if (m_line)
    m_line->setPoints(m_p1, m_p2);
  else
    m_line = new Line(m_p1, m_p2);
}

void LineTrack::draw()
{
  m_line->setDotted(!m_fActive);
  m_line->setDirected(m_fDirected);

  drawChildren();

  m_line->draw();
  m_joint1->draw();
  m_joint2->draw();
}

Widget *LineTrack::hitTest(float x, float y)
{
  Point2D p(x, y);
  Widget* hit = NULL;
  Color color;
  float ppos = m_line->getParallelPosition(p);
  if (m_joint1->hitTest(x, y) || m_joint2->hitTest(x, y) ||
      ppos > 0 && ppos < 1 && fabs(m_line->getDistance(p)) < 5) {
    color = Color(1, .5, 0, 1);
    hit = this;
  } else
    color = Color(0, 0, 0, 1);

  m_line->setColor(color);
  return hit;
}

void LineTrack::toOutboundPacketStream(osc::OutboundPacketStream& ps) const
{
  ps.Clear();
  ps << osc::BeginMessage("/object/track/line")
     << osc::Symbol(m_uuid.c_str()) << osc::Symbol(m_parent->getUuid())
     << m_p1.x << m_p1.y << m_p2.x << m_p2.y
     << osc::EndMessage;
}

Point2D LineTrack::getNextPos(bool fReverse, float distance, Point2D pos)
{
  float dx = m_p2.x - m_p1.x,
        dy = m_p2.y - m_p1.y;
  float dist = Point2D::distance(m_p1, m_p2);
  Point2D res;

  if (!fReverse) {
    res.x = pos.x + dx / dist * distance;
    res.y = pos.y + dy / dist * distance;
  } else {
    res.x = pos.x - dx / dist * distance;
    res.y = pos.y - dy / dist * distance;
  }

  if (Point2D::distance(res, m_p1) > dist)
    res = m_p2;
  else if (Point2D::distance(res, m_p2) > dist)
    res = m_p1;

  return res;
}

bool LineTrack::handleHover(float x, float y)
{
  ((Engine*)getEngine())->setSelectedWidget(this);

  return true;
}

// ======================
// Plucker implementation
// ======================

Plucker::Plucker(Track *track, Joint *startJoint, Joint *endJoint) :
  m_track(track),
  m_startJoint(startJoint),
  m_endJoint(endJoint)
{
  if (track) {
    // If joints don't match track
    if (!((track->getJoint1() == startJoint && track->getJoint2() == endJoint) ||
          (track->getJoint2() == startJoint && track->getJoint1() == endJoint)) ||
          (!startJoint || !endJoint)) {
      m_startJoint = track->getJoint1();
      m_endJoint = track->getJoint2();
    }

    if (!m_startJoint)
      std::cerr << "Plucker::Plucker: start joint is NULL" << std::endl;
    m_pos = m_startJoint->getCenter();
    m_circle = new Spiral(m_pos, 360, 10, 0, 10);
    m_circle->setFilled(true);
    m_circle->setColor(Color(0, 0, 0, 0.5));
  } else
    std::cerr << "Plucker::Plucker: track is NULL" << std::endl;
}

std::vector<Plucker *> Plucker::split()
{
  // std::cerr << "Plucker splitting" << std::endl;
  std::vector<Plucker *> results;

  if (isAtEnd()) {
    std::vector<Track *> tracks = *m_endJoint->getTracks();
    //std::cerr << tracks.size() << std::endl;
    for (int i = 0; i < tracks.size();i ++)
      // Split if there are other not directed paths or directed paths
      // with their start joints being the end joints of the current path
      if (tracks[i] != m_track &&
          (tracks[i]->isEnabled()) && // Make sure that it's not a drawing one
          (!tracks[i]->isDirected() || tracks[i]->getJoint1() == m_endJoint)) {
        Plucker *plucker = new Plucker(tracks[i], m_endJoint,
          (tracks[i]->getJoint1() == m_endJoint) ? tracks[i]->getJoint2() : tracks[i]->getJoint1());

        tracks[i]->addChild(plucker);
        results.push_back(plucker);
      }
  }

  return results;
}

Side Plucker::getSideOfString(String* string)
{
  StringStateMap::iterator sit = m_stringStates.find(string->getUuid());
  if (sit == m_stringStates.end())
    updateSideOfString(string);
  return sit->second;
}

Side Plucker::updateSideOfString(String* string)
{
  StringStateMap::iterator sit = m_stringStates.find(string->getUuid());
  if (sit == m_stringStates.end()) {
    StringStateData data(string->getUuid(), 0);
    sit = m_stringStates.insert(data).first;
  }
  float dist = string->getLine()->getDistance(m_pos);
  if (dist < 0)
    sit->second = LEFT_SIDE;
  else
    sit->second = RIGHT_SIDE;
  return sit->second;
}

// Check whether the plucker has reached the end point of a track
bool Plucker::isAtEnd()
{
  //std::cerr << "Plucker::isAtEnd: " << m_pos.x << ", " << m_pos.y << " " << m_endJoint->toString() << std::endl;
  return m_endJoint->hitTest(m_pos.x, m_pos.y);
}

void Plucker::draw()
{
  m_circle->draw();
}

void Plucker::tick()
{
  RoundPad *pad[2];
  pad[0] = m_track->getJoint1()->getParentRoundPad();
  pad[1] = m_track->getJoint2()->getParentRoundPad();
  for (int i = 0; i < 2; i ++)
  {
    if (!pad[i])
      continue;
    const std::vector<Widget *> *uncles = pad[i]->getChildren();
    for (int j = 0; j < uncles->size(); j ++) {
      String* padString = dynamic_cast<String*>(uncles->at(j));
      if (padString) {
        float dist = padString->getLine()->getDistance(m_pos);
        float ppos = padString->getLine()->getParallelPosition(m_pos);
        if (ppos > 0 && ppos < 1 && dist < 10 && dist > -10 && dist * getSideOfString(padString) < 0)
          padString->pluck(this);
        updateSideOfString(padString);
      }
    }
  }

  if (!isAtEnd()) {
    m_pos = m_track->getNextPos(!(m_startJoint == m_track->getJoint1()),
                                PLUCKER_SPEED, m_pos);
    m_circle->setCenter(m_pos);
  }
}

Plucker::~Plucker()
{
  if (m_circle)
    delete m_circle;
}

String::String(Point2D p1, Point2D p2, float radius) :
  m_line(NULL),
  m_lastPlucker(NULL),
  m_mouseSide(0)
{
  initialize(p1, p2, radius);
  m_line->setLineWidth(2);
}

String::~String()
{
  delete m_line;
  delete m_p1Dot;
  delete m_p2Dot;
  SoundSource::lockGlobals();
  {
    // Erases are super fast...right?
    SoundSource::getAllForAudio()->erase(m_uuid);
  }
  SoundSource::unlockGlobals();
  SoundSource::getAllForEngine()->erase(m_uuid);
}

void String::initialize(Point2D p1, Point2D p2, float radius)
{
  m_p1 = p1;
  m_p2 = p2;

  m_p1Dot = new Spiral(m_p1, 360, 5, 0, 5);
  m_p1Dot->setFilled(true);
  m_p2Dot = new Spiral(m_p2, 360, 5, 0, 5);
  m_p2Dot->setFilled(true);

  if (m_line)
    m_line->setPoints(m_p1, m_p2);
  else
    m_line = new Line(m_p1, m_p2);
  
  setPadRadius(radius);
}

Side String::getMouseSide(float x, float y)
{
  if (m_mouseSide == 0)
    updateMouseSide(x, y);
  return m_mouseSide;
}

Side String::updateMouseSide(float x, float y)
{
  float dist = m_line->getDistance(Point2D(x, y));
  if (dist < 0)
    m_mouseSide = LEFT_SIDE;
  else
    m_mouseSide = RIGHT_SIDE;
  return m_mouseSide;
}

void String::setPadRadius(float radius)
{
  m_plucked.noteOff(1);

  if (radius < 0.001)
    m_freq = 880.;
  else {
    m_freq = std::min(1.f, Point2D::distance(m_p1, m_p2) / radius);
    m_freq = 880. - m_freq * 770.;
  }

  m_plucked.noteOn(m_freq, 1);
}

Widget *String::hitTest(float x, float y)
{
  Point2D p(x, y);
  float ppos = m_line->getParallelPosition(p);
  if (Point2D::distance(m_p1, p) <= m_p1Dot->getStartRadius() ||
      Point2D::distance(m_p2, p) <= m_p2Dot->getStartRadius() ||
      ppos > 0 && ppos < 1 && fabs(m_line->getDistance(p)) < 5) {
    Color color(1, .5, 0, 1);
    m_line->setColor(color);
    m_p1Dot->setColor(color);
    m_p2Dot->setColor(color);
    return this;
  }

  Color color(0, 0, 0, 1);
  m_line->setColor(color);
  m_p1Dot->setColor(color);
  m_p2Dot->setColor(color);
  return NULL;
}

void String::toOutboundPacketStream(osc::OutboundPacketStream& ps) const
{
  ps.Clear();
  ps << osc::BeginMessage("/object/track/string")
     << osc::Symbol(m_uuid.c_str()) << osc::Symbol(m_parent->getUuid())
     << m_p1.x << m_p1.y << m_p2.x << m_p2.y
     << osc::EndMessage;
}

bool String::pluck(Plucker *plucker)
{
  m_plucked.noteOn(m_freq, 1);
  return true;
}

void String::draw()
{
  m_line->draw();
  m_p1Dot->draw();
  m_p2Dot->draw();
}

SAMPLE String::tick()
{
  return m_plucked.tick();
}

bool String::handleHover(float x, float y)
{
  ((Engine*)getEngine())->setSelectedWidget(this);

  Point2D pos(x, y);
  float dist = m_line->getDistance(pos);
  float ppos = m_line->getParallelPosition(pos);
  if (ppos > 0 && ppos < 1 && dist * getMouseSide(x, y) < 0)
    pluck(NULL);
  updateMouseSide(x, y);

  return true;
}

Line* String::getLine()
{
  return m_line;
}
