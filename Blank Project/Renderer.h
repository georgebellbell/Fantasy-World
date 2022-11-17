#pragma once
#include "../nclgl/OGLRenderer.h"
#include "../nclgl/SceneNode.h"
#include "../nclgl/Frustum.h"
class HeightMap;
class Camera;
class Mesh;
class SceneNode;
class MeshMaterial;
class MeshAnimation;

class Renderer : public OGLRenderer
{
public:
	Renderer(Window& parent);

	void CreateSceneNodes(Vector3* dimensions);

	void LoadMeshes();

	
	~Renderer(void);
	void RenderScene() override;
	void ResetProjectionMatrix();
	void UpdateScene(float dt) override;

	void toggleSplitScreen() { splitScreenEnabled = !splitScreenEnabled; }

protected:
	void	BuildNodeLists(SceneNode* from, int* camera);
	void	SortNodeLists();
	void	ClearNodeLists();
	void	DrawNodes();
	void	DrawNode(SceneNode* n);
	void DrawCharacter(SceneNode*n);
	void	DrawSkybox();

	void PresentScene();
	void DrawPostProcess();
	void DrawScene();


	void GenerateFrameBuffers();
	void GenerateSceneBuffer(int width, int height);
	void GeneratePlayerBuffer(GLuint* buffer, int width, int height, int player);
	void GeneratePostProcessBuffer(int width, int height);


	void LoadTextures();
	void DrawTerrain();
	void FillTerrain(int* camera);

	Vector3 FindTreePosition();

	void CreateTextureUBO();
	void CreateMatrixUBO();
	void CreateLightsUBO();
	void CreateCameraUBO();

	unsigned int uboTextures;
	unsigned int uboMatrices;
	unsigned int uboLights;
	unsigned int uboCamera;


	HeightMap* heightMap;
	Shader* terrainShader;
	Shader* nodeShader;
	Shader* skyboxShader;
	Shader* simpleShader;
	Shader* fogShader;
	Shader* characterShader;
	Camera* camera[2];
	Light* light;

	vector<Vector3> validPositions;

	GLuint terrainTextures[8];
	GLuint towerTextures[2];
	GLuint treeTextures[4];
	GLuint cubeMap;

	SceneNode* root;

	Frustum frameFrustum;

	vector<SceneNode*> transparentNodeList;
	vector<SceneNode*> nodeList;

	Mesh* tower;
	Mesh* quad;
	Mesh* tree;

	Mesh* eggMesh;
	MeshMaterial* eggMaterial;
	vector<GLuint> eggTextures;
	vector<GLuint> eggNormals;
	MeshAnimation* eggAnim;


	GLuint playerFBO[2];
	GLuint playerColourTex[2];
	GLuint playerDepthTex[2];
	GLuint playerPositionTex[2];

	GLuint processFBO;
	GLuint processTexture;

	GLuint sceneFBO;
	GLuint sceneColourTex;
	GLuint sceneDepthTex;
	GLuint scenePositionTex;

	bool splitScreenEnabled = true;

	int currentFrame;
	float frameTime;
};

