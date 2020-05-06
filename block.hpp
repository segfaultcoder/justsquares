#ifndef __BLOCK
#define __BLOCK
#include "lights.hpp"
#include <string>
#include <vector>

using namespace std;

class block {
  public:

  string type;
  string wall;
  int lightlevel; // 0-7
  int waterlevel; // 0-7
  vector<lightpoint> lights;

  block() {}

  block(string _type) {
    type = _type;
    lightlevel = 0;
    waterlevel = 0;
  }

  void updatelight() {
    lightlevel = 0;
    for (int i = 0; i < lights.size(); i++) {
      lightlevel = lights[i].level>lightlevel?lights[i].level:lightlevel;
    }
  }

  void eraselight(int ind) {
    lights.erase(lights.begin() + ind);
  }
};
#endif