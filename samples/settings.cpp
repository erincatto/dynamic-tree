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
#include "settings.h"
#include "sajson/sajson.h"
#include <stdio.h>

static const char* fileName = "settings.ini";
static const char* testIndexKey = "testIndex";

// Load a file. You must free the character array.
static bool sReadFile(char*& data, int& size, const char* filename)
{
	FILE* file = fopen(filename, "rb");
	if (file == nullptr)
	{
		return false;
	}

	fseek(file, 0, SEEK_END);
	size = ftell(file);
	fseek(file, 0, SEEK_SET);

	if (size == 0)
	{
		return false;
	}

	data = (char*)malloc(size + 1);
	fread(data, size, 1, file);
	fclose(file);
	data[size] = 0;

	return true;
}

void Settings::Save()
{
	FILE* file = fopen(fileName, "w");
	fprintf(file, "{\n");
	fprintf(file, "  \"%s\": %d\n", testIndexKey, m_testIndex);
	fprintf(file, "}\n");
	fclose(file);
}

void Settings::Load()
{
	char* data = nullptr;
	int size = 0;
	bool found = sReadFile(data, size, fileName);
	if (found ==  false)
	{
		return;
	}

	const sajson::document& document = sajson::parse(sajson::dynamic_allocation(), sajson::mutable_string_view(size, data));

	sajson::value root = document.get_root();

	if (root.get_type() == sajson::TYPE_OBJECT)
	{
		size_t index = root.find_object_key(sajson::string(testIndexKey, strlen(testIndexKey)));
		if (index < root.get_length())
		{
			sajson::value testIndex = root.get_object_value(index);
			if (testIndex.get_type() == sajson::TYPE_INTEGER)
			{
				m_testIndex = testIndex.get_integer_value();
			}
		}
	}

	free(data);
}
