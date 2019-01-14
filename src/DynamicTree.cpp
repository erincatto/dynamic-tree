/*
* Copyright (c) 2009 Erin Catto http://www.box2d.org
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

#include "Box2D/Collision/b2DynamicTree.h"
#include <string.h>
#include <queue>

b2DynamicTree::b2DynamicTree()
{
	m_root = b2_nullNode;

	m_nodeCapacity = 16;
	m_nodeCount = 0;
	m_nodes = (b2TreeNode*)b2Alloc(m_nodeCapacity * sizeof(b2TreeNode));
	memset(m_nodes, 0, m_nodeCapacity * sizeof(b2TreeNode));

	// Build a linked list for the free list.
	for (int32 i = 0; i < m_nodeCapacity - 1; ++i)
	{
		m_nodes[i].next = i + 1;
		m_nodes[i].height = -1;
	}
	m_nodes[m_nodeCapacity-1].next = b2_nullNode;
	m_nodes[m_nodeCapacity-1].height = -1;
	m_freeList = 0;

	m_path = 0;

	m_insertionCount = 0;
}

b2DynamicTree::~b2DynamicTree()
{
	// This frees the entire tree in one shot.
	b2Free(m_nodes);
}

// Allocate a node from the pool. Grow the pool if necessary.
int32 b2DynamicTree::AllocateNode()
{
	// Expand the node pool as needed.
	if (m_freeList == b2_nullNode)
	{
		b2Assert(m_nodeCount == m_nodeCapacity);

		// The free list is empty. Rebuild a bigger pool.
		b2TreeNode* oldNodes = m_nodes;
		m_nodeCapacity *= 2;
		m_nodes = (b2TreeNode*)b2Alloc(m_nodeCapacity * sizeof(b2TreeNode));
		memcpy(m_nodes, oldNodes, m_nodeCount * sizeof(b2TreeNode));
		b2Free(oldNodes);

		// Build a linked list for the free list. The parent
		// pointer becomes the "next" pointer.
		for (int32 i = m_nodeCount; i < m_nodeCapacity - 1; ++i)
		{
			m_nodes[i].next = i + 1;
			m_nodes[i].height = -1;
		}
		m_nodes[m_nodeCapacity-1].next = b2_nullNode;
		m_nodes[m_nodeCapacity-1].height = -1;
		m_freeList = m_nodeCount;
	}

	// Peel a node off the free list.
	int32 nodeId = m_freeList;
	m_freeList = m_nodes[nodeId].next;
	m_nodes[nodeId].parent = b2_nullNode;
	m_nodes[nodeId].child1 = b2_nullNode;
	m_nodes[nodeId].child2 = b2_nullNode;
	m_nodes[nodeId].height = 0;
	m_nodes[nodeId].userData = nullptr;
	++m_nodeCount;
	return nodeId;
}

// Return a node to the pool.
void b2DynamicTree::FreeNode(int32 nodeId)
{
	b2Assert(0 <= nodeId && nodeId < m_nodeCapacity);
	b2Assert(0 < m_nodeCount);
	m_nodes[nodeId].next = m_freeList;
	m_nodes[nodeId].height = -1;
	m_freeList = nodeId;
	--m_nodeCount;
}

// Create a proxy in the tree as a leaf node. We return the index
// of the node instead of a pointer so that we can grow
// the node pool.
int32 b2DynamicTree::CreateProxy(const b2AABB& aabb, void* userData)
{
	int32 proxyId = AllocateNode();

	// Fatten the aabb.
	b2Vec2 r(b2_aabbExtension, b2_aabbExtension);
	m_nodes[proxyId].aabb.lowerBound = aabb.lowerBound - r;
	m_nodes[proxyId].aabb.upperBound = aabb.upperBound + r;
	m_nodes[proxyId].userData = userData;
	m_nodes[proxyId].height = 0;

	InsertLeaf(proxyId);

	return proxyId;
}

void b2DynamicTree::DestroyProxy(int32 proxyId)
{
	b2Assert(0 <= proxyId && proxyId < m_nodeCapacity);
	b2Assert(m_nodes[proxyId].IsLeaf());

	RemoveLeaf(proxyId);
	FreeNode(proxyId);
}

bool b2DynamicTree::MoveProxy(int32 proxyId, const b2AABB& aabb, const b2Vec2& displacement)
{
	b2Assert(0 <= proxyId && proxyId < m_nodeCapacity);

	b2Assert(m_nodes[proxyId].IsLeaf());

	if (m_nodes[proxyId].aabb.Contains(aabb))
	{
		return false;
	}

	RemoveLeaf(proxyId);

	// Extend AABB.
	b2AABB b = aabb;
	b2Vec2 r(b2_aabbExtension, b2_aabbExtension);
	b.lowerBound = b.lowerBound - r;
	b.upperBound = b.upperBound + r;

	// Predict AABB displacement.
	b2Vec2 d = b2_aabbMultiplier * displacement;

	if (d.x < 0.0f)
	{
		b.lowerBound.x += d.x;
	}
	else
	{
		b.upperBound.x += d.x;
	}

	if (d.y < 0.0f)
	{
		b.lowerBound.y += d.y;
	}
	else
	{
		b.upperBound.y += d.y;
	}

	m_nodes[proxyId].aabb = b;

	InsertLeaf(proxyId);
	return true;
}

#if 0

struct b2CandidateNode
{
	int32 index;
	float inducedCost;
};

static inline bool b2CompareCandidates(const b2CandidateNode& a, const b2CandidateNode& b)
{
	return a.inducedCost > b.inducedCost;
}

// Insert using branch and bound
void b2DynamicTree::InsertLeaf(int32 leaf)
{
	++m_insertionCount;

	if (m_root == b2_nullNode)
	{
		m_root = leaf;
		m_nodes[m_root].parent = b2_nullNode;
		return;
	}

	b2AABB aabbQ = m_nodes[leaf].aabb;
	float areaQ = aabbQ.GetPerimeter();

	// Stage 1: find the best sibling for this node
	std::priority_queue<b2CandidateNode, std::vector<b2CandidateNode>, decltype(&b2CompareCandidates)> queue(b2CompareCandidates);

	b2CandidateNode candidate;
	candidate.index = m_root;
	candidate.inducedCost = 0.0f;
	queue.push(candidate);

	float bestCost = b2_maxFloat;
	int32 bestSibling = m_root;

	while (queue.empty() == false)
	{
		candidate = queue.top();
		queue.pop();

		int32 index = candidate.index;
		float inducedCost = candidate.inducedCost;

		if (inducedCost + areaQ >= bestCost)
		{
			// Optimimum found
			break;
		}

		const b2TreeNode& node = m_nodes[index];
		b2AABB aabbG;
		aabbG.Combine(node.aabb, aabbQ);

		float directCost = aabbG.GetPerimeter();
		float totalCost = inducedCost + directCost;

		if (totalCost < bestCost)
		{
			bestCost = totalCost;
			bestSibling = index;
		}

		if (node.IsLeaf())
		{
			continue;
		}

		inducedCost = totalCost - node.aabb.GetPerimeter();
		float lowerBoundCost = inducedCost + areaQ;
		if (lowerBoundCost < bestCost)
		{
			b2CandidateNode candidate1;
			candidate1.index = node.child1;
			candidate1.inducedCost = inducedCost;

			queue.push(candidate1);

			b2CandidateNode candidate2;
			candidate2.index = node.child2;
			candidate2.inducedCost = inducedCost;

			queue.push(candidate2);
		}
	}

	int32 sibling = bestSibling;

	// Stage 2: create a new parent
	int32 oldParent = m_nodes[sibling].parent;
	int32 newParent = AllocateNode();
	m_nodes[newParent].parent = oldParent;
	m_nodes[newParent].userData = nullptr;
	m_nodes[newParent].aabb.Combine(aabbQ, m_nodes[sibling].aabb);
	m_nodes[newParent].height = m_nodes[sibling].height + 1;

	if (oldParent != b2_nullNode)
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
	int32 index = m_nodes[leaf].parent;
	while (index != b2_nullNode)
	{
		int32 child1 = m_nodes[index].child1;
		int32 child2 = m_nodes[index].child2;

		b2Assert(child1 != b2_nullNode);
		b2Assert(child2 != b2_nullNode);

		m_nodes[index].height = 1 + b2Max(m_nodes[child1].height, m_nodes[child2].height);
		m_nodes[index].aabb.Combine(m_nodes[child1].aabb, m_nodes[child2].aabb);

		Rotate(index);

		index = m_nodes[index].parent;
	}

	Validate();
}
#else

void b2DynamicTree::InsertLeaf(int32 leaf)
{
	++m_insertionCount;

	if (m_root == b2_nullNode)
	{
		m_root = leaf;
		m_nodes[m_root].parent = b2_nullNode;
		return;
	}

	b2AABB aabbQ = m_nodes[leaf].aabb;

	// Stage 1: find the best sibling for this node
	int32 index = m_root;
	while (m_nodes[index].IsLeaf() == false)
	{
		int32 child1 = m_nodes[index].child1;
		int32 child2 = m_nodes[index].child2;

		float32 areaP = m_nodes[index].aabb.GetPerimeter();

		b2AABB aabbG;
		aabbG.Combine(m_nodes[index].aabb, aabbQ);
		float32 areaG = aabbG.GetPerimeter();

		// Cost of creating a grand parent G for this node P and the new leaf Q
		float32 Cb = areaG;

		// Minimum cost of pushing the leaf further down the tree
		float32 deltaAreaP = areaG - areaP;

		// Cost of descending into child 1
		float32 C1;
		if (m_nodes[child1].IsLeaf())
		{
			// Child 1 is a leaf
			// Cost of creating new node X and increasing area of node P
			b2AABB aabbX;
			aabbX.Combine(aabbQ, m_nodes[child1].aabb);
			C1 = deltaAreaP + aabbX.GetPerimeter();
		}
		else
		{
			// Child 1 is an internal node
			// Cost of creating new node Y and increasing area of node P and node 1
			b2AABB aabb1;
			aabb1.Combine(aabbQ, m_nodes[child1].aabb);
			float32 deltaArea1 = aabb1.GetPerimeter() - m_nodes[child1].aabb.GetPerimeter();
			float32 areaY = aabbQ.GetPerimeter();
			C1 = deltaAreaP + deltaArea1 + areaY;
		}

		// Cost of descending into child 2
		float32 C2;
		if (m_nodes[child2].IsLeaf())
		{
			// Child 2 is a leaf
			// Cost of creating new node X and increasing area of node P
			b2AABB aabbX;
			aabbX.Combine(aabbQ, m_nodes[child2].aabb);
			C2 = deltaAreaP + aabbX.GetPerimeter();
		}
		else
		{
			// Child 2 is an internal node
			// Cost of creating new node Y and increasing area of node P and node 2
			b2AABB aabb2;
			aabb2.Combine(aabbQ, m_nodes[child2].aabb);
			float32 deltaArea2 = aabb2.GetPerimeter() - m_nodes[child2].aabb.GetPerimeter();
			float32 areaY = aabbQ.GetPerimeter();
			C2 = deltaAreaP + deltaArea2 + areaY;
		}

		// Descend according to the minimum cost.
		if (0.9f * Cb < C1 && 0.9f * Cb < C2)
		{
			break;
		}

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

	int32 sibling = index;

	// Stage 2: create a new parent
	int32 oldParent = m_nodes[sibling].parent;
	int32 newParent = AllocateNode();
	m_nodes[newParent].parent = oldParent;
	m_nodes[newParent].userData = nullptr;
	m_nodes[newParent].aabb.Combine(aabbQ, m_nodes[sibling].aabb);
	m_nodes[newParent].height = m_nodes[sibling].height + 1;

	if (oldParent != b2_nullNode)
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
	while (index != b2_nullNode)
	{
		int32 child1 = m_nodes[index].child1;
		int32 child2 = m_nodes[index].child2;

		b2Assert(child1 != b2_nullNode);
		b2Assert(child2 != b2_nullNode);

		m_nodes[index].height = 1 + b2Max(m_nodes[child1].height, m_nodes[child2].height);
		m_nodes[index].aabb.Combine(m_nodes[child1].aabb, m_nodes[child2].aabb);

		//Rotate(index);

		index = m_nodes[index].parent;
	}

	Validate();
}

#endif

void b2DynamicTree::RemoveLeaf(int32 leaf)
{
	if (leaf == m_root)
	{
		m_root = b2_nullNode;
		return;
	}

	int32 parent = m_nodes[leaf].parent;
	int32 grandParent = m_nodes[parent].parent;
	int32 sibling;
	if (m_nodes[parent].child1 == leaf)
	{
		sibling = m_nodes[parent].child2;
	}
	else
	{
		sibling = m_nodes[parent].child1;
	}

	if (grandParent != b2_nullNode)
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
		int32 index = grandParent;
		while (index != b2_nullNode)
		{
			index = Balance(index);

			int32 child1 = m_nodes[index].child1;
			int32 child2 = m_nodes[index].child2;

			m_nodes[index].aabb.Combine(m_nodes[child1].aabb, m_nodes[child2].aabb);
			m_nodes[index].height = 1 + b2Max(m_nodes[child1].height, m_nodes[child2].height);

			index = m_nodes[index].parent;
		}
	}
	else
	{
		m_root = sibling;
		m_nodes[sibling].parent = b2_nullNode;
		FreeNode(parent);
	}

	//Validate();
}

enum b2TreeRotate
{
	b2_rotateNone,
	b2_rotateBF,
	b2_rotateBG,
	b2_rotateCD,
	b2_rotateCE
};

// Perform a left or right rotation if node A is imbalanced.
// Returns the new root index.
void b2DynamicTree::Rotate(int32 iA)
{
	b2Assert(iA != b2_nullNode);

	b2TreeNode* A = m_nodes + iA;
	if (A->height < 2)
	{
		return;
	}

	int32 iB = A->child1;
	int32 iC = A->child2;
	b2Assert(0 <= iB && iB < m_nodeCapacity);
	b2Assert(0 <= iC && iC < m_nodeCapacity);

	b2TreeNode* B = m_nodes + iB;
	b2TreeNode* C = m_nodes + iC;

	if (B->height == 0)
	{
		// B is a leaf
		b2Assert(C->height > 0);

		int32 iF = C->child1;
		int32 iG = C->child2;
		b2TreeNode* F = m_nodes + iF;
		b2TreeNode* G = m_nodes + iG;
		b2Assert(0 <= iF && iF < m_nodeCapacity);
		b2Assert(0 <= iG && iG < m_nodeCapacity);

		// Base cost
		float costBase = C->aabb.GetPerimeter();

		// Cost of swapping B and F
		b2AABB aabbBG;
		aabbBG.Combine(B->aabb, G->aabb);
		float costBF = aabbBG.GetPerimeter();

		// Cost of swapping B and G
		b2AABB aabbBF;
		aabbBF.Combine(B->aabb, F->aabb);
		float costBG = aabbBF.GetPerimeter();

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
			C->height = 1 + b2Max(B->height, G->height);
			A->height = 1 + b2Max(C->height, F->height);
		}
		else
		{
			// Swap B and G
			A->child1 = iG;
			C->child2 = iB;

			B->parent = iC;
			G->parent = iA;

			C->aabb = aabbBF;
			C->height = 1 + b2Max(B->height, F->height);
			A->height = 1 + b2Max(C->height, G->height);
		}
	}
	else if (C->height == 0)
	{
		// C is a leaf
		b2Assert(B->height > 0);

		int32 iD = B->child1;
		int32 iE = B->child2;
		b2TreeNode* D = m_nodes + iD;
		b2TreeNode* E = m_nodes + iE;
		b2Assert(0 <= iD && iD < m_nodeCapacity);
		b2Assert(0 <= iE && iE < m_nodeCapacity);

		// Base cost
		float costBase = B->aabb.GetPerimeter();

		// Cost of swapping C and D
		b2AABB aabbCE;
		aabbCE.Combine(C->aabb, E->aabb);
		float costCD = aabbCE.GetPerimeter();

		// Cost of swapping C and E
		b2AABB aabbCD;
		aabbCD.Combine(C->aabb, D->aabb);
		float costCE = aabbCD.GetPerimeter();

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
			B->height = 1 + b2Max(C->height, E->height);
			A->height = 1 + b2Max(B->height, D->height);
		}
		else
		{
			// Swap C and E
			A->child2 = iE;
			B->child2 = iC;

			C->parent = iB;
			E->parent = iA;

			B->aabb = aabbCD;
			B->height = 1 + b2Max(C->height, D->height);
			A->height = 1 + b2Max(B->height, E->height);
		}
	}
	else
	{
		int32 iD = B->child1;
		int32 iE = B->child2;
		int32 iF = C->child1;
		int32 iG = C->child2;

		b2TreeNode* D = m_nodes + iD;
		b2TreeNode* E = m_nodes + iE;
		b2TreeNode* F = m_nodes + iF;
		b2TreeNode* G = m_nodes + iG;

		b2Assert(0 <= iD && iD < m_nodeCapacity);
		b2Assert(0 <= iE && iE < m_nodeCapacity);
		b2Assert(0 <= iF && iF < m_nodeCapacity);
		b2Assert(0 <= iG && iG < m_nodeCapacity);

		// Base cost
		float areaB = B->aabb.GetPerimeter();
		float areaC = C->aabb.GetPerimeter();
		float costBase = areaB + areaC;
		b2TreeRotate bestRotation = b2_rotateNone;
		float bestCost = costBase;

		// Cost of swapping B and F
		b2AABB aabbBG;
		aabbBG.Combine(B->aabb, G->aabb);
		float costBF = areaB + aabbBG.GetPerimeter();
		if (costBF < bestCost)
		{
			bestRotation = b2_rotateBF;
			bestCost = costBF;
		}

		// Cost of swapping B and G
		b2AABB aabbBF;
		aabbBF.Combine(B->aabb, F->aabb);
		float costBG = areaB + aabbBF.GetPerimeter();
		if (costBG < bestCost)
		{
			bestRotation = b2_rotateBG;
			bestCost = costBG;
		}

		// Cost of swapping C and D
		b2AABB aabbCE;
		aabbCE.Combine(C->aabb, E->aabb);
		float costCD = areaC + aabbCE.GetPerimeter();
		if (costCD < bestCost)
		{
			bestRotation = b2_rotateCD;
			bestCost = costCD;
		}

		// Cost of swapping C and E
		b2AABB aabbCD;
		aabbCD.Combine(C->aabb, D->aabb);
		float costCE = areaC + aabbCD.GetPerimeter();
		if (costCE < bestCost)
		{
			bestRotation = b2_rotateCE;
			bestCost = costCE;
		}

		switch (bestRotation)
		{
		case b2_rotateNone:
			break;

		case b2_rotateBF:
			A->child1 = iF;
			C->child1 = iB;

			B->parent = iC;
			F->parent = iA;

			C->aabb = aabbBG;
			C->height = 1 + b2Max(B->height, G->height);
			A->height = 1 + b2Max(C->height, F->height);
			break;

		case b2_rotateBG:
			A->child1 = iG;
			C->child2 = iB;

			B->parent = iC;
			G->parent = iA;

			C->aabb = aabbBF;
			C->height = 1 + b2Max(B->height, F->height);
			A->height = 1 + b2Max(C->height, G->height);
			break;

		case b2_rotateCD:
			A->child2 = iD;
			B->child1 = iC;

			C->parent = iB;
			D->parent = iA;

			B->aabb = aabbCE;
			B->height = 1 + b2Max(C->height, E->height);
			A->height = 1 + b2Max(B->height, D->height);
			break;

		case b2_rotateCE:
			A->child2 = iE;
			B->child2 = iC;

			C->parent = iB;
			E->parent = iA;

			B->aabb = aabbCD;
			B->height = 1 + b2Max(C->height, D->height);
			A->height = 1 + b2Max(B->height, E->height);
			break;

		default:
			b2Assert(false);
			break;
		}
	}
}

// Perform a left or right rotation if node A is imbalanced.
// Returns the new root index.
int32 b2DynamicTree::Balance(int32 iA)
{
	b2Assert(iA != b2_nullNode);

	b2TreeNode* A = m_nodes + iA;
	if (A->IsLeaf() || A->height < 2)
	{
		return iA;
	}

	int32 iB = A->child1;
	int32 iC = A->child2;
	b2Assert(0 <= iB && iB < m_nodeCapacity);
	b2Assert(0 <= iC && iC < m_nodeCapacity);

	b2TreeNode* B = m_nodes + iB;
	b2TreeNode* C = m_nodes + iC;

	int32 balance = C->height - B->height;

	// Rotate C up
	if (balance > 1)
	{
		int32 iF = C->child1;
		int32 iG = C->child2;
		b2TreeNode* F = m_nodes + iF;
		b2TreeNode* G = m_nodes + iG;
		b2Assert(0 <= iF && iF < m_nodeCapacity);
		b2Assert(0 <= iG && iG < m_nodeCapacity);

		// Swap A and C
		C->child1 = iA;
		C->parent = A->parent;
		A->parent = iC;

		// A's old parent should point to C
		if (C->parent != b2_nullNode)
		{
			if (m_nodes[C->parent].child1 == iA)
			{
				m_nodes[C->parent].child1 = iC;
			}
			else
			{
				b2Assert(m_nodes[C->parent].child2 == iA);
				m_nodes[C->parent].child2 = iC;
			}
		}
		else
		{
			m_root = iC;
		}

		// Rotate
		if (F->height > G->height)
		{
			C->child2 = iF;
			A->child2 = iG;
			G->parent = iA;
			A->aabb.Combine(B->aabb, G->aabb);
			C->aabb.Combine(A->aabb, F->aabb);

			A->height = 1 + b2Max(B->height, G->height);
			C->height = 1 + b2Max(A->height, F->height);
		}
		else
		{
			C->child2 = iG;
			A->child2 = iF;
			F->parent = iA;
			A->aabb.Combine(B->aabb, F->aabb);
			C->aabb.Combine(A->aabb, G->aabb);

			A->height = 1 + b2Max(B->height, F->height);
			C->height = 1 + b2Max(A->height, G->height);
		}

		return iC;
	}
	
	// Rotate B up
	if (balance < -1)
	{
		int32 iD = B->child1;
		int32 iE = B->child2;
		b2TreeNode* D = m_nodes + iD;
		b2TreeNode* E = m_nodes + iE;
		b2Assert(0 <= iD && iD < m_nodeCapacity);
		b2Assert(0 <= iE && iE < m_nodeCapacity);

		// Swap A and B
		B->child1 = iA;
		B->parent = A->parent;
		A->parent = iB;

		// A's old parent should point to B
		if (B->parent != b2_nullNode)
		{
			if (m_nodes[B->parent].child1 == iA)
			{
				m_nodes[B->parent].child1 = iB;
			}
			else
			{
				b2Assert(m_nodes[B->parent].child2 == iA);
				m_nodes[B->parent].child2 = iB;
			}
		}
		else
		{
			m_root = iB;
		}

		// Rotate
		if (D->height > E->height)
		{
			B->child2 = iD;
			A->child1 = iE;
			E->parent = iA;
			A->aabb.Combine(C->aabb, E->aabb);
			B->aabb.Combine(A->aabb, D->aabb);

			A->height = 1 + b2Max(C->height, E->height);
			B->height = 1 + b2Max(A->height, D->height);
		}
		else
		{
			B->child2 = iE;
			A->child1 = iD;
			D->parent = iA;
			A->aabb.Combine(C->aabb, D->aabb);
			B->aabb.Combine(A->aabb, E->aabb);

			A->height = 1 + b2Max(C->height, D->height);
			B->height = 1 + b2Max(A->height, E->height);
		}

		return iB;
	}

	return iA;
}

int32 b2DynamicTree::GetHeight() const
{
	if (m_root == b2_nullNode)
	{
		return 0;
	}

	return m_nodes[m_root].height;
}

//
float32 b2DynamicTree::GetAreaRatio() const
{
	if (m_root == b2_nullNode)
	{
		return 0.0f;
	}

	const b2TreeNode* root = m_nodes + m_root;
	float32 rootArea = root->aabb.GetPerimeter();

	float32 totalArea = 0.0f;
	for (int32 i = 0; i < m_nodeCapacity; ++i)
	{
		const b2TreeNode* node = m_nodes + i;
		if (node->height < 0)
		{
			// Free node in pool
			continue;
		}

		totalArea += node->aabb.GetPerimeter();
	}

	return totalArea / rootArea;
}

// Compute the height of a sub-tree.
int32 b2DynamicTree::ComputeHeight(int32 nodeId) const
{
	b2Assert(0 <= nodeId && nodeId < m_nodeCapacity);
	b2TreeNode* node = m_nodes + nodeId;

	if (node->IsLeaf())
	{
		return 0;
	}

	int32 height1 = ComputeHeight(node->child1);
	int32 height2 = ComputeHeight(node->child2);
	return 1 + b2Max(height1, height2);
}

int32 b2DynamicTree::ComputeHeight() const
{
	int32 height = ComputeHeight(m_root);
	return height;
}

void b2DynamicTree::ValidateStructure(int32 index) const
{
	if (index == b2_nullNode)
	{
		return;
	}

	if (index == m_root)
	{
		b2Assert(m_nodes[index].parent == b2_nullNode);
	}

	const b2TreeNode* node = m_nodes + index;

	int32 child1 = node->child1;
	int32 child2 = node->child2;

	if (node->IsLeaf())
	{
		b2Assert(child1 == b2_nullNode);
		b2Assert(child2 == b2_nullNode);
		b2Assert(node->height == 0);
		return;
	}

	b2Assert(0 <= child1 && child1 < m_nodeCapacity);
	b2Assert(0 <= child2 && child2 < m_nodeCapacity);

	b2Assert(m_nodes[child1].parent == index);
	b2Assert(m_nodes[child2].parent == index);

	ValidateStructure(child1);
	ValidateStructure(child2);
}

void b2DynamicTree::ValidateMetrics(int32 index) const
{
	if (index == b2_nullNode)
	{
		return;
	}

	const b2TreeNode* node = m_nodes + index;

	int32 child1 = node->child1;
	int32 child2 = node->child2;

	if (node->IsLeaf())
	{
		b2Assert(child1 == b2_nullNode);
		b2Assert(child2 == b2_nullNode);
		b2Assert(node->height == 0);
		return;
	}

	b2Assert(0 <= child1 && child1 < m_nodeCapacity);
	b2Assert(0 <= child2 && child2 < m_nodeCapacity);

	int32 height1 = m_nodes[child1].height;
	int32 height2 = m_nodes[child2].height;
	int32 height;
	height = 1 + b2Max(height1, height2);
	b2Assert(node->height == height);

	b2AABB aabb;
	aabb.Combine(m_nodes[child1].aabb, m_nodes[child2].aabb);

	b2Assert(aabb.lowerBound == node->aabb.lowerBound);
	b2Assert(aabb.upperBound == node->aabb.upperBound);

	ValidateMetrics(child1);
	ValidateMetrics(child2);
}

void b2DynamicTree::Validate() const
{
#if defined(b2DEBUG)
	ValidateStructure(m_root);
	ValidateMetrics(m_root);

	int32 freeCount = 0;
	int32 freeIndex = m_freeList;
	while (freeIndex != b2_nullNode)
	{
		b2Assert(0 <= freeIndex && freeIndex < m_nodeCapacity);
		freeIndex = m_nodes[freeIndex].next;
		++freeCount;
	}

	b2Assert(GetHeight() == ComputeHeight());

	b2Assert(m_nodeCount + freeCount == m_nodeCapacity);
#endif
}

int32 b2DynamicTree::GetMaxBalance() const
{
	int32 maxBalance = 0;
	for (int32 i = 0; i < m_nodeCapacity; ++i)
	{
		const b2TreeNode* node = m_nodes + i;
		if (node->height <= 1)
		{
			continue;
		}

		b2Assert(node->IsLeaf() == false);

		int32 child1 = node->child1;
		int32 child2 = node->child2;
		int32 balance = b2Abs(m_nodes[child2].height - m_nodes[child1].height);
		maxBalance = b2Max(maxBalance, balance);
	}

	return maxBalance;
}

void b2DynamicTree::RebuildBottomUp()
{
	int32* nodes = (int32*)b2Alloc(m_nodeCount * sizeof(int32));
	int32 count = 0;

	// Build array of leaves. Free the rest.
	for (int32 i = 0; i < m_nodeCapacity; ++i)
	{
		if (m_nodes[i].height < 0)
		{
			// free node in pool
			continue;
		}

		if (m_nodes[i].IsLeaf())
		{
			m_nodes[i].parent = b2_nullNode;
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
		float32 minCost = b2_maxFloat;
		int32 iMin = -1, jMin = -1;
		for (int32 i = 0; i < count; ++i)
		{
			b2AABB aabbi = m_nodes[nodes[i]].aabb;

			for (int32 j = i + 1; j < count; ++j)
			{
				b2AABB aabbj = m_nodes[nodes[j]].aabb;
				b2AABB b;
				b.Combine(aabbi, aabbj);
				float32 cost = b.GetPerimeter();
				if (cost < minCost)
				{
					iMin = i;
					jMin = j;
					minCost = cost;
				}
			}
		}

		int32 index1 = nodes[iMin];
		int32 index2 = nodes[jMin];
		b2TreeNode* child1 = m_nodes + index1;
		b2TreeNode* child2 = m_nodes + index2;

		int32 parentIndex = AllocateNode();
		b2TreeNode* parent = m_nodes + parentIndex;
		parent->child1 = index1;
		parent->child2 = index2;
		parent->height = 1 + b2Max(child1->height, child2->height);
		parent->aabb.Combine(child1->aabb, child2->aabb);
		parent->parent = b2_nullNode;

		child1->parent = parentIndex;
		child2->parent = parentIndex;

		nodes[jMin] = nodes[count-1];
		nodes[iMin] = parentIndex;
		--count;
	}

	m_root = nodes[0];
	b2Free(nodes);

	Validate();
}

void b2DynamicTree::ShiftOrigin(const b2Vec2& newOrigin)
{
	// Build array of leaves. Free the rest.
	for (int32 i = 0; i < m_nodeCapacity; ++i)
	{
		m_nodes[i].aabb.lowerBound -= newOrigin;
		m_nodes[i].aabb.upperBound -= newOrigin;
	}
}

void b2DynamicTree::WriteDot() const
{
	if (m_nodeCapacity > 50)
	{
		b2Log("graph\n");
		b2Log("{\n");
		b2Log("node[shape = point]\n");

		float totalArea = 0.0f;
		for (int32 i = 0; i < m_nodeCapacity; ++i)
		{
			if (m_nodes[i].height == -1)
			{
				continue;
			}

			if (m_nodes[i].IsLeaf())
			{
				continue;
			}

			if (i != m_root)
			{
				float area = m_nodes[i].aabb.GetPerimeter();
				totalArea += area;
			}

			b2Log("%d -- %d\n", i, m_nodes[i].child1);
			b2Log("%d -- %d\n", i, m_nodes[i].child2);
		}

		b2Log("%d [shape=box, label=\"inner area = %.f\"]\n", m_nodeCapacity, totalArea);
		b2Log("}\n");
	}
	else
	{
		b2Log("graph\n");
		b2Log("{\n");
		for (int32 i = 0; i < m_nodeCapacity; ++i)
		{
			if (m_nodes[i].height == -1)
			{
				continue;
			}

			if (m_nodes[i].IsLeaf())
			{
				continue;
			}

			b2Log("%d -- %d\n", i, m_nodes[i].child1);
			b2Log("%d -- %d\n", i, m_nodes[i].child2);
		}

		float totalArea = 0.0f;
		for (int32 i = 0; i < m_nodeCapacity; ++i)
		{
			if (m_nodes[i].height == -1)
			{
				continue;
			}

			if (m_nodes[i].IsLeaf())
			{
				//float area = m_nodes[i].aabb.GetPerimeter();
				//b2Log("%d [shape=box, label=\"%.f\"]\n", i, area);
				b2Log("%d [shape=point]\n", i);
			}
			else
			{
				float area = m_nodes[i].aabb.GetPerimeter();
				b2Log("%d [shape=circle, label=\"%.f\"]\n", i, area);
				if (i != m_root)
				{
					totalArea += area;
				}
			}
		}

		b2Log("%d [shape=box, label=\"inner area = %.f\"]\n", m_nodeCapacity, totalArea);
		b2Log("}\n");
	}
}
