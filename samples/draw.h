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

struct Camera
{
	Camera();

	void Resize(int width, int height);
	void Reset();

	const dtVec GetUp() const;
	const dtVec GetRight() const;
	const dtVec GetForward() const;

	// Update position and rotation
	void Update();

	dtMtx GetCameraMatrix() const;
	dtMtx GetViewMatrix() const;
	dtMtx GetPerspectiveMatrix() const;

	void ConvertScreenToWorldRay(dtVec& p1, dtVec& p2, float xs, float ys) const;
	bool ConvertWorldToScreen(float& xs, float& ys, dtVec p) const;

	dtMtx m_matrix;

	// Vertical field of view
	float m_fovy;
	float m_tany;

	float m_yaw;   // [0, 2pi]
	float m_pitch; // [0,  pi]

	float m_near;
	float m_far;

	// Screen width and height
	int m_ws;
	int m_hs;

	float m_ratio;

	// Near plane width and height
	float m_wc;
	float m_hc;

	// Far plane width and height
	float m_wf;
	float m_hf;
};

struct Color
{
	Color() {}
	Color(float red, float green, float blue)
		: r(red), g(green), b(blue), a(1.0f) {}
	Color(float red, float green, float blue, float alpha)
		: r(red), g(green), b(blue), a(alpha) {}

	void Set(float red, float green, float blue)
	{
		r = red;
		g = green;
		b = blue;
		a = 1.0f;
	}
	void Set(float red, float green, float blue, float alpha)
	{
		r = red;
		g = green;
		b = blue;
		a = alpha;
	}
	float r, g, b, a;
};

struct Draw
{
    Draw();
    ~Draw();

    void Create();
    void Destroy();

    void DrawBox(const dtAABB&, float extension, const Color& color);

    void DrawSegment(dtVec point1, dtVec point2, const Color& color);

    void DrawPoint(dtVec point, float size, const Color& color);

	void DrawAxes();

    void DrawString(float x, float y, const char* sz, ...);

    void DrawString(dtVec p, const Color& color, const char* sz, ...);

    void Flush();

	Camera m_camera;
    struct GLDynamicPoints* m_points;
    struct GLDynamicLines* m_lines;
	float m_uiScale;
	bool m_showUI;
};
