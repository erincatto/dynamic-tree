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
#include "dynamic-tree/tree.h"
#include <string.h>
#include <algorithm>
#include <assert.h>

#define DT_VALIDATE 0

dtTree::dtTree()
{
	m_root = dt_nullNode;
	m_nodeCapacity = 16;
	m_nodeCount = 0;
	m_proxyCount = 0;

	m_nodes = (dtNode*)malloc(m_nodeCapacity * sizeof(dtNode));
	memset(m_nodes, 0, m_nodeCapacity * sizeof(dtNode));

	// Build a linked list for the free list.
	for (int i = 0; i < m_nodeCapacity - 1; ++i)
	{
		m_nodes[i].next = i + 1;
		m_nodes[i].height = dt_nullNode;
	}

	m_nodes[m_nodeCapacity-1].next = dt_nullNode;
	m_nodes[m_nodeCapacity-1].height = dt_nullNode;

	m_freeList = 0;

	m_countBF = 0;
	m_countBG = 0;
	m_countCD = 0;
	m_countCE = 0;

	m_path = 0;
	m_insertionCount = 0;
	m_heap.reserve(128);
	m_maxHeapCount = 0;

	m_heuristic = dt_sah;
}

dtTree::~dtTree()
{
	// This frees the entire tree in one shot.
	free(m_nodes);
}

//
void dtTree::Clear()
{
	m_root = dt_nullNode;
	m_nodeCount = 0;
	m_proxyCount = 0;

	for (int i = 0; i < m_nodeCapacity - 1; ++i)
	{
		m_nodes[i].next = i + 1;
		m_nodes[i].height = dt_nullNode;
	}

	m_nodes[m_nodeCapacity - 1].next = dt_nullNode;
	m_nodes[m_nodeCapacity - 1].height = dt_nullNode;

	m_freeList = 0;
	m_path = 0;
	m_insertionCount = 0;

	m_countBF = 0;
	m_countBG = 0;
	m_countCD = 0;
	m_countCE = 0;

	m_maxHeapCount = 0;
}

//
const dtAABB& dtTree::GetAABB(int proxyId) const
{
	assert(0 <= proxyId && proxyId < m_nodeCapacity);
	return m_nodes[proxyId].aabb;
}

// Allocate a node from the pool. Grow the pool if necessary.
int dtTree::AllocateNode()
{
	// Expand the node pool as needed.
	if (m_freeList == dt_nullNode)
	{
		assert(m_nodeCount == m_nodeCapacity);

		// The free list is empty. Rebuild a bigger pool.
		dtNode* oldNodes = m_nodes;
		m_nodeCapacity *= 2;
		m_nodes = (dtNode*)malloc(m_nodeCapacity * sizeof(dtNode));
		memcpy(m_nodes, oldNodes, m_nodeCount * sizeof(dtNode));
		free(oldNodes);

		// Build a linked list for the free list. The parent
		// pointer becomes the "next" pointer.
		for (int i = m_nodeCount; i < m_nodeCapacity - 1; ++i)
		{
			m_nodes[i].next = i + 1;
			m_nodes[i].height = dt_nullNode;
		}
		m_nodes[m_nodeCapacity-1].next = dt_nullNode;
		m_nodes[m_nodeCapacity-1].height = dt_nullNode;
		m_freeList = m_nodeCount;
	}

	// Peel a node off the free list.
	int nodeId = m_freeList;
	m_freeList = m_nodes[nodeId].next;
	m_nodes[nodeId].objectIndex = -1;
	m_nodes[nodeId].parent = dt_nullNode;
	m_nodes[nodeId].child1 = dt_nullNode;
	m_nodes[nodeId].child2 = dt_nullNode;
	m_nodes[nodeId].height = 0;
	m_nodes[nodeId].isLeaf = false;
	++m_nodeCount;
	return nodeId;
}

// Return a node to the pool.
void dtTree::FreeNode(int nodeId)
{
	assert(0 <= nodeId && nodeId < m_nodeCapacity);
	assert(0 < m_nodeCount);
	m_nodes[nodeId].next = m_freeList;
	m_nodes[nodeId].height = dt_nullNode;
	m_freeList = nodeId;
	--m_nodeCount;
}

// Create a proxy in the tree as a leaf node. We return the index
// of the node instead of a pointer so that we can grow
// the node pool.
int dtTree::CreateProxy(const dtAABB& aabb, int objectIndex)
{
	int proxyId = AllocateNode();

	m_nodes[proxyId].aabb.lowerBound = aabb.lowerBound;
	m_nodes[proxyId].aabb.upperBound = aabb.upperBound;
	m_nodes[proxyId].height = 0;
	m_nodes[proxyId].objectIndex = objectIndex;
	m_nodes[proxyId].isLeaf = true;

	InsertLeaf(proxyId);

	++m_proxyCount;

	return proxyId;
}

//
void dtTree::DestroyProxy(int proxyId)
{
	assert(0 <= proxyId && proxyId < m_nodeCapacity);
	assert(m_nodes[proxyId].isLeaf);

	RemoveLeaf(proxyId);
	FreeNode(proxyId);

	--m_proxyCount;
}

static inline bool operator < (const dtCandidateNode& a, const dtCandidateNode& b)
{
	return a.inheritanceCost > b.inheritanceCost;
}

// Insert using branch and bound. Push children without consideration.
void dtTree::InsertLeafBittner(int leaf)
{
	++m_insertionCount;

	if (m_root == dt_nullNode)
	{
		m_root = leaf;
		m_nodes[m_root].parent = dt_nullNode;
		return;
	}

	dtAABB aabbL = m_nodes[leaf].aabb;
	float areaL = dtArea(aabbL);

	// Stage 1: find the best sibling for this node
	dtCandidateNode candidate;
	candidate.index = m_root;
	candidate.inheritanceCost = 0.0f;
	m_heap.clear();
	m_heap.push_back(candidate);

	float bestCost = FLT_MAX;
	int bestSibling = m_root;

	while (m_heap.size() > 0)
	{
		std::pop_heap(m_heap.begin(), m_heap.end());
		candidate = m_heap.back();
		m_heap.pop_back();

		int index = candidate.index;
		float inheritanceCost = candidate.inheritanceCost;
		if (inheritanceCost + areaL > bestCost)
		{
			// Optimum found
			break;
		}

		const dtNode& node = m_nodes[index];
		float directCost = dtArea(dtUnion(node.aabb, aabbL));
		float totalCost = inheritanceCost + directCost;

		if (totalCost <= bestCost)
		{
			bestCost = totalCost;
			bestSibling = index;
		}

		if (node.isLeaf)
		{
			continue;
		}

		inheritanceCost += directCost - dtArea(node.aabb);
		float lowerBoundCost = inheritanceCost + areaL;
		if (lowerBoundCost <= bestCost)
		{
			dtCandidateNode candidate1;
			candidate1.index = node.child1;
			candidate1.inheritanceCost = inheritanceCost;

			m_heap.push_back(candidate1);
			std::push_heap(m_heap.begin(), m_heap.end());

			dtCandidateNode candidate2;
			candidate2.index = node.child2;
			candidate2.inheritanceCost = inheritanceCost;

			m_heap.push_back(candidate2);
			std::push_heap(m_heap.begin(), m_heap.end());
		}
	}

	m_maxHeapCount = dtMax(m_maxHeapCount, int(m_heap.size()));

	//printf("%d %g\n", bestSibling, bestCost);

#if 0
	// Compare with brute force
	// Passed on BlizzardLand
	float bestCost2 = FLT_MAX;
	int bestSibling2 = dt_nullNode;
	for (int i = 0; i < m_nodeCount; ++i)
	{
		if (i == leaf)
		{
			continue;
		}

		const dtNode& node = m_nodes[i];
		if (node.height == dt_nullNode)
		{
			continue;
		}

		float cost = dtArea(dtUnion(aabbL, node.aabb));
		int parentIndex = node.parent;
		while (parentIndex != dt_nullNode)
		{
			const dtNode& parent = m_nodes[parentIndex];
			cost += dtArea(dtUnion(aabbL, parent.aabb)) - dtArea(parent.aabb);
			parentIndex = parent.parent;
		}

		if (cost < bestCost2)
		{
			bestCost2 = cost; 
			bestSibling2 = i;
		}
	}

	if (dtAbs(bestCost2 - bestCost) > 0.0001f + 0.0001f * dtAbs(bestCost))
	{
		bestCost2 += 0.0f;
	}
#endif

	int sibling = bestSibling;

	// Stage 2: create a new parent
	int oldParent = m_nodes[sibling].parent;
	int newParent = AllocateNode();
	m_nodes[newParent].parent = oldParent;
	m_nodes[newParent].aabb = dtUnion(aabbL, m_nodes[sibling].aabb);
	m_nodes[newParent].height = m_nodes[sibling].height + 1;

	if (oldParent != dt_nullNode)
	{
		// The sibling was not the root.
		if (m_nodes[oldParent].child1 == sibling)
		{
			m_nodes[oldParent].child1 = newParent;
		}
		else
		{
			m_nodes[oldParent].child2 = newParent;
		}

		m_nodes[newParent].child1 = sibling;
		m_nodes[newParent].child2 = leaf;
		m_nodes[sibling].parent = newParent;
		m_nodes[leaf].parent = newParent;
	}
	else
	{
		// The sibling was the root.
		m_nodes[newParent].child1 = sibling;
		m_nodes[newParent].child2 = leaf;
		m_nodes[sibling].parent = newParent;
		m_nodes[leaf].parent = newParent;
		m_root = newParent;
	}

	// Stage 3: walk back up the tree fixing heights and AABBs
	int index = m_nodes[leaf].parent;
	while (index != dt_nullNode)
	{
		int child1 = m_nodes[index].child1;
		int child2 = m_nodes[index].child2;

		assert(child1 != dt_nullNode);
		assert(child2 != dt_nullNode);

		m_nodes[index].height = 1 + dtMax(m_nodes[child1].height, m_nodes[child2].height);
		m_nodes[index].aabb = dtUnion(m_nodes[child1].aabb, m_nodes[child2].aabb);

		index = m_nodes[index].parent;
	}

	Validate();
}

