#include "SceneNode.h"
#include <iostream>

SceneNode::SceneNode(Shader* shader, Mesh* mesh, Vector4 colour) {
	this->mesh = mesh;
	this->colour = colour;
	parent = NULL;
	boundingRadius = 1.0f;
	distanceFromCamera = 0.0f;
	texture = 0;
	modelScale = Vector3(1, 1, 1);
	nodeShader = shader;
}

SceneNode::~SceneNode(void) {
	for (unsigned int i = 0; i < children.size(); ++i)
	{
		delete children[i];
	}
}

void SceneNode::AddChild(SceneNode* s) {
	children.push_back(s);
	s->parent = this;
}

void SceneNode::Draw(const OGLRenderer& r) {
	if (mesh) mesh->Draw();
}

void SceneNode::Update(float dt) {
	
	worldTransform = parent ? parent->worldTransform * transform : transform;
	int iter = 0;
	for (vector<SceneNode*>::iterator i = children.begin(); i != children.end(); ++i) {
		(*i)->Update(dt);
	}
}
