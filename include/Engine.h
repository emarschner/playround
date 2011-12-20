#ifndef __ENGINE_H_
#define __ENGINE_H_

#include <vector>
#include <list>
#include <float.h>

#include "Widget.h"

/**
* The root class that handles all the user interaction and graphical interface rendering.
*/
class Engine : public Widget
{
public:
  /**
  * The text input behavior when the user strokes the keyboard
  */
  enum TextMode
  {
    TEXT_REPLACE,
    TEXT_APPEND
  };

  Engine();
  ~Engine();

  static SoundSourceMap* getSoundSources();

  bool onKeyDown(unsigned char key, float x, float y);

  virtual void draw();
  virtual bool handleDraw(float x, float y);
  virtual bool handleDrawEnd(float x, float y);

  void setSelectedWidget(Widget* widget) { m_selectedWidget = widget; }

  void setMouseCursorPosition(float, float);
  void setSize(int, int);
  void setCursorText(std::string text);
  void setTextMode(TextMode mode) { m_textMode = mode; }
  TextMode getTextMode() { return m_textMode; }

private:
  RoundPad *m_newRoundPad;
  Widget* m_selectedWidget;
  Spiral* m_mouseCursor;
  Text* m_cursorText;
  TextMode m_textMode;
  Line m_xLine, m_yLine;
  int m_width, m_height;
  float m_maxRadius;
};

#endif
