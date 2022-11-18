#pragma once
#include "Matrix4.h"
#include "Vector3.h"
#include <vector>
#include "CameraPoint.h"


class Camera {
	
public:
	Camera(void) {
		yaw = 0.0f;
		pitch = 0.0f;
	}

	Camera(float pitch, float yaw, Vector3 position, std::vector<CameraPoint> path) {
		this->pitch = pitch;
		this->yaw = yaw;
		this->position = position;

		cameraPath = path;
		targetCameraPoint = cameraPath[currentPoint];

		currentCameraPoint.pitch = pitch;
		currentCameraPoint.yaw = yaw;
		currentCameraPoint.position = position;
	}

	~Camera(void) {};

	void UpdateCamera(float dt = 1.0f);



	Matrix4 BuildViewMatrix();

	Vector3 GetPosition() const { return position; }
	void SetPosition(Vector3 val) { position = val; }

	float GetYaw() const { return yaw; }
	void SetYaw(float y) { yaw = y; }

	float GetPitch() const { return pitch; }
	void SetPitch(float p) { pitch = p; }

	void ToggleCameraMode() { manual = !manual; }


protected:
	void ManualUpdate(float dt);
	void AutomaticUpdate(float dt);

	float CalculateDistanceBetweenPoints(Vector3 target, Vector3 current);
	
	std::vector<CameraPoint> cameraPath;
	CameraPoint targetCameraPoint;
	CameraPoint currentCameraPoint;
	int currentPoint = 0;
	bool manual = true;

	bool cameraMoving = false;
	Vector3 movementDirection;
	float yawToUse;
	float pitchToUse;
	Vector3 pathToUse;




	float yaw;
	float pitch;
	Vector3 position;
};