// Insert using branch and bound. Consider children before pushing.
void dtTree::InsertLeafSAH(int leaf)
{
	++m_insertionCount;

	if (m_root == dt_nullNode)
	{
		m_root = leaf;
		m_nodes[m_root].parent = dt_nullNode;
		return;
	}

	dtAABB aabbL = m_nodes[leaf].aabb;
	float areaL = dtArea(aabbL);

	// Stage 1: find the best sibling for this node
	dtCandidateNode candidate;
	candidate.index = m_root;
	
	int bestSibling = m_root;
	float bestCost;
	{
		const dtNode& node = m_nodes[m_root];
		bestCost = dtArea(dtUnion(node.aabb, aabbL));
		candidate.inheritanceCost = bestCost - dtArea(node.aabb);
	}

	m_heap.clear();
	m_heap.push_back(candidate);

	while (m_heap.size() > 0)
	{
		std::pop_heap(m_heap.begin(), m_heap.end());
		candidate = m_heap.back();
		m_heap.pop_back();

		int index = candidate.index;
		float lowerBoundCost = candidate.inheritanceCost + areaL;
		if (lowerBoundCost > bestCost)
		{
			// Optimum found
			break;
		}

		const dtNode& node = m_nodes[index];
		if (node.isLeaf)
		{
			continue;
		}

		// Child push order doesn't matter since the heap will sort them.

		{
			const dtNode& child1 = m_nodes[node.child1];
			float directCost = dtArea(dtUnion(child1.aabb, aabbL));
			float totalCost = directCost + candidate.inheritanceCost;
			if (totalCost <= bestCost)
			{
				bestCost = totalCost;
				bestSibling = node.child1;
			}

			float inheritanceCost = totalCost - dtArea(child1.aabb);
			if (inheritanceCost + areaL <= bestCost)
			{
				dtCandidateNode candidate1;
				candidate1.index = node.child1;
				candidate1.inheritanceCost = inheritanceCost;
				m_heap.push_back(candidate1);
				std::push_heap(m_heap.begin(), m_heap.end());
			}
		}

		{
			const dtNode& child2 = m_nodes[node.child2];
			float directCost = dtArea(dtUnion(child2.aabb, aabbL));
			float totalCost = directCost + candidate.inheritanceCost;
			if (totalCost <= bestCost)
			{
				bestCost = totalCost;
				bestSibling = node.child2;
			}

			float inheritanceCost = totalCost - dtArea(child2.aabb);
			if (inheritanceCost + areaL <= bestCost)
			{
				dtCandidateNode candidate2;
				candidate2.index = node.child2;
				candidate2.inheritanceCost = inheritanceCost;
				m_heap.push_back(candidate2);
				std::push_heap(m_heap.begin(), m_heap.end());
			}
		}
	}

	m_maxHeapCount = dtMax(m_maxHeapCount, int(m_heap.size()));

	//printf("%d %g\n", bestSibling, bestCost);

#if 0
	// Compare with brute force
	// Passed on BlizzardLand
	float bestCost2 = FLT_MAX;
	int bestSibling2 = dt_nullNode;
	for (int i = 0; i < m_nodeCount; ++i)
	{
		if (i == leaf)
		{
			continue;
		}

		const dtNode& node = m_nodes[i];
		if (node.height == dt_nullNode)
		{
			continue;
		}

		float cost = dtArea(dtUnion(aabbL, node.aabb));
		int parentIndex = node.parent;
		while (parentIndex != dt_nullNode)
		{
			const dtNode& parent = m_nodes[parentIndex];
			cost += dtArea(dtUnion(aabbL, parent.aabb)) - dtArea(parent.aabb);
			parentIndex = parent.parent;
		}

		if (cost < bestCost2)
		{
			bestCost2 = cost;
			bestSibling2 = i;
		}
	}

	if (dtAbs(bestCost2 - bestCost) > 0.0001f + 0.0001f * dtAbs(bestCost))
	{
		bestCost2 += 0.0f;
	}
#endif

	int sibling = bestSibling;

	// Stage 2: create a new parent
	int oldParent = m_nodes[sibling].parent;
	int newParent = AllocateNode();
	m_nodes[newParent].parent = oldParent;
	m_nodes[newParent].aabb = dtUnion(aabbL, m_nodes[sibling].aabb);
	m_nodes[newParent].height = m_nodes[sibling].height + 1;

	if (oldParent != dt_nullNode)
	{
		// The sibling was not the root.
		if (m_nodes[oldParent].child1 == sibling)
		{
			m_nodes[oldParent].child1 = newParent;
		}
		else
		{
			m_nodes[oldParent].child2 = newParent;
		}

		m_nodes[newParent].child1 = sibling;
		m_nodes[newParent].child2 = leaf;
		m_nodes[sibling].parent = newParent;
		m_nodes[leaf].parent = newParent;
	}
	else
	{
		// The sibling was the root.
		m_nodes[newParent].child1 = sibling;
		m_nodes[newParent].child2 = leaf;
		m_nodes[sibling].parent = newParent;
		m_nodes[leaf].parent = newParent;
		m_root = newParent;
	}

	// Stage 3: walk back up the tree fixing heights and AABBs
	int index = oldParent;
	while (index != dt_nullNode)
	{
		int child1 = m_nodes[index].child1;
		int child2 = m_nodes[index].child2;

		assert(child1 != dt_nullNode);
		assert(child2 != dt_nullNode);

		m_nodes[index].height = 1 + dtMax(m_nodes[child1].height, m_nodes[child2].height);
		m_nodes[index].aabb = dtUnion(m_nodes[child1].aabb, m_nodes[child2].aabb);

		if (m_heuristic == dt_sah_rotate)
		{
			Rotate(index);
		}

		index = m_nodes[index].parent;
	}

	Validate();
}

