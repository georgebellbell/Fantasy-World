#pragma once
#include "Vector3.h"
#include <vector>
class CameraPoint
{
public:
	CameraPoint(float pitch, float yaw, Vector3 position) {
		this->position = position;
		this->yaw = yaw;
		this->pitch = pitch;
	}
	CameraPoint() {
		this->position = Vector3();
		this->yaw = 0.0f;
		this->pitch = 0.0f;
	}
	Vector3 position;
	float yaw;
	float pitch;
};

