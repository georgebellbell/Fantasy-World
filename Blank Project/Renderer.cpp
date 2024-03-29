#include "Renderer.h"
#include "../nclgl/Camera.h"
#include "../nclgl/HeightMap.h"
#include "../nclgl/Light.h"
#include "../nclgl/MeshMaterial.h"
#include "../nclgl/MeshAnimation.h"
#include <time.h>
#include <algorithm>
#include <iostream>
#include <vector>
#include <random>


struct textureStruct {
	GLuint64 values[24];
} textures;

struct lightStruct {
	Vector4 lightColour;
	Vector3 lightPosition;
	float lightRadius;
	Vector4 lightSpecular;
} mainLight;

const int TREE_COUNT = 200;
#define SHADOWSIZE 8000

Renderer::Renderer(Window& parent) : OGLRenderer(parent) {
	heightMap = new HeightMap(TEXTUREDIR"Fantasy_Heightmap.png");
	Vector3 dimensions = heightMap->GetHeightMapSize();
	vector<CameraPoint> camera1Path = { CameraPoint(-40, 90, dimensions * Vector3(0.3, 1, 0.3)),
									    CameraPoint(-40, 180, dimensions * Vector3(0.7, 1, 0.3)),
										CameraPoint(-40, 270, dimensions * Vector3(0.7, 1, 0.7)),
										CameraPoint(-40, 360, dimensions * Vector3(0.3, 1, 0.8))};

	vector<CameraPoint> camera2Path = camera1Path;
	std::reverse(camera2Path.begin(), camera2Path.end());

	camera[0] = new Camera(-40, 270, dimensions * Vector3(0.5, 1, 0.5), camera1Path);
	camera[1] = new Camera(-40, 90, dimensions * Vector3(0.3, 0.5, 0.5), camera2Path);

	light = new Light(dimensions * Vector3(0.5f, 1.5f, 0.5f), Vector4(1, 1, 1, 1), dimensions.x * 50.0f, Vector4(1, 1, 1, 1));

	terrainShader = new Shader("TerrainVertex.glsl", "TerrainFragment.glsl");
	nodeShader = new Shader("SceneVertex.glsl", "SceneFragment.glsl");
	skyboxShader = new Shader("SkyboxVertex.glsl", "SkyboxFragment.glsl");
	simpleShader = new Shader("TexturedVertex.glsl", "TexturedFragment.glsl");
	fogShader = new Shader("FogVertex.glsl", "FogFragment.glsl");
	characterShader = new Shader("SkinningVertex.glsl", "SkinningFragment.glsl");
	shadowShader = new Shader("shadowVert.glsl", "shadowFrag.glsl");
	reflectShader = new Shader("ReflectVertex.glsl", "ReflectFragment.glsl");

	if (!terrainShader->LoadSuccess() || !nodeShader->LoadSuccess()	 || 
		!skyboxShader->LoadSuccess()  || !simpleShader->LoadSuccess()|| 
		!fogShader->LoadSuccess()	  || !characterShader->LoadSuccess() || 
		!shadowShader->LoadSuccess()  || !reflectShader->LoadSuccess())
		return;

	LoadMeshes();
	LoadTextures();

	projMatrix = Matrix4::Perspective(1.0f, 15000.0f, (float)width / (float)height, 45.0f);

	CreateMatrixUBO();
	CreateTextureUBO();
	CreateLightsUBO();
	CreateCameraUBO();

	mainLight.lightColour = light->GetColour();
	mainLight.lightPosition = light->GetPosition();
	mainLight.lightRadius = light->GetRadius();
	mainLight.lightSpecular = light->GetSpecular();

	glBindBuffer(GL_UNIFORM_BUFFER, uboLights);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(lightStruct), &mainLight);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	GenerateFrameBuffers();

	CreateSceneNodes(&dimensions);

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

	sceneTime = 0;
	init = true;
}