float dtTree::SiblingCost(const dtAABB& aabbL, int sibling)
{
	assert(0 <= sibling && sibling < m_nodeCapacity);
	const dtNode& node = m_nodes[sibling];

	float directCost = dtArea(dtUnion(node.aabb, aabbL));
	float cost = directCost;

	int parent = node.parent;
	while (parent != dt_nullNode)
	{
		const dtNode& n = m_nodes[parent];
		cost += dtArea(dtUnion(n.aabb, aabbL)) - dtArea(n.aabb);
		parent = n.parent;
	}

	return cost;
}

int g_earlyOut = 0;
int g_sameCount = 0;

//
int dtTree::SiblingApproxSAH(const dtAABB& aabbL, std::vector<int>& path, float& cost)
{
	dtVec centerL = dtCenter(aabbL);

	float areaBase = dtArea(m_nodes[m_root].aabb);
	float directCost = dtArea(dtUnion(m_nodes[m_root].aabb, aabbL));
	float inheritedCost = 0.0f;

	int bestSibling = m_root;
	float bestCost = directCost;

	float areaL = dtArea(aabbL);

	int index = m_root;
	while (m_nodes[index].isLeaf == false)
	{
		int child1 = m_nodes[index].child1;
		int child2 = m_nodes[index].child2;

		// Cost of creating a new parent for this node P and the new leaf L
		float Cp = directCost + inheritedCost;
		if (Cp <= bestCost)
		{
			bestSibling = index;
			bestCost = Cp;
		}

		// Inheritance cost seen by children
		inheritedCost += directCost - areaBase;

		// Cost of descending into child 1
		float lowerC1 = FLT_MAX;
		float directCost1;
		float area1 = 0.0f;
		if (m_nodes[child1].isLeaf)
		{
			// Child 1 is a leaf
			// Cost of creating new node and increasing area of node P
			directCost1 = dtArea(dtUnion(m_nodes[child1].aabb, aabbL));
			float C1 = directCost1 + inheritedCost;

			// Need this here due to while condition above
			if (C1 <= bestCost)
			{
				bestSibling = child1;
				bestCost = C1;
			}
		}
		else
		{
			// Child 1 is an internal node
			area1 = dtArea(m_nodes[child1].aabb);
			directCost1 = dtArea(dtUnion(m_nodes[child1].aabb, aabbL));
			float deltaArea1 = directCost1 - area1;

			// Lower bound cost of inserting under child 1. This is not the cost
			// of child 1, it is the best we can hope for under child 1.
			lowerC1 = areaL + inheritedCost + deltaArea1;
		}

		// Cost of descending into child 2
		float lowerC2 = FLT_MAX;
		float directCost2;
		float area2 = 0.0f;
		if (m_nodes[child2].isLeaf)
		{
			// Child 2 is a leaf
			// Cost of creating new node and increasing area of node P
			directCost2 = dtArea(dtUnion(m_nodes[child2].aabb, aabbL));
			float C2 = directCost2 + inheritedCost;

			// Need this here due to while condition above
			if (C2 <= bestCost)
			{
				bestSibling = child2;
				bestCost = C2;
			}
		}
		else
		{
			// Child 2 is an internal node
			area2 = dtArea(m_nodes[child2].aabb);
			directCost2 = dtArea(dtUnion(m_nodes[child2].aabb, aabbL));
			float deltaArea2 = directCost2 - area2;

			// Lower bound cost of inserting under child 2. This is not the cost
			// of child 2, it is the best we can hope for under child 2.
			lowerC2 = areaL + inheritedCost + deltaArea2;
		}

		// Can the cost possibly be decreased?
		if (bestCost <= lowerC1 && bestCost <= lowerC2)
		{
			// Found best sibling
			++g_earlyOut;
			break;
		}

		if (lowerC1 == lowerC2)
		{
			// No clear choice based on lower bound surface area. This can happen when both
			// children fully contain L. Fallback to node distance.
			dtVec d1 = dtCenter(m_nodes[child1].aabb) - centerL;
			dtVec d2 = dtCenter(m_nodes[child2].aabb) - centerL;
			lowerC1 = dtGetX(dtDot3(d1, d1));
			lowerC2 = dtGetX(dtDot3(d2, d2));
			++g_sameCount;
		}

		// Descend
		if (lowerC1 < lowerC2 && m_nodes[child1].isLeaf == false)
		{
			index = child1;
			areaBase = area1;
			directCost = directCost1;
		}
		else
		{
			index = child2;
			areaBase = area2;
			directCost = directCost2;
		}

		path.push_back(index);
	}

	//float cost = SiblingCost(aabbL, bestSibling);
	//if (cost != bestCost)
	//{
	//	cost += 0.0f;
	//}

	cost = bestCost;
	return bestSibling;
}

int dtTree::SiblingApproxOmohundro(const dtAABB& aabbL, std::vector<int>& path, float& cost)
{
	float directCost = dtArea(dtUnion(m_nodes[m_root].aabb, aabbL));
	float inheritedCost = 0.0f;

	int bestSibling = m_root;
	float bestCost = directCost;

	float areaL = dtArea(aabbL);
	dtVec centerL = dtCenter(aabbL);

	int index = m_root;
	while (m_nodes[index].isLeaf == false)
	{
		const dtNode& n = m_nodes[index];
		inheritedCost += directCost - dtArea(n.aabb);

		// modification: add areaL
		if (inheritedCost + areaL >= bestCost)
		{
			break;
		}

		const dtNode& child1 = m_nodes[n.child1];
		const dtNode& child2 = m_nodes[n.child2];

		float directCost1 = dtArea(dtUnion(child1.aabb, aabbL));
		float directCost2 = dtArea(dtUnion(child2.aabb, aabbL));

		if (inheritedCost + directCost1 < bestCost)
		{
			bestSibling = n.child1;
			bestCost = inheritedCost + directCost1;
		}

		if (inheritedCost + directCost2 < bestCost)
		{
			bestSibling = n.child2;
			bestCost = inheritedCost + directCost2;
		}

		float delta1 = directCost1 - dtArea(child1.aabb);
		float delta2 = directCost2 - dtArea(child2.aabb);

		// modification: deal with indecision
		//if (delta1 == 0.0f && delta2 == 0.0f)
		//{
		//	dtVec d1 = dtCenter(child1.aabb) - centerL;
		//	dtVec d2 = dtCenter(child2.aabb) - centerL;
		//	delta1 = dtGetX(dtDot3(d1, d1));
		//	delta2 = dtGetX(dtDot3(d2, d2));
		//}

		// modification: check for leaf
		if (delta1 <= delta2 && child1.isLeaf == false)
		{
			directCost = directCost1;
			index = n.child1;
		}
		else
		{
			directCost = directCost2;
			index = n.child2;
		}

		path.push_back(index);
	}

	cost = bestCost;
	return bestSibling;
}

void dtTree::InsertLeafApproxSAH(int leaf)
{
	++m_insertionCount;

	if (m_root == dt_nullNode)
	{
		m_root = leaf;
		m_nodes[m_root].parent = dt_nullNode;
		return;
	}

	dtAABB aabbL = m_nodes[leaf].aabb;
	std::vector<int> path1, path2;
	float cost1, cost2;
	int sibling2 = SiblingApproxSAH(aabbL, path1, cost1);
	int sibling = SiblingApproxOmohundro(aabbL, path2, cost2);

	if (sibling != sibling2)
	{
		sibling += 0;
	}

	// Stage 2: create a new parent
	int oldParent = m_nodes[sibling].parent;
	int newParent = AllocateNode();
	m_nodes[newParent].parent = oldParent;
	m_nodes[newParent].aabb = dtUnion(aabbL, m_nodes[sibling].aabb);
	m_nodes[newParent].height = m_nodes[sibling].height + 1;

	if (oldParent != dt_nullNode)
	{
		// The sibling was not the root.
		if (m_nodes[oldParent].child1 == sibling)
		{
			m_nodes[oldParent].child1 = newParent;
		}
		else
		{
			m_nodes[oldParent].child2 = newParent;
		}

		m_nodes[newParent].child1 = sibling;
		m_nodes[newParent].child2 = leaf;
		m_nodes[sibling].parent = newParent;
		m_nodes[leaf].parent = newParent;
	}
	else
	{
		// The sibling was the root.
		m_nodes[newParent].child1 = sibling;
		m_nodes[newParent].child2 = leaf;
		m_nodes[sibling].parent = newParent;
		m_nodes[leaf].parent = newParent;
		m_root = newParent;
	}

	// Stage 3: walk back up the tree fixing heights and AABBs
	int index = m_nodes[leaf].parent;
	while (index != dt_nullNode)
	{
		int child1 = m_nodes[index].child1;
		int child2 = m_nodes[index].child2;

		assert(child1 != dt_nullNode);
		assert(child2 != dt_nullNode);

		m_nodes[index].height = 1 + dtMax(m_nodes[child1].height, m_nodes[child2].height);
		m_nodes[index].aabb = dtUnion(m_nodes[child1].aabb, m_nodes[child2].aabb);

		if (m_heuristic == dt_approx_sah_rotate)
		{
			Rotate(index);
		}

		index = m_nodes[index].parent;
	}

	Validate();
}


