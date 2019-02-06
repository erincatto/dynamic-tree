#define _CRT_SECURE_NO_WARNINGS
#include "test.h"
#include <stdio.h>
#include <stdlib.h>

struct Test3 : Test
{
	const char* GetName() const override
	{
		return "Overwatch Map";
	}

	void Load(const char* fileName)
	{
		m_vertices = nullptr;
		m_vertexCount = 0;

		FILE* file = fopen(fileName, "r");
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
				++m_vertexCount;
				break;
			}
		}

		rewind(file);

		m_vertices = (dtVec*)malloc(m_vertexCount * sizeof(dtVec));

		int index = 0;
		while (fgets(buffer, k_bufferSize, file))
		{
			dtVec* v = m_vertices + index;
			float x, y, z;

			switch (buffer[0])
			{
			case 'v':
				sscanf(buffer, "v %f %f %f", &x, &y, &z);
				m_vertices[index] = dtVecSet(x, y, z);
				++index;
				break;
			}
		}

		fclose(file);
	}

	void Create(dtTreeHeuristic heuristic, bool rotate) override
	{
		m_tree.m_heuristic = heuristic;
		Load("data/tree03.txt");

		int aabbCount = m_vertexCount / 2;
		for (int i = 0; i < aabbCount; ++i)
		{
			dtAABB box;
			box.lowerBound = m_vertices[2 * i + 0];
			box.upperBound = m_vertices[2 * i + 1];
			m_tree.CreateProxy(box, rotate);
		}
	}

	void Destroy() override
	{
		m_tree.Clear();
		free(m_vertices);
		m_vertices = nullptr;
		m_vertexCount = 0;
	}

	dtVec* m_vertices;
	int m_vertexCount;
};

static Test3 s_test;
Test* g_test3 = &s_test;