void Renderer::GenerateFrameBuffers() {
	GenerateSceneBuffer(width, height);
	GenerateShadowBuffer();
	GeneratePlayerBuffer(&playerFBO[0], width / 2, height, 0);
	GeneratePlayerBuffer(&playerFBO[1], width / 2, height, 1);
	GeneratePostProcessBuffer(width, height);
}

void Renderer::GenerateSceneBuffer(int width, int height) {
	glGenFramebuffers(1, &sceneFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, sceneFBO);

	//Create colour texture
	{
		glGenTextures(1, &sceneColourTex);
		glBindTexture(GL_TEXTURE_2D, sceneColourTex);

		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, sceneColourTex, 0);
		glObjectLabel(GL_TEXTURE, sceneColourTex, -1, "Scene Colour Texture");
	}

	//Create depth texture
	{
		glGenTextures(1, &sceneDepthTex);
		glBindTexture(GL_TEXTURE_2D, sceneDepthTex);

		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, width, height, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, NULL);

		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, sceneDepthTex, 0);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_TEXTURE_2D, sceneDepthTex, 0);

		glObjectLabel(GL_TEXTURE, sceneDepthTex, -1, "Scene Depth Texture");
	}

	//Create position texture
	{
		glGenTextures(1, &scenePositionTex);
		glBindTexture(GL_TEXTURE_2D, scenePositionTex);

		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, width, height, 0, GL_RGB, GL_FLOAT, NULL);

		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, scenePositionTex, 0);

		glObjectLabel(GL_TEXTURE, scenePositionTex, -1, "Scene Position");
	}

	GLuint attachments[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
	glDrawBuffers(2, attachments);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE || !sceneColourTex || !sceneDepthTex) {
		return;
	}

	glBindTexture(GL_TEXTURE_2D, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

}

void Renderer::GenerateShadowBuffer()
{
	glGenTextures(1, &shadowTex);
	glBindTexture(GL_TEXTURE_2D, shadowTex);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOWSIZE, SHADOWSIZE, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);

	glBindTexture(GL_TEXTURE_2D, 0);

	glGenFramebuffers(1, &shadowFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadowTex, 0);
	glDrawBuffer(GL_NONE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	for (int i = 0; i < 2; i++)
	{
		glGenTextures(1, &playerShadowTex[i]);
		glBindTexture(GL_TEXTURE_2D, playerShadowTex[i]);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOWSIZE, SHADOWSIZE, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);

		glBindTexture(GL_TEXTURE_2D, 0);

		glGenFramebuffers(1, &playerShadowFBO[i]);
		glBindFramebuffer(GL_FRAMEBUFFER, playerShadowFBO[i]);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, playerShadowTex[i], 0);
		glDrawBuffer(GL_NONE);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

	}
}

void Renderer::GeneratePlayerBuffer(GLuint* buffer, int width, int height, int player) {
	glGenFramebuffers(1, buffer);
	glBindFramebuffer(GL_FRAMEBUFFER, *buffer);

	//Create colour texture
	{
		glGenTextures(1, &playerColourTex[player]);
		glBindTexture(GL_TEXTURE_2D, playerColourTex[player]);

		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, playerColourTex[player], 0);
		glObjectLabel(GL_TEXTURE, playerColourTex[player], -1, "Player Colour");
	}

	//Create depth texture
	{
		glGenTextures(1, &playerDepthTex[player]);
		glBindTexture(GL_TEXTURE_2D, playerDepthTex[player]);

		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, width, height, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, NULL);

		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, playerDepthTex[player], 0);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_TEXTURE_2D, playerDepthTex[player], 0);

		glObjectLabel(GL_TEXTURE, playerDepthTex[player], -1, "Player Depth");
	}


	//Create position texture
	{
		glGenTextures(1, &playerPositionTex[player]);
		glBindTexture(GL_TEXTURE_2D, playerPositionTex[player]);

		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, width, height, 0, GL_RGB, GL_FLOAT, NULL);

		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, playerPositionTex[player], 0);

		glObjectLabel(GL_TEXTURE, scenePositionTex, -1, "Player Position");
	}

	GLuint attachments[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
	glDrawBuffers(2, attachments);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE || !playerDepthTex[player] || !playerColourTex[player]) {
		return;
	}

	glBindTexture(GL_TEXTURE_2D, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

}

