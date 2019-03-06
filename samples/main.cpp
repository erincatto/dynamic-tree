/*
* Copyright (c) 2019 Erin Catto http://www.box2d.org
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

#define IMGUI_DISABLE_OBSOLETE_FUNCTIONS 1

#include "imgui/imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "dynamic-tree/tree.h"

#include "draw.h"
#include "settings.h"
#include "test.h"

#include <algorithm>

bool CompareTests(const Test* a, const Test* b)
{
	int result = strcmp(a->GetCategory(), b->GetCategory());
	if (result == 0)
	{
		result = strcmp(a->GetName(), b->GetName());
	}

	return result < 0;
}

namespace
{
	GLFWwindow* g_window = nullptr;

	Draw g_draw;
	Camera& g_camera = g_draw.m_camera;
	Settings g_settings;

	float g_mouseX = 0.0f;
	float g_mouseY = 0.0f;

	int g_width = 1280;
	int g_height = 720;

	Test* g_tests[128];
	int g_testCount = 0;
	Test* g_test = nullptr;

	int g_proxyCount = 0;
	int g_nodeCount = 0;
	int g_treeHeight = 0;
	int g_heapCount = 0;
	float g_treeArea = 0.0f;

	dtTreeHeuristic g_heuristic = dt_sah;
	bool g_rotate = true;
	bool g_drawInternal = true;

	int g_reinsertIter = 0;
	int g_shuffleIter = 0;

	float g_uiScale = 1.0f;
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
	extern Test* g_test4;

	g_tests[0] = g_test1;
	g_tests[1] = g_test2;
	g_tests[2] = g_test3;
	g_tests[3] = g_test4;
	std::sort(g_tests, g_tests + g_testCount, CompareTests);
	g_testCount = 4;
}

static void InitTest(int index)
{
	g_tests[g_settings.m_testIndex]->Destroy();

	g_settings.m_testIndex = dtClamp(index, 0, g_testCount - 1);

	g_test = g_tests[g_settings.m_testIndex];
	g_test->Create(g_heuristic, g_rotate);

	g_proxyCount = g_test->m_tree.GetProxyCount();
	g_nodeCount = g_test->m_tree.m_nodeCount;
	g_treeHeight = g_test->m_tree.GetHeight();
	g_heapCount = g_test->m_tree.m_maxHeapCount;
	g_treeArea = g_test->m_tree.GetAreaRatio();
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

	case GLFW_KEY_TAB:
		g_draw.m_showUI = !g_draw.m_showUI;
		break;

	case GLFW_KEY_LEFT_BRACKET:
		{
			int testIndex = dtMax(0, g_settings.m_testIndex - 1);
			InitTest(testIndex);
		}
		break;

	case GLFW_KEY_RIGHT_BRACKET:
		{
			int testIndex = dtMin(g_testCount - 1, g_settings.m_testIndex + 1);
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
	if (g_draw.m_showUI == false)
	{
		return;
	}

	float menuWidth = 150.0f;
	ImGui::SetNextWindowPos(ImVec2(g_camera.m_ws - (menuWidth + 10.0f) * g_uiScale, 10.0f * g_uiScale));
	ImGui::SetNextWindowSize(ImVec2(menuWidth * g_uiScale, g_camera.m_hs - 20.0f * g_uiScale));
	ImGui::Begin("##Controls", &g_draw.m_showUI, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar);

	ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_None;
	if (ImGui::BeginTabBar("ControlTabs", tab_bar_flags))
	{
		if (ImGui::BeginTabItem("Controls"))
		{
			bool rotate = ImGui::Checkbox("Rotate", &g_rotate);
			if (rotate)
			{
				InitTest(g_settings.m_testIndex);
			}

			ImGui::SliderInt("##Reinsert", &g_reinsertIter, 0, 100, "reinsert %d");
			ImGui::SliderInt("##Shuffle", &g_shuffleIter, 0, 100, "shuffle %d");

			static int heuristic = int(g_heuristic);
			ImGui::RadioButton("SAH", &heuristic, int(dt_sah));
			ImGui::RadioButton("Bittner", &heuristic, int(dt_bittner));
			ImGui::RadioButton("Box2D", &heuristic, int(dt_box2d));
			ImGui::RadioButton("Manhattan", &heuristic, int(dt_manhattan));
			if (heuristic != g_heuristic)
			{
				g_heuristic = dtTreeHeuristic(heuristic);
				InitTest(g_settings.m_testIndex);
			}

			ImGui::Separator();

			ImGui::Checkbox("Draw Internal", &g_drawInternal);
			if (ImGui::Button("Write Dot"))
			{
				g_test->m_tree.WriteDot("dot.txt");
			}

			if (ImGui::Button("Bottom Up"))
			{
				g_test->RebuildBottomUp();
				g_treeHeight = g_test->m_tree.GetHeight();
				g_treeArea = g_test->m_tree.GetAreaRatio();
			}

			if (ImGui::Button("Top Down SAH"))
			{
				g_test->RebuildTopDownSAH();
				g_treeHeight = g_test->m_tree.GetHeight();
				g_treeArea = g_test->m_tree.GetAreaRatio();
			}

			if (ImGui::Button("Top Down Median"))
			{
				g_test->RebuildTopDownMedian();
				g_treeHeight = g_test->m_tree.GetHeight();
				g_treeArea = g_test->m_tree.GetAreaRatio();
			}

			ImGui::Separator();

			if (ImGui::Button("Quit"))
			{
				glfwSetWindowShouldClose(g_window, GL_TRUE);
			}			ImGui::EndTabItem();
		}

		if (g_testCount == 0)
		{
			return;
		}

		ImGuiTreeNodeFlags leafNodeFlags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;
		leafNodeFlags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;

		ImGuiTreeNodeFlags nodeFlags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;

		if (ImGui::BeginTabItem("Tests"))
		{
			int categoryIndex = 0;
			const char* category = g_tests[categoryIndex]->GetCategory();
			int i = 0;
			while (i < g_testCount)
			{
				bool categorySelected = strcmp(category, g_tests[g_settings.m_testIndex]->GetCategory()) == 0;
				ImGuiTreeNodeFlags nodeSelectionFlags = categorySelected ? ImGuiTreeNodeFlags_Selected : 0;
				bool nodeOpen = ImGui::TreeNodeEx(category, nodeFlags | nodeSelectionFlags);

				if (nodeOpen)
				{
					while (i < g_testCount && strcmp(category, g_tests[i]->GetCategory()) == 0)
					{
						ImGuiTreeNodeFlags selectionFlags = 0;
						if (g_settings.m_testIndex == i)
						{
							selectionFlags = ImGuiTreeNodeFlags_Selected;
						}
						ImGui::TreeNodeEx((void*)(intptr_t)i, leafNodeFlags | selectionFlags, "%s", g_tests[i]->GetName());
						if (ImGui::IsItemClicked())
						{
							InitTest(i);
						}
						++i;
					}
					ImGui::TreePop();
				}
				else
				{
					while (i < g_testCount && strcmp(category, g_tests[i]->GetCategory()) == 0)
					{
						++i;
					}
				}

				if (i < g_testCount)
				{
					category = g_tests[i]->GetCategory();
					categoryIndex = i;
				}
			}
			ImGui::EndTabItem();
		}
		ImGui::EndTabBar();
	}

	ImGui::Separator();
	ImGui::End();
}

static void Resize(GLFWwindow*, int w, int h)
{
	g_camera.Resize(w, h);
}

int main(int, char**)
{
	g_settings.Load();

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
	g_uiScale = xscale;
	g_draw.m_uiScale = g_uiScale;

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

	const char* fontPath = "data/DroidSans.ttf";
	ImGuiIO& io = ImGui::GetIO();
	io.Fonts->AddFontFromFileTTF(fontPath, 13.f * g_uiScale);

	g_draw.Create();

	glError = glGetError();
	if (glError != GL_NO_ERROR)
	{
		assert(false);
	}

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_DEPTH_TEST);
	glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
	glViewport(0, 0, g_width, g_height);

	InitTestArray();
	InitTest(g_settings.m_testIndex);

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

		UpdateCamera();

		//InitTest(g_settings.m_testIndex);

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		// Globally position text
		if (g_draw.m_showUI)
		{
			ImGui::SetNextWindowPos(ImVec2(10.0f * g_uiScale, 10.0f * g_uiScale));
			ImGui::Begin("Overlay", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar);
			ImGui::End();

			g_draw.DrawString(5.0f, 5.0f, "%s", g_test->GetName());

			char buffer[64];
			sprintf(buffer, "proxies %d, nodes %d, height %d, heap %d, area %.2f", g_proxyCount, g_nodeCount, g_treeHeight, g_heapCount, g_treeArea);
			g_draw.DrawString(5.0f, 20.0f, buffer);

			const dtTree& tree = g_test->m_tree;
			sprintf(buffer, "BF %d, BF %d, CD %d, CE %d", tree.m_countBF, tree.m_countBG, tree.m_countCD, tree.m_countCE);
			g_draw.DrawString(5.0f, 35.0f, buffer);
		}

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

		g_draw.DrawAxes();

		g_test->Update(g_draw, g_reinsertIter, g_shuffleIter);

		g_draw.Flush();

		DrawUI();

		//ImGui::ShowDemoWindow();

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
	g_settings.Save();
	return 0;
}
