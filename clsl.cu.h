#define CLSL_CUDA
#define CLSL_HAS_MATRIX_MULT

#define CLSL_INLINE inline __device__

#define CLSL_CTOR(x) x
#define CLSL_CTOR_BEGIN(x) x(
#define CLSL_CTOR_END )

#define CLSL_DEFINE_STRUCT(name) struct name
	
#define CLSL_OUT_PARAM(pt) pt*
#define CLSL_DEREF_OUT_PARAM(p) *(p)

#define CLSL_OR |
#define CLSL_AND &

#include <cuda.h>
#define MATHX_MINIMAL
#define GLM_NO_ASSERT
#include "mathx"

namespace clsl
{
	using namespace glm;

	typedef vec2 float2;
	typedef vec3 float3;
	typedef vec4 float4;

	typedef ivec2 int2;
	typedef ivec3 int3;
	typedef ivec4 int4;

	typedef uvec2 uint2;
	typedef uvec3 uint3;
	typedef uvec4 uint4;

	typedef bvec2 bool2;
	typedef bvec3 bool3;
	typedef bvec4 bool4;

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

	template <class T>
	CLSL_INLINE auto fabs(T const& v) -> decltype(abs(v)) { return abs(v); }

} // namespace
