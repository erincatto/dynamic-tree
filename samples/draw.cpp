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
#include "draw.h"

#if defined(__APPLE_CC__)
#define GLFW_INCLUDE_GLCOREARB
#include <OpenGL/gl3.h>
#else
#include "glad.h"
#endif

#include "glfw/glfw3.h"

#define IMGUI_DISABLE_OBSOLETE_FUNCTIONS 1

#include "imgui/imgui.h"

#include <algorithm>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#define BUFFER_OFFSET(x)  ((const void*) (x))

//
Camera::Camera()
{
	Reset();
	Resize(1600, 900);
	Update();
}

//
void Camera::Reset()
{
	m_fovy = 45.0f * dtPi / 180.0f;
	m_tany = tanf(0.5f * m_fovy);
	m_yaw = 0.0f;
	m_pitch = 0.0f;
	m_matrix.cx = dtVec_UnitX;
	m_matrix.cy = dtVec_UnitY;
	m_matrix.cz = dtVec_UnitZ;
	m_matrix.cw = dtVecSet(0.0f, 0.0f, 10.0f);
	m_near = 0.1f;
	m_far = 1000.0f;
}

//
void Camera::Resize(int width, int height)
{
	m_ws = width;
	m_hs = height;
	m_ratio = float(m_ws) / float(m_hs);

	// Size of near plane rectangle
	m_hc = 2.0f * m_near * m_tany;
	m_wc = m_ratio * m_hc;

	// Size of far plane rectangle
	m_wf = m_far * (m_wc / m_near);
	m_hf = m_far * (m_hc / m_near);
}

//
const dtVec Camera::GetUp() const
{
	return m_matrix.cy;
}

//
const dtVec Camera::GetRight() const
{
	return m_matrix.cx;
}

//
const dtVec Camera::GetForward() const
{
	return m_matrix.cz;
}

//
void Camera::Update()
{
	float sinYaw = sinf(m_yaw);
	float cosYaw = cosf(m_yaw);
	float sinPitch = sinf(m_pitch);
	float cosPitch = cosf(m_pitch);

	// Compute forward axis by finding point on sphere
	dtVec forward = dtVecSet(sinYaw * cosPitch, sinPitch, cosYaw * cosPitch);

	// Use Gram–Schmidt to get correct up vector
	dtVec up = dtVecSet(0.0f, 1.0f, 0.0f);
	up = dtNormalize3(up - dtDot3(forward, up) * forward);

	dtVec right = dtNormalize3(dtCross(up, forward));

	m_matrix.cx = right;
	m_matrix.cy = up;
	m_matrix.cz = forward;
}

//
dtMtx Camera::GetCameraMatrix() const
{
	return m_matrix;
}

//
dtMtx Camera::GetViewMatrix() const
{
	dtMtx view = dtMtx_InvertOrtho(m_matrix);
	view.cw.w = 1.0f;
	return view;
}

// Implement gluProjection
// http://www.songho.ca/opengl/gl_projectionmatrix.html
// https://unspecified.wordpress.com/2012/06/21/calculating-the-gluperspective-matrix-and-other-opengl-matrix-maths/
// https://software.intel.com/en-us/articles/alternatives-to-using-z-bias-to-fix-z-fighting-issues
dtMtx Camera::GetPerspectiveMatrix() const
{
	float e = 1.0f / m_tany;
	float n = m_near;
	float f = m_far;

	dtMtx m;
	m.cx = dtVecSet(e / m_ratio, 0.0f, 0.0f, 0.0f);
	m.cy = dtVecSet(0.0f, e, 0.0f, 0.0f);
	m.cz = dtVecSet(0.0f, 0.0f, (f + n) / (n - f), -1.0f);
	m.cw = dtVecSet(0.0f, 0.0f, 2.0f * f * n / (n - f), 0.0f);
	return m;
}

// Create a world ray from a screen point, extending from the near plane to the far plane
void Camera::ConvertScreenToWorldRay(dtVec& p1, dtVec& p2, float xs, float ys) const
{
	// Scale factors
	float ax = xs / m_ws - 0.5f;
	float ay = 0.5f - ys / m_hs;

	// Near plane
	float xc = ax * m_wc;
	float yc = ay * m_hc;

	// Far plane
	float xf = ax * m_wf;
	float yf = ay * m_hf;

	dtVec cn = dtVecSet(xc, yc, -m_near);
	dtVec cf = dtVecSet(xf, yf, -m_far);

	p1 = dtTransformPoint(m_matrix, cn);
	p2 = dtTransformPoint(m_matrix, cf);
}

