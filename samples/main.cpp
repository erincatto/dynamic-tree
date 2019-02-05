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

#if defined(__APPLE_CC__)
#include <OpenGL/gl3.h>
#define GLFW_INCLUDE_GLCOREARB
#else
#include "glad.h"
#endif

#include "glfw/glfw3.h"
#include "imgui/imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "dynamic-tree/tree.h"

#include "draw.h"
#include "test.h"

namespace
{
	GLFWwindow* g_window = nullptr;

	Draw g_draw;
	Camera& g_camera = g_draw.m_camera;

	float g_mouseX = 0.0f;
	float g_mouseY = 0.0f;

	int g_width = 1280;
	int g_height = 720;

	Test* g_tests[128];
	int g_testCount = 0;
	int g_testIndex = 0;
	Test* g_test = nullptr;

	bool g_showUI = true;
	bool g_rotate = true;
	bool g_drawInternal = true;
}

static void glfwErrorCallback(int error, const char* description)
{
	printf("GLFW error %d: %s\n", error, description);
}

static void InitTestArray()
{
	extern Test* g_test1;
	extern Test* g_test2;
	extern Test* g_test3;

	g_tests[0] = g_test1;
	g_tests[1] = g_test2;
	g_tests[2] = g_test3;
	g_testCount = 3;
}

static void InitTest(int index)
{
	g_tests[g_testIndex]->Destroy();
	g_test = g_tests[index];
	g_testIndex = index;
	g_test->Create(g_rotate);
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
		glfwSetWindowShouldClose(g_window, GL_TRUE);
		break;

	case GLFW_KEY_LEFT_BRACKET:
		{
			int testIndex = dtMax(0, g_testIndex - 1);
			InitTest(testIndex);
		}
		break;

	case GLFW_KEY_RIGHT_BRACKET:
		{
			int testIndex = dtMin(g_testCount - 1, g_testIndex + 1);
			InitTest(testIndex);
		}
		break;
	}
}

static void MouseClick(GLFWwindow* window, int button, int action, int mods)
{
	if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS)
	{
		// Update mouse so camera doesn't jump
		double xd, yd;
		glfwGetCursorPos(window, &xd, &yd);
		g_mouseX = float(xd);
		g_mouseY = float(yd);
	}
}

static void MouseMove(GLFWwindow* window, double xd, double yd)
{
	float x = float(xd);
	float y = float(yd);

	bool rightButton = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;

	if (rightButton)
	{
		const float rate = 0.2f * dtPi / 180.0f;
		g_camera.m_yaw -= rate * (x - g_mouseX);
		g_camera.m_pitch += rate * (y - g_mouseY);

		g_mouseX = x;
		g_mouseY = y;
	}
}

static void UpdateCamera()
{
	float speed = 0.5f;

	int leftShift = glfwGetKey(g_window, GLFW_KEY_LEFT_SHIFT);
	int rightShift = glfwGetKey(g_window, GLFW_KEY_RIGHT_SHIFT);
	int leftCtrl = glfwGetKey(g_window, GLFW_KEY_LEFT_CONTROL);
	int rightCtrl = glfwGetKey(g_window, GLFW_KEY_RIGHT_CONTROL);

	if (leftShift == GLFW_PRESS || rightShift == GLFW_PRESS)
	{
		speed *= 4.0f;
	}

	if (leftCtrl == GLFW_PRESS || rightCtrl == GLFW_PRESS)
	{
		speed *= 0.25f;
	}

	if (glfwGetKey(g_window, GLFW_KEY_W) == GLFW_PRESS)
	{
		g_camera.m_matrix.cw -= speed * g_camera.GetForward();
	}

	if (glfwGetKey(g_window, GLFW_KEY_S) == GLFW_PRESS)
	{
		g_camera.m_matrix.cw += speed * g_camera.GetForward();
	}

	if (glfwGetKey(g_window, GLFW_KEY_A) == GLFW_PRESS)
	{
		g_camera.m_matrix.cw -= speed * g_camera.GetRight();
	}

	if (glfwGetKey(g_window, GLFW_KEY_D) == GLFW_PRESS)
	{
		g_camera.m_matrix.cw += speed * g_camera.GetRight();
	}

	if (glfwGetKey(g_window, GLFW_KEY_Q) == GLFW_PRESS)
	{
		g_camera.m_matrix.cw += speed * g_camera.GetUp();
	}

	if (glfwGetKey(g_window, GLFW_KEY_E) == GLFW_PRESS)
	{
		g_camera.m_matrix.cw -= speed * g_camera.GetUp();
	}

	g_camera.Update();
}

static void DrawUI()
{
	if (g_showUI == false)
	{
		return;
	}

	float menuWidth = 200.0f;
	ImGui::SetNextWindowPos(ImVec2(g_camera.m_ws - menuWidth - 10.0f, 10.0f));
	ImGui::SetNextWindowSize(ImVec2(menuWidth, g_camera.m_hs - 20.0f));
	ImGui::Begin("Controls", &g_showUI, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);
	ImGui::PushAllowKeyboardFocus(false);

	bool changed = ImGui::Checkbox("Rotate", &g_rotate);
	if (changed)
	{
		InitTest(g_testIndex);
	}

	ImGui::Checkbox("Draw Internal", &g_drawInternal);

	ImGui::Separator();

	if (ImGui::Button("Quit"))
	{
		glfwSetWindowShouldClose(g_window, GL_TRUE);
	}

	ImGui::PopAllowKeyboardFocus();
	ImGui::End();
}

