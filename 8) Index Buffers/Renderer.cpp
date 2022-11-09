#include "Renderer.h"
#include "../nclgl/Camera.h"
#include "../nclgl/HeightMap.h"


Renderer::Renderer(Window& parent) : OGLRenderer(parent) {
	heightMap = new HeightMap(TEXTUREDIR"Fantasy_Heightmap.png");
	camera = new Camera(-40, 270, Vector3());

	Vector3 dimensions = heightMap->GetHeightMapSize();
	camera->SetPosition(dimensions * Vector3(0.5, 2 / 5, 0.5));

	shader = new Shader("TexturedVertex.glsl", "TexturedFragment.glsl");

	if (!shader->LoadSuccess())
		return;

	terrainTex = SOIL_load_OGL_texture(TEXTUREDIR"Barren Reds.JPG", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);

	snowTex = SOIL_load_OGL_texture(TEXTUREDIR"snow.jpg", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
	sandTex = SOIL_load_OGL_texture(TEXTUREDIR"sand.jpg", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);
	grassTex = SOIL_load_OGL_texture(TEXTUREDIR"grass.jpg", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS);



	if (!terrainTex ||!snowTex ||!sandTex ||!grassTex) 
		return;

	SetTextureRepeating(terrainTex, true);
	SetTextureRepeating(snowTex, true);
	SetTextureRepeating(sandTex, true);
	SetTextureRepeating(grassTex, true);

	GLuint64 handler = glGetTextureHandleARB(sandTex);
	glMakeTextureHandleResidentARB(handler);

	struct samplers {
		GLuint64 arr[1];
	} textures;

	textures.arr[0] = handler;

	unsigned int uniformBlockIndexTextures = glGetUniformBlockIndex(shader->GetProgram(), "textures");
	glUniformBlockBinding(shader->GetProgram(), uniformBlockIndexTextures, 0);

	glGenBuffers(1, &uboTextures);
	glBindBuffer(GL_UNIFORM_BUFFER, uboTextures);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(samplers), NULL, GL_STATIC_DRAW);
	glBindBufferRange(GL_UNIFORM_BUFFER, 0, uboTextures, 0, sizeof(samplers));
	glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(samplers), textures.arr);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	projMatrix = Matrix4::Perspective(1.0f, 10000.0f, (float)width / (float)height, 45.0f);

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	init = true;
}

Renderer::~Renderer(void) {
	delete heightMap;
	delete camera;
	delete shader;
}

void Renderer::UpdateScene(float dt) {
	camera->UpdateCamera(dt);
	viewMatrix = camera->BuildViewMatrix();
}


void Renderer::RenderScene() {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	BindShader(shader);
	UpdateShaderMatrices();

	glUniform1f(glGetUniformLocation(shader->GetProgram(), "terrainHeight"), heightMap->GetHeightMapSize().y);
	glUniform1i(glGetUniformLocation(shader->GetProgram(), "diffuseTex"), 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, terrainTex);
	
	glUniform1i(glGetUniformLocation(shader->GetProgram(), "snowTex"), 1);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, snowTex);

	glUniform1i(glGetUniformLocation(shader->GetProgram(), "grassTex"), 2);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, grassTex);

	/*glUniform1i(glGetUniformLocation(shader->GetProgram(), "sandTex"), 3);
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, sandTex);*/


	heightMap->Draw();
}