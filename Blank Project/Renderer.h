#pragma once

#include "../nclgl/OGLRenderer.h"
#include "../nclgl/Camera.h"
#include "../nclgl/SceneNode.h"
#include "../nclgl/CubeRobot.h"

class Renderer : public OGLRenderer
{
public:
	Renderer(Window& parent);
	~Renderer(void);

	void UpdateScene(float dt) override;
	void RenderScene() override;

protected:
	void DrawNode(SceneNode* n);
	void PresentScene();
	void DrawPostProcess();
	void DrawScene();
	void GenerateSceneBuffer(GLuint* buffer, int width, int height, int player);
	void GeneratePostProcessBuffer(int width, int height);


	SceneNode* root;
	Camera* camera[2];
	Mesh* cube;
	Shader* sceneShader;
	Shader* finalShader;
	Mesh* quad;

	GLuint bufferFBO[2];
	GLuint processFBO;
	GLuint bufferColourTex[2];
	GLuint bufferDepthTex[2];
	GLuint processTexture;
};

