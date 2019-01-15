I’m planning to give a talk on Dynamic AABB trees. I can do this in 25 minutes and I think it can be mostly visual. Key concepts:


- Build tree of AABB to speed up ray casts and overlap tests for large worlds
- Build the tree incrementally to handle dynamic worlds and streaming
- Use enlarged AABBs so that objects have wiggle room before needing to update the tree
- Introduce the Surface Area Heuristic as an optimization strategy to make ray casts faster
- Show why SAH is better than a balanced tree or median split
- Discuss using the SAH in a greedy fashion when inserting new nodes
- Discuss using tree rotations to improve the SAH metric
- Discuss incremental optimization of the tree
- Discuss ray-cast/overlap test


Title:
Incremental Bounding Volume Hierarchies

Session Summary:
Bounding volume hierarchies are used to accelerate ray casts, overlap queries, and closest point queries for large game worlds. Incremental hierarchies allow objects to be created, destroyed, and moved while maintaining fast queries. The performance of these hierarchies can be improved by optimizing the surface area of the nodes.

Takeaway:
This session demonstrates how to build an incremental bounding volume hierarchy while optimizing for query speed.

Intended Audience:
Anyone interested in ray casting and large game worlds. Knowledge of basic data structures such as binary trees is assumed.

Speaker Bio:
Erin Catto, Lead Software Engineer - Blizzard Entertainment, Shared Game Engine
Erin got his start in the game industry at Crystal Dynamics where he wrote the physics engine for Tomb Raider: Legend. He is currently at Blizzard Entertainment working on the Domino physics engine used by Diablo3, StarCraft, Overwatch, World of WarCraft, Heroes of the Storm, and Call of Duty. He is the author of the Box2D open source physics engine, used to create Crayon Physics, Limbo, and Angry Birds.


/d/graphvis-2.38/bin/dot -Tsvg input.txt > output.svg

TODO
- height control example


Tiles 2: 12x2, pyramid 8
No Rotations
Greedy: area = 136
Branch and Bound: area = 134

With Rotations
Greedy: area = 113
Branch and Bound: area = 112

Tiles 3: 200x10, pyramid 20

No Rotations
Greedy: area = 12854, build time = 3.2ms
Branch and Bound: area = 23032, build time = 23.2ms

With Rotations
Greedy: area = 5259, build time = 1.4ms
Branch and Bound: area = 4960, build time = 4.4ms

