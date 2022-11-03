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
	void GenerateSceneBuffer(int width, int height);
	void GeneratePostProcessBuffer(int width, int height);


	SceneNode* root;
	Camera* camera;
	Mesh* cube;
	Shader* sceneShader;
	Shader* processShader;
	Shader* finalShader;
	Mesh* quad;

	GLuint bufferFBO;
	GLuint processFBO;
	GLuint bufferColourTex;
	GLuint bufferDepthTex;
	GLuint processTexture;
};