//
bool Camera::ConvertWorldToScreen(float& xs, float& ys, dtVec p) const
{
	// Get point in camera frame
	dtVec pc = dtInvTransformPoint(m_matrix, p);

	float xc = pc.x;
	float yc = pc.y;
	float zc = pc.z;

	if (zc < -m_far || -m_near < zc)
	{
		return false;
	}

	float hz = -2.0f * zc * m_tany;
	float wz = m_ratio * hz;

	float ax = xc / wz;
	float ay = yc / hz;

	xs = (0.5f + ax) * m_ws;
	ys = (0.5f - ay) * m_hs;

	return true;
}

//
static void sCheckGLError()
{
    GLenum errCode = glGetError();
    if (errCode != GL_NO_ERROR)
    {
        fprintf(stderr, "OpenGL error = %d\n", errCode);
        assert(false);
    }
}

// Prints shader compilation errors
static void sPrintLog(GLuint object)
{
    GLint log_length = 0;
    if (glIsShader(object))
    {
        glGetShaderiv(object, GL_INFO_LOG_LENGTH, &log_length);
    }
    else if (glIsProgram(object))
    {
        glGetProgramiv(object, GL_INFO_LOG_LENGTH, &log_length);
    }
    else
    {
        fprintf(stderr, "printlog: Not a shader or a program\n");
        return;
    }

    char* log = (char*)malloc(log_length);

    if (glIsShader(object))
    {
        glGetShaderInfoLog(object, log_length, nullptr, log);
    }
    else if (glIsProgram(object))
    {
        glGetProgramInfoLog(object, log_length, nullptr, log);
    }

    fprintf(stderr, "%s", log);
    free(log);
}

//
static GLuint sCreateShaderFromString(const char* source, GLenum type)
{
    GLuint res = glCreateShader(type);
    const char* sources[] = { source };
    glShaderSource(res, 1, sources, nullptr);
    glCompileShader(res);
    GLint compile_ok = GL_FALSE;
    glGetShaderiv(res, GL_COMPILE_STATUS, &compile_ok);
    if (compile_ok == GL_FALSE)
    {
        fprintf(stderr, "Error compiling shader of type %d!\n", type);
        sPrintLog(res);
        glDeleteShader(res);
        return 0;
    }

    return res;
}

// 
static GLuint sCreateShaderProgram(const char* vs, const char* fs)
{
    GLuint vsId = sCreateShaderFromString(vs, GL_VERTEX_SHADER);
    GLuint fsId = sCreateShaderFromString(fs, GL_FRAGMENT_SHADER);
    assert(vsId != 0 && fsId != 0);

    GLuint programId = glCreateProgram();
    glAttachShader(programId, vsId);
    glAttachShader(programId, fsId);
    glBindFragDataLocation(programId, 0, "color");
    glLinkProgram(programId);

    glDeleteShader(vsId);
    glDeleteShader(fsId);

    GLint status = GL_FALSE;
    glGetProgramiv(programId, GL_LINK_STATUS, &status);
    assert(status != GL_FALSE);

    return programId;
}

//
struct GLDynamicPoints
{
    void Create(const Camera* camera)
    {
		m_camera = camera;

        const char* vs = \
            "#version 330\n"
            "uniform mat4 projectionMatrix;\n"
            "uniform mat4 viewMatrix;\n"
            "layout(location = 0) in vec4 p;\n"
            "layout(location = 1) in vec4 c;\n"
            "layout(location = 2) in float s;\n"
            "out vec4 f_color;\n"
            "void main(void)\n"
            "{\n"
            "	f_color = c;\n"
            "	gl_Position = projectionMatrix * viewMatrix * p;\n"
            "   gl_PointSize = s;\n"
            "}\n";

        const char* fs = \
            "#version 330\n"
            "in vec4 f_color;\n"
            "out vec4 color;\n"
            "void main(void)\n"
            "{\n"
            "	color = f_color;\n"
            "}\n";

        m_programId = sCreateShaderProgram(vs, fs);
        m_projectionUniform = glGetUniformLocation(m_programId, "projectionMatrix");
        m_viewUniform = glGetUniformLocation(m_programId, "viewMatrix");
        m_vertexAttribute = 0;
        m_colorAttribute = 1;
        m_sizeAttribute = 2;

        // Generate
        glGenVertexArrays(1, &m_vaoId);
        glGenBuffers(3, m_vboIds);

        glBindVertexArray(m_vaoId);
        glEnableVertexAttribArray(m_vertexAttribute);
        glEnableVertexAttribArray(m_colorAttribute);
        glEnableVertexAttribArray(m_sizeAttribute);

        // Vertex buffer
        glBindBuffer(GL_ARRAY_BUFFER, m_vboIds[0]);
        glVertexAttribPointer(m_vertexAttribute, 4, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));
        glBufferData(GL_ARRAY_BUFFER, sizeof(m_vertices), m_vertices, GL_DYNAMIC_DRAW);

