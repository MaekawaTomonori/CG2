#pragma once
struct Vector4{
    float x,
        y,
        z,
        w;

    Vector4& operator=(float f[4]) {
        x = f[0];
        y = f[1];
        z = f[2];
        w = f[3];
        return *this;
    }
};