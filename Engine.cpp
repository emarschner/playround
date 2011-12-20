#include <iostream>
#include <stdlib.h>

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

#include "Engine.h"
#include "Network.h"

Engine::Engine() :
    m_selectedWidget(NULL),
    m_newRoundPad(NULL),
    m_mouseCursor(new Spiral(Point2D(0, 0), 360, 5, 0, 5)),
    m_cursorText(new Text(Point2D(-100, 100), "")),
    m_textMode(TEXT_REPLACE),
    m_xLine(Point2D(0, 0), Point2D(0, 0)),
    m_yLine(Point2D(0, 0), Point2D(0, 0))
{
  m_mouseCursor->setColor(Color(51 / (float)255, 153 / (float)255, 1, 1));
  m_mouseCursor->setFilled(true);
  m_cursorText->setColor(Color(51 / (float)255, 153 / (float)255, 1, 1));
  m_xLine.setColor(Color(0.95, 0.95, 0.95, 0.5));
  m_xLine.setLineWidth(0.1);
  m_yLine.setColor(Color(0.95, 0.95, 0.95, 0.5));
  m_yLine.setLineWidth(0.1);
  m_parent = this;
}

Engine::~Engine()
{
  delete m_mouseCursor;
  delete m_cursorText;
}

bool Engine::handleDraw(float x, float y)
{
  m_maxRadius = FLT_MAX;
  for (std::vector<Widget*>::iterator cit = m_children.begin(); cit != m_children.end(); cit++) {
    RoundPad* pad = dynamic_cast<RoundPad*>(*cit);
    if (pad)
      m_maxRadius = std::min(m_maxRadius,
                             Point2D::distance(m_mouseDownPos,
                                               pad->getCenter()) - pad->getRadius());
  }

  if (m_newRoundPad)
    m_newRoundPad->setRadius(std::min(m_maxRadius,
                                      Point2D::distance(m_newRoundPad->getCenter(),
                                                        Point2D(x, y))));
  else
    m_newRoundPad = new RoundPad(m_mouseDownPos, std::min(m_maxRadius,
                                                          Point2D::distance(m_mouseDownPos,
                                                                            Point2D(x, y))));
  return true;
}

bool Engine::handleDrawEnd(float x, float y)
{
  if (m_newRoundPad) {
    Widget::getAll()->insert(WidgetData(m_newRoundPad->getUuid(), m_newRoundPad));
    this->addChild(m_newRoundPad);
    s_network->sendObjectMessage(m_newRoundPad);
    m_newRoundPad = NULL;
  }
  return true;
}

bool Engine::onKeyDown(unsigned char key, float x, float y)
{
   switch(key) {
     case 127:
       if (m_textMode == TEXT_REPLACE && m_selectedWidget &&
           (m_selectedWidget = m_selectedWidget->getParent()->removeChild(m_selectedWidget))) {
         for (int i = 0; i < 5; i++)
           s_network->sendObjectMessage(m_selectedWidget, true);
         delete m_selectedWidget;
         m_selectedWidget = NULL;
       } else if (m_textMode == TEXT_APPEND) {
         std::string text;
         if (m_selectedWidget && dynamic_cast<RoundPad*>(m_selectedWidget)) {
           RoundPad* pad = (RoundPad*)m_selectedWidget;
           text = pad->getCommentText()->getText();
           pad->setCommentText(text.substr(0, text.length() - 1));
           s_network->sendRoundPadTextMessage(pad);
         } else {
           text = m_cursorText->getText();
           m_cursorText->setText(text.substr(0, text.length() - 1));
           s_network->sendPeerTextMessage(m_cursorText->getText());
         }
       }
       break;
     case 27:
       exit(1);
       break;
     default:
       std::string character(1, key);
       if (m_selectedWidget && dynamic_cast<RoundPad*>(m_selectedWidget)) {
         RoundPad* pad = (RoundPad*)m_selectedWidget;
         if (m_textMode == TEXT_REPLACE) {
           pad->setCommentText(character);
           m_textMode = TEXT_APPEND;
         } else
           pad->appendCommentText(character);
         s_network->sendRoundPadTextMessage(pad);
       } else {
         if (m_textMode == TEXT_REPLACE) {
           m_cursorText->setText(character);
           m_textMode = TEXT_APPEND;
         } else
           m_cursorText->setText(m_cursorText->getText() + character);
         s_network->sendPeerTextMessage(m_cursorText->getText());
       }
   }

   glutPostRedisplay( );
   return true;
}

void Engine::draw()
{
  // clear the color and depth buffers
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  // Draw the elements
  for (int i = 0; i < m_children.size(); i ++)
    m_children[i]->draw();

  // Draw in-progress new round pad, if any
  if (m_newRoundPad)
    m_newRoundPad->draw();

  Network::PeerMap* peers = s_network->getPeers();
  for (Network::PeerMap::iterator pit = peers->begin(); pit != peers->end(); pit++)
    pit->second->draw();

  m_xLine.draw();
  m_yLine.draw();
  m_mouseCursor->draw();
  m_cursorText->draw();
}

void Engine::setMouseCursorPosition(float x, float y)
{
  m_mouseCursor->setCenter(Point2D(x, y));
  m_cursorText->setPos(Point2D(x + 10, y + 5));

  for (std::vector<Widget *>::iterator cit = m_children.begin(); cit != m_children.end(); cit++)
    if ((*cit)->hitTest(x, y)) {
      m_xLine.setPoints(Point2D(0, 0), Point2D(0, 0));
      m_yLine.setPoints(Point2D(0, 0), Point2D(0, 0));
      return;
    }

  m_xLine.setPoints(Point2D(0, y), Point2D(m_width, y));
  m_yLine.setPoints(Point2D(x, 0), Point2D(x, m_height));
  return;
}

void Engine::setSize(int w, int h)
{
  m_width = w;
  m_height = h;
}

void Engine::setCursorText(std::string text)
{
  m_cursorText->setText(text);
}
