#ifndef __POINT_H
#define __POINT_H

#include <iostream>
#include <math.h>

/**
* Three dimensional point data structure
*/
struct Point3D
{
  Point3D (float x, float y, float z) : x(x), y(y), z(z) {}
  Point3D () { Point3D(0, 0, 0); }

  float x;
  float y;
  float z;

  inline Point3D& operator=(const Point3D& p) {
    x = p.x;
    y = p.y;
    z = p.z;
    return *this;
  }
};

/**
* Two dimensional point data structure
*/
struct Point2D
{
  Point2D (float x, float y) : x(x), y(y) {}
  Point2D () { Point2D(0, 0); }

  float x;
  float y;

  static inline float distance(Point2D p1, Point2D p2) {
    float dx = p1.x - p2.x,
          dy = p1.y - p2.y;
    return sqrt(dx * dx + dy * dy);
  }

  inline Point2D& operator=(const Point2D& p) {
    x = p.x;
    y = p.y;
    return *this;
  }
};

#endif
