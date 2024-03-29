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

#include "dynamic-tree/utils.h"
#include <vector>

#define dt_nullNode (-1)

enum dtInsertionHeuristic
{
	dt_sah = 0,
	dt_sah_rotate,
	dt_bittner,
	dt_approx_sah,
	dt_approx_sah_rotate,
	dt_manhattan
};

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

	// leaf = 0, free node = dt_nullNode
	int height;

	int objectIndex;

	bool isLeaf;
};

struct dtCandidateNode
{
	int index;
	float inheritanceCost;
};

struct dtCost
{
	int node;
	float cost;
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
	int CreateProxy(const dtAABB& aabb, int objectIndex);

	/// Destroy a proxy. This asserts if the id is invalid.
	void DestroyProxy(int proxyId);

	/// Get the fat AABB for a proxy.
	const dtAABB& GetAABB(int proxyId) const;

	/// Validate this tree. For testing.
	void Validate() const;

	/// Get the proxy/leaf count
	int GetProxyCount() const;

	/// Get the cached height.
	int GetHeight() const;

	/// Get the maximum balance of an node in the tree. The balance is the difference
	/// in height of the two children of a node.
	int GetMaxBalance() const;

	/// Get the ratio of the sum of the internal node areas to the root area.
	float GetAreaRatio() const;

	/// Get the area of the internal nodes
	float GetArea() const;

	/// Build an optimal tree. Very expensive. For testing.
	void RebuildBottomUp();

	/// Build top down using SAH
	void BuildTopDownSAH(int* proxies, dtAABB* aabbs, int count);
	int BinSortBoxes(int parentIndex, dtNode* leaves, int count, struct dtTreeBin* bins, struct dtTreePlane* planes);

	/// Build top down using the median split
	void BuildTopDownMedianSplit(int* proxies, dtAABB* aabbs, int count);
	int PartitionBoxes(int parentIndex, dtNode* leaves, int count);

	void WriteDot(const char* fileName) const;

	int AllocateNode();
	void FreeNode(int node);

	void InsertLeaf(int leaf);
	void InsertLeafSAH(int leaf);
	void InsertLeafBittner(int leaf);
	void InsertLeafApproxSAH(int leaf);
	void InsertLeafManhattan(int leaf);
	void RemoveLeaf(int leaf);

	dtCost MinCost(int index, const dtAABB& box);

	float SiblingCost(const dtAABB& aabbL, int sibling);
	int SiblingApproxSAH(const dtAABB& aabbL);
	//int SiblingApproxSAH2(const dtAABB& aabbL);
	int SiblingApproxOmohundro(const dtAABB& aabbL, std::vector<int>& path, float& cost);
	void Rotate(int index);

	void Optimize(int iterations);
	void Shuffle(int index);

	int ComputeHeight() const;
	int ComputeHeight(int nodeId) const;

	void ValidateStructure(int index) const;
	void ValidateMetrics(int index) const;

	int m_root;

	dtNode* m_nodes;
	int m_nodeCount;
	int m_nodeCapacity;
	int m_proxyCount;

	int m_freeList;

	int m_countBF;
	int m_countBG;
	int m_countCD;
	int m_countCE;

	/// This is used to incrementally traverse the tree for re-balancing.
	int m_path;

	int m_insertionCount;

	dtInsertionHeuristic m_heuristic;
	
	std::vector<dtCandidateNode> m_heap;
	int m_maxHeapCount;
};