        glBindBuffer(GL_ARRAY_BUFFER, m_vboIds[1]);
        glVertexAttribPointer(m_colorAttribute, 4, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));
        glBufferData(GL_ARRAY_BUFFER, sizeof(m_colors), m_colors, GL_DYNAMIC_DRAW);

        glBindBuffer(GL_ARRAY_BUFFER, m_vboIds[2]);
        glVertexAttribPointer(m_sizeAttribute, 1, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));
        glBufferData(GL_ARRAY_BUFFER, sizeof(m_sizes), m_sizes, GL_DYNAMIC_DRAW);

        sCheckGLError();

        // Cleanup
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);

        m_count = 0;
    }

    void Destroy()
    {
        if (m_vaoId)
        {
            glDeleteVertexArrays(1, &m_vaoId);
            glDeleteBuffers(2, m_vboIds);
            m_vaoId = 0;
        }

        if (m_programId)
        {
            glDeleteProgram(m_programId);
            m_programId = 0;
        }
    }

    void Draw(dtVec v, const Color& c, float size)
    {
        if (m_count == e_maxVertices)
        {
            Flush();
        }

        m_vertices[m_count] = v;
		m_vertices[m_count].w = 1.0f;
        m_colors[m_count] = c;
        m_sizes[m_count] = size;
        ++m_count;
    }

    void Flush()
    {
        if (m_count == 0)
            return;

        glUseProgram(m_programId);

        dtMtx proj = m_camera->GetPerspectiveMatrix();
        dtMtx view = m_camera->GetViewMatrix();

        glUniformMatrix4fv(m_projectionUniform, 1, GL_FALSE, (float*)&proj);
        glUniformMatrix4fv(m_viewUniform, 1, GL_FALSE, (float*)&view);

        glBindVertexArray(m_vaoId);

        glBindBuffer(GL_ARRAY_BUFFER, m_vboIds[0]);
        glBufferSubData(GL_ARRAY_BUFFER, 0, m_count * sizeof(dtVec), m_vertices);

        glBindBuffer(GL_ARRAY_BUFFER, m_vboIds[1]);
        glBufferSubData(GL_ARRAY_BUFFER, 0, m_count * sizeof(Color), m_colors);

        glBindBuffer(GL_ARRAY_BUFFER, m_vboIds[2]);
        glBufferSubData(GL_ARRAY_BUFFER, 0, m_count * sizeof(float), m_sizes);

        glEnable(GL_PROGRAM_POINT_SIZE);
        glDrawArrays(GL_POINTS, 0, m_count);
        glDisable(GL_PROGRAM_POINT_SIZE);

        sCheckGLError();

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
        glUseProgram(0);

        m_count = 0;
    }

    enum { e_maxVertices = 2048 };

	const Camera* m_camera;
    dtVec m_vertices[e_maxVertices];
    Color m_colors[e_maxVertices];
    float m_sizes[e_maxVertices];

    int m_count;

    GLuint m_vaoId;
    GLuint m_vboIds[3];
    GLuint m_programId;
    GLint m_projectionUniform;
    GLint m_viewUniform;
    GLint m_vertexAttribute;
    GLint m_colorAttribute;
    GLint m_sizeAttribute;
};

