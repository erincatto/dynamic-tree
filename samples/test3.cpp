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

#include <stdio.h>
#include <vector>

struct Test3 : Test
{
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
		const char* fileName = "data/BlizzardLandEditorTree.txt";

		int vertexCount = 0;

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

		m_count = 1000;
	}
};

static Test3 s_test;
Test* g_test3 = &s_test;