
CLSL_DEFINE_STRUCT(ray3) { float3 o; float3 d; };
CLSL_DEFINE_STRUCT(aabb3) { float3 min; float3 max; };

#define PI 3.14159265358979323846264f

#define saturate(x) clamp((x), 0.0f, 1.0f)
#define clampdot(v1, v2) saturate(dot((v1), (v2)))

#define epseq(x, y, eps) fabs((x) - (y)) < eps
#define nonneg3(x) all(lessThanEqual(CLSL_CTOR(float3)(0.0f), (x)))
#define nonneg3i(x) all(lessThanEqual(CLSL_CTOR(int3)(0), (x)))

CLSL_INLINE float sign1(float v) { return (v < 0.0f) ? -1.0f : 1.0f; }
CLSL_INLINE float2 sign2(float2 v) { return CLSL_CTOR(float2)(sign1(v.x), sign1(v.y)); }
CLSL_INLINE float3 sign3(float3 v) { return CLSL_CTOR(float3)(sign1(v.x), sign1(v.y), sign1(v.z)); }

CLSL_INLINE float mageps1(float v, float eps) { return v + (v < 0.0f ? -eps : eps); }
CLSL_INLINE float2 mageps2(float2 v, float eps) { return CLSL_CTOR(float2)(mageps1(v.x, eps), mageps1(v.y, eps)); }
CLSL_INLINE float3 mageps3(float3 v, float eps) { return CLSL_CTOR(float3)(mageps1(v.x, eps), mageps1(v.y, eps), mageps1(v.z, eps)); }

CLSL_INLINE float min2(float2 v) { return min(v.x, v.y); }
CLSL_INLINE float max2(float2 v) { return max(v.x, v.y); }
CLSL_INLINE float min3(float3 v) { return min(v.x, min(v.y, v.z)); }
CLSL_INLINE float max3(float3 v) { return max(v.x, max(v.y, v.z)); }
CLSL_INLINE float min4(float4 v) { return min(min(v.x, v.y), min(v.z, v.w)); }
CLSL_INLINE float max4(float4 v) { return max(max(v.x, v.y), max(v.z, v.w)); }

CLSL_INLINE float lengthsq3(float3 v) { return dot(v, v); }
CLSL_INLINE float3 normalize_eps3(float3 v, float eps) { return v / (length(v) + eps); }
CLSL_INLINE float3 coflip3(float3 n, float3 a) { return (dot(n, a) >= 0.0f) ? n : -n; }

CLSL_INLINE float3 transform3(mat3 matrix, float3 vector)
{
#ifdef CLSL_HAS_MATRIX_MULT
	return matrix * vector;
#else
	return (float3)(
		  dot((float3)(matrix.c[0].x, matrix.c[1].x, matrix.c[2].x), vector)
		, dot((float3)(matrix.c[0].y, matrix.c[1].y, matrix.c[2].y), vector)
		, dot((float3)(matrix.c[0].z, matrix.c[1].z, matrix.c[2].z), vector)
	);
#endif
}
CLSL_INLINE float4 transform4(mat4 matrix, float4 vector)
{
#ifdef CLSL_HAS_MATRIX_MULT
	return matrix * vector;
#else
	return (float4)(
		  dot((float4)(matrix.c[0].x, matrix.c[1].x, matrix.c[2].x, matrix.c[3].x), vector)
		, dot((float4)(matrix.c[0].y, matrix.c[1].y, matrix.c[2].y, matrix.c[3].y), vector)
		, dot((float4)(matrix.c[0].z, matrix.c[1].z, matrix.c[2].z, matrix.c[3].z), vector)
		, dot((float4)(matrix.c[0].w, matrix.c[1].w, matrix.c[2].w, matrix.c[3].w), vector)
	);
#endif
}
CLSL_INLINE float4 transform4div4(mat4 matrix, float4 vector)
{
	float4 r = transform4(matrix, vector);
	return CLSL_CTOR(float4)( CLSL_CTOR(float3)(r) / r.w, r.w );
}

CLSL_INLINE float2 viewportCoords(int2 screenPos, int2 screenDim)
{
	return (convert_float2(screenPos) + 0.5f) / convert_float2(screenDim) * 2.0f - 1.0f;
}

CLSL_INLINE ray3 rayFromVPI(float2 viewportCoords, mat4 viewProjInverse)
{
	float3 rayBase = CLSL_CTOR(float3)(transform4div4( viewProjInverse, CLSL_CTOR(float4)(viewportCoords, -16.0f, 1.0f) ));
	ray3 ray;
	ray.o = CLSL_CTOR(float3)(transform4div4( viewProjInverse, CLSL_CTOR(float4)(viewportCoords, -1.0f, 1.0f) ));
	ray.d = normalize(ray.o - rayBase);
	return ray;
}

CLSL_INLINE float rayIntersectAABB(ray3 ray, aabb3 box, CLSL_OUT_PARAM(float3) pointOut)
{
	float3 plus = float_from_bool3(lessThan(ray.o, box.min));
	float3 minus = float_from_bool3(lessThan(box.max,  ray.o));
	if (all(equal(plus + minus, CLSL_CTOR(vec3)(0.0f)))) {
		CLSL_DEREF_OUT_PARAM(pointOut) = ray.o;
		return 0.0f;
	}

	float3 delta = (plus * (box.min - ray.o) + minus * (box.max - ray.o)) / mageps3(ray.d, 1.0e-16f);
	if (any(lessThan(delta, CLSL_CTOR(vec3)(0.0f))))
		return -1.0f;

	float t = delta.x;
	int ta = 0;
	if (delta.y > delta.x && delta.y > delta.z) {
		t = delta.y;
		ta = 1;
	} else if (delta.z > delta.x) {
		t = delta.z;
		ta = 2;
	}

	float3 p = ray.o + (t + 0.0001f) * ray.d;

	if (all(equal(CLSL_CTOR(ivec3)(ta), CLSL_CTOR(ivec3)(0, 1, 2)) CLSL_OR lessThanEqual(box.min, p) CLSL_AND lessThanEqual(p, box.max)))
	{
		CLSL_DEREF_OUT_PARAM(pointOut) = p;
		return t;
	}
	else
		return -1.0f;
}

CLSL_INLINE float rayIntersectAABBBack(ray3 ray, aabb3 box, CLSL_OUT_PARAM(float3) pointOut)
{
	float3 plus = float_from_bool3(lessThan(float3(0.0f), ray.d));

	float3 delta = (mix(box.min, box.max, plus) - ray.o) / mageps3(ray.d, 1.0e-16f);
	if (any(lessThan(delta, CLSL_CTOR(vec3)(0.0f))))
		return -1.0f;

	float t = delta.x;
	int ta = 0;
	if (delta.y < delta.x && delta.y < delta.z) {
		t = delta.y;
		ta = 1;
	} else if (delta.z < delta.x) {
		t = delta.z;
		ta = 2;
	}

	float3 p = ray.o + (t + 0.0001f) * ray.d;

	if (all(equal(CLSL_CTOR(ivec3)(ta), CLSL_CTOR(ivec3)(0, 1, 2)) CLSL_OR lessThanEqual(box.min, p) CLSL_AND lessThanEqual(p, box.max)))
	{
		CLSL_DEREF_OUT_PARAM(pointOut) = p;
		return t;
	}
	else
		return -1.0f;
}
