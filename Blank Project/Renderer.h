#pragma once
#include "../nclgl/OGLRenderer.h"
#include "../nclgl/SceneNode.h"
#include "../nclgl/Frustum.h"
class HeightMap;
class Camera;
class Mesh;
class SceneNode;

class Renderer : public OGLRenderer
{
public:
	Renderer(Window& parent);
	
	~Renderer(void);
	void RenderScene() override;
	void UpdateScene(float dt) override;
protected:
	void	BuildNodeLists(SceneNode* from);
	void	SortNodeLists();
	void	ClearNodeLists();
	void	DrawNodes();
	void	DrawNode(SceneNode* n);

	void LoadTerrainTextures();
	void DrawTerrain();
	void FillTerrain();

	void DrawSkybox();

	HeightMap* heightMap;
	Shader* terrainShader;
	Shader* cubeShader;
	Shader* skyboxShader;
	Camera* camera;
	Light* light;

	GLuint terrainTextures[8];

	GLuint cubeTex;
	GLuint cubeMap;

	unsigned int uboTextures;
	unsigned int uboMatrices;

	SceneNode* root;

	Frustum frameFrustum;

	vector<SceneNode*> transparentNodeList;
	vector<SceneNode*> nodeList;

	Mesh* cube;
	Mesh* quad;
};

