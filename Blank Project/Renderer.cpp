#include "Renderer.h"
#include "../nclgl/Camera.h"
#include "../nclgl/HeightMap.h"
#include "../nclgl/Light.h"
#include <algorithm>


struct textureStruct {
	GLuint64 texture[8];
} terrain;

Renderer::Renderer(Window& parent) : OGLRenderer(parent) {
	heightMap = new HeightMap(TEXTUREDIR"Fantasy_Heightmap.png");
	Vector3 dimensions = heightMap->GetHeightMapSize();
	camera = new Camera(-40, 270, dimensions * Vector3(0.5, 1, 0.5));
	light = new Light(dimensions * Vector3(0.5f, 1.5f, 0.5f), Vector4(1, 1, 1, 1), dimensions.x * 0.6f, Vector4(1, 1, 1, 1));
	
	terrainShader = new Shader("TerrainVertex.glsl", "TerrainFragment.glsl");
	cubeShader = new Shader("SceneVertex.glsl", "SceneFragment.glsl");
	skyboxShader = new Shader("SkyboxVertex.glsl", "SkyboxFragment.glsl");

	if (!terrainShader->LoadSuccess() || !cubeShader->LoadSuccess() || !skyboxShader->LoadSuccess())
		return;
	cube = Mesh::LoadFromMeshFile("Tower.msh");
	quad = Mesh::GenerateQuad();
	
	LoadTerrainTextures();
	cubeTex = SOIL_load_OGL_texture(TEXTUREDIR"stainedglass.tga", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, 0);

	cubeMap = SOIL_load_OGL_cubemap(TEXTUREDIR "SkyCloudy_Right.png", TEXTUREDIR "SkyCloudy_Left.png",
		TEXTUREDIR "SkyCloudy_Top.png", TEXTUREDIR "SkyCloudy_Bottom.png",
		TEXTUREDIR "SkyCloudy_Front.png", TEXTUREDIR "SkyCloudy_Back.png",
		SOIL_LOAD_RGB, SOIL_CREATE_NEW_ID, 0);

	for (int i = 0; i < 8; i++)
	{
		if (!terrainTextures[i])
			return;
		SetTextureRepeating(terrainTextures[i], true);
		GLuint64 handler = glGetTextureHandleARB(terrainTextures[i]);
		glMakeTextureHandleResidentARB(handler);

		terrain.texture[i] = handler;
	}

	projMatrix = Matrix4::Perspective(1.0f, 15000.0f, (float)width / (float)height, 45.0f);

	

	

	glGenBuffers(1, &uboMatrices);
	glBindBuffer(GL_UNIFORM_BUFFER, uboMatrices);
	glBufferData(GL_UNIFORM_BUFFER, 2 * sizeof(Matrix4), projMatrix.values, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	unsigned int uniformBlockIndexTerrain = glGetUniformBlockIndex(terrainShader->GetProgram(), "Matrices");
	unsigned int uniformBlockIndexSkybox = glGetUniformBlockIndex(skyboxShader->GetProgram(), "Matrices");
	unsigned int uniformBlockIndexCube = glGetUniformBlockIndex(cubeShader->GetProgram(), "Matrices");

	glBindBufferBase(GL_UNIFORM_BUFFER, 0, uboMatrices);

	glUniformBlockBinding(terrainShader->GetProgram(), uniformBlockIndexTerrain, 0);
	glUniformBlockBinding(skyboxShader->GetProgram(), uniformBlockIndexSkybox, 0);
	glUniformBlockBinding(cubeShader->GetProgram(), uniformBlockIndexCube, 0);



	
	glGenBuffers(1, &uboTextures);
	glBindBuffer(GL_UNIFORM_BUFFER, uboTextures);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(textureStruct), terrain.texture, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	unsigned int uniformBlockIndexTextures = glGetUniformBlockIndex(terrainShader->GetProgram(), "textures");

	glBindBufferBase(GL_UNIFORM_BUFFER, 1, uboTextures);

	glUniformBlockBinding(terrainShader->GetProgram(), uniformBlockIndexTextures, 1);


	root = new SceneNode(cubeShader);

	for (int i = 5; i > 0; --i)
	{
		SceneNode* s = new SceneNode(cubeShader);
		s->SetColour(Vector4(1.0f, 1.0f, 1.0f, 0.5f));
		s->SetTransform(Matrix4::Translation(
			dimensions * Vector3(0.5, 0.2 * i, 0.5)));
		s->SetModelScale(Vector3(100.0f, 100.0f, 100.0f));
		s->SetBoundingRadius(100.0f);
		s->SetMesh(cube);
		s->SetTexture(cubeTex);
		root->AddChild(s);
	}

	projMatrix = Matrix4::Perspective(1.0f, 15000.0f, (float)width / (float)height, 45.0f);

	//glBindBuffer(GL_UNIFORM_BUFFER, uboMatrices);
	//glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(Matrix4), projMatrix.values);
	//glBindBuffer(GL_UNIFORM_BUFFER, 0);

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
	init = true;
}

void Renderer::LoadTerrainTextures()
{
	terrainTextures[0] = SOIL_load_OGL_texture(TEXTUREDIR"snow.jpg", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
	terrainTextures[1] = SOIL_load_OGL_texture(TEXTUREDIR"snowBump.jpg", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);

	terrainTextures[2] = SOIL_load_OGL_texture(TEXTUREDIR"rock.jpg", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
	terrainTextures[3] = SOIL_load_OGL_texture(TEXTUREDIR"rockBump.jpg", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);

	terrainTextures[4] = SOIL_load_OGL_texture(TEXTUREDIR"grass.jpg", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
	terrainTextures[5] = SOIL_load_OGL_texture(TEXTUREDIR"grassBump.jpg", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);

	terrainTextures[6] = SOIL_load_OGL_texture(TEXTUREDIR"sand.jpg", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
	terrainTextures[7] = SOIL_load_OGL_texture(TEXTUREDIR"sandBump.jpg", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
}

Renderer::~Renderer(void) {
	delete heightMap;
	delete camera;
	delete terrainShader;
	delete cubeShader;
	delete skyboxShader;
	delete root;
	delete light;
	delete quad;
	delete cube;
	glDeleteTextures(8, terrainTextures);
	glDeleteTextures(1, &cubeTex);
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
	//UpdateShaderMatrices();

	quad->Draw();

	glDepthMask(GL_TRUE);
}

void Renderer::DrawTerrain()
{
	BindShader(terrainShader);
	//UpdateShaderMatrices();
	SetShaderLight(*light);
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
	if (n->GetMesh()) {
		Matrix4 model = n->GetWorldTransform() * Matrix4::Scale(n->GetModelScale());
		Shader* nodeShader = n->GetShader();
		BindShader(nodeShader);
		glUniform1i(glGetUniformLocation(nodeShader->GetProgram(), "diffuseTex"), 0);
		//UpdateNodeShaderMatrices();

		glUniformMatrix4fv(glGetUniformLocation(nodeShader->GetProgram(), "modelMatrix"), 1, false, model.values);
		glUniform4fv(glGetUniformLocation(nodeShader->GetProgram(), "nodeColour"), 1, (float*)&n->GetColour());

		GLuint texture = n->GetTexture();
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, texture);
		glUniform1i(glGetUniformLocation(nodeShader->GetProgram(), "useTexture"), texture);

		n->Draw(*this);
	}
}

void Renderer::ClearNodeLists() {
	transparentNodeList.clear();
	nodeList.clear();
}