#if 0
void dtTree::InsertLeafApproxSAH(int leaf)
{
	++m_insertionCount;

	if (m_root == dt_nullNode)
	{
		m_root = leaf;
		m_nodes[m_root].parent = dt_nullNode;
		return;
	}

	dtAABB aabbL = m_nodes[leaf].aabb;
	float areaL = dtArea(aabbL);
	dtVec centerL = dtCenter(aabbL);

	float directCost = dtArea(dtUnion(m_nodes[m_root].aabb, aabbL));
	float inheritedCost = directCost - dtArea(m_nodes[m_root].aabb);

	int bestSibling = m_root;
	float bestCost = directCost;

	// Stage 1: find the best sibling for this node
	int index = m_root;
	while (m_nodes[index].isLeaf == false)
	{
		int child1 = m_nodes[index].child1;
		int child2 = m_nodes[index].child2;

		float ic = 0.0f;
		int i = index;
		while (i != dt_nullNode)
		{
			const dtAABB& box = m_nodes[i].aabb;
			ic += dtArea(dtUnion(box, aabbL)) - dtArea(box);
			i = m_nodes[i].parent;
		}

		if (dtAbs(ic - inheritedCost) > 0.001f)
		{
			ic += 0.0f;
		}

		// Cost of child 1
		float directCost1 = dtArea(dtUnion(m_nodes[child1].aabb, aabbL));
		float C1 = directCost1 + inheritedCost;
		if (C1 < bestCost)
		{
			bestSibling = child1;
			bestCost = C1;
		}

		// Cost of child 2
		float directCost2 = dtArea(dtUnion(m_nodes[child2].aabb, aabbL));
		float C2 = directCost2 + inheritedCost;
		if (C2 < bestCost)
		{
			bestSibling = child2;
			bestCost = C2;
		}

		float deltaArea1 = directCost1 - dtArea(m_nodes[child1].aabb);
		float deltaArea2 = directCost2 - dtArea(m_nodes[child2].aabb);
		//float lowerC1 = areaL + inheritedCost + deltaArea1;
		//float lowerC2 = areaL + inheritedCost + deltaArea2;

		//// Can the cost possibly be decreased?
		//if (bestCost < lowerC1 && bestCost < lowerC2)
		//{
		//	// Found best sibling
		//	break;
		//}

		// If both children grow by the same amount, then another heuristic is needed to decide
		// on the best path.
		//if (dtAbs(deltaArea1 - deltaArea2) < 0.001f * areaL)
		//{
		//	dtVec d1 = dtCenter(m_nodes[child1].aabb) - centerL;
		//	dtVec d2 = dtCenter(m_nodes[child2].aabb) - centerL;
		//	float distSqr1 = dtGetX(dtDot3(d1, d1));
		//	float distSqr2 = dtGetX(dtDot3(d2, d2));
		//	++g_sameCount;

		//	if (distSqr1 < distSqr2)
		//	{
		//		inheritedCost += deltaArea1;
		//		index = child1;
		//	}
		//	else
		//	{
		//		inheritedCost += deltaArea2;
		//		index = child2;
		//	}
		//}
		//else
			
			if (C1 < C2)
		{
			inheritedCost += deltaArea1;
			index = child1;
		}
		else
		{
			inheritedCost += deltaArea2;
			index = child2;
		}
	}

	int sibling = bestSibling;

	// Stage 2: create a new parent
	int oldParent = m_nodes[sibling].parent;
	int newParent = AllocateNode();
	m_nodes[newParent].parent = oldParent;
	m_nodes[newParent].aabb = dtUnion(aabbL, m_nodes[sibling].aabb);
	m_nodes[newParent].height = m_nodes[sibling].height + 1;

	if (oldParent != dt_nullNode)
	{
		// The sibling was not the root.
		if (m_nodes[oldParent].child1 == sibling)
		{
			m_nodes[oldParent].child1 = newParent;
		}
		else
		{
			m_nodes[oldParent].child2 = newParent;
		}

		m_nodes[newParent].child1 = sibling;
		m_nodes[newParent].child2 = leaf;
		m_nodes[sibling].parent = newParent;
		m_nodes[leaf].parent = newParent;
	}
	else
	{
		// The sibling was the root.
		m_nodes[newParent].child1 = sibling;
		m_nodes[newParent].child2 = leaf;
		m_nodes[sibling].parent = newParent;
		m_nodes[leaf].parent = newParent;
		m_root = newParent;
	}

	// Stage 3: walk back up the tree fixing heights and AABBs
	index = m_nodes[leaf].parent;
	while (index != dt_nullNode)
	{
		int child1 = m_nodes[index].child1;
		int child2 = m_nodes[index].child2;

		assert(child1 != dt_nullNode);
		assert(child2 != dt_nullNode);

		m_nodes[index].height = 1 + dtMax(m_nodes[child1].height, m_nodes[child2].height);
		m_nodes[index].aabb = dtUnion(m_nodes[child1].aabb, m_nodes[child2].aabb);

		if (m_heuristic == dt_approx_sah_rotate)
		{
			Rotate(index);
		}

		index = m_nodes[index].parent;
	}

	Validate();
}

#endif

// 
static inline float dtManhattan(const dtAABB& a, const dtAABB& b)
{
	dtVec d = (a.lowerBound + a.upperBound) - (b.lowerBound + b.upperBound);
	dtVec absD = dtAbs(d);
	return dtGetX(absD) + dtGetY(absD) + dtGetZ(absD);
}

//
void dtTree::InsertLeafManhattan(int leaf)
{
	++m_insertionCount;

	if (m_root == dt_nullNode)
	{
		m_root = leaf;
		m_nodes[m_root].parent = dt_nullNode;
		return;
	}

	dtAABB aabbL = m_nodes[leaf].aabb;

	// Stage 1: find the best sibling for this node
	int index = m_root;
	while (m_nodes[index].isLeaf == false)
	{
		int child1 = m_nodes[index].child1;
		int child2 = m_nodes[index].child2;

		// Manhattan distance heuristic from Presson
		float C1 = dtManhattan(aabbL, m_nodes[child1].aabb);
		float C2 = dtManhattan(aabbL, m_nodes[child2].aabb);

		// Descend
		if (C1 < C2)
		{
			index = child1;
		}
		else
		{
			index = child2;
		}
	}

	int sibling = index;

	// Stage 2: create a new parent
	int oldParent = m_nodes[sibling].parent;
	int newParent = AllocateNode();
	m_nodes[newParent].parent = oldParent;
	m_nodes[newParent].aabb = dtUnion(aabbL, m_nodes[sibling].aabb);
	m_nodes[newParent].height = m_nodes[sibling].height + 1;

	if (oldParent != dt_nullNode)
	{
		// The sibling was not the root.
		if (m_nodes[oldParent].child1 == sibling)
		{
			m_nodes[oldParent].child1 = newParent;
		}
		else
		{
			m_nodes[oldParent].child2 = newParent;
		}

		m_nodes[newParent].child1 = sibling;
		m_nodes[newParent].child2 = leaf;
		m_nodes[sibling].parent = newParent;
		m_nodes[leaf].parent = newParent;
	}
	else
	{
		// The sibling was the root.
		m_nodes[newParent].child1 = sibling;
		m_nodes[newParent].child2 = leaf;
		m_nodes[sibling].parent = newParent;
		m_nodes[leaf].parent = newParent;
		m_root = newParent;
	}

	// Stage 3: walk back up the tree fixing heights and AABBs
	index = m_nodes[leaf].parent;
	while (index != dt_nullNode)
	{
		int child1 = m_nodes[index].child1;
		int child2 = m_nodes[index].child2;

		assert(child1 != dt_nullNode);
		assert(child2 != dt_nullNode);

		m_nodes[index].height = 1 + dtMax(m_nodes[child1].height, m_nodes[child2].height);
		m_nodes[index].aabb = dtUnion(m_nodes[child1].aabb, m_nodes[child2].aabb);

		index = m_nodes[index].parent;
	}

	Validate();
}

