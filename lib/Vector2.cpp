#include "Vector2.h"
#include <cmath>

Vector2::Vector2(int x, int y) {
  this->x = x;
  this->y = y;
};

void Vector2::set(Vector2 other) {
  this->x = other.x;
  this->y = other.y;
};

int Vector2::distance(Vector2 other) {
  return std::sqrt(pow(this->x - other.x, 2) + pow(this->y - other.y, 2));
}