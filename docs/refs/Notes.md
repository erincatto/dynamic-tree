# J. Goldsmith (1987) - Automatic Creation of Object Hierarchies
Orignal SAH paper. SAH insertion BVH build. Handles case where new leaf is fully contained by two child nodes:
- search both
- choose closest center
Considered insertion order. Found orderd insertion is bad (e.g. tile-based worlds, checker board)
Funny: written before the term AABB existed.

# S. Omohundro (1989) - Five Balltree Construction Algorithms

# G. van den Bergen (1998) - Efficient Collision Detection of Complex Deformable Models using AABB Trees

mid-point splits, binary, refits. Unaware of SAH.

# T. Larsson (2006) - A Dynamic Bounding Volume Hierarchy for Generalized Collision Detection

AABB tree, Mid-point splits, 8-ary with culling, refits, rebuilds sub-trees, lazy update. Rules out multithreaded queries. Uses volume heuristic. Unaware of SAH.

# M. Otaduy (2007) - Balanced Hierarchies for Collision Detection between Fracturing Objects

median splits, balanced tree, AVL rotations for rebalancing when adding/removing triangles, grandchildren permutation to optimize squared volume heuristic. Unaware of SAH.

# A. Kensler (2008) - Tree Rotations for Improving Bounding Volume Hierarchies

Original rotation idea. Used hill climbing and simulated annealing to apply rotations to improve tree quality

# D. Kopta (with A. Kensler) (2012) - Fast, Effective BVH Updates for Animated Scenes

Rotations applied on animating scenes. Refit and rotations applied accross whole tree each frame.

# J. Bittner (2013) - Fast Insertion-Based Optimization of Bounding Volume Hierarchies

takes existing BVH and removes a node and re-inserts sub-trees. Uses SAH inheritance cost to find insertion. Uses priority queue (branch and bound) method to find optimial sibling. Node removal based on (node area) * (node area / smallest child area) *  (node area / sum child area). Builds tree one triangle per leaf then collapses several triangles under a subtree to minimize SAH.

# J. Bittner (2015) - Incremental BVH Construction for Ray Tracing

SAH BVH insertion build. Search for best insertion via branch and bound with priority queue.
Incremental optimization by node removal and re-insertion in batches. Removals restricted to region of tree that had insertions. Similar to Bittner 2013.

# Nathanael Presson, btDbvt

Incremental BVH builder. Uses dist_x + dist_y + dist_z of centers for child selection (Proximity). Tiles example creates tree of height 209. Takes 49300 iterations to get to height of 14 using round-robbin remove-insert.