#version 330 core

#include <renderer.glsl.h>

layout(binding = 0, std140) uniform Camera
{
	CameraConstants camera;
};

#ifdef IN_VS

in uvec3 widgetDataIn;
out flat uvec3 widgetData;

void main()
{
	widgetData = widgetDataIn;
}

#endif

#ifdef IN_GS

layout(points) in;
layout(triangle_strip, max_vertices = 4) out;

in flat uvec3 widgetData[1];
out flat uint widgetClass;
out vec4 widgetPxF;

void main()
{
	ivec2 leftTopPx = ivec2(int(widgetData[0].x) & 0xffff, int(widgetData[0].x) >> 16);
	ivec2 sizePx = ivec2(int(widgetData[0].y) & 0xffff, int(widgetData[0].y) >> 16);
	ivec2 bottomRightPx = leftTopPx + sizePx;

	vec2 leftTop = vec2(leftTopPx) * camera.PixelWidth * vec2(2.0f, -2.0f) - vec2(1.0f, -1.0f);
	vec2 bottomRight = vec2(bottomRightPx) * camera.PixelWidth * vec2(2.0f, -2.0f) - vec2(1.0f, -1.0f);

	widgetClass = widgetData[0].z;
	
	gl_Position = vec4(leftTop, 0.0, 1.0);
	widgetPxF = vec4(0.0f, 0.0f, vec2(sizePx));
	EmitVertex();
	gl_Position = vec4(bottomRight.x, leftTop.y, 0.0, 1.0);
	widgetPxF = vec4(float(sizePx.x), 0.0f, 0.0f, float(sizePx.y));
	EmitVertex();
	gl_Position = vec4(leftTop.x, bottomRight.y, 0.0, 1.0);
	widgetPxF = vec4(0.0f, float(sizePx.y), float(sizePx.x), 0.0f);
	EmitVertex();
	gl_Position = vec4(bottomRight, 0.0, 1.0);
	widgetPxF = vec4(vec2(sizePx), 0.0f, 0.0f);
	EmitVertex();
	EndPrimitive();
}

#endif

#ifdef IN_FS

in flat uint widgetClass;
in vec4 widgetPxF;

layout(location = 0) out vec4 colorOut;

float min4(vec4 v) { return min(min(v.x, v.y), min(v.z, v.w)); }

void main()
{
	float borderDist = min4(widgetPxF);
	vec4 color = vec4(1.0f);
	if (widgetClass == 1)
		color.xyz = vec3(0.2f);
	else
		color.xyz = (borderDist <= 1.0f) ? vec3(0.1f) : vec3(1.0f);
	colorOut = color;
}

#endif