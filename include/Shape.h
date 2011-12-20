#ifndef __SHAPE_H
#define __SHAPE_H

#include <vector>
#include <algorithm>

#include "Common.h"
#include "Point.h"
#include "Color.h"

/**
* Base class for all the drawable components 
*/
class Shape
{
public:
  virtual void draw() = 0;
  virtual void setColor(Color color) { m_color = color; }

  virtual void setDotted(bool fDotted) { m_fDotted = fDotted; }
  virtual void setDirected(bool fDirected) { m_fDirected = fDirected; }
  virtual void setLineWidth(float lineWidth) { m_lineWidth = lineWidth; }

protected:
  Color m_color;
  bool m_fDotted;
  bool m_fDirected;
  float m_lineWidth;
};

/**
* A Spiral shape that allows different start and end radius
*/ 
class Spiral : public Shape
{
public:
  Spiral(Point2D center, float startAngle, float startRadius, float endAngle, float endRadius);

  virtual void draw();
  virtual void setStartRadius(float radius) { m_startRadius = radius; }
  virtual void setEndRadius(float radius) { m_endRadius = radius; }
  virtual void setRadii(float startRadius, float endRadius) {
    m_startRadius = startRadius;
    m_endRadius = endRadius;
  }
  virtual void setCenter(Point2D center) { m_center = center; }
  virtual void setStartAngle(float angle) { m_startAngle = angle; }
  virtual void setEndAngle(float angle) { m_endAngle = angle; }
  virtual void setAngles(float startAngle, float endAngle) {
    m_startAngle = startAngle;
    m_endAngle = endAngle;
  }

  virtual float getStartRadius() { return m_startRadius; }
  virtual float getEndRadius() { return m_endRadius; }

  static float getAngle(Point2D center, Point2D p);
  /**
  * Utility function that returns the angle of a point relative to the center passed in. 
  */
  static Point2D getPointFromRadius(Point2D center, float radius, float angle);
  /**
  * Utility function that translates an angle to the range between 0 and 360
  */
  static float clampAngle(float angle);

  void setFilled(bool filled);
  void setColor(const Color&);

protected:
  Point2D m_center;
  float m_startRadius, m_endRadius,
        m_startAngle, m_endAngle;
  bool m_filled;
};
/**
* A line shape
*/
class Line : public Shape
{
public:
  Line(Point2D p1, Point2D p2);

  virtual void draw();
  virtual Point2D getP1() { return m_p1; }
  virtual Point2D getP2() { return m_p2; }
  virtual void setPoints(Point2D p1, Point2D p2) { m_p1 = p1; m_p2 = p2; }
  
  /**
  * Returns the distance with another point
  */
  virtual float getDistance(Point2D pos);
  virtual float getParallelPosition(Point2D pos);

protected:
  Point2D m_p1, m_p2;
};

/**
* The text that can be displayed on the screen.
*/
class Text : public Shape
{
public:
  Text(Point2D pos, std::string str);
  virtual void draw();
  virtual void setPos(Point2D pos) { m_pos = pos; }
  virtual Point2D getPos() { return m_pos; }
  virtual void setText(std::string str) { m_str = str; }
  virtual std::string getText() { return m_str; }
  virtual int getWidth();

private:
  Point2D m_pos;
  std::string m_str;
};
#endif
