#ifndef __COLOR_H
#define __COLOR_H

/**
* The data type for presenting color in floating point rgb format.
*/
class Color
{
public:
  Color() {}
  Color(float r, float g, float b) : r(r), g(g), b(b), a(1) {}
  Color(float r, float g, float b, float a) : r(r), g(g), b(b), a(a) {}
  float r, g, b, a;
  virtual Color operator+(Color color);
  virtual Color operator-(Color color);
  virtual Color operator*(float num);
  virtual Color operator/(float num);
  /**
  * Returns a random color
  */
  static Color generateRandomColor();
};

#endif