void dtTree::InsertLeaf(int leaf)
{
	switch (m_heuristic)
	{
	case dt_sah:
	case dt_sah_rotate:
		InsertLeafSAH(leaf);
		return;

	case dt_bittner:
		InsertLeafBittner(leaf);
		return;

	case dt_approx_sah:
	case dt_approx_sah_rotate:
		InsertLeafApproxSAH(leaf);
		return;

	case dt_manhattan:
		InsertLeafManhattan(leaf);
		return;
	}
}

void dtTree::RemoveLeaf(int leaf)
{
	if (leaf == m_root)
	{
		m_root = dt_nullNode;
		return;
	}

	int parent = m_nodes[leaf].parent;
	int grandParent = m_nodes[parent].parent;
	int sibling;
	if (m_nodes[parent].child1 == leaf)
	{
		sibling = m_nodes[parent].child2;
	}
	else
	{
		sibling = m_nodes[parent].child1;
	}

	if (grandParent != dt_nullNode)
	{
		// Destroy parent and connect sibling to grandParent.
		if (m_nodes[grandParent].child1 == parent)
		{
			m_nodes[grandParent].child1 = sibling;
		}
		else
		{
			m_nodes[grandParent].child2 = sibling;
		}
		m_nodes[sibling].parent = grandParent;
		FreeNode(parent);

		// Adjust ancestor bounds.
		int index = grandParent;
		while (index != dt_nullNode)
		{
			int child1 = m_nodes[index].child1;
			int child2 = m_nodes[index].child2;

			assert(child1 != dt_nullNode);
			assert(child2 != dt_nullNode);

			m_nodes[index].aabb = dtUnion(m_nodes[child1].aabb, m_nodes[child2].aabb);
			m_nodes[index].height = 1 + dtMax(m_nodes[child1].height, m_nodes[child2].height);

			index = m_nodes[index].parent;
		}
	}
	else
	{
		m_root = sibling;
		m_nodes[sibling].parent = dt_nullNode;
		FreeNode(parent);
	}

	Validate();
}

enum dtTreeRotate
{
	dt_rotateNone,
	dt_rotateBF,
	dt_rotateBG,
	dt_rotateCD,
	dt_rotateCE
};

// Perform a left or right rotation if node A is imbalanced.
// Returns the new root index.
void dtTree::Rotate(int iA)
{
	assert(iA != dt_nullNode);

	dtNode* A = m_nodes + iA;
	if (A->height < 2)
	{
		return;
	}

	int iB = A->child1;
	int iC = A->child2;
	assert(0 <= iB && iB < m_nodeCapacity);
	assert(0 <= iC && iC < m_nodeCapacity);

	dtNode* B = m_nodes + iB;
	dtNode* C = m_nodes + iC;

	if (B->height == 0)
	{
		// B is a leaf
		assert(C->height > 0);

		int iF = C->child1;
		int iG = C->child2;
		dtNode* F = m_nodes + iF;
		dtNode* G = m_nodes + iG;
		assert(0 <= iF && iF < m_nodeCapacity);
		assert(0 <= iG && iG < m_nodeCapacity);

		// Base cost
		float costBase = dtArea(C->aabb);

		// Cost of swapping B and F
		dtAABB aabbBG = dtUnion(B->aabb, G->aabb);
		float costBF = dtArea(aabbBG);

		// Cost of swapping B and G
		dtAABB aabbBF = dtUnion(B->aabb, F->aabb);
		float costBG = dtArea(aabbBF);

		if (costBase < costBF && costBase < costBG)
		{
			// Rotation does not improve cost
			return;
		}

		if (costBF < costBG)
		{
			// Swap B and F
			A->child1 = iF;
			C->child1 = iB;

			B->parent = iC;
			F->parent = iA;

			C->aabb = aabbBG;
			C->height = 1 + dtMax(B->height, G->height);
			A->height = 1 + dtMax(C->height, F->height);

			++m_countBF;
		}
		else
		{
			// Swap B and G
			A->child1 = iG;
			C->child2 = iB;

			B->parent = iC;
			G->parent = iA;

			C->aabb = aabbBF;
			C->height = 1 + dtMax(B->height, F->height);
			A->height = 1 + dtMax(C->height, G->height);

			++m_countBG;
		}
	}
	else if (C->height == 0)
	{
		// C is a leaf
		assert(B->height > 0);

		int iD = B->child1;
		int iE = B->child2;
		dtNode* D = m_nodes + iD;
		dtNode* E = m_nodes + iE;
		assert(0 <= iD && iD < m_nodeCapacity);
		assert(0 <= iE && iE < m_nodeCapacity);

		// Base cost
		float costBase = dtArea(B->aabb);

		// Cost of swapping C and D
		dtAABB aabbCE = dtUnion(C->aabb, E->aabb);
		float costCD = dtArea(aabbCE);

		// Cost of swapping C and E
		dtAABB aabbCD = dtUnion(C->aabb, D->aabb);
		float costCE = dtArea(aabbCD);

		if (costBase < costCD && costBase < costCE)
		{
			// Rotation does not improve cost
			return;
		}

		if (costCD < costCE)
		{
			// Swap C and D
			A->child2 = iD;
			B->child1 = iC;

			C->parent = iB;
			D->parent = iA;

			B->aabb = aabbCE;
			B->height = 1 + dtMax(C->height, E->height);
			A->height = 1 + dtMax(B->height, D->height);

			++m_countCD;
		}
		else
		{
			// Swap C and E
			A->child2 = iE;
			B->child2 = iC;

			C->parent = iB;
			E->parent = iA;

			B->aabb = aabbCD;
			B->height = 1 + dtMax(C->height, D->height);
			A->height = 1 + dtMax(B->height, E->height);

			++m_countCE;
		}
	}
	else
	{
		int iD = B->child1;
		int iE = B->child2;
		int iF = C->child1;
		int iG = C->child2;

		dtNode* D = m_nodes + iD;
		dtNode* E = m_nodes + iE;
		dtNode* F = m_nodes + iF;
		dtNode* G = m_nodes + iG;

		assert(0 <= iD && iD < m_nodeCapacity);
		assert(0 <= iE && iE < m_nodeCapacity);
		assert(0 <= iF && iF < m_nodeCapacity);
		assert(0 <= iG && iG < m_nodeCapacity);

		// Base cost
		float areaB = dtArea(B->aabb);
		float areaC = dtArea(C->aabb);
		float costBase = areaB + areaC;
		dtTreeRotate bestRotation = dt_rotateNone;
		float bestCost = costBase;

		// Cost of swapping B and F
		dtAABB aabbBG = dtUnion(B->aabb, G->aabb);
		float costBF = areaB + dtArea(aabbBG);
		if (costBF < bestCost)
		{
			bestRotation = dt_rotateBF;
			bestCost = costBF;
		}

		// Cost of swapping B and G
		dtAABB aabbBF = dtUnion(B->aabb, F->aabb);
		float costBG = areaB + dtArea(aabbBF);
		if (costBG < bestCost)
		{
			bestRotation = dt_rotateBG;
			bestCost = costBG;
		}

		// Cost of swapping C and D
		dtAABB aabbCE = dtUnion(C->aabb, E->aabb);
		float costCD = areaC + dtArea(aabbCE);
		if (costCD < bestCost)
		{
			bestRotation = dt_rotateCD;
			bestCost = costCD;
		}

		// Cost of swapping C and E
		dtAABB aabbCD = dtUnion(C->aabb, D->aabb);
		float costCE = areaC + dtArea(aabbCD);
		if (costCE < bestCost)
		{
			bestRotation = dt_rotateCE;
			bestCost = costCE;
		}

		switch (bestRotation)
		{
		case dt_rotateNone:
			break;

		case dt_rotateBF:
			A->child1 = iF;
			C->child1 = iB;

			B->parent = iC;
			F->parent = iA;

			C->aabb = aabbBG;
			C->height = 1 + dtMax(B->height, G->height);
			A->height = 1 + dtMax(C->height, F->height);

			++m_countBF;
			break;

		case dt_rotateBG:
			A->child1 = iG;
			C->child2 = iB;

			B->parent = iC;
			G->parent = iA;

			C->aabb = aabbBF;
			C->height = 1 + dtMax(B->height, F->height);
			A->height = 1 + dtMax(C->height, G->height);

			++m_countBG;
			break;

		case dt_rotateCD:
			A->child2 = iD;
			B->child1 = iC;

			C->parent = iB;
			D->parent = iA;

			B->aabb = aabbCE;
			B->height = 1 + dtMax(C->height, E->height);
			A->height = 1 + dtMax(B->height, D->height);

			++m_countCD;
			break;

		case dt_rotateCE:
			A->child2 = iE;
			B->child2 = iC;

			C->parent = iB;
			E->parent = iA;

			B->aabb = aabbCD;
			B->height = 1 + dtMax(C->height, D->height);
			A->height = 1 + dtMax(B->height, E->height);

			++m_countCE;
			break;

		default:
			assert(false);
			break;
		}
	}
}

