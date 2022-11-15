#pragma once
#include "../nclgl/OGLRenderer.h"
#include "../nclgl/SceneNode.h"
#include "../nclgl/Frustum.h"
class HeightMap;
class Camera;
class Mesh;
class SceneNode;
class MeshMaterial;

class Renderer : public OGLRenderer
{
public:
	Renderer(Window& parent);

	void CreateSceneNodes(Vector3& dimensions);

	void LoadMeshes();

	
	~Renderer(void);
	void RenderScene() override;
	void UpdateScene(float dt) override;
protected:
	void	BuildNodeLists(SceneNode* from);
	void	SortNodeLists();
	void	ClearNodeLists();
	void	DrawNodes();
	void	DrawNode(SceneNode* n);

	void LoadTextures();
	void DrawTerrain();
	void FillTerrain();

	Vector3 FindTreePosition();

	void CreateTextureUBO();
	void CreateMatrixUBO();
	void CreateLightsUBO();

	void DrawSkybox();

	HeightMap* heightMap;
	Shader* terrainShader;
	Shader* nodeShader;
	Shader* skyboxShader;
	Camera* camera;
	Light* light;

	vector<Vector3> validPositions;

	GLuint terrainTextures[8];
	GLuint towerTextures[2];
	GLuint treeTextures[4];
	GLuint cubeMap;

	unsigned int uboTextures;
	unsigned int uboMatrices;
	unsigned int uboLights;

	SceneNode* root;

	Frustum frameFrustum;

	vector<SceneNode*> transparentNodeList;
	vector<SceneNode*> nodeList;

	Mesh* tower;
	Mesh* quad;
	Mesh* tree;
};

