#ifndef __PLAYER
#define __PLAYER
#include "entity.hpp"
#include "inventory.hpp"

class player : public entity {
  public:

  olc::Sprite *psprt;
  olc::Decal *pdecal;

  player(vec2d _pos, string name) : entity(_pos) {
    if (DEBUG_MODE) {
      cout << "Making player(1 of 4)..." << endl;
    }
    if (sysmode != "server") {
      psprt = new olc::Sprite("game/internal/textures/player.png");
    }
    if (DEBUG_MODE) {
      cout << "Making player(2 of 4)..." << endl;
    }
    if (sysmode != "server") {
      pdecal = new olc::Decal(psprt);
    } else {
      pdecal = nullptr;
    }
    if (DEBUG_MODE) {
      cout << "Making player(3 of 4)..." << endl;
    }
    entitydata["type"] = "player";
    if (DEBUG_MODE) {
      cout << "Making player(4 of 4)..." << endl;
    }
    entitydata["name"] = name;
  }

  olc::Decal *ondraw() {
    return pdecal;
  }

  void ontick() {
    
  }
};

#endif