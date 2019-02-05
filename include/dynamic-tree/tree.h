/*
* Copyright (c) 2019 Erin Catto http://www.box2d.org
*
* This software is provided 'as-is', without any express or implied
* warranty.  In no event will the authors be held liable for any damages
* arising from the use of this software.
* Permission is granted to anyone to use this software for any purpose,
* including commercial applications, and to alter it and redistribute it
* freely, subject to the following restrictions:
* 1. The origin of this software must not be misrepresented; you must not
* claim that you wrote the original software. If you use this software
* in a product, an acknowledgment in the product documentation would be
* appreciated but is not required.
* 2. Altered source versions must be plainly marked as such, and must not be
* misrepresented as being the original software.
* 3. This notice may not be removed or altered from any source distribution.
*/

#pragma once

#include "dynamic-tree/utils.h"

#define dt_nullNode (-1)

/// A node in the dynamic tree. The client does not interact with this directly.
struct dtNode
{
	/// Enlarged AABB
	dtAABB aabb;

	union
	{
		int parent;
		int next;
	};

	int child1;
	int child2;

	// leaf = 0, free node = -1
	int height;

	bool isLeaf;
};

/// A dynamic AABB tree broad-phase, inspired by Nathanael Presson's btDbvt.
/// A dynamic tree arranges data in a binary tree to accelerate
/// queries such as volume queries and ray casts. Leafs are proxies
/// with an AABB. In the tree we expand the proxy AABB by b2_fatAABBFactor
/// so that the proxy AABB is bigger than the client object. This allows the client
/// object to move by small amounts without triggering a tree update.
///
/// Nodes are pooled and relocatable, so we use node indices rather than pointers.
struct dtTree
{
	/// Constructing the tree initializes the node pool.
	dtTree();

	/// Destroy the tree, freeing the node pool.
	~dtTree();

	void Clear();

	/// Create a proxy. Provide a tight fitting AABB and a userData pointer.
	int CreateProxy(const dtAABB& aabb, bool rotate);

	/// Destroy a proxy. This asserts if the id is invalid.
	void DestroyProxy(int proxyId);

	/// Move a proxy with a swepted AABB.
	/// @return true if the proxy was re-inserted.
	bool MoveProxy(int proxyId, const dtVec& displacement);

	/// Get the fat AABB for a proxy.
	const dtAABB& GetAABB(int proxyId) const;

	/// Validate this tree. For testing.
	void Validate() const;

	/// Compute the height of the binary tree in O(N) time. Should not be
	/// called often.
	int GetHeight() const;

	/// Get the maximum balance of an node in the tree. The balance is the difference
	/// in height of the two children of a node.
	int GetMaxBalance() const;

	/// Get the ratio of the sum of the node areas to the root area.
	float GetAreaRatio() const;

	/// Build an optimal tree. Very expensive. For testing.
	void RebuildBottomUp();

	void WriteDot() const;

	int AllocateNode();
	void FreeNode(int node);

	void InsertLeaf(int node, bool rotate);
	void RemoveLeaf(int node);

	void Rotate(int index);

	int ComputeHeight() const;
	int ComputeHeight(int nodeId) const;

	void ValidateStructure(int index) const;
	void ValidateMetrics(int index) const;

	int m_root;

	dtNode* m_nodes;
	int m_nodeCount;
	int m_nodeCapacity;

	int m_freeList;

	/// This is used to incrementally traverse the tree for re-balancing.
	unsigned m_path;

	int m_insertionCount;
};
