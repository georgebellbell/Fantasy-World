#pragma once
#include "SceneNode.h"
class CubeRobot : public SceneNode
{
public:
	CubeRobot(Shader* shader, Mesh* cube);
	~CubeRobot(void) {};
	void Update(float dt) override;

protected:
	SceneNode* head;
	SceneNode* leftArm;
	SceneNode* rightArm;
};