void dtTree::Shuffle(int index)
{
	dtNode& A = m_nodes[index];
	assert(A.child1 != dt_nullNode && A.child2 != dt_nullNode && A.isLeaf == false);

	dtNode& B = m_nodes[A.child1];
	dtNode& C = m_nodes[A.child2];

	if (B.isLeaf || C.isLeaf)
	{
		return;
	}

	assert(B.child1 != dt_nullNode && B.child2 != dt_nullNode);
	assert(C.child1 != dt_nullNode && C.child2 != dt_nullNode);

	dtNode& D = m_nodes[B.child1];
	dtNode& E = m_nodes[B.child2];
	dtNode& F = m_nodes[C.child1];
	dtNode& G = m_nodes[C.child2];

	float costBase = dtArea(B.aabb) + dtArea(C.aabb);

	dtAABB DF = dtUnion(D.aabb, F.aabb);
	dtAABB DG = dtUnion(D.aabb, G.aabb);
	dtAABB EF = dtUnion(E.aabb, F.aabb);
	dtAABB EG = dtUnion(E.aabb, G.aabb);

	float costDF = dtArea(DF) + dtArea(EG);
	float costDG = dtArea(DG) + dtArea(EF);

	if (costDF > costBase && costDG > costBase)
	{
		return;
	}

	if (costDF < costDG)
	{
		dtSwap(B.child2, C.child1);
		F.parent = A.child1;
		E.parent = A.child2;
		B.aabb = DF;
		C.aabb = EG;
		B.height = 1 + dtMax(D.height, F.height);
		C.height = 1 + dtMax(E.height, G.height);
	}
	else
	{
		dtSwap(B.child2, C.child2);
		G.parent = A.child1;
		E.parent = A.child2;
		B.aabb = DG;
		C.aabb = EF;
		B.height = 1 + dtMax(D.height, G.height);
		C.height = 1 + dtMax(E.height, F.height);
	}
}

void dtTree::Optimize(int iterations)
{
	for (int i = 0; i < iterations; ++i)
	{
		if (m_path > m_nodeCapacity)
		{
			m_path = 0;
		}

		while (m_nodes[m_path].height == dt_nullNode || m_nodes[m_path].height < 2)
		{
			++m_path;
			if (m_path > m_nodeCapacity)
			{
				m_path = 0;
			}
		}

		Shuffle(m_path);

		++m_path;
	}
}

int dtTree::GetProxyCount() const
{
	return m_proxyCount;
}

int dtTree::GetHeight() const
{
	if (m_root == dt_nullNode)
	{
		return 0;
	}

	return m_nodes[m_root].height;
}

//
float dtTree::GetAreaRatio() const
{
	if (m_root == dt_nullNode)
	{
		return 0.0f;
	}

	const dtNode* root = m_nodes + m_root;
	float rootArea = dtArea(root->aabb);

	float totalArea = 0.0f;
	for (int i = 0; i < m_nodeCapacity; ++i)
	{
		const dtNode* node = m_nodes + i;
		if (node->height < 0 || node->isLeaf || i == m_root)
		{
			continue;
		}

		totalArea += dtArea(node->aabb);
	}

	return totalArea / rootArea;
}

//
float dtTree::GetArea() const
{
	if (m_root == dt_nullNode)
	{
		return 0.0f;
	}

	float area = 0.0f;
	for (int i = 0; i < m_nodeCapacity; ++i)
	{
		const dtNode* node = m_nodes + i;
		if (node->height < 0 || node->isLeaf || i == m_root)
		{
			continue;
		}

		area += dtArea(node->aabb);
	}

	return area;
}

// Compute the height of a sub-tree.
int dtTree::ComputeHeight(int nodeId) const
{
	assert(0 <= nodeId && nodeId < m_nodeCapacity);
	dtNode* node = m_nodes + nodeId;

	if (node->isLeaf)
	{
		return 0;
	}

	int height1 = ComputeHeight(node->child1);
	int height2 = ComputeHeight(node->child2);
	return 1 + dtMax(height1, height2);
}

int dtTree::ComputeHeight() const
{
	int height = ComputeHeight(m_root);
	return height;
}

void dtTree::ValidateStructure(int index) const
{
	if (index == dt_nullNode)
	{
		return;
	}

	if (index == m_root)
	{
		assert(m_nodes[index].parent == dt_nullNode);
	}

	const dtNode* node = m_nodes + index;

	int child1 = node->child1;
	int child2 = node->child2;

	if (node->isLeaf)
	{
		assert(child1 == dt_nullNode);
		assert(child2 == dt_nullNode);
		assert(node->height == 0);
		return;
	}

	assert(0 <= child1 && child1 < m_nodeCapacity);
	assert(0 <= child2 && child2 < m_nodeCapacity);

	assert(m_nodes[child1].parent == index);
	assert(m_nodes[child2].parent == index);

	ValidateStructure(child1);
	ValidateStructure(child2);
}

void dtTree::ValidateMetrics(int index) const
{
	if (index == dt_nullNode)
	{
		return;
	}

	const dtNode* node = m_nodes + index;

	int child1 = node->child1;
	int child2 = node->child2;

	if (node->isLeaf)
	{
		assert(child1 == dt_nullNode);
		assert(child2 == dt_nullNode);
		assert(node->height == 0);
		return;
	}

	assert(0 <= child1 && child1 < m_nodeCapacity);
	assert(0 <= child2 && child2 < m_nodeCapacity);

	int height1 = m_nodes[child1].height;
	int height2 = m_nodes[child2].height;
	int height;
	height = 1 + dtMax(height1, height2);
	assert(node->height == height);

	dtAABB aabb = dtUnion(m_nodes[child1].aabb, m_nodes[child2].aabb);

	assert(aabb.lowerBound == node->aabb.lowerBound);
	assert(aabb.upperBound == node->aabb.upperBound);

	ValidateMetrics(child1);
	ValidateMetrics(child2);
}

