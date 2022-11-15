#include "Renderer.h"
#include "../nclgl/Camera.h"
#include "../nclgl/HeightMap.h"
#include "../nclgl/Light.h"
#include "../nclgl/MeshMaterial.h"
#include <time.h>
#include <algorithm>
#include <iostream>
#include <vector>
#include <random>


struct textureStruct {
	GLuint64 values[24] ;
} textures;

struct lightStruct {
	Vector4 lightColour;
	Vector3 lightPosition;
	float lightRadius;
	Vector4 lightSpecular;
} mainLight;

const int TREE_COUNT = 300;
Renderer::Renderer(Window& parent) : OGLRenderer(parent) {
	heightMap = new HeightMap(TEXTUREDIR"Fantasy_Heightmap.png");
	Vector3 dimensions = heightMap->GetHeightMapSize();
	camera = new Camera(-40, 270, dimensions * Vector3(0.5, 1, 0.5));
	light = new Light(dimensions * Vector3(0.5f, 1.5f, 0.5f), Vector4(1, 1, 1, 1), dimensions.x * 5.0f, Vector4(1, 1, 1, 1));
	
	terrainShader = new Shader("TerrainVertex.glsl", "TerrainFragment.glsl");
	nodeShader = new Shader("SceneVertex.glsl", "SceneFragment.glsl");
	skyboxShader = new Shader("SkyboxVertex.glsl", "SkyboxFragment.glsl");

	if (!terrainShader->LoadSuccess() || !nodeShader->LoadSuccess() || !skyboxShader->LoadSuccess())
		return;

	LoadMeshes();
	LoadTextures();

	projMatrix = Matrix4::Perspective(1.0f, 15000.0f, (float)width / (float)height, 45.0f);

	CreateMatrixUBO();
	CreateTextureUBO();
	CreateLightsUBO();

	mainLight.lightColour = light->GetColour();
	mainLight.lightPosition = light->GetPosition();
	mainLight.lightRadius = light->GetRadius();
	mainLight.lightSpecular = light->GetSpecular();

	glBindBuffer(GL_UNIFORM_BUFFER, uboLights);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(lightStruct), &mainLight);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	CreateSceneNodes(dimensions);
	
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
	init = true;
}

void Renderer::CreateSceneNodes(Vector3& dimensions)
{
	root = new SceneNode(nodeShader);

	SceneNode* forest = new SceneNode(nodeShader);
	forest->SetTransform(Matrix4::Translation(dimensions * Vector3(0.25, 0, 0)));
	root->AddChild(forest);

	SceneNode* towerNode = new SceneNode(nodeShader);
	towerNode->SetTransform(Matrix4::Translation(dimensions * Vector3(0.6, 0.25, 0.55)));
	towerNode->SetModelScale(Vector3(300.0f, 300.0f, 300.0f));
	towerNode->SetBoundingRadius(3000.0f);
	towerNode->SetMesh(tower);
	towerNode->SetTexture(8);
	root->AddChild(towerNode);

	Vector3* vertices = heightMap->GetVertices();
	for (int i = 0; i < heightMap->GetVertexCount(); i++)
	{
		Vector3 vPos = vertices[i];

		if (vPos.y < 1010 || vPos.y > 1100)
			continue;
		validPositions.push_back(vPos);
	}

	std::random_shuffle(validPositions.begin(), validPositions.end());

	for (int i = 0; i < TREE_COUNT; i++)
	{
		SceneNode* treeNode = new SceneNode(nodeShader);
		treeNode->SetColour(Vector4(1.0f, 1.0f, 1.0f, 1.0f));
		treeNode->SetTransform(Matrix4::Translation(FindTreePosition()));
		treeNode->SetModelScale(Vector3(50.0f, 50.0f, 50.0f));
		treeNode->SetBoundingRadius(10000.0f);
		treeNode->SetMesh(tree);
		treeNode->SetTexture(10);
		forest->AddChild(treeNode);
	}
}

void Renderer::LoadMeshes()
{
	quad = Mesh::GenerateQuad();
	tower = Mesh::LoadFromMeshFile("Tower.msh");
	tree = Mesh::LoadFromMeshFile("Tree1.msh");
}

Vector3 Renderer::FindTreePosition() {
	int random_pos = rand() % validPositions.size();
	Vector3 newPos = validPositions[random_pos];
	return Vector3(newPos.x, -1000, newPos.z);

}

