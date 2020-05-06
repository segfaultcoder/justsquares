#ifndef __LIGHTS
#define __LIGHTS
class lightpoint {
  public:

  int x;
  int y;
  int level;

  lightpoint(int _x, int _y, int _level) {
    x = _x;
    y = _y;
    level = _level;
  }
};
#endif