void dtTree::Validate() const
{
#if DT_VALIDATE == 1
	ValidateStructure(m_root);
	ValidateMetrics(m_root);

	int freeCount = 0;
	int freeIndex = m_freeList;
	while (freeIndex != dt_nullNode)
	{
		assert(0 <= freeIndex && freeIndex < m_nodeCapacity);
		freeIndex = m_nodes[freeIndex].next;
		++freeCount;
	}

	assert(GetHeight() == ComputeHeight());

	assert(m_nodeCount + freeCount == m_nodeCapacity);
#endif
}

int dtTree::GetMaxBalance() const
{
	int maxBalance = 0;
	for (int i = 0; i < m_nodeCapacity; ++i)
	{
		const dtNode* node = m_nodes + i;
		if (node->height <= 1)
		{
			continue;
		}

		assert(node->isLeaf == false);

		int child1 = node->child1;
		int child2 = node->child2;
		int balance = dtAbs(m_nodes[child2].height - m_nodes[child1].height);
		maxBalance = dtMax(maxBalance, balance);
	}

	return maxBalance;
}

void dtTree::RebuildBottomUp()
{
	int* nodes = (int*)malloc(m_nodeCount * sizeof(int));
	int count = 0;

	// Build array of leaves. Free the rest.
	for (int i = 0; i < m_nodeCapacity; ++i)
	{
		if (m_nodes[i].height < 0)
		{
			// node unused
			continue;
		}

		if (m_nodes[i].isLeaf)
		{
			m_nodes[i].parent = dt_nullNode;
			nodes[count] = i;
			++count;
		}
		else
		{
			FreeNode(i);
		}
	}

	while (count > 1)
	{
		float minCost = FLT_MAX;
		int iMin = -1, jMin = -1;
		for (int i = 0; i < count; ++i)
		{
			dtAABB aabbi = m_nodes[nodes[i]].aabb;

			for (int j = i + 1; j < count; ++j)
			{
				dtAABB aabbj = m_nodes[nodes[j]].aabb;
				dtAABB b = dtUnion(aabbi, aabbj);
				float cost = dtArea(b);
				if (cost < minCost)
				{
					iMin = i;
					jMin = j;
					minCost = cost;
				}
			}
		}

		int index1 = nodes[iMin];
		int index2 = nodes[jMin];
		dtNode* child1 = m_nodes + index1;
		dtNode* child2 = m_nodes + index2;

		int parentIndex = AllocateNode();
		dtNode* parent = m_nodes + parentIndex;
		parent->child1 = index1;
		parent->child2 = index2;
		parent->height = 1 + dtMax(child1->height, child2->height);
		parent->aabb = dtUnion(child1->aabb, child2->aabb);
		parent->parent = dt_nullNode;

		child1->parent = parentIndex;
		child2->parent = parentIndex;

		nodes[jMin] = nodes[count-1];
		nodes[iMin] = parentIndex;
		--count;
	}

	m_root = nodes[0];
	free(nodes);

	Validate();
}

void dtTree::WriteDot(const char* fileName) const
{
	FILE* file = fopen(fileName, "w");
	if (file == nullptr)
	{
		return;
	}

	float areaRatio = GetAreaRatio();

	if (m_proxyCount < 26)
	{
		fprintf(file, "graph\n");
		fprintf(file, "{\n");

		for (int i = 0; i < m_nodeCapacity; ++i)
		{
			if (m_nodes[i].height == -1)
			{
				continue;
			}

			if (m_nodes[i].isLeaf)
			{
				fprintf(file, "%d [shape=box, width=0.25, height=0.25, label=\"%c\"]\n", i, 'A' + m_nodes[i].objectIndex);
			}
			else
			{
				fprintf(file, "%d [shape=circle, width=0.25, height=0.25, label=\"\"]\n", i);
			}
		}

		for (int i = 0; i < m_nodeCapacity; ++i)
		{
			if (m_nodes[i].height == -1)
			{
				continue;
			}

			if (m_nodes[i].isLeaf)
			{
				continue;
			}

			fprintf(file, "%d -- %d\n", i, m_nodes[i].child1);
			fprintf(file, "%d -- %d\n", i, m_nodes[i].child2);
		}

		fprintf(file, "%d [shape=box, fontsize=10, label=\"area ratio = %.2f\"]\n", m_nodeCapacity, areaRatio);
		fprintf(file, "}\n");
	}
	else
	{
		fprintf(file, "graph\n");
		fprintf(file, "{\n");
		for (int i = 0; i < m_nodeCapacity; ++i)
		{
			if (m_nodes[i].height == -1)
			{
				continue;
			}

			if (m_nodes[i].isLeaf)
			{
				fprintf(file, "%d [shape=point]\n", i);
			}
			else
			{
				float area = dtArea(m_nodes[i].aabb);
				fprintf(file, "%d [shape=circle, label=\"%.f\"]\n", i, area);
			}
		}

		for (int i = 0; i < m_nodeCapacity; ++i)
		{
			if (m_nodes[i].height == -1)
			{
				continue;
			}

			if (m_nodes[i].isLeaf)
			{
				continue;
			}

			fprintf(file, "%d -- %d\n", i, m_nodes[i].child1);
			fprintf(file, "%d -- %d\n", i, m_nodes[i].child2);
		}

		fprintf(file, "%d [shape=box, fontsize=10, label=\"area ratio = %.f\"]\n", m_nodeCapacity, areaRatio);
		fprintf(file, "}\n");
	}

	fclose(file);
}

#define dt_binCount 64

struct dtTreeBin
{
	dtAABB aabb;
	int count;
};

struct dtTreePlane
{
	dtAABB leftAABB;
	dtAABB rightAABB;
	int leftCount;
	int rightCount;
};

void dtTree::BuildTopDownSAH(int* proxies, dtAABB* boxes, int count)
{
	free(m_nodes);

	m_freeList = dt_nullNode;
	m_nodeCapacity = 2 * count - 1;
	m_nodes = (dtNode*)malloc(m_nodeCapacity * sizeof(dtNode));
	memset(m_nodes, 0, m_nodeCapacity * sizeof(dtNode));

	m_nodeCount = count;
	for (int i = 0; i < count; ++i)
	{
		m_nodes[i].aabb = boxes[i];
		// Use child1 to store the proxy index
		m_nodes[i].child1 = i;
		m_nodes[i].child2 = dt_nullNode;
		m_nodes[i].height = 0;
		m_nodes[i].isLeaf = true;
		m_nodes[i].next = -1;
		m_nodes[i].parent = dt_nullNode;
	}

	dtTreeBin bins[dt_binCount];
	dtTreePlane planes[dt_binCount - 1];
	m_root = BinSortBoxes(dt_nullNode, m_nodes, count, bins, planes);

	assert(m_nodeCount == m_nodeCapacity);

	for (int i = 0; i < m_nodeCount; ++i)
	{
		dtNode& n = m_nodes[i];
		if (n.isLeaf)
		{
			assert(0 <= n.child1 && n.child1 < count);
			proxies[n.child1] = i;
			n.child1 = dt_nullNode;
		}
	}

	Validate();
}

