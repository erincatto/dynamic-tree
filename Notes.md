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