void Renderer::GeneratePostProcessBuffer(int width, int height) {
	glGenFramebuffers(1, &processFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, processFBO);

	glGenTextures(1, &processTexture);
	glBindTexture(GL_TEXTURE_2D, processTexture);

	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, processTexture, 0);

	glObjectLabel(GL_TEXTURE, processTexture, -1, "Post-Processing Colour");

	//Check framebuffer status
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		//Should probably print an error here 
		return;
	}

	glBindTexture(GL_TEXTURE_2D, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::CreateSceneNodes(Vector3* dimensions)
{
	root = new SceneNode(nodeShader);

	SceneNode* forest = new SceneNode(nodeShader);
	forest->SetTransform(Matrix4::Translation(*dimensions * Vector3(0.25, 0, 0)));
	root->AddChild(forest);

	SceneNode* towerNode = new SceneNode(nodeShader);
	towerNode->SetTransform(Matrix4::Translation(*dimensions * Vector3(0.6, 0.25, 0.55)));
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

	SceneNode* characterNode = new SceneNode(characterShader);
	characterNode->SetTransform(Matrix4::Translation(*dimensions * Vector3(0.8, 0.25, 0.8)) * Matrix4::Rotation(45, Vector3(0,1,0)));
	characterNode->SetModelScale(Vector3(200.0f, 200.0f, 200.0f));
	characterNode->SetBoundingRadius(10000.0f);
	characterNode->SetCharacter();
	root->AddChild(characterNode);
}

void Renderer::LoadMeshes()
{
	quad = Mesh::GenerateQuad();
	tower = Mesh::LoadFromMeshFile("Tower.msh");
	tree = Mesh::LoadFromMeshFile("Tree1.msh");

	eggMesh = Mesh::LoadFromMeshFile("Egg.msh");
	eggAnim = new MeshAnimation("Egg.anm");
	eggMaterial = new MeshMaterial("Egg.mat");

	for (int i = 0; i < eggMesh->GetSubMeshCount(); i++)
	{
		const MeshMaterialEntry* matEntry = eggMaterial->GetMaterialForLayer(i);

		const string* filename = nullptr;
		matEntry->GetEntry("Diffuse", &filename);
		string path = TEXTUREDIR + *filename;
		GLuint texID = SOIL_load_OGL_texture(path.c_str(),
			SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
		eggTextures.emplace_back(texID);

		matEntry->GetEntry("Bump", &filename);
		path = TEXTUREDIR + *filename;
		texID = SOIL_load_OGL_texture(path.c_str(),
			SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
		eggNormals.emplace_back(texID);

	}

	currentFrame = 0;
	frameTime = 0.0f;
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

	waterTextures[0] = SOIL_load_OGL_texture(TEXTUREDIR "water.TGA", SOIL_LOAD_AUTO,
		SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);

	waterTextures[1] = SOIL_load_OGL_texture(TEXTUREDIR "waterbump.png", SOIL_LOAD_AUTO,
		SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);


}

void Renderer::CreateMatrixUBO()
{
	glGenBuffers(1, &uboMatrices);
	glBindBuffer(GL_UNIFORM_BUFFER, uboMatrices);
	glBufferData(GL_UNIFORM_BUFFER, 3 * sizeof(Matrix4), projMatrix.values, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	unsigned int uniformBlockIndexTerrain = glGetUniformBlockIndex(terrainShader->GetProgram(), "Matrices");
	unsigned int uniformBlockIndexSkybox = glGetUniformBlockIndex(skyboxShader->GetProgram(), "Matrices");
	unsigned int uniformBlockIndexNode = glGetUniformBlockIndex(nodeShader->GetProgram(), "Matrices");
	unsigned int uniformBlockIndexSimple = glGetUniformBlockIndex(simpleShader->GetProgram(), "Matrices");
	unsigned int uniformBlockIndexFog = glGetUniformBlockIndex(fogShader->GetProgram(), "Matrices");
	unsigned int uniformBlockIndexSkeleton = glGetUniformBlockIndex(characterShader->GetProgram(), "Matrices");
	unsigned int uniformBlockIndexShadow = glGetUniformBlockIndex(shadowShader->GetProgram(), "Matrices");
	unsigned int uniformBlockIndexReflect = glGetUniformBlockIndex(reflectShader->GetProgram(), "Matrices");

	glBindBufferBase(GL_UNIFORM_BUFFER, 0, uboMatrices);

	glUniformBlockBinding(terrainShader->GetProgram(), uniformBlockIndexTerrain, 0);
	glUniformBlockBinding(skyboxShader->GetProgram(), uniformBlockIndexSkybox, 0);
	glUniformBlockBinding(nodeShader->GetProgram(), uniformBlockIndexNode, 0);
	glUniformBlockBinding(simpleShader->GetProgram(), uniformBlockIndexSimple, 0);
	glUniformBlockBinding(fogShader->GetProgram(), uniformBlockIndexFog, 0);
	glUniformBlockBinding(characterShader->GetProgram(), uniformBlockIndexSkeleton, 0);
	glUniformBlockBinding(shadowShader->GetProgram(), uniformBlockIndexShadow, 0);
	glUniformBlockBinding(reflectShader->GetProgram(), uniformBlockIndexReflect, 0);
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

	for (int i = 0; i < 2; i++)
	{
		if (!waterTextures[i])
			return;
		SetTextureRepeating(waterTextures[i], true);
		GLuint64 handler = glGetTextureHandleARB(waterTextures[i]);
		glMakeTextureHandleResidentARB(handler);
		textures.values[17 + i] = handler;
	}

	
	glGenBuffers(1, &uboTextures);
	glBindBuffer(GL_UNIFORM_BUFFER, uboTextures);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(textureStruct), textures.values, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	unsigned int uniformBlockIndexTerrain = glGetUniformBlockIndex(terrainShader->GetProgram(), "textures");
	unsigned int uniformBlockIndexNode = glGetUniformBlockIndex(nodeShader->GetProgram(), "textures");
	unsigned int uniformBlockIndexCharacter = glGetUniformBlockIndex(characterShader->GetProgram(), "textures");
	unsigned int uniformBlockIndexReflect = glGetUniformBlockIndex(reflectShader->GetProgram(), "textures");


	glBindBufferBase(GL_UNIFORM_BUFFER, 1, uboTextures);

	glUniformBlockBinding(terrainShader->GetProgram(), uniformBlockIndexTerrain, 1);
	glUniformBlockBinding(nodeShader->GetProgram(), uniformBlockIndexNode, 1);
	glUniformBlockBinding(characterShader->GetProgram(), uniformBlockIndexCharacter, 1);
	glUniformBlockBinding(reflectShader->GetProgram(), uniformBlockIndexReflect, 1);
}

void Renderer::CreateLightsUBO()
{
	glGenBuffers(1, &uboLights);
	glBindBuffer(GL_UNIFORM_BUFFER, uboLights);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(lightStruct), NULL, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	unsigned int uniformBlockIndexTerrain = glGetUniformBlockIndex(terrainShader->GetProgram(), "light");
	unsigned int uniformBlockIndexNode = glGetUniformBlockIndex(nodeShader->GetProgram(), "light");
	unsigned int uniformBlockIndexCharacter = glGetUniformBlockIndex(characterShader->GetProgram(), "light");
	unsigned int uniformBlockIndexReflect = glGetUniformBlockIndex(reflectShader->GetProgram(), "light");

	glBindBufferBase(GL_UNIFORM_BUFFER, 2, uboLights);

	glUniformBlockBinding(terrainShader->GetProgram(), uniformBlockIndexTerrain, 2);
	glUniformBlockBinding(nodeShader->GetProgram(), uniformBlockIndexNode, 2);
	glUniformBlockBinding(characterShader->GetProgram(), uniformBlockIndexCharacter, 2);
	glUniformBlockBinding(reflectShader->GetProgram(), uniformBlockIndexReflect, 2);
}

void Renderer::CreateCameraUBO() {
	glGenBuffers(1, &uboCamera);
	glBindBuffer(GL_UNIFORM_BUFFER, uboCamera);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(Vector3), NULL, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	unsigned int uniformBlockIndexTerrain = glGetUniformBlockIndex(terrainShader->GetProgram(), "camera");
	unsigned int uniformBlockIndexNode = glGetUniformBlockIndex(nodeShader->GetProgram(), "camera");
	unsigned int uniformBlockIndexFog = glGetUniformBlockIndex(fogShader->GetProgram(), "camera");
	unsigned int uniformBlockIndexCharacter = glGetUniformBlockIndex(characterShader->GetProgram(), "camera");
	unsigned int uniformBlockIndexReflect = glGetUniformBlockIndex(reflectShader->GetProgram(), "camera");

	glBindBufferBase(GL_UNIFORM_BUFFER, 3, uboCamera);

	glUniformBlockBinding(terrainShader->GetProgram(), uniformBlockIndexTerrain, 3);
	glUniformBlockBinding(nodeShader->GetProgram(), uniformBlockIndexNode, 3);
	glUniformBlockBinding(fogShader->GetProgram(), uniformBlockIndexFog, 3);
	glUniformBlockBinding(characterShader->GetProgram(), uniformBlockIndexCharacter, 3);
	glUniformBlockBinding(reflectShader->GetProgram(), uniformBlockIndexReflect, 3);
}

Renderer::~Renderer(void) {
	delete heightMap;
	delete camera[0];
	delete camera[1];

	delete terrainShader;
	delete nodeShader;
	delete skyboxShader;
	delete simpleShader;
	delete fogShader;
	delete shadowShader;
	delete reflectShader;

	delete root;
	delete light;
	delete quad;
	delete tower;
	delete tree;
	glDeleteTextures(8, terrainTextures);
	glDeleteTextures(2, towerTextures);

	glDeleteTextures(2, playerColourTex);
	glDeleteTextures(2, playerDepthTex);
	glDeleteTextures(2, playerPositionTex);

	glDeleteTextures(1, &sceneColourTex);
	glDeleteTextures(1, &sceneDepthTex);
	glDeleteTextures(1, &scenePositionTex);

	glDeleteFramebuffers(2, playerFBO);
	glDeleteFramebuffers(1, &processFBO);

	glDeleteTextures(1, &shadowTex);
	glDeleteFramebuffers(1, &shadowFBO);
	
	glDeleteTextures(2, playerShadowTex);
	glDeleteFramebuffers(2, playerShadowFBO);
}

void Renderer::UpdateScene(float dt) {
	camera[0]->UpdateCamera(dt);
	camera[1]->UpdateCamera(dt);
	root->Update(dt);

	sceneTime += dt;

	frameTime -= dt;
	while (frameTime < 0.0f) {
		currentFrame = (currentFrame + 1) % eggAnim->GetFrameCount();
		frameTime += 1.0f / eggAnim->GetFrameRate();
	}

	waterRotate += dt * 2.0f;
	waterCycle += dt * 0.25f;
}

void Renderer::ToggleCameraMode()
{
	camera[0]->ToggleCameraMode();
	camera[1]->ToggleCameraMode();
}

void Renderer::RenderScene() 
{
	if (splitScreenEnabled) {
		for (int i = 0; i < 2; i++)
		{
			viewMatrix = camera[i]->BuildViewMatrix();

			glBindBuffer(GL_UNIFORM_BUFFER, uboMatrices);
			glBufferSubData(GL_UNIFORM_BUFFER, sizeof(Matrix4), sizeof(Matrix4), viewMatrix.values);
			glBindBuffer(GL_UNIFORM_BUFFER, 0);

			glBindBuffer(GL_UNIFORM_BUFFER, uboCamera);
			glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(Vector3), &(camera[i]->GetPosition()));
			glBindBuffer(GL_UNIFORM_BUFFER, 0);

			DrawShadowScene(&playerShadowFBO[i], &i);

			GLuint64 handler = glGetTextureHandleARB(playerShadowTex[i]);
			glMakeTextureHandleResidentARB(handler);
			textures.values[20] = handler;

			glBindBuffer(GL_UNIFORM_BUFFER, uboTextures);
			glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(textureStruct), textures.values);
			glBindBuffer(GL_UNIFORM_BUFFER, 0);

			DrawScene(&playerFBO[i], &i);
		}
	}
	else {
		viewMatrix = camera[0]->BuildViewMatrix();

		glBindBuffer(GL_UNIFORM_BUFFER, uboMatrices);
		glBufferSubData(GL_UNIFORM_BUFFER, sizeof(Matrix4), sizeof(Matrix4), viewMatrix.values);
		glBindBuffer(GL_UNIFORM_BUFFER, 0);

		glBindBuffer(GL_UNIFORM_BUFFER, uboCamera);
		glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(Vector3), &(camera[0]->GetPosition()));
		glBindBuffer(GL_UNIFORM_BUFFER, 0);

		int tempIndex = 0;
		DrawShadowScene(&shadowFBO, &tempIndex);

		GLuint64 handler = glGetTextureHandleARB(shadowTex);
		glMakeTextureHandleResidentARB(handler);
		textures.values[20] = handler;

		glBindBuffer(GL_UNIFORM_BUFFER, uboTextures);
		glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(textureStruct), textures.values);
		glBindBuffer(GL_UNIFORM_BUFFER, 0);

		DrawScene(&sceneFBO,&tempIndex);
	}
	
	DrawPostProcess();
	PresentScene();
	ResetProjectionMatrix();

}

