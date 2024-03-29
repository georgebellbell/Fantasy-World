# pragma once

#include "Vector4.h"
#include "Vector3.h"

class Light {
public:
	Light() {}
	Light(const Vector3 & position, const Vector4 & colour, float radius, const Vector4 & specular) {
		this -> position = position;
		this -> colour = colour;
		this -> radius = radius;
		this->specular = specular;
	}
	
	~Light(void) {};
	
	Vector3 GetPosition() const { return position; }
	void SetPosition(const Vector3 & val) { position = val; }
	
	float GetRadius() const { return radius; }
	void SetRadius(float val) { radius = val; }
	
	Vector4 GetColour() const { return colour; }
	void SetColour(const Vector4 & val) { colour = val; }

	Vector4 GetSpecular() const { return specular; }
	void SetSpecular(const Vector4 & val) { specular = val; }
	
protected:
	Vector3 position;
	float radius;
	Vector4 colour;
	Vector4 specular;
};
