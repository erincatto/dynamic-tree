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
#include "test.h"
#include "draw.h"
#include "imgui/imgui.h"
#include <stdio.h>
#include <vector>

struct Test3 : Test
{
	enum
	{
		e_fileCount = 8
	};

	Test3()
	{
		m_fileIndex = 1;
		m_fileNames[0] = "BlizzardLand";
		m_fileNames[1] = "BlizzardLandEditor";
		m_fileNames[2] = "Gibraltar";
		m_fileNames[3] = "Himalayas";
		m_fileNames[4] = "Mexico";
		m_fileNames[5] = "BlizzardLandDynamic";
		m_fileNames[6] = "BlizzardLandKinematic";
		m_fileNames[7] = "BlizzardLandStatic";
	}

	const char* GetCategory() const
	{
		return "Benchmark";
	}

	const char* GetName() const override
	{
		return "Overwatch Map";
	}

	void CreateBoxes() override
	{
		const char* fileName = m_fileNames[m_fileIndex];

		char filePath[128];
		sprintf_s(filePath, "data/%s.txt", fileName);

		int vertexCount = 0;

		FILE* file = fopen(filePath, "r");
		if (file == nullptr)
		{
			return;
		}

		const int k_bufferSize = 256;
		char buffer[k_bufferSize];

		while (fgets(buffer, k_bufferSize, file))
		{
			switch (buffer[0])
			{
			case 'v':
				++vertexCount;
				break;
			}
		}

		rewind(file);

		if (vertexCount == 0)
		{
			fclose(file);
			return;
		}

		std::vector<dtVec> vertices(vertexCount);

		int index = 0;
		while (fgets(buffer, k_bufferSize, file))
		{
			float x, y, z;

			switch (buffer[0])
			{
			case 'v':
				sscanf(buffer, "v %f %f %f", &x, &y, &z);
				vertices[index] = dtVecSet(x, y, z);
				++index;
				break;
			}
		}

		fclose(file);

		Allocate(vertexCount / 2);

		for (int i = 0; i < m_count; ++i)
		{
			m_boxes[i].lowerBound = vertices[2 * i + 0];
			m_boxes[i].upperBound = vertices[2 * i + 1];
		}

		//m_count = 100;
	}

	void Update(Draw& draw, int reinsertIter, int shuffleIter) override
	{
		if (draw.m_showUI == false)
		{
			Test::Update(draw, reinsertIter, shuffleIter);
			return;
		}

		float scale = draw.m_uiScale;
		float menuWidth = 150.0f;
		ImGui::SetNextWindowPos(ImVec2(10.0f * scale, 300.0f * scale));
		ImGui::SetNextWindowSize(ImVec2(250.0f * scale, 100.0f * scale));
		ImGui::Begin("Test Params", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize);

		if (ImGui::Combo("File", &m_fileIndex, m_fileNames, e_fileCount))
		{
			dtInsertionHeuristic heuristic = m_tree.m_heuristic;
			Destroy();
			Create(heuristic);
		}

		ImGui::End();

		Test::Update(draw, reinsertIter, shuffleIter);
	}

	const char* m_fileNames[e_fileCount];
	int m_fileIndex;
};

static Test3 s_test;
Test* g_test3 = &s_test;