void Renderer::DrawShadowScene(GLuint* framebuffer, int* cameraIndex) {
	glBindFramebuffer(GL_FRAMEBUFFER, *framebuffer);

	glClear(GL_DEPTH_BUFFER_BIT);
	glViewport(0, 0, SHADOWSIZE, SHADOWSIZE);
	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

	BindShader(shadowShader);

	viewMatrix = Matrix4::BuildViewMatrix(light->GetPosition(), Vector3(heightMap->GetHeightMapSize().x * 0.7,
		heightMap->GetHeightMapSize().y * 0.25, heightMap->GetHeightMapSize().z * 0.7));
	int splitWidth = (splitScreenEnabled) ? width / 2 : width;
	projMatrix = Matrix4::Perspective(400, 12000, (float) width / (float) height , 60);
	shadowMatrix = projMatrix * viewMatrix;

	glBindBuffer(GL_UNIFORM_BUFFER, uboMatrices);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(Matrix4), projMatrix.values);
	glBufferSubData(GL_UNIFORM_BUFFER, sizeof(Matrix4), sizeof(Matrix4), viewMatrix.values);
	glBufferSubData(GL_UNIFORM_BUFFER, 2 * sizeof(Matrix4), sizeof(Matrix4), shadowMatrix.values);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	DrawTerrain();
	FillTerrain(cameraIndex);

	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

	glViewport(0, 0, width, height);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	viewMatrix = camera[*cameraIndex]->BuildViewMatrix();
	projMatrix = Matrix4::Perspective(1.0f, 15000.0f, (float)width / (float)height, 45.0f);
	glBindBuffer(GL_UNIFORM_BUFFER, uboMatrices);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(Matrix4), projMatrix.values);
	glBufferSubData(GL_UNIFORM_BUFFER, sizeof(Matrix4), sizeof(Matrix4), viewMatrix.values);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

}

