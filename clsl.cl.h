#define CLSL_CL
#define CLSL_FAKE_VEC3

#define CLSL_INLINE inline

#define CLSL_CTOR(x) (x)
#define CLSL_CTOR_BEGIN(x) (x){
#define CLSL_CTOR_END }

#define CLSL_DEFINE_STRUCT(name) struct name; typedef struct name name; struct name
	
#define CLSL_OUT_PARAM(pt) pt*
#define CLSL_DEREF_OUT_PARAM(p) *(p)

#define CLSL_OR ||
#define CLSL_AND &&

typedef float2 vec2;
typedef float3 vec3;
typedef float4 vec4;

typedef int2 ivec2;
typedef int3 ivec3;
typedef int4 ivec4;

typedef uint2 uvec2;
typedef uint3 uvec3;
typedef uint4 uvec4;

typedef int2 bvec2;
typedef int3 bvec3;
typedef int4 bvec4;

CLSL_DEFINE_STRUCT(mat3) { float3 c[3]; };
CLSL_DEFINE_STRUCT(mat4) { float4 c[4]; };

#define int_from_bool(x) (convert_int(x) & 1)
#define int_from_bool2(x) (convert_int2(x) & 1)
#define int_from_bool3(x) (convert_int3(x) & 1)
#define int_from_bool4(x) (convert_int4(x) & 1)

#define uint_from_bool(x) (convert_uint(x) & 1)
#define uint_from_bool2(x) (convert_uint2(x) & 1)
#define uint_from_bool3(x) (convert_uint3(x) & 1)
#define uint_from_bool4(x) (convert_uint4(x) & 1)

#define float_from_bool(x) convert_float(int_from_bool(x))
#define float_from_bool2(x) convert_float2(int_from_bool2(x))
#define float_from_bool3(x) convert_float3(int_from_bool3(x))
#define float_from_bool4(x) convert_float4(int_from_bool4(x))

#define equal(a, b) ((a) == (b))
#define lessThan(a, b) ((a) < (b))
#define lessThanEqual(a, b) ((a) <= (b))
#define greaterThan(a, b) ((a) > (b))
#define greaterThanEqual(a, b) ((a) >= (b))