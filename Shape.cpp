#include <math.h>

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

#include <iostream>
#include <time.h>
#include <stdlib.h>

#include "Shape.h"

// ==================
// Arc implementation
// ==================

Spiral::Spiral(Point2D center, float startAngle, float startRadius,
                               float endAngle, float endRadius) :
  m_center(center),
  m_startRadius(startRadius),
  m_endRadius(endRadius),
  m_startAngle(startAngle),
  m_endAngle(endAngle),
  m_filled(false)
{
  m_fDotted = false;
  m_fDirected = false;
  m_lineWidth = 1;
  m_color = Color(0, 0, 0);
}

void Spiral::setFilled(bool filled)
{
  m_filled = filled;
}

void Spiral::setColor(const Color& color)
{
  m_color = color;
}

void Spiral::draw()
{
  float end = m_endAngle;
  if (m_endAngle > m_startAngle)
    end -= 360;

  glColor4f(m_color.r, m_color.g, m_color.b, m_color.a);

  /*
  if (m_fDotted) {
    glEnable(GL_LINE_STIPPLE);
    glLineStipple(1, 0xFF00);
  }
  */

  float angle, x, y, interpRadius;
  float maxRadius = std::max(m_startRadius, m_endRadius);
  float radiusDiff = m_startRadius - m_endRadius;
  float newAngleDiff, baseAngleDiff = m_startAngle - end;

  if (!m_filled) {
    glLineWidth(m_lineWidth);
    glBegin(GL_LINE_STRIP);
  } else
    glBegin(GL_POLYGON);
  {
    // Draw counter-clockwise
    x = m_center.x + m_endRadius * cos((double) m_endAngle * MY_PI / 180);
    y = m_center.y - m_endRadius * sin((double) m_endAngle * MY_PI / 180);
    glVertex2f(x, y);

    for (angle = end + 180 / maxRadius; angle < m_startAngle; angle += 180 / maxRadius) {
      newAngleDiff = Spiral::clampAngle(angle) - m_endAngle;
      if (Spiral::clampAngle(angle) < m_endAngle)
        newAngleDiff += 360;
      interpRadius = m_endRadius + radiusDiff * newAngleDiff / baseAngleDiff;
      x = m_center.x + interpRadius * cos((double) angle * MY_PI / 180);
      y = m_center.y - interpRadius * sin((double) angle * MY_PI / 180);
      glVertex2f(x, y);
    }

    x = m_center.x + m_startRadius * cos((double) m_startAngle * MY_PI / 180);
    y = m_center.y - m_startRadius * sin((double) m_startAngle * MY_PI / 180);
    glVertex2f(x, y);
  }
  glEnd();

  /*
  if (m_fDotted)
    glDisable(GL_LINE_STIPPLE);
  */
}

// Utility function that returns the angle of a point around the center.
// Positive x-axis is 0 degree
float Spiral::getAngle(Point2D center, Point2D p)
{
  // Flip the y because the direction of y is reversed
  float dx = p.x - center.x,
        dy = center.y - p.y;
  float adx = abs(dx),
        ady = abs(dy);
  float baseAngle = 0.0f;

  // The same point
  if (adx == 0 && ady == 0)
    return 0;

  // Perpendicular
  if (adx == 0) {
    if (dy > 0)
      return 90;
    else
      return 270;
  } else if (ady == 0) {
    if (dx > 0)
      return 0;
    else
      return 180;
  }

  baseAngle = atan(ady / adx) * 180 / MY_PI;

  if (dx < 0)
    baseAngle = 180 - baseAngle;

  if (dy < 0)
    baseAngle = 360 - baseAngle;

  return baseAngle;
}

Point2D Spiral::getPointFromRadius(Point2D center, float radius, float angle)
{
  float x = center.x + radius * cos((double) angle * MY_PI / 180),
        y = center.y - radius * sin((double) angle * MY_PI / 180);
  return Point2D(x, y);
}

float Spiral::clampAngle(float angle) {
  for (; angle < 0; angle += 360);
  for (; angle > 360; angle -= 360);
  return angle;
}

// ===================
// Line implementation
// ===================

Line::Line(Point2D p1, Point2D p2) : m_p1(p1), m_p2(p2)
{
  m_fDotted = false;
  m_fDirected = false;
  m_lineWidth = 1;
  m_color = Color(0, 0, 0);
}

void Line::draw()
{
  glColor3f(m_color.r, m_color.g, m_color.b);

  if (m_fDotted) {
    glEnable(GL_LINE_STIPPLE);
    glLineStipple(1, 0xFF00);
  }

  glLineWidth(m_lineWidth);
  glBegin(GL_LINES);
  {
    glVertex2f(m_p1.x, m_p1.y);
    glVertex2f(m_p2.x, m_p2.y);
  }
  glEnd();

  if (m_fDotted)
    glDisable(GL_LINE_STIPPLE);

  // Draw the indicator at the start and end part of the line
  if (m_fDirected) {
    glBegin(GL_POINTS);
    {
      glPointSize(10);
      glVertex2f(m_p1.x, m_p1.y);
      glPointSize(5);
      glVertex2f(m_p2.x, m_p2.y);
    }
    glEnd();
  }
}

float Line::getDistance(Point2D pos)
{
  // pos - start
  Point2D a;
  a.x = pos.x - m_p1.x;
  a.y = pos.y - m_p1.y;

  // end - start
  Point2D b;
  b.x = -(m_p2.y - m_p1.y);
  b.y = m_p2.x - m_p1.x;

  // length of end - start
  float amp = sqrt(b.x * b.x + b.y * b.y);

  // distance from line to point
  // a dot b / amp
  return (a.x * b.x + a.y * b.y) / amp;
}

float Line::getParallelPosition(Point2D pos)
{
  // pos - start
  Point2D a;
  a.x = pos.x - m_p1.x;
  a.y = pos.y - m_p1.y;

  // end - start
  Point2D b;
  b.x = m_p2.x - m_p1.x;
  b.y = m_p2.y - m_p1.y;

  // length of end - start
  float amp = sqrt(b.x * b.x + b.y * b.y);

  // position between start & end, from 0 -to-> 1
  // a dot b / length^2
  // < 0 or > 1 if beyond endpoints
  return (a.x * b.x + a.y * b.y) / (amp * amp);
}


Text::Text(Point2D pos, std::string str) : m_pos(pos), m_str(str)
{
  m_color = Color(0, 0, 0);
}

void Text::draw()
{
  glColor3f(m_color.r, m_color.g, m_color.b);
  glRasterPos2i(m_pos.x, m_pos.y);
  int lines = 0;
  for(const char *p = m_str.c_str(); *p; p++) {
    while (*p == '\n') {
      lines++;
      p++;
      glRasterPos2i(m_pos.x, m_pos.y + (lines * 18));
    }
    glutBitmapCharacter(GLUT_BITMAP_9_BY_15, *p);
  }
}

int Text::getWidth()
{
  int maxLineWidth = 0, width = 0;
  for(const char *p = m_str.c_str(); *p; p++) {
    if (*p == '\n') {
      maxLineWidth = std::max(width, maxLineWidth);
      width = 0;
    }
    width += glutBitmapWidth(GLUT_BITMAP_9_BY_15, *p);
  }
  maxLineWidth = std::max(width, maxLineWidth);
  return maxLineWidth;
}
