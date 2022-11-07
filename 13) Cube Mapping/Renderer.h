#pragma once
#include "../nclgl/OGLRenderer.h"
class Camera;
class Shader;
class HeightMap;

class Renderer : public OGLRenderer
{
public:
	Renderer(Window& parent);
	~Renderer(void);

	void UpdateScene(float dt) override;
	void RenderScene() override;
protected:
	void DrawHeightMap();
	void DrawWater();
	void DrawSkybox();

	unsigned int uboMatrices;

	Shader* lightShader;
	Shader* reflectShader;
	Shader* skyboxShader;

	HeightMap* heightMap;
	Mesh* quad;

	Light* light;
	Camera* camera;

	GLuint cubeMap;
	GLuint waterTex;
	GLuint earthTex;
	GLuint earthBump;

	float waterRotate;
	float waterCycle;
};

