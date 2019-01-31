/*
* Copyright (c) 2006-2007 Erin Catto http://www.gphysics.com
*
* Permission to use, copy, modify, distribute and sell this software
* and its documentation for any purpose is hereby granted without fee,
* provided that the above copyright notice appear in all copies.
* Erin Catto makes no representations about the suitability 
* of this software for any purpose.  
* It is provided "as is" without express or implied warranty.
*/

#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>

#include "imgui/imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl2.h"

#include "GLFW/glfw3.h"

#include "dynamic-tree/tree.h"

namespace
{
	GLFWwindow* mainWindow = NULL;

	dtTree tree;

	int demoIndex = 0;

	int width = 1280;
	int height = 720;
	float zoom = 10.0f;
	float pan_y = 8.0f;
}

static void glfwErrorCallback(int error, const char* description)
{
	printf("GLFW error %d: %s\n", error, description);
}

static void DrawText(int x, int y, const char* string)
{
	ImVec2 p;
	p.x = float(x);
	p.y = float(y);
	ImGui::Begin("Overlay", NULL, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar);
	ImGui::SetCursorPos(p);
	ImGui::TextColored(ImColor(230, 153, 153, 255), "%s", string);
	ImGui::End();
}

static void DrawBox(const dtAABB& aabb)
{
	dtVec c = aabb.upperBound - aabb.lowerBound;
	dtVec h = 0.5f * (aabb.lowerBound + aabb.upperBound);

	dtVec v1 = c + dtVecSet(-h.x, -h.y, 0.0f);
	dtVec v2 = c + dtVecSet( h.x, -h.y, 0.0f);
	dtVec v3 = c + dtVecSet( h.x,  h.y, 0.0f);
	dtVec v4 = c + dtVecSet(-h.x,  h.y, 0.0f);

	glColor3f(0.8f, 0.8f, 0.9f);

	glBegin(GL_LINE_LOOP);
	glVertex2f(v1.x, v1.y);
	glVertex2f(v2.x, v2.y);
	glVertex2f(v3.x, v3.y);
	glVertex2f(v4.x, v4.y);
	glEnd();
}

// Single box
static void Demo1()
{
	dtAABB b;
	b.lowerBound = dtVecSet(-0.5f, -0.5f, -0.5f);
	b.upperBound = dtVecSet(0.5f, 0.5f, 0.5f);
	tree.CreateProxy(b);
}

void (*demos[])() = {Demo1};
const char* demoStrings[] = {
	"Demo 1: A Single Box"};

static void InitDemo(int index)
{
	tree.Clear();
	demoIndex = index;
	demos[index]();
}

static void Keyboard(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (action != GLFW_PRESS)
	{
		return;
	}

	switch (key)
	{
	case GLFW_KEY_ESCAPE:
		// Quit
		glfwSetWindowShouldClose(mainWindow, GL_TRUE);
		break;

	case '1':
		InitDemo(key - GLFW_KEY_1);
		break;
	}
}

static void Reshape(GLFWwindow*, int w, int h)
{
	width = w;
	height = h > 0 ? h : 1;

	glViewport(0, 0, width, height);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	float aspect = float(width) / float(height);
	if (width >= height)
	{
		// aspect >= 1, set the height from -1 to 1, with larger width
		glOrtho(-zoom * aspect, zoom * aspect, -zoom + pan_y, zoom + pan_y, -1.0, 1.0);
	}
	else
	{
		// aspect < 1, set the width to -1 to 1, with larger height
		glOrtho(-zoom, zoom, -zoom / aspect + pan_y, zoom / aspect + pan_y, -1.0, 1.0);
	}
}

int main(int, char**)
{
	glfwSetErrorCallback(glfwErrorCallback);

	if (glfwInit() == 0)
	{
		fprintf(stderr, "Failed to initialize GLFW\n");
		return -1;
	}

	mainWindow = glfwCreateWindow(width, height, "box2d-lite", NULL, NULL);
	if (mainWindow == NULL)
	{
		fprintf(stderr, "Failed to open GLFW mainWindow.\n");
		glfwTerminate();
		return -1;
	}

	glfwMakeContextCurrent(mainWindow);
	glfwSwapInterval(1);
	glfwSetWindowSizeCallback(mainWindow, Reshape);
	glfwSetKeyCallback(mainWindow, Keyboard);

	float xscale, yscale;
	glfwGetWindowContentScale(mainWindow, &xscale, &yscale);
	float uiScale = xscale;

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui::StyleColorsClassic();
	ImGui_ImplGlfw_InitForOpenGL(mainWindow, true);
	ImGui_ImplOpenGL2_Init();
	ImGuiIO& io = ImGui::GetIO();
	io.FontGlobalScale = uiScale;

	glViewport(0, 0, width, height);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	
	float aspect = float(width) / float(height);
	if (width >= height)
	{
		// aspect >= 1, set the height from -1 to 1, with larger width
		glOrtho(-zoom * aspect, zoom * aspect, -zoom + pan_y, zoom + pan_y, -1.0, 1.0);
	}
	else
	{
		// aspect < 1, set the width to -1 to 1, with larger height
		glOrtho(-zoom, zoom, -zoom / aspect + pan_y, zoom / aspect + pan_y, -1.0, 1.0);
	}

	InitDemo(0);

	while (!glfwWindowShouldClose(mainWindow))
	{
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		ImGui_ImplOpenGL2_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		// Globally position text
		ImGui::SetNextWindowPos(ImVec2(10.0f, 10.0f));
		ImGui::Begin("Overlay", NULL, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar);
		ImGui::End();

		DrawText(5, 5, demoStrings[demoIndex]);
		DrawText(5, 35, "Keys: 1-9 Demos");

		char buffer[64];
		sprintf(buffer, "height %d", tree.ComputeHeight());
		DrawText(5, 65, buffer);

		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();

		for (int i = 0; i < tree.m_nodeCapacity; ++i)
		{
			if (tree.m_nodes[i].next)
			{
				DrawBox(tree.m_nodes[i].aabb);
			}
		}

		ImGui::Render();
		ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());

		glfwPollEvents();
		glfwSwapBuffers(mainWindow);
	}

	glfwTerminate();
	return 0;
}