void Renderer::ResetProjectionMatrix()
{
	projMatrix = Matrix4::Perspective(1.0f, 15000.0f, (float)width / (float)height, 45.0f);
	glBindBuffer(GL_UNIFORM_BUFFER, uboMatrices);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(Matrix4), projMatrix.values);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void Renderer::DrawScene(GLuint* framebuffer, int* cameraIndex)
{
		glBindFramebuffer(GL_FRAMEBUFFER, *framebuffer);
		glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
		frameFrustum.FromMatrix(projMatrix * viewMatrix);
		DrawSkybox();
		DrawTerrain();
		DrawWater();
		FillTerrain(cameraIndex);

		glBindFramebuffer(GL_FRAMEBUFFER, 0);	
	
}
void Renderer::DrawPostProcess()
{
	glBindFramebuffer(GL_FRAMEBUFFER, processFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, processTexture, 0);
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

	BindShader(fogShader);
	modelMatrix.ToIdentity();
	viewMatrix.ToIdentity();
	projMatrix.ToIdentity();

	glBindBuffer(GL_UNIFORM_BUFFER, uboMatrices);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(Matrix4), projMatrix.values);
	glBufferSubData(GL_UNIFORM_BUFFER, sizeof(Matrix4), sizeof(Matrix4), viewMatrix.values);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
	glUniformMatrix4fv(glGetUniformLocation(fogShader->GetProgram(), "modelMatrix"), 1, false, modelMatrix.values);

	glUniform1f(glGetUniformLocation(fogShader->GetProgram(), "fogFactor"), sin(sceneTime / 2));


	glDisable(GL_DEPTH_TEST);

	if (splitScreenEnabled) {
		for (int i = 0; i < 2; i++)
		{
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, playerColourTex[i]);
			glUniform1i(glGetUniformLocation(fogShader->GetProgram(), "diffuseTex"), 0);

			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, playerPositionTex[i]);
			glUniform1i(glGetUniformLocation(fogShader->GetProgram(), "positionTex"), 1);


			glBindBuffer(GL_UNIFORM_BUFFER, uboCamera);
			glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(Vector3), &(camera[i]->GetPosition()));
			glBindBuffer(GL_UNIFORM_BUFFER, 0);

			modelMatrix = Matrix4::Translation(Vector3(-0.5 + i, 0, 0)) * Matrix4::Scale(Vector3(0.5, 1.0, 1.0));
			glUniformMatrix4fv(glGetUniformLocation(fogShader->GetProgram(), "modelMatrix"), 1, false, modelMatrix.values);
			quad->Draw();
		}
	}
	else {
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, sceneColourTex);
		glUniform1i(glGetUniformLocation(fogShader->GetProgram(), "diffuseTex"), 0);

		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, scenePositionTex);
		glUniform1i(glGetUniformLocation(fogShader->GetProgram(), "positionTex"), 1);

		quad->Draw();
	}


	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glEnable(GL_DEPTH_TEST);
}

