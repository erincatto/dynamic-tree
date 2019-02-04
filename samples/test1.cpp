#include "test.h"
#include "draw.h"

struct Test1 : Test
{
	void Create() override
	{
		dtAABB b;
		b.lowerBound = dtVecSet(-0.5f, -0.5f, -0.5f);
		b.upperBound = dtVecSet(0.5f, 0.5f, 0.5f);
		m_tree.CreateProxy(b);
	}
};

static Test1 s_test1;
Test* g_test1 = &s_test1;