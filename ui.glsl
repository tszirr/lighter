#version 330 core

#include <renderer.glsl.h>

layout(std140) uniform Camera
{
	CameraConstants camera;
};

#ifdef IN_VS

layout(location = 0) in uvec4 widgetDataIn;
flat out uvec4 widgetData;

void main()
{
	widgetData = widgetDataIn;
}

#endif

#ifdef IN_GS

layout(points) in;
layout(triangle_strip, max_vertices = 4) out;

flat in uvec4 widgetData[1];
flat out uvec2 widgetClassEx;
out vec4 widgetPxF;
out vec2 widgetF;

void main()
{
	widgetClassEx = widgetData[0].zw;
	
	ivec2 coords1 = ivec2(int(widgetData[0].x) & 0xffff, int(widgetData[0].x) >> 16);
	ivec2 coords2 = ivec2(int(widgetData[0].y) & 0xffff, int(widgetData[0].y) >> 16);

	int lineWidthPx = int(widgetClassEx.x >> 24);

	if (lineWidthPx == 0)
	{
		ivec2 leftTopPx = coords1;
		ivec2 sizePx = coords2;
		ivec2 bottomRightPx = leftTopPx + sizePx;

		vec2 leftTop = vec2(leftTopPx) * camera.PixelWidth * vec2(2.0f, -2.0f) - vec2(1.0f, -1.0f);
		vec2 bottomRight = vec2(bottomRightPx) * camera.PixelWidth * vec2(2.0f, -2.0f) - vec2(1.0f, -1.0f);

		gl_Position = vec4(leftTop, 0.0, 1.0);
		widgetPxF = vec4(0.0f, 0.0f, vec2(sizePx));
		widgetF = vec2(0.0f);
		EmitVertex();
		gl_Position = vec4(bottomRight.x, leftTop.y, 0.0, 1.0);
		widgetPxF = vec4(float(sizePx.x), 0.0f, 0.0f, float(sizePx.y));
		widgetF = vec2(1.0f, 0.0f);
		EmitVertex();
		gl_Position = vec4(leftTop.x, bottomRight.y, 0.0, 1.0);
		widgetPxF = vec4(0.0f, float(sizePx.y), float(sizePx.x), 0.0f);
		widgetF = vec2(0.0f, 1.0f);
		EmitVertex();
		gl_Position = vec4(bottomRight, 0.0, 1.0);
		widgetPxF = vec4(vec2(sizePx), 0.0f, 0.0f);
		widgetF = vec2(1.0f, 1.0f);
		EmitVertex();
		EndPrimitive();
	}
	else
	{
		ivec2 leftTopPx = coords1;
		ivec2 bottomRightPx = coords2;

		vec2 leftTop = vec2(leftTopPx) * camera.PixelWidth * vec2(2.0f, -2.0f) - vec2(1.0f, -1.0f);
		vec2 bottomRight = vec2(bottomRightPx) * camera.PixelWidth * vec2(2.0f, -2.0f) - vec2(1.0f, -1.0f);

		vec2 dir = normalize(bottomRight - leftTop);
		vec2 dirOrtho = vec2(-dir.y, dir.x);

		float lengthPx = distance(vec2(leftTopPx), vec2(bottomRightPx));
		float radiusPx = float(lineWidthPx) * 0.5f;
		vec2 radiusOrtho = dirOrtho * radiusPx * camera.PixelWidth * 2.0f;

		gl_Position = vec4(leftTop - radiusOrtho, 0.0, 1.0);
		widgetPxF = vec4(-radiusPx, 0.0f, radiusPx, lengthPx);
		widgetF = vec2(0.0f);
		EmitVertex();
		gl_Position = vec4(leftTop + radiusOrtho, 0.0, 1.0);
		widgetPxF = vec4(radiusPx, 0.0f, -radiusPx, lengthPx);
		widgetF = vec2(1.0f, 0.0f);
		EmitVertex();
		gl_Position = vec4(bottomRight - radiusOrtho, 0.0, 1.0);
		widgetPxF = vec4(-radiusPx, lengthPx, radiusPx, 0.0f);
		widgetF = vec2(0.0f, 1.0f);
		EmitVertex();
		gl_Position = vec4(bottomRight + radiusOrtho, 0.0, 1.0);
		widgetPxF = vec4(radiusPx, lengthPx, -radiusPx, 0.0f);
		widgetF = vec2(1.0f, 1.0f);
		EmitVertex();
		EndPrimitive();
	}
}

