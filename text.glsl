#version 330 core

#include <renderer.glsl.h>

layout(binding = 0, std140) uniform Camera
{
	CameraConstants camera;
};

#ifdef IN_VS

in uvec3 glyphDataIn;
out flat uvec3 glyphData;

void main()
{
	glyphData = glyphDataIn;
}

#endif

#ifdef IN_GS

layout(points) in;
layout(triangle_strip, max_vertices = 4) out;

in flat uvec3 glyphData[1];
out vec2 atlasPxF;

void main()
{
	ivec2 leftTopPx = ivec2(int(glyphData[0].x) & 0xffff, int(glyphData[0].x) >> 16);
	ivec2 atlasPx = ivec2(int(glyphData[0].y) & 0xffff, int(glyphData[0].y) >> 16);
	ivec2 sizePx = ivec2(int(glyphData[0].z) & 0xffff, int(glyphData[0].z) >> 16);
	ivec2 atlasEndPx = atlasPx + sizePx;
	ivec2 bottomRightPx = leftTopPx + sizePx;

	vec2 leftTop = vec2(leftTopPx) * camera.PixelWidth * vec2(2.0f, -2.0f) - vec2(1.0f, -1.0f);
	vec2 bottomRight = vec2(bottomRightPx) * camera.PixelWidth * vec2(2.0f, -2.0f) - vec2(1.0f, -1.0f);

	gl_Position = vec4(leftTop, 0.0, 1.0);
	atlasPxF = vec2(atlasPx);
	EmitVertex();
	gl_Position = vec4(bottomRight.x, leftTop.y, 0.0, 1.0);
	atlasPxF = vec2(atlasEndPx.x, atlasPx.y);
	EmitVertex();
	gl_Position = vec4(leftTop.x, bottomRight.y, 0.0, 1.0);
	atlasPxF = vec2(atlasPx.x, atlasEndPx.y);
	EmitVertex();
	gl_Position = vec4(bottomRight, 0.0, 1.0);
	atlasPxF = vec2(atlasEndPx);
	EmitVertex();
	EndPrimitive();
}

#endif

#ifdef IN_FS

in vec2 atlasPxF;

layout(binding = 0) uniform sampler2D fontCache;
layout(location = 0) out vec4 ldrColorOut;

void main()
{
	vec4 pxColor = texelFetch(fontCache, ivec2(atlasPxF), 0);
	pxColor.xyz *= 0.0f; // font color
	pxColor.w = 1.0f - pow(1.0f - pxColor.w, 1.5f);
	ldrColorOut = pxColor;
//	ldrColorOut = vec4(1.0f, 0.0f, 0.0f, 1.0f);
//	ldrColorOut.xyz /= (1.0f + dot(ldrColorOut.xyz, vec3(0.3f, 0.5f, 0.2f)));
}

#endif