//
struct GLDynamicLines
{
    //
    void Create(const Camera* camera)
    {
		m_camera = camera;

        const char* vs = \
            "#version 330\n"
            "uniform mat4 projectionMatrix;\n"
            "uniform mat4 viewMatrix;\n"
            "layout(location = 0) in vec4 p;\n"
            "layout(location = 1) in vec4 c;\n"
            "out vec4 f_color;\n"
            "void main(void)\n"
            "{\n"
            "	f_color = c;\n"
            "	gl_Position = projectionMatrix * viewMatrix * p;\n"
            "}\n";

        const char* fs = \
            "#version 330\n"
            "in vec4 f_color;\n"
            "out vec4 color;\n"
            "void main(void)\n"
            "{\n"
            "	color = f_color;\n"
            "}\n";

        m_programId = sCreateShaderProgram(vs, fs);
        m_projectionUniform = glGetUniformLocation(m_programId, "projectionMatrix");
        m_viewUniform = glGetUniformLocation(m_programId, "viewMatrix");

        m_vertexAttribute = 0;
        m_colorAttribute = 1;

        // Generate
        glGenVertexArrays(1, &m_vaoId);
        glGenBuffers(2, m_vboIds);

        glBindVertexArray(m_vaoId);
        glEnableVertexAttribArray(m_vertexAttribute);
        glEnableVertexAttribArray(m_colorAttribute);

        // Vertex buffer
        glBindBuffer(GL_ARRAY_BUFFER, m_vboIds[0]);
        glVertexAttribPointer(m_vertexAttribute, 4, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));
        glBufferData(GL_ARRAY_BUFFER, sizeof(m_vertices), m_vertices, GL_DYNAMIC_DRAW);

        glBindBuffer(GL_ARRAY_BUFFER, m_vboIds[1]);
        glVertexAttribPointer(m_colorAttribute, 4, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));
        glBufferData(GL_ARRAY_BUFFER, sizeof(m_colors), m_colors, GL_DYNAMIC_DRAW);

        sCheckGLError();

        // Cleanup
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);

        m_count = 0;
    }

    //
    void Destroy()
    {
        if (m_vaoId)
        {
            glDeleteVertexArrays(1, &m_vaoId);
            glDeleteBuffers(2, m_vboIds);
            m_vaoId = 0;
        }

        if (m_programId)
        {
            glDeleteProgram(m_programId);
            m_programId = 0;
        }
    }

    //
    void Draw(dtVec v1, dtVec v2, const Color& c)
    {
        if (m_count + 1 >= e_maxVertices)
        {
            Flush();
        }

		m_vertices[m_count] = v1;
		m_vertices[m_count].w = 1.0f;
        m_colors[m_count] = c;
        ++m_count;
 
		m_vertices[m_count] = v2;
		m_vertices[m_count].w = 1.0f;
        m_colors[m_count] = c;
        ++m_count;
    }

    //
    void Flush()
    {
        if (m_count == 0)
        {
            return;
        }

        glUseProgram(m_programId);

        dtMtx proj = m_camera->GetPerspectiveMatrix();
        dtMtx view = m_camera->GetViewMatrix();

        glUniformMatrix4fv(m_projectionUniform, 1, GL_FALSE, (float*)&proj);
        glUniformMatrix4fv(m_viewUniform, 1, GL_FALSE, (float*)&view);

        glBindVertexArray(m_vaoId);

        glBindBuffer(GL_ARRAY_BUFFER, m_vboIds[0]);
        glBufferSubData(GL_ARRAY_BUFFER, 0, m_count * sizeof(dtVec), m_vertices);

        glBindBuffer(GL_ARRAY_BUFFER, m_vboIds[1]);
        glBufferSubData(GL_ARRAY_BUFFER, 0, m_count * sizeof(Color), m_colors);

        glDrawArrays(GL_LINES, 0, m_count);

        sCheckGLError();

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
        glUseProgram(0);

        m_count = 0;
    }

	const Camera* m_camera;

    enum { e_maxVertices = 2 * 10 * 1024 };
    dtVec m_vertices[e_maxVertices];
    Color m_colors[e_maxVertices];

    int m_count;

    GLuint m_vaoId;
    GLuint m_vboIds[2];
    GLuint m_programId;
    GLint m_projectionUniform;
    GLint m_viewUniform;
    GLint m_vertexAttribute;
    GLint m_colorAttribute;
};

//
struct TempEdge
{
	dtVec p1, p2;

	// i1 > i2 for sorting
	int i1, i2;