#endif

#ifdef IN_FS

flat in uvec2 widgetClassEx;
in vec4 widgetPxF;
in vec2 widgetF;

layout(location = 0) out vec4 colorOut;

float min4(vec4 v) { return min(min(v.x, v.y), min(v.z, v.w)); }

vec3 fromHSV(float hue, float saturation, float value)
{
	float hue6 = hue * 6.0f;
	float minValue = 1.0f - saturation;

	vec3 color;
	color.r = mix(minValue, 1.0f, clamp(2.0f - abs(hue6 < 3.0f ? hue6 : hue6 - 6.0f), 0.0f, 1.0f));
	color.g = mix(minValue, 1.0f, clamp(2.0f - abs(hue6 - 2.0f), 0.0f, 1.0f));
	color.b = mix(minValue, 1.0f, clamp(2.0f - abs(hue6 - 4.0f), 0.0f, 1.0f));

	// "Linear HSV"
	color *= color;

	color *= value;

	return color;
}

void main()
{
	float barGray = 0.2f;
	float borderGray = 0.1f;

	float borderDist = min4(widgetPxF);
	vec4 color = vec4(1.0f, 0.0f, 1.0f, 1.0f);
	uint classID = widgetClassEx.x & 0xffU;
	// Bar
	if (classID == 1U)
		color.xyz = vec3(barGray);
	// Ticks
	else if (classID == 2U)
	{
		color.xyz = vec3(0.0f);
		float tickDelta = uintBitsToFloat(widgetClassEx.y);
		float tickDist = abs(mod(widgetPxF.x + 0.5f * tickDelta, tickDelta) - 0.5f * tickDelta);
		color.w = (tickDist <= 0.5f) ? 0.9f : 0.0f; //  || widgetPxF.w <= 1.0f && 2.5f <= tickDist && tickDist < 3.5f
	}
	// Lines
	else if (classID == 3U)
		color.xyz = vec3(barGray);
	else if (classID == 4U)
	{
		color.xyz = vec3(barGray);
		color.w = fract(gl_FragCoord.y / 6.0f) > 0.5f ? 0.5f : 1.0f;
		color.xyz *= color.w;
	}
	// Frames
	else
	{
		if (borderDist <= 1.0f)
			color.xyz = vec3(borderGray);
		else
		{
			// Hue
			if (classID == 5U)
			{
				float hue = widgetF.x + uintBitsToFloat(widgetClassEx.y);
				float value = float((widgetClassEx.x >> 16U) & 0xffU) / 255.0f;
				float saturation = float((widgetClassEx.x >> 8U) & 0xffU) / 255.0f;
				color.rgb = fromHSV(hue, saturation, value);
			}
			// Saturation
			else if (classID == 6U)
			{
				float hue = uintBitsToFloat(widgetClassEx.y);
				float value = float((widgetClassEx.x >> 16U) & 0xffU) / 255.0f;
				float saturation = widgetF.x;
				color.rgb = fromHSV(hue, saturation, value);
			}
			// Value
			else if (classID == 7U)
			{
				float hue = uintBitsToFloat(widgetClassEx.y);
				float value = widgetF.x;
				float saturation = float((widgetClassEx.x >> 8U) & 0xffU) / 255.0f;
				color.rgb = fromHSV(hue, saturation, value);
			}
			else
				color.xyz = vec3(1.0f);
		}
	}
	colorOut = color;
}

#endif