void Renderer::PresentScene()
{
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

	BindShader(simpleShader);
	modelMatrix.ToIdentity();
	viewMatrix.ToIdentity();
	projMatrix.ToIdentity();

	glBindBuffer(GL_UNIFORM_BUFFER, uboMatrices);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(Matrix4), projMatrix.values);
	glBufferSubData(GL_UNIFORM_BUFFER, sizeof(Matrix4), sizeof(Matrix4), viewMatrix.values);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
	glUniformMatrix4fv(glGetUniformLocation(simpleShader->GetProgram(), "modelMatrix"), 1, false, modelMatrix.values);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, processTexture);
	glUniform1i(glGetUniformLocation(simpleShader->GetProgram(), "diffuseTex"), 0);
	quad->Draw();
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


	heightMap->Draw();
}

void Renderer::DrawWater() {
	BindShader(reflectShader);

	glUniform1i(glGetUniformLocation(reflectShader->GetProgram(), "cubeTex"), 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMap);

	Vector3 hSize = heightMap->GetHeightMapSize();

	modelMatrix = Matrix4::Translation(Vector3(hSize.x * 0.5f, hSize.y * 0.15f , hSize.z * 0.5f)) * 
				Matrix4::Scale(Vector3(5000,5000,5000)) * Matrix4::Rotation(-90, Vector3(1, 0, 0));

	textureMatrix =
		Matrix4::Translation(Vector3(waterCycle, 0.0f, waterCycle)) *
		Matrix4::Scale(Vector3(10, 10, 10)) *
		Matrix4::Rotation(waterRotate, Vector3(0, 0, 1));

	glUniformMatrix4fv(glGetUniformLocation(reflectShader->GetProgram(), "modelMatrix"), 1, false, modelMatrix.values);
	glUniformMatrix4fv(glGetUniformLocation(reflectShader->GetProgram(), "textureMatrix"), 1, false, textureMatrix.values);

	quad->Draw();

	modelMatrix.ToIdentity();

}
void Renderer::FillTerrain(int* camera) {
	BuildNodeLists(root, camera);
	SortNodeLists();
	DrawNodes();
	ClearNodeLists();
}