static void Resize(GLFWwindow*, int w, int h)
{
	g_camera.Resize(w, h);
}

int main(int, char**)
{
	GLenum glError;

	glfwSetErrorCallback(glfwErrorCallback);

	if (glfwInit() == 0)
	{
		fprintf(stderr, "Failed to initialize GLFW\n");
		return -1;
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	g_window = glfwCreateWindow(g_camera.m_ws, g_camera.m_hs, "dynamic-tree", NULL, NULL);
	if (g_window == NULL)
	{
		fprintf(stderr, "Failed to open GLFW g_window.\n");
		glfwTerminate();
		return -1;
	}

	glfwMakeContextCurrent(g_window);

	// Load OpenGL functions using glad
	if (gladLoadGLLoader((GLADloadproc)glfwGetProcAddress) == 0)
	{
		printf("Failed to initialize OpenGL context\n");
		return -1;
	}

	glfwSwapInterval(1);
	glfwSetWindowSizeCallback(g_window, Resize);
	glfwSetKeyCallback(g_window, Keyboard);
	glfwSetMouseButtonCallback(g_window, MouseClick);
	glfwSetCursorPosCallback(g_window, MouseMove);

	glError = glGetError();
	if (glError != GL_NO_ERROR)
	{
		assert(false);
	}

	float xscale, yscale;
	glfwGetWindowContentScale(g_window, &xscale, &yscale);
	float uiScale = xscale;

	bool success;
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui::StyleColorsClassic();

	success = ImGui_ImplGlfw_InitForOpenGL(g_window, true);
	if (success == false)
	{
		printf("ImGui_ImplGlfw_InitForOpenGL failed\n");
		assert(false);
	}

	success = ImGui_ImplOpenGL3_Init();
	if (success == false)
	{
		printf("ImGui_ImplOpenGL3_Init failed\n");
		assert(false);
	}

	ImGuiIO& io = ImGui::GetIO();
	io.FontGlobalScale = uiScale;

	g_draw.Create();

	glError = glGetError();
	if (glError != GL_NO_ERROR)
	{
		assert(false);
	}

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_DEPTH_TEST);
	glClearColor(0.3f, 0.3f, 0.3f, 1.0f);
	glViewport(0, 0, g_width, g_height);

	InitTestArray();
	InitTest(0);

	const int colorCount = 12;
	Color colors[colorCount] =
	{
		{0.3f, 0.3f, 0.8f, 1.0f},
		{0.8f, 0.3f, 0.3f, 1.0f},
		{0.3f, 0.8f, 0.3f, 1.0f},
		{0.8f, 0.8f, 0.3f, 1.0f},
		{0.3f, 0.8f, 0.8f, 1.0f},
		{0.8f, 0.3f, 0.8f, 1.0f},
		{0.3f, 0.6f, 0.8f, 1.0f},
		{0.6f, 0.3f, 0.8f, 1.0f},
		{0.8f, 0.6f, 0.3f, 1.0f},
		{0.8f, 0.3f, 0.6f, 1.0f},
		{0.6f, 0.8f, 0.3f, 1.0f},
		{0.3f, 0.8f, 0.6f, 1.0f}
	};

	while (glfwWindowShouldClose(g_window) == false)
	{
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glError = glGetError();
		if (glError != GL_NO_ERROR)
		{
			assert(false);
		}

		glfwGetFramebufferSize(g_window, &g_width, &g_height);
		glViewport(0, 0, g_width, g_height);

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		// Globally position text
		ImGui::SetNextWindowPos(ImVec2(10.0f, 10.0f));
		ImGui::Begin("Overlay", NULL, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar);
		ImGui::End();

		g_draw.DrawString(5, 5, "Test %d: %s", g_testIndex, g_test->GetName());

		char buffer[64];
		sprintf(buffer, "height %d, area ratio %g", g_test->m_tree.ComputeHeight(), g_test->m_tree.GetAreaRatio());
		g_draw.DrawString(5, 30, buffer);

		UpdateCamera();

		Color color(0.3f, 0.3f, 0.8f);
		for (int i = 0; i < g_test->m_tree.m_nodeCapacity; ++i)
		{
			dtNode& n = g_test->m_tree.m_nodes[i];
			if (n.height == -1)
			{
				continue;
			}

			if (n.isLeaf == false && g_drawInternal == false)
			{
				continue;
			}

			float extension = 0.1f * n.height;
			Color color = colors[n.height % colorCount];
			g_draw.DrawBox(n.aabb, extension, color);
		}

		g_draw.DrawPoint(dtVec_Zero, 10.0f, Color(1.0f, 0.0f, 0.0f));
		g_draw.DrawAxes();
		g_draw.Flush();

		DrawUI();

		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		glfwSwapBuffers(g_window);

		glError = glGetError();
		if (glError != GL_NO_ERROR)
		{
			assert(false);
		}

		glfwPollEvents();
	}

	g_draw.Destroy();
	glfwTerminate();
	return 0;
}
