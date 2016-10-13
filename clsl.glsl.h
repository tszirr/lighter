#define CLSL_GLSL
#define CLSL_HAS_MATRIX_MULT

#define CLSL_INLINE

#define CLSL_CTOR(x) x
#define CLSL_CTOR_BEGIN(x) x(
#define CLSL_CTOR_END )

#define CLSL_DEFINE_STRUCT(name) struct name

#define CLSL_INOUT(t) inout t
#define CLSL_OUT_PARAM(pt) out pt
#define CLSL_DEREF_OUT_PARAM(p) (p)

#define CLSL_OR ||
#define CLSL_AND &&

#define float2 vec2
#define float3 vec3
#define float4 vec4

#define int2 ivec2
#define int3 ivec3
#define int4 ivec4

#define uint2 uvec2
#define uint3 uvec3
#define uint4 uvec4

#define bool2 bvec2
#define bool3 bvec3
#define bool4 bvec4

#define convert_float(x) float(x)
#define convert_float2(x) vec2(x)
#define convert_float3(x) vec3(x)
#define convert_float4(x) vec4(x)

#define convert_int(x) int(x)
#define convert_int2(x) ivec2(x)
#define convert_int3(x) ivec3(x)
#define convert_int4(x) ivec4(x)

#define convert_uint(x) uint(x)
#define convert_uint2(x) uvec2(x)
#define convert_uint3(x) uvec3(x)
#define convert_uint4(x) uvec4(x)

#define int_from_bool(x) int(x)
#define int_from_bool2(x) ivec2(x)
#define int_from_bool3(x) ivec3(x)
#define int_from_bool4(x) ivec4(x)

#define uint_from_bool(x) uint(x)
#define uint_from_bool2(x) uvec2(x)
#define uint_from_bool3(x) uvec3(x)
#define uint_from_bool4(x) uvec4(x)

#define float_from_bool(x) float(x)
#define float_from_bool2(x) vec2(x)
#define float_from_bool3(x) vec3(x)
#define float_from_bool4(x) vec4(x)

#define fabs(x) abs(x)
#define fast_log2(x) log2(x)
#define fast_exp2(x) exp2(x)
#define copysign(x, y) (abs(x) * sign(y))