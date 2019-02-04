#pragma once

#include "dynamic-tree/tree.h"

struct Test
{
	virtual void Create() = 0;

	dtTree m_tree;
};
