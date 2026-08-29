#ifndef VECTOR2_H
#define VECTOR2_H

class Vector2 {

  public:
    int x;
    int y;
    Vector2(int x, int y);
    void set(Vector2 other);
    int distance(Vector2 other);
};

#endif