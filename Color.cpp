#include <stdlib.h>
#include <time.h>

#include "Color.h"

Color Color::operator+(Color color)
{
  Color c;
  c.r = color.r + this->r;
  c.g = color.g + this->g;
  c.b = color.b + this->b;
  c.a = color.a + this->a;

  return c;
}

Color Color::operator-(Color color)
{
  Color c;
  c.r = this->r - color.r;
  c.g = this->g - color.g;
  c.b = this->b - color.b;
  c.a = this->a - color.a;

  return c;
}

Color Color::operator*(float num)
{
  Color c;
  c.r = this->r * num;
  c.g = this->g * num;
  c.b = this->b * num;
  c.a = this->a * num;

  return c;
}

Color Color::operator/(float num)
{
  Color c;
  c.r = this->r / num;
  c.g = this->g / num;
  c.b = this->b / num;
  c.a = this->a / num;

  return c;
}

Color Color::generateRandomColor()
{
  Color c((float)(rand() % 100) / 100,
          (float)(rand() % 100) / 100,
          (float)(rand() % 100) / 100);
  return c;
}