	bool isolated;

	bool operator < (const TempEdge& e) const
	{
		if (i1 < e.i1)
		{
			return true;
		}
		else if (i1 > e.i1)
		{
			return false;
		}
		else
		{
			return i2 < e.i2;
		}
	}
};

//
Draw::Draw()
{
    m_points = nullptr;
    m_lines = nullptr;
	m_showUI = true;
}

//
Draw::~Draw()
{
    assert(m_points == nullptr);
    assert(m_lines == nullptr);
}

//
void Draw::Create()
{
    m_points = new GLDynamicPoints;
    m_points->Create(&m_camera);
    m_lines = new GLDynamicLines;
    m_lines->Create(&m_camera);
}

//
void Draw::Destroy()
{
    m_points->Destroy();
    delete m_points;
    m_points = nullptr;

    m_lines->Destroy();
    delete m_lines;
    m_lines = nullptr;
}

//
void Draw::DrawBox(const dtAABB& box, float extension, const Color& color)
{
	dtVec extents = 0.5f * (box.upperBound - box.lowerBound);
	dtVec center = 0.5f * (box.lowerBound + box.upperBound);
    float hx = extents.x + extension;
    float hy = extents.y + extension;
    float hz = extents.z + extension;

	dtVec p1 = dtVecSet(-hx, -hy, hz) + center;
    dtVec p2 = dtVecSet(-hx, hy, hz) + center;
    dtVec p3 = dtVecSet(-hx, hy, -hz) + center;
    dtVec p4 = dtVecSet(-hx, -hy, -hz) + center;

    dtVec p5 = dtVecSet(hx, -hy, hz) + center;
    dtVec p6 = dtVecSet(hx, hy, hz) + center;
    dtVec p7 = dtVecSet(hx, hy, -hz) + center;
    dtVec p8 = dtVecSet(hx, -hy, -hz) + center;

    // negative x loop
    m_lines->Draw(p1, p2, color);
    m_lines->Draw(p2, p3, color);
    m_lines->Draw(p3, p4, color);
    m_lines->Draw(p4, p1, color);

    // positive x loop
    m_lines->Draw(p5, p6, color);
    m_lines->Draw(p6, p7, color);
    m_lines->Draw(p7, p8, color);
    m_lines->Draw(p8, p5, color);

    // connect loops
    m_lines->Draw(p1, p5, color);
    m_lines->Draw(p2, p6, color);
    m_lines->Draw(p3, p7, color);
    m_lines->Draw(p4, p8, color);
}

//
void Draw::DrawSegment(dtVec point1, dtVec point2, const Color& color)
{
    m_lines->Draw(point1, point2, color);
}

//
void Draw::DrawPoint(dtVec point, float size, const Color& color)
{
    m_points->Draw(point, color, size);
}

void Draw::DrawAxes()
{
	m_lines->Draw(dtVec_Zero, dtVec_UnitX, Color(1.0f, 0.0f, 0.0f));
	m_lines->Draw(dtVec_Zero, dtVec_UnitY, Color(0.0f, 1.0f, 0.0f));
	m_lines->Draw(dtVec_Zero, dtVec_UnitZ, Color(0.0f, 0.0f, 1.0f));
}

//
void Draw::DrawString(int x, int y, const char* sz, ...)
{
	if (m_showUI == false)
	{
		return;
	}

    va_list arg;
    va_start(arg, sz);
	ImGui::Begin("Overlay", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar);
	ImGui::SetCursorPos(ImVec2(float(x), float(y)));
    ImGui::TextColoredV(ImColor(230, 153, 153, 255), sz, arg);
    ImGui::End();
    va_end(arg);
}

//
void Draw::DrawString(dtVec p, const Color& color, const char* sz, ...)
{
    float xs, ys;
    bool success = m_camera.ConvertWorldToScreen(xs, ys, p);
	if (success == false)
	{
		return;
	}

    ImVec2 ps(xs, ys);

    va_list arg;
    va_start(arg, sz);
	ImGui::Begin("Overlay", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar);
	ImGui::SetCursorPos(ps);
    ImGui::TextColoredV(ImColor(color.r, color.g, color.b, 1.0f), sz, arg);
    ImGui::End();
    va_end(arg);
}

//
void Draw::Flush()
{
    m_points->Flush();
    m_lines->Flush();
}
