#pragma once
#include "../nclgl/OGLRenderer.h"

class HeightMap;
class Camera;
class Light;
class Shader;

class Renderer : public OGLRenderer
{
public:
	Renderer(Window& parent);
	~Renderer(void);

	void UpdateScene(float dt) override;
	void RenderScene() override;

protected:
	HeightMap* heightMap;
	Shader* shader;
	Camera* camera;
	Light* light;
	GLuint texture;
	GLuint bumpmap;
};