void Renderer::BuildNodeLists(SceneNode* from, int* cameraIndex) {
	if (frameFrustum.InsideFrustum(*from)) {
		Vector3 dir = from->GetWorldTransform().GetPositionVector() - camera[*cameraIndex]->GetPosition();
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
		BuildNodeLists((*i), cameraIndex);
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
	if (n->IsCharacter()) {

		DrawCharacter(n);
		return;
	}
	
	if (!n->GetMesh()) return;

	Matrix4 model = n->GetWorldTransform() * Matrix4::Scale(n->GetModelScale());
	Shader* currentShader = n->GetShader();
	Mesh* nodeMesh = n->GetMesh();
	BindShader(currentShader);

	glUniformMatrix4fv(glGetUniformLocation(currentShader->GetProgram(), "modelMatrix"), 1, false, model.values);

	glUniform1i(glGetUniformLocation(currentShader->GetProgram(), "textureIndex"), n->GetTexture());


	if (nodeMesh->GetSubMeshCount() == 1) {
		n->Draw(*this);
	}
	else {
		for (int i = 0; i < nodeMesh->GetSubMeshCount(); i++)
		{
			if (i % 2) {
				glUniform1i(glGetUniformLocation(currentShader->GetProgram(), "textureIndex"), n->GetTexture() + i + 1);
			}
			else {
				glUniform1i(glGetUniformLocation(currentShader->GetProgram(), "textureIndex"), n->GetTexture() + i);
			}
			nodeMesh->DrawSubMesh(i);
		}
	}


}

void Renderer::DrawCharacter(SceneNode* n)
{
	Matrix4 model = n->GetWorldTransform() * Matrix4::Scale(n->GetModelScale());
	Shader* currentShader = n->GetShader();
	BindShader(currentShader);

	glUniformMatrix4fv(glGetUniformLocation(currentShader->GetProgram(), "modelMatrix"), 1, false, model.values);

	vector<Matrix4> frameMatrices;

	const Matrix4* invBindPose = eggMesh->GetInverseBindPose();
	const Matrix4* frameData = eggAnim->GetJointData(currentFrame);

	for (unsigned int i = 0; i < eggMesh->GetJointCount(); i++)
	{
		frameMatrices.emplace_back(frameData[i] * invBindPose[i]);
	}

	int j = glGetUniformLocation(currentShader->GetProgram(), "joints");
	glUniformMatrix4fv(j, frameMatrices.size(), false, (float*)frameMatrices.data());


	for (int i = 0; i < eggMesh->GetSubMeshCount(); i++)
	{
		glUniform1i(glGetUniformLocation(currentShader->GetProgram(),
			"diffuseTex"), 0);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, eggTextures[i]);

		glUniform1i(glGetUniformLocation(currentShader->GetProgram(),
			"normalTex"), 1);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, eggNormals[i]);

		eggMesh->DrawSubMesh(i);
	}
}

void Renderer::ClearNodeLists() {
	transparentNodeList.clear();
	nodeList.clear();
}
