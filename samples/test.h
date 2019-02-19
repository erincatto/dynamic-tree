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

#pragma once

#include "dynamic-tree/tree.h"

struct Draw;

struct Test
{
	virtual const char* GetCategory() const = 0;
	virtual const char* GetName() const = 0;
	virtual void Create(dtTreeHeuristic heuristic, bool rotate) = 0;
	virtual void Destroy() = 0;
	virtual void Update(Draw&, int /* reinsertIter */, int /* shuffleIter */) {}

	dtTree m_tree;
};