void Renderer::LoadTextures()
{
	cubeMap = SOIL_load_OGL_cubemap(TEXTUREDIR "SkyCloudy_Right.png", TEXTUREDIR "SkyCloudy_Left.png",
		TEXTUREDIR "SkyCloudy_Top.png", TEXTUREDIR "SkyCloudy_Bottom.png",
		TEXTUREDIR "SkyCloudy_Front.png", TEXTUREDIR "SkyCloudy_Back.png",
		SOIL_LOAD_RGB, SOIL_CREATE_NEW_ID, 0);

	terrainTextures[0] = SOIL_load_OGL_texture(TEXTUREDIR"snow.jpg", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
	terrainTextures[1] = SOIL_load_OGL_texture(TEXTUREDIR"snowBump.jpg", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);

	terrainTextures[2] = SOIL_load_OGL_texture(TEXTUREDIR"rock.jpg", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
	terrainTextures[3] = SOIL_load_OGL_texture(TEXTUREDIR"rockBump.jpg", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);

	terrainTextures[4] = SOIL_load_OGL_texture(TEXTUREDIR"grass.jpg", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
	terrainTextures[5] = SOIL_load_OGL_texture(TEXTUREDIR"grassBump.jpg", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);

	terrainTextures[6] = SOIL_load_OGL_texture(TEXTUREDIR"sand.jpg", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
	terrainTextures[7] = SOIL_load_OGL_texture(TEXTUREDIR"sandBump.jpg", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);

	towerTextures[0] = SOIL_load_OGL_texture(TEXTUREDIR"towerDiffuse.png", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
	towerTextures[1] = SOIL_load_OGL_texture(TEXTUREDIR"towerNormal.jpg", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
	
	treeTextures[2] = SOIL_load_OGL_texture(TEXTUREDIR"TreeDiffuse1.png", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
	treeTextures[3] = SOIL_load_OGL_texture(TEXTUREDIR"TreeBump1.png", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
	treeTextures[0] = SOIL_load_OGL_texture(TEXTUREDIR"TreeDiffuse2.png", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
	treeTextures[1] = SOIL_load_OGL_texture(TEXTUREDIR"TreeBump2.png", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);


}

void Renderer::CreateMatrixUBO()
{
	glGenBuffers(1, &uboMatrices);
	glBindBuffer(GL_UNIFORM_BUFFER, uboMatrices);
	glBufferData(GL_UNIFORM_BUFFER, 2 * sizeof(Matrix4), projMatrix.values, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	unsigned int uniformBlockIndexTerrain = glGetUniformBlockIndex(terrainShader->GetProgram(), "Matrices");
	unsigned int uniformBlockIndexSkybox = glGetUniformBlockIndex(skyboxShader->GetProgram(), "Matrices");
	unsigned int uniformBlockIndexNode = glGetUniformBlockIndex(nodeShader->GetProgram(), "Matrices");

	glBindBufferBase(GL_UNIFORM_BUFFER, 0, uboMatrices);

	glUniformBlockBinding(terrainShader->GetProgram(), uniformBlockIndexTerrain, 0);
	glUniformBlockBinding(skyboxShader->GetProgram(), uniformBlockIndexSkybox, 0);
	glUniformBlockBinding(nodeShader->GetProgram(), uniformBlockIndexNode, 0);
}

void Renderer::CreateTextureUBO()
{
	for (int i = 0; i < 8; i++)
	{
		if (!terrainTextures[i])
			return;
		SetTextureRepeating(terrainTextures[i], true);
		GLuint64 handler = glGetTextureHandleARB(terrainTextures[i]);
		glMakeTextureHandleResidentARB(handler);

		textures.values[i] = handler;
	}

	for (int i = 0; i < 2; i++)
	{
		if (!towerTextures[i])
			return;
		SetTextureRepeating(towerTextures[i], true);
		GLuint64 handler = glGetTextureHandleARB(towerTextures[i]);
		glMakeTextureHandleResidentARB(handler);
		textures.values[i + 8] = handler;
	}

	for (int i = 0; i < 4; i++)
	{
		if (!treeTextures[i])
			return;
		SetTextureRepeating(treeTextures[i], true);
		GLuint64 handler = glGetTextureHandleARB(treeTextures[i]);
		glMakeTextureHandleResidentARB(handler);
		textures.values[i + 10] = handler;
	}

	glGenBuffers(1, &uboTextures);
	glBindBuffer(GL_UNIFORM_BUFFER, uboTextures);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(textureStruct), textures.values, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	unsigned int uniformBlockIndexTerrain = glGetUniformBlockIndex(terrainShader->GetProgram(), "textures");
	unsigned int uniformBlockIndexNode = glGetUniformBlockIndex(nodeShader->GetProgram(), "textures");

	glBindBufferBase(GL_UNIFORM_BUFFER, 1, uboTextures);

	glUniformBlockBinding(terrainShader->GetProgram(), uniformBlockIndexTerrain, 1);
	glUniformBlockBinding(nodeShader->GetProgram(), uniformBlockIndexNode, 1);
}

void Renderer::CreateLightsUBO()
{
	glGenBuffers(1, &uboLights);
	glBindBuffer(GL_UNIFORM_BUFFER, uboLights);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(lightStruct), NULL, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	unsigned int uniformBlockIndexTerrain = glGetUniformBlockIndex(terrainShader->GetProgram(), "light");
	unsigned int uniformBlockIndexNode = glGetUniformBlockIndex(nodeShader->GetProgram(), "light");

	glBindBufferBase(GL_UNIFORM_BUFFER, 2, uboLights);

	glUniformBlockBinding(terrainShader->GetProgram(), uniformBlockIndexTerrain, 2);
	glUniformBlockBinding(nodeShader->GetProgram(), uniformBlockIndexNode, 2);
}

Renderer::~Renderer(void) {
	delete heightMap;
	delete camera;
	delete terrainShader;
	delete nodeShader;
	delete skyboxShader;
	delete root;
	delete light;
	delete quad;
	delete tower;
	delete tree;
	glDeleteTextures(8, terrainTextures);
	glDeleteTextures(2, towerTextures);
}

void Renderer::UpdateScene(float dt) {
	camera->UpdateCamera(dt);
	viewMatrix = camera->BuildViewMatrix();
	glBindBuffer(GL_UNIFORM_BUFFER, uboMatrices);
	glBufferSubData(GL_UNIFORM_BUFFER, sizeof(Matrix4), sizeof(Matrix4), viewMatrix.values);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
	frameFrustum.FromMatrix(projMatrix * viewMatrix);

	root->Update(dt);
}

void Renderer::RenderScene() {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	DrawSkybox();
	
	DrawTerrain();
	FillTerrain();
}

void Renderer::DrawSkybox() {
	glDepthMask(GL_FALSE);

	BindShader(skyboxShader);

	quad->Draw();

	glDepthMask(GL_TRUE);
}

void Renderer::DrawTerrain()
{
	BindShader(terrainShader);

	glUniformMatrix4fv(glGetUniformLocation(terrainShader->GetProgram(), "modelMatrix"), 1, false, modelMatrix.values);

	glUniform1f(glGetUniformLocation(terrainShader->GetProgram(), "terrainHeight"), heightMap->GetHeightMapSize().y);

	glUniform3fv(glGetUniformLocation(terrainShader->GetProgram(), "cameraPos"), 1, (float*)&camera->GetPosition());

	heightMap->Draw();
}

void Renderer::FillTerrain() {
	BuildNodeLists(root);
	SortNodeLists();
	DrawNodes();
	ClearNodeLists();
}

void Renderer::BuildNodeLists(SceneNode* from) {
	if (frameFrustum.InsideFrustum(*from)) {
		Vector3 dir = from->GetWorldTransform().GetPositionVector() - camera->GetPosition();
		from->SetCameraDistance(Vector3::Dot(dir, dir));

		if (from->GetColour().w < 1.0f) {
			transparentNodeList.push_back(from);
		}
		else {
			nodeList.push_back(from);
		}
	}
	for (vector<SceneNode*>::const_iterator i = from->GetChildIteratorStart(); i != from->GetChildIteratorEnd(); ++i)
	{
		BuildNodeLists((*i));
	}
}

void Renderer::SortNodeLists() {
	std::sort(transparentNodeList.rbegin(), transparentNodeList.rend(), SceneNode::CompareByCameraDistance);
	std::sort(nodeList.begin(), nodeList.end(), SceneNode::CompareByCameraDistance);
}

void Renderer::DrawNodes() {
	for (const auto& i : nodeList) {
		DrawNode(i);
	}
	for (const auto& i : transparentNodeList) {
		DrawNode(i);
	}
}

void Renderer::DrawNode(SceneNode* n) {
	if (!n->GetMesh()) return;

	Matrix4 model = n->GetWorldTransform() * Matrix4::Scale(n->GetModelScale());
	Shader* nodeShader = n->GetShader();
	Mesh* nodeMesh = n->GetMesh();
	BindShader(nodeShader);

	glUniformMatrix4fv(glGetUniformLocation(nodeShader->GetProgram(), "modelMatrix"), 1, false, model.values);

	glUniform1i(glGetUniformLocation(nodeShader->GetProgram(), "textureIndex"), n->GetTexture());

	glUniform3fv(glGetUniformLocation(terrainShader->GetProgram(), "cameraPos"), 1, (float*)&camera->GetPosition());

	if (nodeMesh->GetSubMeshCount() == 1) {
		n->Draw(*this);
	}
	else {
		for (int i = 0; i < nodeMesh->GetSubMeshCount(); i++)
		{
			if (i % 2) {
				glUniform1i(glGetUniformLocation(nodeShader->GetProgram(), "textureIndex"), n->GetTexture() + i + 1);
			}
			else {
				glUniform1i(glGetUniformLocation(nodeShader->GetProgram(), "textureIndex"), n->GetTexture() + i);
			}
			nodeMesh->DrawSubMesh(i);
		}
	}


}

void Renderer::ClearNodeLists() {
	transparentNodeList.clear();
	nodeList.clear();
}
