#include "Camera.h"
#include "Window.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <algorithm>
#include <vector>

void Camera::UpdateCamera(float dt) {
	if (manual) {
		ManualUpdate(dt);
	}
	else {
		AutomaticUpdate(200);
	}

}

void Camera::ManualUpdate(float dt)
{
	pitch -= (Window::GetMouse()->GetRelativePosition().y);
	yaw -= (Window::GetMouse()->GetRelativePosition().x);

	pitch = std::min(pitch, 90.0f);
	pitch = std::max(pitch, -90.0f);

	if (yaw < 0)		yaw += 360.0f;

	if (yaw > 360.0f)	yaw -= 360.0f;

	Matrix4 rotation = Matrix4::Rotation(yaw, Vector3(0, 1, 0));

	Vector3 forward = rotation * Vector3(0, 0, -1);
	Vector3 right = rotation * Vector3(1, 0, 0);

	float speed = 1000.0f * dt;

	if (Window::GetKeyboard()->KeyDown(KEYBOARD_W)) position += forward * speed;
	if (Window::GetKeyboard()->KeyDown(KEYBOARD_S)) position -= forward * speed;
	if (Window::GetKeyboard()->KeyDown(KEYBOARD_A)) position -= right * speed;
	if (Window::GetKeyboard()->KeyDown(KEYBOARD_D)) position += right * speed;


	if (Window::GetKeyboard()->KeyDown(KEYBOARD_SHIFT)) position.y -= speed;
	if (Window::GetKeyboard()->KeyDown(KEYBOARD_SPACE)) position.y += speed;
}


void Camera::AutomaticUpdate(float dt) {
	Vector3 direction = targetCameraPoint.position - position;
	float yawDelta = targetCameraPoint.yaw - yaw;
	float pitchDelta = targetCameraPoint.pitch - pitch;
	if (!cameraMoving) {
		pathToUse = direction;
		pitchToUse = pitchDelta;
		yawToUse = yawDelta;
		cameraMoving = true;

	}
	position = position + (pathToUse / dt);
	pitch = pitch + (pitchToUse / dt);
	yaw = yaw + (yawToUse / dt);

	float distance = CalculateDistanceBetweenPoints(targetCameraPoint.position, position);

	if (distance < 100) {
		cameraMoving = false;
		currentPoint = (currentPoint + 1) % cameraPath.size();
		targetCameraPoint = cameraPath[currentPoint];
}

}
Matrix4 Camera::BuildViewMatrix() {
	return	Matrix4::Rotation(-pitch, Vector3(1, 0, 0)) *
		Matrix4::Rotation(-yaw, Vector3(0, 1, 0)) *
		Matrix4::Translation(-position);
}

float Camera::CalculateDistanceBetweenPoints(Vector3 target, Vector3 current)
{
	return sqrt(pow(target.x - current.x, 2) +
		pow(target.y - current.y, 2) +
		pow(target.z - current.z, 2));
}