// "On Fast Construction of SAH-based Bounding Volume Hierarchies" by Ingo Wald
int dtTree::BinSortBoxes(int parentIndex, dtNode* leaves, int count, dtTreeBin* bins, dtTreePlane* planes)
{
	if (count == 1)
	{
		leaves[0].parent = parentIndex;
		return int(leaves - m_nodes);
	}

	dtVec center = dtCenter(leaves[0].aabb);
	dtAABB centroidAABB;
	centroidAABB.lowerBound = center;
	centroidAABB.upperBound = center;

	for (int i = 1; i < count; ++i)
	{
		center = dtCenter(leaves[i].aabb);
		centroidAABB.lowerBound = dtMin(centroidAABB.lowerBound, center);
		centroidAABB.upperBound = dtMax(centroidAABB.upperBound, center);
	}

	dtVec d = centroidAABB.upperBound - centroidAABB.lowerBound;

	int axisIndex;
	float invD;
	if (dtGetX(d) > dtGetY(d) && dtGetX(d) > dtGetZ(d))
	{
		axisIndex = 0;
		invD = dtGetX(d);
	}
	else if (dtGetY(d) > dtGetZ(d))
	{
		axisIndex = 1;
		invD = dtGetY(d);
	}
	else
	{
		axisIndex = 2;
		invD = dtGetZ(d);
	}

	invD = invD > 0.0f ? 1.0f / invD : 0.0f;

	for (int i = 0; i < dt_binCount; ++i)
	{
		bins[i].aabb.lowerBound = dtSplat(FLT_MAX);
		bins[i].aabb.upperBound = dtSplat(-FLT_MAX);
		bins[i].count = 0;
	}

	float binCount = float(dt_binCount);
	float minC = dtGet(centroidAABB.lowerBound, axisIndex);
	for (int i = 0; i < count; ++i)
	{
		dtVec c = dtCenter(leaves[i].aabb);
		int binIndex = int(binCount * (dtGet(c, axisIndex) - minC) * invD);
		binIndex = dtClamp(binIndex, 0, dt_binCount - 1);
		leaves[i].next = binIndex;
		bins[binIndex].count += 1;
		bins[binIndex].aabb = dtUnion(bins[binIndex].aabb, leaves[i].aabb);
	}

	int planeCount = dt_binCount - 1;

	planes[0].leftCount = bins[0].count;
	planes[0].leftAABB = bins[0].aabb;
	for (int i = 1; i < planeCount; ++i)
	{
		planes[i].leftCount = planes[i - 1].leftCount + bins[i].count;
		planes[i].leftAABB = dtUnion(planes[i - 1].leftAABB, bins[i].aabb);
	}

	planes[planeCount - 1].rightCount = bins[planeCount].count;
	planes[planeCount - 1].rightAABB = bins[planeCount].aabb;
	for (int i = planeCount - 2; i >= 0; --i)
	{
		planes[i].rightCount = planes[i + 1].rightCount + bins[i + 1].count;
		planes[i].rightAABB = dtUnion(planes[i + 1].rightAABB, bins[i + 1].aabb);
	}

	float minCost = FLT_MAX;
	int bestPlane = 0;
	for (int i = 0; i < planeCount; ++i)
	{
		float leftArea = dtArea(planes[i].leftAABB);
		float rightArea = dtArea(planes[i].rightAABB);
		int leftCount = planes[i].leftCount;
		int rightCount = planes[i].rightCount;

		float cost = leftCount * leftArea + rightCount * rightArea;
		if (cost < minCost)
		{
			bestPlane = i;
			minCost = cost;
		}
	}

	assert(m_nodeCount < m_nodeCapacity);
	int nodeIndex = m_nodeCount++;
	dtNode& node = m_nodes[nodeIndex];
	node.aabb = dtUnion(planes[bestPlane].leftAABB, planes[bestPlane].rightAABB);
	node.parent = parentIndex;
	node.isLeaf = false;

	int i1 = -1;
	for (int i2 = 0; i2 < count; ++i2)
	{
		int binIndex = leaves[i2].next;
		if (binIndex <= bestPlane)
		{
			++i1;
			dtSwap(leaves[i1], leaves[i2]);
		}
	}

	int leftCount = i1 + 1;
	int rightCount = count - leftCount;

	if (leftCount == 0)
	{
		leftCount = 1;
		rightCount -= 1;
	}
	else if (rightCount == 0)
	{
		leftCount -= 1;
		rightCount = 1;
	}

	node.child1 = BinSortBoxes(nodeIndex, leaves, leftCount, bins, planes);
	node.child2 = BinSortBoxes(nodeIndex, leaves + leftCount, rightCount, bins, planes);

	const dtNode& child1 = m_nodes[node.child1];
	const dtNode& child2 = m_nodes[node.child2];

	node.height = 1 + dtMax(child1.height, child2.height);

	return nodeIndex;
}

void dtTree::BuildTopDownMedianSplit(int* proxies, dtAABB* boxes, int count)
{
	free(m_nodes);

	m_freeList = dt_nullNode;
	m_nodeCapacity = 2 * count - 1;
	m_nodes = (dtNode*)malloc(m_nodeCapacity * sizeof(dtNode));
	memset(m_nodes, 0, m_nodeCapacity * sizeof(dtNode));

	m_nodeCount = count;
	for (int i = 0; i < count; ++i)
	{
		m_nodes[i].aabb = boxes[i];
		m_nodes[i].child1 = dt_nullNode;
		m_nodes[i].child2 = dt_nullNode;
		m_nodes[i].height = 0;
		m_nodes[i].isLeaf = true;
		m_nodes[i].objectIndex = i;
		m_nodes[i].next = -1;
		m_nodes[i].parent = dt_nullNode;
	}

	m_root = PartitionBoxes(dt_nullNode, m_nodes, count);

	assert(m_nodeCount == m_nodeCapacity);

	for (int i = 0; i < m_nodeCount; ++i)
	{
		dtNode& n = m_nodes[i];
		if (n.isLeaf)
		{
			assert(0 <= n.objectIndex && n.objectIndex < count);
			proxies[n.objectIndex] = i;
		}
	}

	Validate();
}

int dtTree::PartitionBoxes(int parentIndex, dtNode* leaves, int count)
{
	if (count == 1)
	{
		leaves[0].parent = parentIndex;
		return int(leaves - m_nodes);
	}

	dtVec mean = dtCenter(leaves[0].aabb);
	dtAABB aabb = leaves[0].aabb;
	for (int i = 1; i < count; ++i)
	{
		aabb = dtUnion(aabb, leaves[i].aabb);
		mean += dtCenter(leaves[i].aabb);
	}

	mean = dtSplat(1.0f / count) * mean;

	dtVec variance = dtVec_Zero;
	for (int i = 0; i < count; ++i)
	{
		dtVec diff = dtCenter(leaves[i].aabb) - mean;
		variance = variance + diff * diff;
	}

	dtVec c = dtCenter(aabb);
	float ca;
	int axisIndex;
	if (dtGetX(variance) > dtGetY(variance) && dtGetX(variance) > dtGetZ(variance))
	{
		ca = dtGetX(c);
		axisIndex = 0;
	}
	else if (dtGetY(variance) > dtGetZ(variance))
	{
		ca = dtGetY(c);
		axisIndex = 1;
	}
	else
	{
		ca = dtGetZ(c);
		axisIndex = 2;
	}

	assert(m_nodeCount < m_nodeCapacity);
	int nodeIndex = m_nodeCount++;
	dtNode& node = m_nodes[nodeIndex];
	node.aabb = aabb;
	node.parent = parentIndex;
	node.isLeaf = false;

	int i1 = -1;
	for (int i2 = 0; i2 < count; ++i2)
	{
		dtVec leafCenter = dtCenter(leaves[i2].aabb);
		float value = dtGet(leafCenter, axisIndex);
		if (value <= ca)
		{
			++i1;
			dtSwap(leaves[i1], leaves[i2]);
		}
	}

	int leftCount = i1 + 1;
	int rightCount = count - leftCount;

#if 0
	// TODO_ERIN validation
	for (int i = 0; i < leftCount; ++i)
	{
		dtVec leafCenter = dtCenter(leaves[i].aabb);
		if (dtGet(leafCenter, axisIndex) > ca)
		{
			ca += 0.0f;
		}
	}

	for (int i = leftCount; i < count; ++i)
	{
		dtVec leafCenter = dtCenter(leaves[i].aabb);
		if (dtGet(leafCenter, axisIndex) < ca)
		{
			ca += 0.0f;
		}
	}
#endif

	if (leftCount == 0 || rightCount == 0)
	{
		leftCount = count / 2;
		rightCount = count - leftCount;
	}

	node.child1 = PartitionBoxes(nodeIndex, leaves, leftCount);
	node.child2 = PartitionBoxes(nodeIndex, leaves + leftCount, rightCount);

	const dtNode& child1 = m_nodes[node.child1];
	const dtNode& child2 = m_nodes[node.child2];

	node.height = 1 + dtMax(child1.height, child2.height);

	return nodeIndex;
}
