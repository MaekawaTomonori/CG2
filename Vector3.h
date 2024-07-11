#pragma once
struct Vector3{
    float x, y, z;

	float length() const {
		return sqrtf(x * x + y * y + z * z);
	}

	void normalize() {
		this->x /= length();
		this->y /= length();
		this->z /= length();
	}
};

