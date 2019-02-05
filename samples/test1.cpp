#include "test.h"

struct Test1 : Test
{
	const char* GetName() const override
	{
		return "Single Box";
	}

	void Create(bool rotate) override
	{
		dtAABB b;
		b.lowerBound = dtVecSet(-0.5f, -0.5f, -0.5f);
		b.upperBound = dtVecSet(0.5f, 0.5f, 0.5f);
		m_tree.CreateProxy(b, rotate);
	}

	void Destroy() override
	{
		m_tree.Clear();
	}
};

static Test1 s_test;
Test* g_test1 = &s_test;