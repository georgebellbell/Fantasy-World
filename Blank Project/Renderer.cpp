#include "Renderer.h"


Renderer::Renderer(Window& parent) : OGLRenderer(parent) {
	cube = Mesh::LoadFromMeshFile("OffsetCubeY.msh");
	camera[0] = new Camera();
	camera[1] = new Camera();
	quad = Mesh::GenerateQuad();
	sceneShader = new Shader("SceneVertex.glsl", "SceneFragment.glsl");
	finalShader = new Shader("TexturedVertex.glsl", "TexturedFragment.glsl");

	if (!sceneShader->LoadSuccess() ||  !finalShader->LoadSuccess()) {
		return;
	}

	GenerateSceneBuffer(&bufferFBO[0], width / 2, height, 0);
	GenerateSceneBuffer(&bufferFBO[1], width / 2, height, 1);
	GeneratePostProcessBuffer(width, height);
	projMatrix = Matrix4::Perspective(1.0f, 10000.0f, (float)width / (float)height, 45.0f);

	camera[0]->SetPosition(Vector3(0, 30, 175));
	camera[1]->SetPosition(Vector3(100, 30, 175));

	root = new SceneNode();
	root->AddChild(new CubeRobot(cube));

	glEnable(GL_DEPTH_TEST);
	init = true;
}

void Renderer::GenerateSceneBuffer(GLuint* buffer, int width, int height, int player) {
	glGenFramebuffers(1, buffer);
	glBindFramebuffer(GL_FRAMEBUFFER, *buffer);

	//Create colour texture
	{
		glGenTextures(1, &bufferColourTex[player]);
		glBindTexture(GL_TEXTURE_2D, bufferColourTex[player]);

		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, bufferColourTex[player], 0);

		glObjectLabel(GL_TEXTURE, bufferColourTex[player], -1, "G-Buffer Colour");
	}

	//Create depth texture
	{
		glGenTextures(1, &bufferDepthTex[player]);
		glBindTexture(GL_TEXTURE_2D, bufferDepthTex[player]);

		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, width, height, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, NULL);

		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, bufferDepthTex[player], 0);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_TEXTURE_2D, bufferDepthTex[player], 0);

		glObjectLabel(GL_TEXTURE, bufferDepthTex[player], -1, "G-Buffer Depth");
	}

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE || !bufferDepthTex[player] || !bufferColourTex[player]) {
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

Renderer::~Renderer(void) {
	delete root;
	delete camera[0];
	delete camera[1];
	delete cube;
	delete sceneShader;
	delete finalShader;
	delete quad;
	//delete camera;

	glDeleteTextures(2, bufferColourTex);
	glDeleteTextures(2, bufferDepthTex);
	glDeleteFramebuffers(2, bufferFBO);
	glDeleteFramebuffers(1, &processFBO);
}

void Renderer::UpdateScene(float dt) {
	camera[0]->UpdateCamera(dt);
	camera[1]->UpdateCamera(dt);
	//viewMatrix = camera->BuildViewMatrix();
	root->Update(dt);
}

void Renderer::RenderScene() {
	DrawScene();
	DrawPostProcess();
	PresentScene();
}


void Renderer::DrawScene() {
	for (int i = 0; i <  2; i++)
	{
		glBindFramebuffer(GL_FRAMEBUFFER, bufferFBO[i]);
		glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
		BindShader(sceneShader);
		viewMatrix = camera[i]->BuildViewMatrix();
		projMatrix = Matrix4::Perspective(1.0f, 10000.0f, (float)width / (float)height, 45.0f);
		UpdateShaderMatrices();
		glUniform1i(glGetUniformLocation(sceneShader->GetProgram(), "diffuseTex"), 1);
		DrawNode(root);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	
}

void Renderer::DrawPostProcess() {
	glBindFramebuffer(GL_FRAMEBUFFER, processFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, processTexture, 0);
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

	BindShader(finalShader);
	modelMatrix.ToIdentity();
	viewMatrix.ToIdentity();
	projMatrix.ToIdentity();
	UpdateShaderMatrices();

	glDisable(GL_DEPTH_TEST);


	
	glActiveTexture(GL_TEXTURE0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, processTexture, 0);
	glBindTexture(GL_TEXTURE_2D, bufferColourTex[0]);
	glUniform1i(glGetUniformLocation(finalShader->GetProgram(), "diffuseTex"), 0);

	modelMatrix = Matrix4::Translation(Vector3(-0.5, 0, 0)) * Matrix4::Scale(Vector3(0.5, 1.0, 1.0));
	UpdateShaderMatrices();
	quad->Draw();

	
	glActiveTexture(GL_TEXTURE0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, processTexture, 0);
	glBindTexture(GL_TEXTURE_2D, bufferColourTex[1]);
	glUniform1i(glGetUniformLocation(finalShader->GetProgram(), "diffuseTex"), 0);

	modelMatrix.ToIdentity();
	modelMatrix = Matrix4::Translation(Vector3(0.5, 0, 0)) * Matrix4::Scale(Vector3(0.5, 1.0, 1.0));
	UpdateShaderMatrices();
	quad->Draw();

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glEnable(GL_DEPTH_TEST);
}

void Renderer::PresentScene() {
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

	BindShader(finalShader);
	modelMatrix.ToIdentity();
	viewMatrix.ToIdentity();
	projMatrix.ToIdentity();
	UpdateShaderMatrices();

	glActiveTexture(GL_TEXTURE0);

	glBindTexture(GL_TEXTURE_2D, processTexture);
	glUniform1i(glGetUniformLocation(finalShader->GetProgram(), "diffuseTex"), 0);
	quad->Draw();


}

void Renderer::DrawNode(SceneNode* n) {
	if (n->GetMesh()) {
		Matrix4 model = n->GetWorldTransform() * Matrix4::Scale(n->GetModelScale());

		glUniformMatrix4fv(
			glGetUniformLocation(sceneShader->GetProgram(),
				"modelMatrix"), 1, false, model.values);

		glUniform4fv(glGetUniformLocation(sceneShader->GetProgram(),
			"nodeColour"), 1, (float*)&n->GetColour());

		glUniform1i(glGetUniformLocation(sceneShader->GetProgram(),
			"useTexture"), 0);

		n->Draw(*this);
	}

	for (vector<SceneNode*>::const_iterator i = n->GetChildIteratorStart(); i != n->GetChildIteratorEnd(); ++i) {
		DrawNode(*i);
	}

}
