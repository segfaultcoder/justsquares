#ifndef __ENTITY
#define __ENTITY
#include <iostream>
#include <cmath>

#define PI 3.14159265

class vec2d {
  public:

  double x;
  double y;

  vec2d() {
    x = 0; y = 0;
  }

  vec2d(double _x, double _y) {
    x = _x;
    y = _y;
  }

  vec2d operator+(const vec2d& v) {
    return vec2d(x+v.x, y+v.y);
  }

  vec2d operator-(const vec2d& v) {
    return vec2d(x-v.x, y-v.y);
  }

  vec2d operator*(const vec2d& v) {
    return vec2d(x*v.x, y*v.y);
  }

  vec2d operator*(const double& v) {
    return vec2d(x*v, y*v);
  }

  vec2d operator/(const vec2d& v) {
    return vec2d(x/v.x, y/v.y);
  }

  vec2d operator/(const double& v) {
    return vec2d(x/v, y/v);
  }

  void operator+=(const vec2d& v) {
    x += v.x;
    y += v.y;
  }

  void operator-=(const vec2d& v) {
    x -= v.x;
    y -= v.y;
  }

  void operator*=(const vec2d& v) {
    x *= v.x;
    y *= v.y;
  }

  void operator/=(const vec2d& v) {
    x /= v.x;
    y /= v.y;
  }

  void operator*=(const double& v) {
    x *= v;
    y *= v;
  }

  void operator/=(const double& v) {
    x /= v;
    y /= v;
  }

  bool operator==(const vec2d& v) {
    return (x == v.x) && (y == v.y);
  }

  bool operator!=(const vec2d& v) {
    return (x != v.x) || (y != v.y);
  }
};

const vec2d nullvec = {-159274.83, 9941522.67};

double dist(double x1, double y1, double x2, double y2) {
	double xx = (x1-x2)*(x1-x2);
	double yy = (y1-y2)*(y1-y2);
	return sqrt(xx + yy);
}

bool inrect(double x, double y, double x1, double y1, double x2, double y2) {
  return ((x >= x1) && (x <= x2)) && ((y >= y1) && (y <= y2));
}

bool inrects(double x1, double y1, double x2, double y2, double x3, double y3, double x4, double y4) {
  return inrect(x1, y1, x3, y3, x4, y4) || inrect(x2, y1, x3, y3, x4, y4) || inrect(x1, y2, x3, y3, x4, y4) || inrect(x2, y2, x3, y3, x4, y4);
}

vec2d intersect(double x1, double y1, double x2, double y2, double x3, double y3, double x4, double y4) {
	if ((x1 == x2 && y1 == y2) || (x3 == x4 && y3 == y4)) {
		return nullvec;
	}

	double denominator = ((y4 - y3) * (x2 - x1) - (x4 - x3) * (y2 - y1));
	if (denominator == 0.0) {
		return nullvec;
	}

	double ua = ((x4 - x3) * (y1 - y3) - (y4 - y3) * (x1 - x3)) / denominator;
	double ub = ((x2 - x1) * (y1 - y3) - (y2 - y1) * (x1 - x3)) / denominator;
	if (ua < 0 || ua > 1 || ub < 0 || ub > 1) {
		return nullvec;
	}
	double x = x1 + ua * (x2 - x1);
	double y = y1 + ua * (y2 - y1);

	return {x, y};
}

class entity {
  public:

  vec2d pos;
  vec2d vel;
  vec2d acc;

  map<string, string> entitydata;

  virtual void ontick() = 0;
  virtual olc::Decal *ondraw() = 0;

  void applyforce(vec2d f) {
    acc += f;
  }

  bool update(double xlim, double ylim) {
    vec2d tmp = {pos.x, pos.y};
    vel += acc;
    vec2d svel = {vel.x, vel.y};
    if (svel.x > xlim) {
    	svel.x = xlim;
    }
    if (svel.y > ylim) {
    	svel.y = ylim;
    }
    if (svel.x < -xlim) {
    	svel.x = -xlim;
    }
    if (svel.y < -ylim) {
    	svel.y = -ylim;
    }
    pos += svel;
    vel *= 0.95;
    acc = {0, 0};
    ontick();
    return !(tmp == pos);
  }

  entity() {
    pos = {0, 0}; vel = {0, 0}; acc = {0, 0}; entitydata["type"] = "placeholder";
  }

  entity(vec2d _pos) {
    pos = _pos; vel = {0, 0}; acc = {0, 0}; entitydata["type"] = "placeholder";
  }

  ~entity() {}
};

map<string, entity *> entities;
vector<string> entityueid;
int ueidfrag = 0;

// UEID: Unique Entity ID, for example, "abcdefghijkl"
#endif