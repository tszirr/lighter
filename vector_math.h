#pragma once

#ifndef SML_API
	#ifdef __CUDACC__
		#define SML_API __host__ __device__
	#else
		#define SML_API
	#endif
#endif

#ifndef SML_MAKE_COMPATIBILITY_CASTS
	// allow for implicit casts from sml vec. Use MAKE_COMPATIBILITY_CAST(type) to add your own type (mapping).
	#define SML_MAKE_COMPATIBILITY_CASTS() SML_MAKE_CUDA_CAST()
#endif

#ifdef _MSC_VER
	#if defined(__CUDACC__) // no C++11 today
		#define SML_TRIVIAL_UNIONS
	#elif !defined(__clang__)
		#define SML_MSVC_LEGACY
		#define SML_MSVC_WORKAROUNDS
	#endif
#endif

namespace sml
{
	template <class T> SML_API inline T min(T a, T b) { return (a < b || !(b == b)) ? a : b; } // min by-value w/ preference for non-NaN
	template <class T> SML_API inline T max(T a, T b) { return (a > b || !(b == b)) ? a : b; } // max by-value w/ preference for non-NaN
	
	template <class T> SML_API inline T pi() { return T(3.14159265359); }
	template <class T> SML_API inline T radians(T deg) { return T(3.14159265359 / 180.0) * deg; }
	template <class T> SML_API inline T degrees(T rad) { return T(180.0 / 3.14159265359) * rad; }
	
	typedef unsigned uint;

	using std::abs;
	using std::sqrt;
	using std::log;
	using std::exp;
	using std::pow;
	using std::floor;
	using std::ceil;
	using std::signbit;
	using std::copysign;
	template <class T> SML_API inline T fract(T x) { return x - floor(x); }
	
	template <class T> SML_API inline T sincos(T a, T* s, T* c) { *s = sin(a); *c = cos(a); }

	template <class A, class B, class C>
	SML_API inline auto clamp(A const& v, B const& min_v, C const& max_v) -> decltype(min(max(v, min_v), max_v)) { return min(max(v, min_v), max_v); }
	template <class A, class B, class C>
	SML_API inline auto lerp(A const& min, B const& max, C const& blend) -> decltype(min + (max - min) * blend) { return min + (max - min) * blend; }
	template <class A, class B, class C>
	SML_API inline auto mix(A const& min, B const& max, C const& blend) -> decltype(min + (max - min) * blend) { return min + (max - min) * blend; }

	template <class A, class B>
	SML_API inline auto greaterThan(A const& a, B const& b) -> decltype(lessThan(b, a)) { return lessThan(b, a); }
	template <class A, class B>
	SML_API inline auto greaterThanEqual(A const& a, B const& b) -> decltype(lessThanEqual(b, a)) { return lessThanEqual(b, a); }
	
	// vector types
	template <class T, int N> struct vec;
	template <class T, int C, int R = C> struct mat;
#ifdef SML_MSVC_LEGACY
	template <class T, int N> struct pod_vec;
	#define SML_POD_VEC pod_vec
#else
	template <class T, int N> using pod_vec = vec<T, N>;
	#define SML_POD_VEC vec
#endif
	
	// vector type analysis
	template <class Vec> struct vec_type;
	template <class T, int N> struct vec_type< vec<T, N> > { static int const dimension = N; typedef T component; };
#ifdef SML_MSVC_LEGACY
	template <class T, int N> struct vec_type< pod_vec<T, N> > { static int const dimension = N; typedef T component; };
#endif	
	// vector type compatibility
	template <class T, int N, class V, bool NEqual = (N == vec_type<V>::dimension)>
	struct is_compatible_vec_type { static bool const value = false; };
	template <class V, int N>
	struct is_compatible_vec_type<typename vec_type<V>::component, N, V, true> { typedef void type; static bool const value = true; };

#if defined(__CUDACC__) && defined(__VECTOR_TYPES_H__)
	// from/to cuda matching
	template <> struct vec_type< ::float1 > { static int const dimension = 1; typedef float component; };
	template <> struct vec_type< ::float2 > { static int const dimension = 2; typedef float component; };
	template <> struct vec_type< ::float3 > { static int const dimension = 3; typedef float component; };
	template <> struct vec_type< ::float4 > { static int const dimension = 4; typedef float component; };

	template <> struct vec_type< ::int1 > { static int const dimension = 1; typedef int component; };
	template <> struct vec_type< ::int2 > { static int const dimension = 2; typedef int component; };
	template <> struct vec_type< ::int3 > { static int const dimension = 3; typedef int component; };
	template <> struct vec_type< ::int4 > { static int const dimension = 4; typedef int component; };

	template <> struct vec_type< ::ushort1 > { static int const dimension = 1; typedef unsigned short component; };
	template <> struct vec_type< ::ushort2 > { static int const dimension = 2; typedef unsigned short component; };
	template <> struct vec_type< ::ushort3 > { static int const dimension = 3; typedef unsigned short component; };
	template <> struct vec_type< ::ushort4 > { static int const dimension = 4; typedef unsigned short component; };

	template <> struct vec_type< ::uint1 > { static int const dimension = 1; typedef unsigned int component; };
	template <> struct vec_type< ::uint2 > { static int const dimension = 2; typedef unsigned int component; };
	template <> struct vec_type< ::uint3 > { static int const dimension = 3; typedef unsigned int component; };
	template <> struct vec_type< ::uint4 > { static int const dimension = 4; typedef unsigned int component; };

	template <class T, int N> struct cuda_type { struct unknown; typedef unknown type; };
	template <> struct cuda_type<float, 1> { typedef ::float1 type; };
	template <> struct cuda_type<float, 2> { typedef ::float2 type; };
	template <> struct cuda_type<float, 3> { typedef ::float3 type; };
	template <> struct cuda_type<float, 4> { typedef ::float4 type; };

	template <> struct cuda_type<int, 1> { typedef ::int1 type; };
	template <> struct cuda_type<int, 2> { typedef ::int2 type; };
	template <> struct cuda_type<int, 3> { typedef ::int3 type; };
	template <> struct cuda_type<int, 4> { typedef ::int4 type; };

	template <> struct cuda_type<unsigned short, 1> { typedef ::ushort1 type; };
	template <> struct cuda_type<unsigned short, 2> { typedef ::ushort2 type; };
	template <> struct cuda_type<unsigned short, 3> { typedef ::ushort3 type; };
	template <> struct cuda_type<unsigned short, 4> { typedef ::ushort4 type; };

	template <> struct cuda_type<unsigned, 1> { typedef ::uint1 type; };
	template <> struct cuda_type<unsigned, 2> { typedef ::uint2 type; };
	template <> struct cuda_type<unsigned, 3> { typedef ::uint3 type; };
	template <> struct cuda_type<unsigned, 4> { typedef ::uint4 type; };

	template <class T> SML_API inline T const& to_cuda(T const& v) { return v; }
	template <class T, int N> SML_API inline typename cuda_type<T, N>::type const& to_cuda(vec<T, N> const& v) { return v; }
#ifdef SML_MSVC_LEGACY
	template <class T, int N> SML_API inline typename cuda_type<T, N>::type const& to_cuda(pod_vec<T, N> const& v) { return v; }
#endif
	#define SML_MAKE_CUDA_CAST() MAKE_COMPATIBILITY_CAST(typename cuda_type<component, dimension>::type)
#else
	#define SML_MAKE_CUDA_CAST() 
#endif

	#define MAKE_COMPATIBILITY_CAST(...) \
		SML_API operator __VA_ARGS__&() { return *reinterpret_cast<__VA_ARGS__*>(reinterpret_cast<T*>(this)); } \
		SML_API operator __VA_ARGS__ const&() const { return *reinterpret_cast<__VA_ARGS__ const*>(reinterpret_cast<T const*>(this)); }

	template <class T> struct id { typedef T t; };
	template <int Stride> struct stride_literal { static int const value = Stride; };
	template <bool Condition> struct enable_if;
	template <> struct enable_if<true> { typedef void type; };

#ifdef SML_MSVC_LEGACY
	template <class T>
	struct pod_vec<T, 0>;
#endif
	template <class T>
	struct vec<T, 0>;

	// vector storage (no default initialization -> aggregate / value initialization)
	template <class T>
	struct SML_POD_VEC<T, 1>
	{
		typedef T component;
		static int const dimension = 1;

		union
		{
			T x;
			T r;
			component c[dimension];
		};
		
		typedef vec<bool, dimension> bvec;
		typedef vec<int , dimension> ivec;
#ifdef SML_MSVC_LEGACY
		typedef vec<T, dimension> vec;
#endif
		
		SML_API T& operator [](int i) { return c[i]; }
		SML_API T  operator [](int i) const { return c[i]; }

#ifdef SML_MSVC_LEGACY
		SML_API SML_POD_VEC& operator = (vec const& r) { return *this = (pod_vec&) r; }
#endif
		SML_API SML_POD_VEC& operator +=(vec const& r) { x += r.x; return *this; }
		SML_API SML_POD_VEC& operator -=(vec const& r) { x -= r.x; return *this; }
		SML_API SML_POD_VEC& operator *=(vec const& r) { x *= r.x; return *this; }
		SML_API SML_POD_VEC& operator /=(vec const& r) { x /= r.x; return *this; }
		SML_API SML_POD_VEC& operator %=(vec const& r) { x %= r.x; return *this; }
		SML_API SML_POD_VEC& operator &=(vec const& r) { x &= r.x; return *this; }
		SML_API SML_POD_VEC& operator |=(vec const& r) { x |= r.x; return *this; }
		SML_API SML_POD_VEC& operator ^=(vec const& r) { x ^= r.x; return *this; }
		
		SML_API SML_POD_VEC& operator *=(T r) { x *= r; return *this; }
		SML_API SML_POD_VEC& operator /=(T r) { x /= r; return *this; }
		
		SML_API friend vec operator + (vec const& l, vec const& r) { vec s; s.x = l.x +  r.x; return s; }
		SML_API friend vec operator - (vec const& l, vec const& r) { vec s; s.x = l.x -  r.x; return s; }
		SML_API friend vec operator * (vec const& l, vec const& r) { vec s; s.x = l.x *  r.x; return s; }
		SML_API friend vec operator / (vec const& l, vec const& r) { vec s; s.x = l.x /  r.x; return s; }
		SML_API friend vec operator % (vec const& l, vec const& r) { vec s; s.x = l.x %  r.x; return s; }
		SML_API friend vec operator & (vec const& l, vec const& r) { vec s; s.x = l.x &  r.x; return s; }
		SML_API friend vec operator | (vec const& l, vec const& r) { vec s; s.x = l.x |  r.x; return s; }
		SML_API friend vec operator ^ (vec const& l, vec const& r) { vec s; s.x = l.x ^  r.x; return s; }
		SML_API friend vec operator <<(vec const& l, vec const& r) { vec s; s.x = l.x << r.x; return s; }
		SML_API friend vec operator >>(vec const& l, vec const& r) { vec s; s.x = l.x >> r.x; return s; }

		SML_API friend vec  operator -(SML_POD_VEC const& l) { vec s;  s.x = -l.x; return s; }
		SML_API friend vec  operator ~(SML_POD_VEC const& l) { vec s;  s.x = ~l.x; return s; }
		SML_API friend bvec operator !(SML_POD_VEC const& l) { bvec s; s.x = !l.x; return s; }

		SML_API friend vec operator +(T l, vec const& r) { vec s; s.x = l + r.x; return s; }
		SML_API friend vec operator -(T l, vec const& r) { vec s; s.x = l - r.x; return s; }
		SML_API friend vec operator *(T l, vec const& r) { vec s; s.x = l * r.x; return s; }
		SML_API friend vec operator /(T l, vec const& r) { vec s; s.x = l / r.x; return s; }
		
		SML_API friend vec operator +(vec const& l, T r) { vec s; s.x = l.x + r; return s; }
		SML_API friend vec operator -(vec const& l, T r) { vec s; s.x = l.x - r; return s; }
		SML_API friend vec operator *(vec const& l, T r) { vec s; s.x = l.x * r; return s; }
		SML_API friend vec operator /(vec const& l, T r) { vec s; s.x = l.x / r; return s; }

		SML_API friend T dot(vec const& l, vec const& r) { return l.x * r.x; }
		template <int S>
		SML_API static T dot_transposed(T const* l, stride_literal<S>, vec const& r) { return l[0] * r.x; }
		template <int S>
		SML_API static vec transpose(T const* t, stride_literal<S>) { vec s; s.x = t[0]; return s; }

#ifdef SML_MSVC_LEGACY
		SML_API friend vec min(pod_vec const& l, pod_vec const& r) { vec s; s.x = min(l.x, r.x); return s; }
		SML_API friend vec max(pod_vec const& l, pod_vec const& r) { vec s; s.x = max(l.x, r.x); return s; }
#endif
		SML_API friend vec min(vec const& l, vec const& r) { vec s; s.x = min(l.x, r.x); return s; }
		SML_API friend vec max(vec const& l, vec const& r) { vec s; s.x = max(l.x, r.x); return s; }
		SML_API friend vec pow(vec const& l, vec const& r) { vec s; s.x = pow(l.x, r.x); return s; }
		SML_API friend vec log(SML_POD_VEC const& l) { vec s; s.x = log(l.x); return s; }
		SML_API friend vec exp(SML_POD_VEC const& l) { vec s; s.x = exp(l.x); return s; }
		SML_API friend vec abs(SML_POD_VEC const& l) { vec s; s.x = abs(l.x); return s; }
		SML_API friend vec sqrt (SML_POD_VEC const& l) { vec s; s.x = sqrt (l.x); return s; }
		SML_API friend vec floor(SML_POD_VEC const& l) { vec s; s.x = floor(l.x); return s; }
		SML_API friend vec ceil (SML_POD_VEC const& l) { vec s; s.x = ceil (l.x); return s; }
		
		SML_API friend ivec signbit(SML_POD_VEC const& l) { ivec s; s.x = signbit(l.x); return s; }
		SML_API friend vec copysign(SML_POD_VEC const& m, SML_POD_VEC const& s) { vec r; r.x = copysign(m.x, s.x); return r; }

		SML_API friend vec mix(vec const& l, vec const& r, bvec const& p) { vec s; s.x = p.x ? r.x : l.x; return s; }
		
		SML_API friend T compMin(SML_POD_VEC const& l) { return l.x; }
		SML_API friend T compMax(SML_POD_VEC const& l) { return l.x; }
		SML_API friend bool all(SML_POD_VEC const& l) { return !!l.x; }
		SML_API friend bool any(SML_POD_VEC const& l) { return !!l.x; }

		SML_API friend bool operator ==(vec const& l, vec const& r) { return l.x == r.x; }
		SML_API friend bool operator !=(vec const& l, vec const& r) { return l.x != r.x; }
		
		SML_API friend bvec lessThan(vec const& l, vec const& r)      { bvec s; s.x = (l.x <  r.x); return s; }
		SML_API friend bvec lessThanEqual(vec const& l, vec const& r) { bvec s; s.x = (l.x <= r.x); return s; }
		SML_API friend bvec equal(vec const& l, vec const& r)         { bvec s; s.x = (l.x == r.x); return s; }
		SML_API friend bvec notEqual(vec const& l, vec const& r)      { bvec s; s.x = (l.x != r.x); return s; }

		template <class Op>
		SML_API friend vec cw(Op&& op, vec const& l) { vec s; s.x = op(l.x); return s; }
		template <class Op>
		SML_API friend vec cw(Op&& op, vec const& l, vec const& r) { vec s; s.x = op(l.x, r.x); return s; }

#ifdef SML_MSVC_LEGACY
		SML_API operator vec&() { return *reinterpret_cast<vec*>(reinterpret_cast<T*>(this)); }
		SML_API operator vec const&() const { return *reinterpret_cast<vec const*>(reinterpret_cast<T const*>(this)); }
#endif
		SML_MAKE_COMPATIBILITY_CASTS()

#ifdef SML_MSVC_LEGACY
	};
	// vector (no-op default initialization)
	template <class T>
	struct vec<T, 1> : pod_vec<T, 1>
	{
#endif
#ifdef SML_MSVC_LEGACY
		SML_API vec() { }
#else
		SML_API vec() = default;
#endif
		SML_API vec(T x) { this->x = x; }
		SML_API vec(vec<T, 0> const& _, T x) { this->x = x; } // required by matrix initialization w/ dehom columns
		
		template <class T2, int N2>
		SML_API explicit vec(SML_POD_VEC<T2, N2> const& v) { this->x = T(v.c[0]); }
		template <int Stride>
		SML_API vec(T const* c, stride_literal<Stride>) { this->x = c[0]; }

		template <class V>
		SML_API vec(V const& v, typename is_compatible_vec_type<T, 1, V>::type *assert_compatible_vec = 0)
		{ *this = *reinterpret_cast<vec const*>(&reinterpret_cast<T const&>(v)); }

		template <int C>
		SML_API static vec base(T x = T(1), T zero = T())
		{ vec s; s.x = (0 == C) ? x : zero; return s; }
		SML_API static vec base(int C, T x = T(1), T zero = T())
		{ vec s; s.x = (0 == C) ? x : zero; return s; }
		
#ifdef SML_MSVC_WORKAROUNDS
		// MSVC++ treats operators specially and requires them to be re-declared here to be instantiated in all cases
		SML_API friend vec operator + (vec const& l, vec const& r);
		SML_API friend vec operator - (vec const& l, vec const& r);
		SML_API friend vec operator * (vec const& l, vec const& r);
		SML_API friend vec operator / (vec const& l, vec const& r);
		SML_API friend vec operator % (vec const& l, vec const& r);
		SML_API friend vec operator & (vec const& l, vec const& r);
		SML_API friend vec operator | (vec const& l, vec const& r);
		SML_API friend vec operator ^ (vec const& l, vec const& r);
		SML_API friend vec operator <<(vec const& l, vec const& r);
		SML_API friend vec operator >>(vec const& l, vec const& r);
		
		SML_API friend vec  operator -(pod_vec const& l);
		SML_API friend vec  operator ~(pod_vec const& l);
		SML_API friend typename vec::bvec operator !(pod_vec const& l);
		
		SML_API friend vec operator +(T l, vec const& r);
		SML_API friend vec operator -(T l, vec const& r);
		SML_API friend vec operator *(T l, vec const& r);
		SML_API friend vec operator /(T l, vec const& r);
		
		SML_API friend vec operator +(vec const& l, T r);
		SML_API friend vec operator -(vec const& l, T r);
		SML_API friend vec operator *(vec const& l, T r);
		SML_API friend vec operator /(vec const& l, T r);
#endif
	};
	// vector storage (no default initialization -> aggregate / value initialization)
	template <class T>
	struct SML_POD_VEC<T, 2>
	{
		typedef T component;
		static int const dimension = 2;

		union
		{
			struct { T x, y; };
			struct { T r, g; };
			component c[dimension];
		};
		
		typedef vec<bool, dimension> bvec;
		typedef vec<int , dimension> ivec;
#ifdef SML_MSVC_LEGACY
		typedef vec<T, dimension> vec;
#endif

		SML_API T& operator [](int i) { return c[i]; }
		SML_API T  operator [](int i) const { return c[i]; }

#ifdef SML_MSVC_LEGACY
		SML_API SML_POD_VEC& operator = (vec const& r) { return *this = (pod_vec&) r; }
#endif
		SML_API SML_POD_VEC& operator +=(vec const& r) { x += r.x; y += r.y; return *this; }
		SML_API SML_POD_VEC& operator -=(vec const& r) { x -= r.x; y -= r.y; return *this; }
		SML_API SML_POD_VEC& operator *=(vec const& r) { x *= r.x; y *= r.y; return *this; }
		SML_API SML_POD_VEC& operator /=(vec const& r) { x /= r.x; y /= r.y; return *this; }
		SML_API SML_POD_VEC& operator %=(vec const& r) { x %= r.x; y %= r.y; return *this; }
		SML_API SML_POD_VEC& operator &=(vec const& r) { x &= r.x; y &= r.y; return *this; }
		SML_API SML_POD_VEC& operator |=(vec const& r) { x |= r.x; y |= r.y; return *this; }
		SML_API SML_POD_VEC& operator ^=(vec const& r) { x ^= r.x; y ^= r.y; return *this; }

		SML_API SML_POD_VEC& operator *=(T r) { x *= r; y *= r; return *this; }
		SML_API SML_POD_VEC& operator /=(T r) { x /= r; y /= r; return *this; }
		
		SML_API friend vec operator + (vec const& l, vec const& r) { vec s; s.x = l.x +  r.x; s.y = l.y +  r.y; return s; }
		SML_API friend vec operator - (vec const& l, vec const& r) { vec s; s.x = l.x -  r.x; s.y = l.y -  r.y; return s; }
		SML_API friend vec operator * (vec const& l, vec const& r) { vec s; s.x = l.x *  r.x; s.y = l.y *  r.y; return s; }
		SML_API friend vec operator / (vec const& l, vec const& r) { vec s; s.x = l.x /  r.x; s.y = l.y /  r.y; return s; }
		SML_API friend vec operator % (vec const& l, vec const& r) { vec s; s.x = l.x %  r.x; s.y = l.y %  r.y; return s; }
		SML_API friend vec operator & (vec const& l, vec const& r) { vec s; s.x = l.x &  r.x; s.y = l.y &  r.y; return s; }
		SML_API friend vec operator | (vec const& l, vec const& r) { vec s; s.x = l.x |  r.x; s.y = l.y |  r.y; return s; }
		SML_API friend vec operator ^ (vec const& l, vec const& r) { vec s; s.x = l.x ^  r.x; s.y = l.y ^  r.y; return s; }
		SML_API friend vec operator <<(vec const& l, vec const& r) { vec s; s.x = l.x << r.x; s.y = l.y << r.y; return s; }
		SML_API friend vec operator >>(vec const& l, vec const& r) { vec s; s.x = l.x >> r.x; s.y = l.y >> r.y; return s; }

		SML_API friend vec  operator -(SML_POD_VEC const& l) { vec s;  s.x = -l.x; s.y = -l.y; return s; }
		SML_API friend vec  operator ~(SML_POD_VEC const& l) { vec s;  s.x = ~l.x; s.y = ~l.y; return s; }
		SML_API friend bvec operator !(SML_POD_VEC const& l) { bvec s; s.x = !l.x; s.y = !l.y; return s; }

		SML_API friend vec operator +(T l, vec const& r) { vec s; s.x = l + r.x; s.y = l + r.y; return s; }
		SML_API friend vec operator -(T l, vec const& r) { vec s; s.x = l - r.x; s.y = l - r.y; return s; }
		SML_API friend vec operator *(T l, vec const& r) { vec s; s.x = l * r.x; s.y = l * r.y; return s; }
		SML_API friend vec operator /(T l, vec const& r) { vec s; s.x = l / r.x; s.y = l / r.y; return s; }
		
		SML_API friend vec operator +(vec const& l, T r) { vec s; s.x = l.x + r; s.y = l.y + r; return s; }
		SML_API friend vec operator -(vec const& l, T r) { vec s; s.x = l.x - r; s.y = l.y - r; return s; }
		SML_API friend vec operator *(vec const& l, T r) { vec s; s.x = l.x * r; s.y = l.y * r; return s; }
		SML_API friend vec operator /(vec const& l, T r) { vec s; s.x = l.x / r; s.y = l.y / r; return s; }

		SML_API friend T dot(vec const& l, vec const& r) { return l.x * r.x + l.y * r.y; }
		template <int S>
		SML_API static T dot_transposed(T const* l, stride_literal<S>, vec const& r) { return l[0] * r.x + l[S] * r.y; }
		template <int S>
		SML_API static vec transpose(T const* t, stride_literal<S>) { vec s; s.x = t[0]; s.y = t[S]; return s; }
		
#ifdef SML_MSVC_LEGACY
		SML_API friend vec min(pod_vec const& l, pod_vec const& r) { vec s; s.x = min(l.x, r.x); s.y = min(l.y, r.y); return s; }
		SML_API friend vec max(pod_vec const& l, pod_vec const& r) { vec s; s.x = max(l.x, r.x); s.y = max(l.y, r.y); return s; }
#endif
		SML_API friend vec min(vec const& l, vec const& r) { vec s; s.x = min(l.x, r.x); s.y = min(l.y, r.y); return s; }
		SML_API friend vec max(vec const& l, vec const& r) { vec s; s.x = max(l.x, r.x); s.y = max(l.y, r.y); return s; }
		SML_API friend vec pow(vec const& l, vec const& r) { vec s; s.x = pow(l.x, r.x); s.y = pow(l.y, r.y); return s; }
		SML_API friend vec abs(SML_POD_VEC const& l) { vec s; s.x = abs(l.x); s.y = abs(l.y); return s; }
		SML_API friend vec log(SML_POD_VEC const& l) { vec s; s.x = log(l.x); s.y = log(l.y); return s; }
		SML_API friend vec exp(SML_POD_VEC const& l) { vec s; s.x = exp(l.x); s.y = exp(l.y); return s; }
		SML_API friend vec sqrt (SML_POD_VEC const& l) { vec s; s.x = sqrt (l.x); s.y = sqrt (l.y); return s; }
		SML_API friend vec floor(SML_POD_VEC const& l) { vec s; s.x = floor(l.x); s.y = floor(l.y); return s; }
		SML_API friend vec ceil (SML_POD_VEC const& l) { vec s; s.x = ceil (l.x); s.y = ceil (l.y); return s; }

		SML_API friend ivec signbit(SML_POD_VEC const& l) { ivec s; s.x = signbit(l.x); s.y = signbit(l.y); return s; }
		SML_API friend vec copysign(SML_POD_VEC const& m, SML_POD_VEC const& s) { vec r; r.x = copysign(m.x, s.x); r.y = copysign(m.y, s.y); return r; }

		SML_API friend vec mix(vec const& l, vec const& r, bvec const& p) { vec s; s.x = p.x ? r.x : l.x; s.y = p.y ? r.y : l.y; return s; }
		
		SML_API friend T compMin(SML_POD_VEC const& l) { return min(l.x, l.y); }
		SML_API friend T compMax(SML_POD_VEC const& l) { return max(l.x, l.y); }
		SML_API friend bool all(SML_POD_VEC const& l) { return !!l.x & !!l.y; }
		SML_API friend bool any(SML_POD_VEC const& l) { return !!l.x | !!l.y; }

		SML_API friend bool operator ==(vec const& l, vec const& r) { return l.x == r.x && l.y == r.y; }
		SML_API friend bool operator !=(vec const& l, vec const& r) { return l.x != r.x || l.y != r.y; }
		
		SML_API friend bvec lessThan(vec const& l, vec const& r)      { bvec s; s.x = (l.x <  r.x); s.y = (l.y <  r.y); return s; }
		SML_API friend bvec lessThanEqual(vec const& l, vec const& r) { bvec s; s.x = (l.x <= r.x); s.y = (l.y <= r.y); return s; }
		SML_API friend bvec equal(vec const& l, vec const& r)         { bvec s; s.x = (l.x == r.x); s.y = (l.y == r.y); return s; }
		SML_API friend bvec notEqual(vec const& l, vec const& r)      { bvec s; s.x = (l.x != r.x); s.y = (l.y != r.y); return s; }

		template <class Op>
		SML_API friend vec cw(Op&& op, vec const& l) { vec s; s.x = op(l.x); s.y = op(l.y); return s; }
		template <class Op>
		SML_API friend vec cw(Op&& op, vec const& l, vec const& r) { vec s; s.x = op(l.x, r.x); s.y = op(l.y, r.y); return s; }

#ifdef SML_MSVC_LEGACY
		SML_API operator vec&() { return *reinterpret_cast<vec*>(reinterpret_cast<T*>(this)); }
		SML_API operator vec const&() const { return *reinterpret_cast<vec const*>(reinterpret_cast<T const*>(this)); }
#endif
		SML_MAKE_COMPATIBILITY_CASTS()
	
#ifdef SML_MSVC_LEGACY
	};
	// vector (no-op default initialization)
	template <class T>
	struct vec<T, 2> : pod_vec<T, 2>
	{
#endif
#ifdef SML_MSVC_LEGACY
		SML_API vec() { }
#else
		SML_API vec() = default;
#endif
		SML_API vec(T x) { this->x = x; this->y = x; }
		SML_API vec(T x, T y) { this->x = x; this->y = y; }
		SML_API vec(vec<T, 1> x, T y) { this->x = x.x; this->y = y; } // required by matrix initialization w/ dehom columns

		template <class T2, int N2>
		SML_API explicit vec(SML_POD_VEC<T2, N2> const& v, typename enable_if<N2 >= 2>::type* assert_size = nullptr)
		{ this->x = T(v.c[0]); this->y = T(v.c[1]); }
		template <int Stride>
		SML_API vec(T const* c, stride_literal<Stride>)
		{ this->x = c[0]; this->y = c[Stride]; }

		template <class V>
		SML_API vec(V const& v, typename is_compatible_vec_type<T, 2, V>::type *assert_compatible_vec = 0)
		{ *this = *reinterpret_cast<vec const*>(&reinterpret_cast<T const&>(v)); }
		
		template <int C>
		SML_API static vec base(T x = T(1), T zero = T())
		{ vec s; s.x = (0 == C) ? x : zero; s.y = (1 == C) ? x : zero; return s; }
		SML_API static vec base(int C, T x = T(1), T zero = T())
		{ vec s; s.x = (0 == C) ? x : zero; s.y = (1 == C) ? x : zero; return s; }
		
#ifdef SML_MSVC_WORKAROUNDS
		// MSVC++ treats operators specially and requires them to be re-declared here to be instantiated in all cases
		SML_API friend vec operator + (vec const& l, vec const& r);
		SML_API friend vec operator - (vec const& l, vec const& r);
		SML_API friend vec operator * (vec const& l, vec const& r);
		SML_API friend vec operator / (vec const& l, vec const& r);
		SML_API friend vec operator % (vec const& l, vec const& r);
		SML_API friend vec operator & (vec const& l, vec const& r);
		SML_API friend vec operator | (vec const& l, vec const& r);
		SML_API friend vec operator ^ (vec const& l, vec const& r);
		SML_API friend vec operator <<(vec const& l, vec const& r);
		SML_API friend vec operator >>(vec const& l, vec const& r);
 		
		SML_API friend vec  operator -(pod_vec const& l);
		SML_API friend vec  operator ~(pod_vec const& l);
		SML_API friend typename vec::bvec operator !(pod_vec const& l);
		
		SML_API friend vec operator +(T l, vec const& r);
		SML_API friend vec operator -(T l, vec const& r);
		SML_API friend vec operator *(T l, vec const& r);
		SML_API friend vec operator /(T l, vec const& r);
		
		SML_API friend vec operator +(vec const& l, T r);
		SML_API friend vec operator -(vec const& l, T r);
		SML_API friend vec operator *(vec const& l, T r);
		SML_API friend vec operator /(vec const& l, T r);
#endif
	};
	// vector storage (no default initialization -> aggregate / value initialization)
	template <class T>
	struct SML_POD_VEC<T, 3>
	{
		typedef T component;
		static int const dimension = 3;

		union
		{
			struct { T x, y, z; };
			struct { T r, g, b; };
#ifndef SML_TRIVIAL_UNIONS
			SML_POD_VEC<T, 2> xy;
			struct { T _; SML_POD_VEC<T, 2> yz; };
#endif
			component c[dimension];
		};
		
		typedef vec<bool, dimension> bvec;
		typedef vec<int , dimension> ivec;
#ifdef SML_MSVC_LEGACY
		typedef vec<T, dimension> vec;
#endif

		SML_API T& operator [](int i) { return c[i]; }
		SML_API T  operator [](int i) const { return c[i]; }

#ifdef SML_MSVC_LEGACY
		SML_API SML_POD_VEC& operator = (vec const& r) { return *this = (pod_vec&) r; }
#endif
		SML_API SML_POD_VEC& operator +=(vec const& r) { x += r.x; y += r.y; z += r.z; return *this; }
		SML_API SML_POD_VEC& operator -=(vec const& r) { x -= r.x; y -= r.y; z -= r.z; return *this; }
		SML_API SML_POD_VEC& operator *=(vec const& r) { x *= r.x; y *= r.y; z *= r.z; return *this; }
		SML_API SML_POD_VEC& operator /=(vec const& r) { x /= r.x; y /= r.y; z /= r.z; return *this; }
		SML_API SML_POD_VEC& operator %=(vec const& r) { x %= r.x; y %= r.y; z %= r.z; return *this; }
		SML_API SML_POD_VEC& operator &=(vec const& r) { x &= r.x; y &= r.y; z &= r.z; return *this; }
		SML_API SML_POD_VEC& operator |=(vec const& r) { x |= r.x; y |= r.y; z |= r.z; return *this; }
		SML_API SML_POD_VEC& operator ^=(vec const& r) { x ^= r.x; y ^= r.y; z ^= r.z; return *this; }
		
		SML_API SML_POD_VEC& operator *=(T r) { x *= r; y *= r; z *= r; return *this; }
		SML_API SML_POD_VEC& operator /=(T r) { x /= r; y /= r; z /= r; return *this; }
		
		SML_API friend vec operator + (vec const& l, vec const& r) { vec s; s.x = l.x +  r.x; s.y = l.y +  r.y; s.z = l.z +  r.z; return s; }
		SML_API friend vec operator - (vec const& l, vec const& r) { vec s; s.x = l.x -  r.x; s.y = l.y -  r.y; s.z = l.z -  r.z; return s; }
		SML_API friend vec operator * (vec const& l, vec const& r) { vec s; s.x = l.x *  r.x; s.y = l.y *  r.y; s.z = l.z *  r.z; return s; }
		SML_API friend vec operator / (vec const& l, vec const& r) { vec s; s.x = l.x /  r.x; s.y = l.y /  r.y; s.z = l.z /  r.z; return s; }
		SML_API friend vec operator % (vec const& l, vec const& r) { vec s; s.x = l.x %  r.x; s.y = l.y %  r.y; s.z = l.z %  r.z; return s; }
		SML_API friend vec operator & (vec const& l, vec const& r) { vec s; s.x = l.x &  r.x; s.y = l.y &  r.y; s.z = l.z &  r.z; return s; }
		SML_API friend vec operator | (vec const& l, vec const& r) { vec s; s.x = l.x |  r.x; s.y = l.y |  r.y; s.z = l.z |  r.z; return s; }
		SML_API friend vec operator ^ (vec const& l, vec const& r) { vec s; s.x = l.x ^  r.x; s.y = l.y ^  r.y; s.z = l.z ^  r.z; return s; }
		SML_API friend vec operator <<(vec const& l, vec const& r) { vec s; s.x = l.x << r.x; s.y = l.y << r.y; s.z = l.z << r.z; return s; }
		SML_API friend vec operator >>(vec const& l, vec const& r) { vec s; s.x = l.x >> r.x; s.y = l.y >> r.y; s.z = l.z >> r.z; return s; }
		
		SML_API friend vec  operator -(SML_POD_VEC const& l) { vec s;  s.x = -l.x; s.y = -l.y; s.z = -l.z; return s; }
		SML_API friend vec  operator ~(SML_POD_VEC const& l) { vec s;  s.x = ~l.x; s.y = ~l.y; s.z = ~l.z; return s; }
		SML_API friend bvec operator !(SML_POD_VEC const& l) { bvec s; s.x = !l.x; s.y = !l.y; s.z = !l.z; return s; }
		
		SML_API friend vec operator +(T l, vec const& r) { vec s; s.x = l + r.x; s.y = l + r.y; s.z = l + r.z; return s; }
		SML_API friend vec operator -(T l, vec const& r) { vec s; s.x = l - r.x; s.y = l - r.y; s.z = l - r.z; return s; }
		SML_API friend vec operator *(T l, vec const& r) { vec s; s.x = l * r.x; s.y = l * r.y; s.z = l * r.z; return s; }
		SML_API friend vec operator /(T l, vec const& r) { vec s; s.x = l / r.x; s.y = l / r.y; s.z = l / r.z; return s; }
		
		SML_API friend vec operator +(vec const& l, T r) { vec s; s.x = l.x + r; s.y = l.y + r; s.z = l.z + r; return s; }
		SML_API friend vec operator -(vec const& l, T r) { vec s; s.x = l.x - r; s.y = l.y - r; s.z = l.z - r; return s; }
		SML_API friend vec operator *(vec const& l, T r) { vec s; s.x = l.x * r; s.y = l.y * r; s.z = l.z * r; return s; }
		SML_API friend vec operator /(vec const& l, T r) { vec s; s.x = l.x / r; s.y = l.y / r; s.z = l.z / r; return s; }
		
		SML_API friend T dot(vec const& l, vec const& r) { return l.x * r.x + l.y * r.y + l.z * r.z; }
		template <int S>
		SML_API static T dot_transposed(T const* l, stride_literal<S>, vec const& r) { return l[0] * r.x + l[S] * r.y + l[2 * S] * r.z; }
		template <int S>
		SML_API static vec transpose(T const* t, stride_literal<S>) { vec s; s.x = t[0]; s.y = t[S]; s.z = t[2 * S]; return s; }

		SML_API friend vec cross(vec const& l, vec const& r)
		{
			return vec(
				  l.y * r.z - l.z * r.y
				, l.z * r.x - l.x * r.z
				, l.x * r.y - l.y * r.x
				);
		}
		
#ifdef SML_MSVC_LEGACY
		SML_API friend vec min(pod_vec const& l, pod_vec const& r) { vec s; s.x = min(l.x, r.x); s.y = min(l.y, r.y); s.z = min(l.z, r.z); return s; }
		SML_API friend vec max(pod_vec const& l, pod_vec const& r) { vec s; s.x = max(l.x, r.x); s.y = max(l.y, r.y); s.z = max(l.z, r.z); return s; }
#endif
		SML_API friend vec min(vec const& l, vec const& r) { vec s; s.x = min(l.x, r.x); s.y = min(l.y, r.y); s.z = min(l.z, r.z); return s; }
		SML_API friend vec max(vec const& l, vec const& r) { vec s; s.x = max(l.x, r.x); s.y = max(l.y, r.y); s.z = max(l.z, r.z); return s; }
		SML_API friend vec pow(vec const& l, vec const& r) { vec s; s.x = pow(l.x, r.x); s.y = pow(l.y, r.y); s.z = pow(l.z, r.z); return s; }
		SML_API friend vec abs(SML_POD_VEC const& l) { vec s; s.x = abs(l.x); s.y = abs(l.y); s.z = abs(l.z); return s; }
		SML_API friend vec log(SML_POD_VEC const& l) { vec s; s.x = log(l.x); s.y = log(l.y); s.z = log(l.z); return s; }
		SML_API friend vec exp(SML_POD_VEC const& l) { vec s; s.x = exp(l.x); s.y = exp(l.y); s.z = exp(l.z); return s; }
		SML_API friend vec sqrt (SML_POD_VEC const& l) { vec s; s.x = sqrt (l.x); s.y = sqrt (l.y); s.z = sqrt (l.z); return s; }
		SML_API friend vec floor(SML_POD_VEC const& l) { vec s; s.x = floor(l.x); s.y = floor(l.y); s.z = floor(l.z); return s; }
		SML_API friend vec ceil (SML_POD_VEC const& l) { vec s; s.x = ceil (l.x); s.y = ceil (l.y); s.z = ceil (l.z); return s; }
		
		SML_API friend ivec signbit(SML_POD_VEC const& l) { ivec s; s.x = signbit(l.x); s.y = signbit(l.y); s.z = signbit(l.z); return s; }
		SML_API friend vec copysign(SML_POD_VEC const& m, SML_POD_VEC const& s) { vec r; r.x = copysign(m.x, s.x); r.y = copysign(m.y, s.y); r.z = copysign(m.z, s.z); return r; }

		SML_API friend vec mix(vec const& l, vec const& r, bvec const& p) { vec s; s.x = p.x ? r.x : l.x; s.y = p.y ? r.y : l.y; s.z = p.z ? r.z : l.z; return s; }

		SML_API friend T compMin(SML_POD_VEC const& l) { return min(min(l.x, l.y), l.z); }
		SML_API friend T compMax(SML_POD_VEC const& l) { return max(max(l.x, l.y), l.z); }
		SML_API friend bool all(SML_POD_VEC const& l) { return !!l.x & !!l.y & !!l.z; }
		SML_API friend bool any(SML_POD_VEC const& l) { return !!l.x | !!l.y | !!l.z; }

		SML_API friend bool operator ==(vec const& l, vec const& r) { return l.x == r.x && l.y == r.y && l.z == r.z; }
		SML_API friend bool operator !=(vec const& l, vec const& r) { return l.x != r.x || l.y != r.y || l.z != r.z; }
		
		SML_API friend bvec lessThan(vec const& l, vec const& r)      { bvec s; s.x = (l.x <  r.x); s.y = (l.y <  r.y); s.z = (l.z <  r.z); return s; }
		SML_API friend bvec lessThanEqual(vec const& l, vec const& r) { bvec s; s.x = (l.x <= r.x); s.y = (l.y <= r.y); s.z = (l.z <= r.z); return s; }
		SML_API friend bvec equal(vec const& l, vec const& r)         { bvec s; s.x = (l.x == r.x); s.y = (l.y == r.y); s.z = (l.z == r.z); return s; }
		SML_API friend bvec notEqual(vec const& l, vec const& r)      { bvec s; s.x = (l.x != r.x); s.y = (l.y != r.y); s.z = (l.z != r.z); return s; }

		template <class Op>
		SML_API friend vec cw(Op&& op, vec const& l) { vec s; s.x = op(l.x); s.y = op(l.y); s.z = op(l.z); return s; }
		template <class Op>
		SML_API friend vec cw(Op&& op, vec const& l, vec const& r) { vec s; s.x = op(l.x, r.x); s.y = op(l.y, r.y); s.z = op(l.z, r.z); return s; }

#ifdef SML_MSVC_LEGACY
		SML_API operator vec&() { return *reinterpret_cast<vec*>(reinterpret_cast<T*>(this)); }
		SML_API operator vec const&() const { return *reinterpret_cast<vec const*>(reinterpret_cast<T const*>(this)); }
#endif
		SML_MAKE_COMPATIBILITY_CASTS()
	
#ifdef SML_MSVC_LEGACY
	};
	// vector (no-op default initialization)
	template <class T>
	struct vec<T, 3> : pod_vec<T, 3>
	{
#endif
#ifdef SML_MSVC_LEGACY
		SML_API vec() { }
#else
		SML_API vec() = default;
#endif
		SML_API vec(T x) { this->x = x; this->y = x; this->z = x; }
		SML_API vec(T x, T y, T z) { this->x = x; this->y = y; this->z = z; }
		SML_API vec(vec<T, 2> const& xy, T z) { this->x = xy.x; this->y = xy.y; this->z = z; }
		SML_API vec(T x, vec<T, 2> const& yz) { this->x = x; this->y = yz.x; this->z = yz.y; }
		
		template <class T2, int N2>
		SML_API explicit vec(SML_POD_VEC<T2, N2> const& v, typename enable_if<N2 >= 3>::type* assert_size = nullptr)
		{ this->x = T(v.c[0]); this->y = T(v.c[1]); this->z = T(v.c[2]); }
		template <int Stride>
		SML_API vec(T const* c, stride_literal<Stride>)
		{ this->x = c[0]; this->y = c[Stride]; this->z = c[2 * Stride]; }

		template <class V>
		SML_API vec(V const& v, typename is_compatible_vec_type<T, 3, V>::type *assert_compatible_vec = 0)
		{ *this = *reinterpret_cast<vec const*>(&reinterpret_cast<T const&>(v)); }
		
		template <int C>
		SML_API static vec base(T x = T(1), T zero = T())
		{ vec s; s.x = (0 == C) ? x : zero; s.y = (1 == C) ? x : zero; s.z = (2 == C) ? x : zero; return s; }
		SML_API static vec base(int C, T x = T(1), T zero = T())
		{ vec s; s.x = (0 == C) ? x : zero; s.y = (1 == C) ? x : zero; s.z = (2 == C) ? x : zero; return s; }

#ifdef SML_MSVC_WORKAROUNDS
		// MSVC++ treats operators specially and requires them to be re-declared here to be instantiated in all cases
		SML_API friend vec operator + (vec const& l, vec const& r);
		SML_API friend vec operator - (vec const& l, vec const& r);
		SML_API friend vec operator * (vec const& l, vec const& r);
		SML_API friend vec operator / (vec const& l, vec const& r);
		SML_API friend vec operator % (vec const& l, vec const& r);
		SML_API friend vec operator & (vec const& l, vec const& r);
		SML_API friend vec operator | (vec const& l, vec const& r);
		SML_API friend vec operator ^ (vec const& l, vec const& r);
		SML_API friend vec operator <<(vec const& l, vec const& r);
		SML_API friend vec operator >>(vec const& l, vec const& r);
		
		SML_API friend vec  operator -(pod_vec const& l);
		SML_API friend vec  operator ~(pod_vec const& l);
		SML_API friend typename vec::bvec operator !(pod_vec const& l);
		
		SML_API friend vec operator +(T l, vec const& r);
		SML_API friend vec operator -(T l, vec const& r);
		SML_API friend vec operator *(T l, vec const& r);
		SML_API friend vec operator /(T l, vec const& r);
		
		SML_API friend vec operator +(vec const& l, T r);
		SML_API friend vec operator -(vec const& l, T r);
		SML_API friend vec operator *(vec const& l, T r);
		SML_API friend vec operator /(vec const& l, T r);
#endif
	};
	// vector storage (no default initialization -> aggregate / value initialization)
	template <class T>
	struct SML_POD_VEC<T, 4>
	{
		typedef T component;
		static int const dimension = 4;

		union
		{
			struct { T x, y, z, w; };
			struct { T r, g, b, a; };
#ifndef SML_TRIVIAL_UNIONS
			SML_POD_VEC<T, 3> xyz;
			struct { SML_POD_VEC<T, 2> xy, zw; };
			struct { T _; union { SML_POD_VEC<T, 3> yzw; SML_POD_VEC<T, 2> yz; }; };
#endif
			component c[dimension];
		};

		typedef vec<bool, dimension> bvec;
		typedef vec<int , dimension> ivec;
#ifdef SML_MSVC_LEGACY
		typedef vec<T, dimension> vec;
#endif

		SML_API T& operator [](int i) { return c[i]; }
		SML_API T  operator [](int i) const { return c[i]; }
		
#ifdef SML_MSVC_LEGACY
		SML_API SML_POD_VEC& operator = (vec const& r) { return *this = (pod_vec&) r; }
#endif
		SML_API SML_POD_VEC& operator +=(vec const& r) { x += r.x; y += r.y; z += r.z; w += r.w; return *this; }
		SML_API SML_POD_VEC& operator -=(vec const& r) { x -= r.x; y -= r.y; z -= r.z; w -= r.w; return *this; }
		SML_API SML_POD_VEC& operator *=(vec const& r) { x *= r.x; y *= r.y; z *= r.z; w *= r.w; return *this; }
		SML_API SML_POD_VEC& operator /=(vec const& r) { x /= r.x; y /= r.y; z /= r.z; w /= r.w; return *this; }
		SML_API SML_POD_VEC& operator %=(vec const& r) { x %= r.x; y %= r.y; z %= r.z; w %= r.w; return *this; }
		SML_API SML_POD_VEC& operator &=(vec const& r) { x &= r.x; y &= r.y; z &= r.z; w &= r.w; return *this; }
		SML_API SML_POD_VEC& operator |=(vec const& r) { x |= r.x; y |= r.y; z |= r.z; w |= r.w; return *this; }
		SML_API SML_POD_VEC& operator ^=(vec const& r) { x ^= r.x; y ^= r.y; z ^= r.z; w ^= r.w; return *this; }

		SML_API SML_POD_VEC& operator *=(T r) { x *= r; y *= r; z *= r; w *= r; return *this; }
		SML_API SML_POD_VEC& operator /=(T r) { x /= r; y /= r; z /= r; w /= r; return *this; }

		SML_API friend vec operator + (vec const& l, vec const& r) { vec s; s.x = l.x +  r.x; s.y = l.y +  r.y; s.z = l.z +  r.z; s.w = l.w +  r.w; return s; }
		SML_API friend vec operator - (vec const& l, vec const& r) { vec s; s.x = l.x -  r.x; s.y = l.y -  r.y; s.z = l.z -  r.z; s.w = l.w -  r.w; return s; }
		SML_API friend vec operator * (vec const& l, vec const& r) { vec s; s.x = l.x *  r.x; s.y = l.y *  r.y; s.z = l.z *  r.z; s.w = l.w *  r.w; return s; }
		SML_API friend vec operator / (vec const& l, vec const& r) { vec s; s.x = l.x /  r.x; s.y = l.y /  r.y; s.z = l.z /  r.z; s.w = l.w /  r.w; return s; }
		SML_API friend vec operator % (vec const& l, vec const& r) { vec s; s.x = l.x %  r.x; s.y = l.y %  r.y; s.z = l.z %  r.z; s.w = l.w %  r.w; return s; }
		SML_API friend vec operator & (vec const& l, vec const& r) { vec s; s.x = l.x &  r.x; s.y = l.y &  r.y; s.z = l.z &  r.z; s.w = l.w &  r.w; return s; }
		SML_API friend vec operator | (vec const& l, vec const& r) { vec s; s.x = l.x |  r.x; s.y = l.y |  r.y; s.z = l.z |  r.z; s.w = l.w |  r.w; return s; }
		SML_API friend vec operator ^ (vec const& l, vec const& r) { vec s; s.x = l.x ^  r.x; s.y = l.y ^  r.y; s.z = l.z ^  r.z; s.w = l.w ^  r.w; return s; }
		SML_API friend vec operator <<(vec const& l, vec const& r) { vec s; s.x = l.x << r.x; s.y = l.y << r.y; s.z = l.z << r.z; s.w = l.w << r.w; return s; }
		SML_API friend vec operator >>(vec const& l, vec const& r) { vec s; s.x = l.x >> r.x; s.y = l.y >> r.y; s.z = l.z >> r.z; s.w = l.w >> r.w; return s; }

		SML_API friend vec  operator -(SML_POD_VEC const& l) { vec s;  s.x = -l.x; s.y = -l.y; s.z = -l.z; s.w = -l.w; return s; }
		SML_API friend vec  operator ~(SML_POD_VEC const& l) { vec s;  s.x = ~l.x; s.y = ~l.y; s.z = ~l.z; s.w = ~l.w; return s; }
		SML_API friend bvec operator !(SML_POD_VEC const& l) { bvec s; s.x = !l.x; s.y = !l.y; s.z = !l.z; s.w = !l.w; return s; }

		SML_API friend vec operator +(T l, vec const& r) { vec s; s.x = l + r.x; s.y = l + r.y; s.z = l + r.z; s.w = l + r.w; return s; }
		SML_API friend vec operator -(T l, vec const& r) { vec s; s.x = l - r.x; s.y = l - r.y; s.z = l - r.z; s.w = l - r.w; return s; }
		SML_API friend vec operator *(T l, vec const& r) { vec s; s.x = l * r.x; s.y = l * r.y; s.z = l * r.z; s.w = l * r.w; return s; }
		SML_API friend vec operator /(T l, vec const& r) { vec s; s.x = l / r.x; s.y = l / r.y; s.z = l / r.z; s.w = l / r.w; return s; }
		
		SML_API friend vec operator +(vec const& l, T r) { vec s; s.x = l.x + r; s.y = l.y + r; s.z = l.z + r; s.w = l.w + r; return s; }
		SML_API friend vec operator -(vec const& l, T r) { vec s; s.x = l.x - r; s.y = l.y - r; s.z = l.z - r; s.w = l.w - r; return s; }
		SML_API friend vec operator *(vec const& l, T r) { vec s; s.x = l.x * r; s.y = l.y * r; s.z = l.z * r; s.w = l.w * r; return s; }
		SML_API friend vec operator /(vec const& l, T r) { vec s; s.x = l.x / r; s.y = l.y / r; s.z = l.z / r; s.w = l.w / r; return s; }
		
		SML_API friend T dot(vec const& l, vec const& r) { return l.x * r.x + l.y * r.y + l.z * r.z + l.w * r.w; }
		template <int S>
		SML_API static T dot_transposed(T const* l, stride_literal<S>, vec const& r) { return l[0] * r.x + l[S] * r.y + l[2 * S] * r.z + l[3 * S] * r.w; }
		template <int S>
		SML_API static vec transpose(T const* t, stride_literal<S>) { vec s; s.x = t[0]; s.y = t[S]; s.z = t[2 * S]; s.w = t[3 * S]; return s; }
		
#ifdef SML_MSVC_LEGACY
		SML_API friend vec min(pod_vec const& l, pod_vec const& r) { vec s; s.x = min(l.x, r.x); s.y = min(l.y, r.y); s.z = min(l.z, r.z); s.w = min(l.w, r.w); return s; }
		SML_API friend vec max(pod_vec const& l, pod_vec const& r) { vec s; s.x = max(l.x, r.x); s.y = max(l.y, r.y); s.z = max(l.z, r.z); s.w = max(l.w, r.w); return s; }
#endif
		SML_API friend vec min(vec const& l, vec const& r) { vec s; s.x = min(l.x, r.x); s.y = min(l.y, r.y); s.z = min(l.z, r.z); s.w = min(l.w, r.w); return s; }
		SML_API friend vec max(vec const& l, vec const& r) { vec s; s.x = max(l.x, r.x); s.y = max(l.y, r.y); s.z = max(l.z, r.z); s.w = max(l.w, r.w); return s; }
		SML_API friend vec pow(vec const& l, vec const& r) { vec s; s.x = pow(l.x, r.x); s.y = pow(l.y, r.y); s.z = pow(l.z, r.z); s.w = pow(l.w, r.w); return s; }
		SML_API friend vec abs(SML_POD_VEC const& l) { vec s; s.x = abs(l.x); s.y = abs(l.y); s.z = abs(l.z); s.w = abs(l.w); return s; }
		SML_API friend vec log(SML_POD_VEC const& l) { vec s; s.x = log(l.x); s.y = log(l.y); s.z = log(l.z); s.w = log(l.w); return s; }
		SML_API friend vec exp(SML_POD_VEC const& l) { vec s; s.x = exp(l.x); s.y = exp(l.y); s.z = exp(l.z); s.w = exp(l.w); return s; }
		SML_API friend vec sqrt (SML_POD_VEC const& l) { vec s; s.x = sqrt (l.x); s.y = sqrt (l.y); s.z = sqrt (l.z); s.w = sqrt (l.w); return s; }
		SML_API friend vec floor(SML_POD_VEC const& l) { vec s; s.x = floor(l.x); s.y = floor(l.y); s.z = floor(l.z); s.w = floor(l.w); return s; }
		SML_API friend vec ceil (SML_POD_VEC const& l) { vec s; s.x = ceil (l.x); s.y = ceil (l.y); s.z = ceil (l.z); s.w = ceil (l.w); return s; }

		SML_API friend ivec signbit(SML_POD_VEC const& l) { ivec s; s.x = signbit(l.x); s.y = signbit(l.y); s.z = signbit(l.z); s.w = signbit(l.w); return s; }
		SML_API friend vec copysign(SML_POD_VEC const& m, SML_POD_VEC const& s) { vec r; r.x = copysign(m.x, s.x); r.y = copysign(m.y, s.y); r.z = copysign(m.z, s.z); r.w = copysign(m.w, s.w); return r; }

		SML_API friend vec mix(vec const& l, vec const& r, bvec const& p) { vec s; s.x = p.x ? r.x : l.x; s.y = p.y ? r.y : l.y; s.z = p.z ? r.z : l.z; s.w = p.w ? r.w : l.w; return s; }
		
		SML_API friend T compMin(SML_POD_VEC const& l) { return min(min(l.x, l.y), min(l.z, l.w)); }
		SML_API friend T compMax(SML_POD_VEC const& l) { return max(max(l.x, l.y), max(l.z, l.w)); }
		SML_API friend bool all(SML_POD_VEC const& l) { return !!l.x & !!l.y & !!l.z & !!l.w; }
		SML_API friend bool any(SML_POD_VEC const& l) { return !!l.x | !!l.y | !!l.z | !!l.w; }

		SML_API friend bool operator ==(vec const& l, vec const& r) { return l.x == r.x && l.y == r.y && l.z == r.z && l.w == r.w; }
		SML_API friend bool operator !=(vec const& l, vec const& r) { return l.x != r.x || l.y != r.y || l.z != r.z || l.w != r.w; }
		
		SML_API friend bvec lessThan(vec const& l, vec const& r)   { bvec s; s.x = (l.x <  r.x); s.y = (l.y <  r.y); s.z = (l.z <  r.z); s.w = (l.w <  r.w); return s; }
		SML_API friend bvec lessThanEqual(vec const& l, vec const& r) { bvec s; s.x = (l.x <= r.x); s.y = (l.y <= r.y); s.z = (l.z <= r.z); s.w = (l.w <= r.w); return s; }
		SML_API friend bvec equal(vec const& l, vec const& r)         { bvec s; s.x = (l.x == r.x); s.y = (l.y == r.y); s.z = (l.z == r.z); s.w = (l.w == r.w); return s; }
		SML_API friend bvec notEqual(vec const& l, vec const& r)      { bvec s; s.x = (l.x != r.x); s.y = (l.y != r.y); s.z = (l.z != r.z); s.w = (l.w != r.w); return s; }

		template <class Op>
		SML_API friend vec cw(Op&& op, vec const& l) { vec s; s.x = op(l.x); s.y = op(l.y); s.z = op(l.z); s.w = op(l.w); return s; }
		template <class Op>
		SML_API friend vec cw(Op&& op, vec const& l, vec const& r) { vec s; s.x = op(l.x, r.x); s.y = op(l.y, r.y); s.z = op(l.z, r.z); s.w = op(l.w, r.w); return s; }

#ifdef SML_MSVC_LEGACY
		SML_API operator vec&() { return *reinterpret_cast<vec*>(reinterpret_cast<T*>(this)); }
		SML_API operator vec const&() const { return *reinterpret_cast<vec const*>(reinterpret_cast<T const*>(this)); }
#endif
		SML_MAKE_COMPATIBILITY_CASTS()

#ifdef SML_MSVC_LEGACY
	};
	// vector (no-op default initialization)
	template <class T>
	struct vec<T, 4> : pod_vec<T, 4>
	{
#endif
#ifdef SML_MSVC_LEGACY
		SML_API vec() { }
#else
		SML_API vec() = default;
#endif
		SML_API vec(T x) { this->x = x; this->y = x; this->z = x; this->w = x; }
		SML_API vec(T x, T y, T z, T w) { this->x = x; this->y = y; this->z = z; this->w = w; }
		SML_API vec(vec<T, 3> const& xyz, T w) { this->x = xyz.x; this->y = xyz.y; this->z = xyz.z; this->w = w; }
		SML_API vec(T x, vec<T, 3> const& yzw) { this->x = x; this->y = yzw.x; this->z = yzw.y; this->w = yzw.z; }
		SML_API vec(vec<T, 2> const& xy, vec<T, 2> const& zw) { this->x = xy.x; this->y = xy.y; this->z = zw.x; this->w = zw.y; }
		SML_API vec(vec<T, 2> const& xy, T z, T w) { this->x = xy.x; this->y = xy.y; this->z = z; this->w = w; }

		template <class T2, int N2>
		SML_API explicit vec(SML_POD_VEC<T2, N2> const& v, typename enable_if<N2 >= 4>::type* assert_size = nullptr)
		{ this->x = T(v.c[0]); this->y = T(v.c[1]); this->z = T(v.c[2]); this->w = T(v.c[3]); }
		template <int Stride>
		SML_API vec(T const* c, stride_literal<Stride>)
		{ this->x = c[0]; this->y = c[Stride]; this->z = c[2 * Stride]; this->w = c[3 * Stride]; }
		
		template <class V>
		SML_API vec(V const& v, typename is_compatible_vec_type<T, 4, V>::type *assert_compatible_vec = 0)
		{ *this = *reinterpret_cast<vec const*>(&reinterpret_cast<T const&>(v)); }

		template <int C>
		SML_API static vec base(T x = T(1), T zero = T())
		{ vec s; s.x = (0 == C) ? x : zero; s.y = (1 == C) ? x : zero; s.z = (2 == C) ? x : zero; s.w = (3 == C) ? x : zero; return s; }
		SML_API static vec base(int C, T x = T(1), T zero = T())
		{ vec s; s.x = (0 == C) ? x : zero; s.y = (1 == C) ? x : zero; s.z = (2 == C) ? x : zero; s.w = (3 == C) ? x : zero; return s; }

#ifdef SML_MSVC_WORKAROUNDS
		// MSVC++ treats operators specially and requires them to be re-declared here to be instantiated in all cases
		SML_API friend vec operator + (vec const& l, vec const& r);
		SML_API friend vec operator - (vec const& l, vec const& r);
		SML_API friend vec operator * (vec const& l, vec const& r);
		SML_API friend vec operator / (vec const& l, vec const& r);
		SML_API friend vec operator % (vec const& l, vec const& r);
		SML_API friend vec operator & (vec const& l, vec const& r);
		SML_API friend vec operator | (vec const& l, vec const& r);
		SML_API friend vec operator ^ (vec const& l, vec const& r);
		SML_API friend vec operator <<(vec const& l, vec const& r);
		SML_API friend vec operator >>(vec const& l, vec const& r);
		
		SML_API friend vec  operator -(pod_vec const& l);
		SML_API friend vec  operator ~(pod_vec const& l);
		SML_API friend typename vec::bvec operator !(pod_vec const& l);
		
		SML_API friend vec operator +(T l, vec const& r);
		SML_API friend vec operator -(T l, vec const& r);
		SML_API friend vec operator *(T l, vec const& r);
		SML_API friend vec operator /(T l, vec const& r);
		
		SML_API friend vec operator +(vec const& l, T r);
		SML_API friend vec operator -(vec const& l, T r);
		SML_API friend vec operator *(vec const& l, T r);
		SML_API friend vec operator /(vec const& l, T r);
#endif
	};
	// vector storage (no default initialization -> aggregate / value initialization)
	template <class T, int N>
	struct SML_POD_VEC
	{
		typedef T component;
		static int const dimension = N;
		
		component c[dimension];
		
		typedef vec<bool, N> bvec;
		typedef vec<int , N> ivec;
#ifdef SML_MSVC_LEGACY
		typedef vec<T, N> vec;
#endif
		
		SML_API T& operator [](int i) { return c[i]; }
		SML_API T  operator [](int i) const { return c[i]; }

#ifdef SML_MSVC_LEGACY
		SML_API SML_POD_VEC& operator = (vec const& r) { return *this = (pod_vec&) r; }
#endif
		SML_API SML_POD_VEC& operator +=(vec const& r) { for (int i = 0; i < N; ++i) c[i] += r.c[i]; return *this; }
		SML_API SML_POD_VEC& operator -=(vec const& r) { for (int i = 0; i < N; ++i) c[i] -= r.c[i]; return *this; }
		SML_API SML_POD_VEC& operator *=(vec const& r) { for (int i = 0; i < N; ++i) c[i] *= r.c[i]; return *this; }
		SML_API SML_POD_VEC& operator /=(vec const& r) { for (int i = 0; i < N; ++i) c[i] /= r.c[i]; return *this; }
		SML_API SML_POD_VEC& operator %=(vec const& r) { for (int i = 0; i < N; ++i) c[i] %= r.c[i]; return *this; }
		SML_API SML_POD_VEC& operator &=(vec const& r) { for (int i = 0; i < N; ++i) c[i] &= r.c[i]; return *this; }
		SML_API SML_POD_VEC& operator |=(vec const& r) { for (int i = 0; i < N; ++i) c[i] |= r.c[i]; return *this; }
		SML_API SML_POD_VEC& operator ^=(vec const& r) { for (int i = 0; i < N; ++i) c[i] ^= r.c[i]; return *this; }

		SML_API SML_POD_VEC& operator *=(T r) { for (int i = 0; i < N; ++i) c[i] *= r; return *this; }
		SML_API SML_POD_VEC& operator /=(T r) { for (int i = 0; i < N; ++i) c[i] /= r; return *this; }
		
		SML_API friend vec operator + (vec const& l, vec const& r) { vec s; for (int i = 0; i < N; ++i) s.c[i] = l.c[i] +  r.c[i]; return s; }
		SML_API friend vec operator - (vec const& l, vec const& r) { vec s; for (int i = 0; i < N; ++i) s.c[i] = l.c[i] -  r.c[i]; return s; }
		SML_API friend vec operator * (vec const& l, vec const& r) { vec s; for (int i = 0; i < N; ++i) s.c[i] = l.c[i] *  r.c[i]; return s; }
		SML_API friend vec operator / (vec const& l, vec const& r) { vec s; for (int i = 0; i < N; ++i) s.c[i] = l.c[i] /  r.c[i]; return s; }
		SML_API friend vec operator % (vec const& l, vec const& r) { vec s; for (int i = 0; i < N; ++i) s.c[i] = l.c[i] %  r.c[i]; return s; }
		SML_API friend vec operator & (vec const& l, vec const& r) { vec s; for (int i = 0; i < N; ++i) s.c[i] = l.c[i] &  r.c[i]; return s; }
		SML_API friend vec operator | (vec const& l, vec const& r) { vec s; for (int i = 0; i < N; ++i) s.c[i] = l.c[i] |  r.c[i]; return s; }
		SML_API friend vec operator ^ (vec const& l, vec const& r) { vec s; for (int i = 0; i < N; ++i) s.c[i] = l.c[i] ^  r.c[i]; return s; }
		SML_API friend vec operator <<(vec const& l, vec const& r) { vec s; for (int i = 0; i < N; ++i) s.c[i] = l.c[i] << r.c[i]; return s; }
		SML_API friend vec operator >>(vec const& l, vec const& r) { vec s; for (int i = 0; i < N; ++i) s.c[i] = l.c[i] >> r.c[i]; return s; }

		SML_API friend vec operator  -(SML_POD_VEC const& l) { vec s; for (int i = 0; i < N; ++i) s.c[i] = -l.c[i]; return s; }
		SML_API friend vec operator  ~(SML_POD_VEC const& l) { vec s; for (int i = 0; i < N; ++i) s.c[i] = ~l.c[i]; return s; }
		SML_API friend bvec operator !(SML_POD_VEC const& l) { bvec s; for (int i = 0; i < N; ++i) s.c[i] = !l.c[i]; return s; }

		SML_API friend vec operator +(T l, vec const& r) { vec s; for (int i = 0; i < N; ++i) s.c[i] = l + r.c[i]; return s; }
		SML_API friend vec operator -(T l, vec const& r) { vec s; for (int i = 0; i < N; ++i) s.c[i] = l - r.c[i]; return s; }
		SML_API friend vec operator *(T l, vec const& r) { vec s; for (int i = 0; i < N; ++i) s.c[i] = l * r.c[i]; return s; }
		SML_API friend vec operator /(T l, vec const& r) { vec s; for (int i = 0; i < N; ++i) s.c[i] = l / r.c[i]; return s; }

		SML_API friend vec operator +(vec const& l, T r) { vec s; for (int i = 0; i < N; ++i) s.c[i] = l.c[i] + r; return s; }
		SML_API friend vec operator -(vec const& l, T r) { vec s; for (int i = 0; i < N; ++i) s.c[i] = l.c[i] - r; return s; }
		SML_API friend vec operator *(vec const& l, T r) { vec s; for (int i = 0; i < N; ++i) s.c[i] = l.c[i] * r; return s; }
		SML_API friend vec operator /(vec const& l, T r) { vec s; for (int i = 0; i < N; ++i) s.c[i] = l.c[i] / r; return s; }
		
		SML_API friend T dot(vec const& l, vec const& r) { T s = T(); for (int i = 0; i < N; ++i) s += l.c[i] * r.c[i]; return s; }
		template <int S>
		SML_API static T dot_transposed(T const* l, stride_literal<S>, vec const& r) { T s = T(); for (int i = 0; i < N; ++i) s += l[i * S] * r.c[i]; return s; }
		template <int S>
		SML_API static vec transpose(T const* t, stride_literal<S>) { vec s; for (int i = 0; i < N; ++i) s.c[i] = t[i * S]; return s; }

#ifdef SML_MSVC_LEGACY
		SML_API friend vec min(pod_vec const& l, pod_vec const& r) { vec s; for (int i = 0; i < N; ++i) s.c[i] = min(l.c[i], r.c[i]); return s; }
		SML_API friend vec max(pod_vec const& l, pod_vec const& r) { vec s; for (int i = 0; i < N; ++i) s.c[i] = max(l.c[i], r.c[i]); return s; }
#endif
		SML_API friend vec min(vec const& l, vec const& r) { vec s; for (int i = 0; i < N; ++i) s.c[i] = min(l.c[i], r.c[i]); return s; }
		SML_API friend vec max(vec const& l, vec const& r) { vec s; for (int i = 0; i < N; ++i) s.c[i] = max(l.c[i], r.c[i]); return s; }
		SML_API friend vec pow(vec const& l, vec const& r) { vec s; for (int i = 0; i < N; ++i) s.c[i] = pow(l.c[i], r.c[i]); return s; }
		SML_API friend vec log(SML_POD_VEC const& l) { vec s; for (int i = 0; i < N; ++i) s.c[i] = log(l.c[i]); return s; }
		SML_API friend vec exp(SML_POD_VEC const& l) { vec s; for (int i = 0; i < N; ++i) s.c[i] = exp(l.c[i]); return s; }
		SML_API friend vec abs(SML_POD_VEC const& l) { vec s; for (int i = 0; i < N; ++i) s.c[i] = abs(l.c[i]); return s; }
		SML_API friend vec sqrt(SML_POD_VEC const& l) { vec s; for (int i = 0; i < N; ++i) s.c[i] = sqrt (l.c[i]); return s; }
		SML_API friend vec floor(SML_POD_VEC const& l) { vec s; for (int i = 0; i < N; ++i) s.c[i] = floor(l.c[i]); return s; }
		SML_API friend vec ceil (SML_POD_VEC const& l) { vec s; for (int i = 0; i < N; ++i) s.c[i] = ceil (l.c[i]); return s; }

		SML_API friend ivec signbit(SML_POD_VEC const& l) { ivec s; for (int i = 0; i < N; ++i) s.c[i] = signbit(l.c[i]); return s; }
		
		SML_API friend vec mix(vec const& l, vec const& r, bvec const& p) { vec s; for (int i = 0; i < N; ++i) s.c[i] = p.c[i] ? r.c[i] : l.c[i]; return s; }

		SML_API friend T compMin(SML_POD_VEC const& l) { T s = l.c[0]; for (int i = 1; i < N; ++i) s = min(l.c[i], s); return s; }
		SML_API friend T compMax(SML_POD_VEC const& l) { T s = l.c[0]; for (int i = 1; i < N; ++i) s = max(l.c[i], s); return s; }
		SML_API friend bool all(SML_POD_VEC const& l) { bool s = !!l.c[0]; for (int i = 1; i < N; ++i) s &= !!l.c[i]; return s; }
		SML_API friend bool any(SML_POD_VEC const& l) { bool s = !!l.c[0]; for (int i = 1; i < N; ++i) s |= !!l.c[i]; return s; }

		SML_API friend bool operator ==(vec const& l, vec const& r) { bool s = true;  for (int i = 0; i < N; ++i) s &= (l.c[i] == r.c[i]); return s; }
		SML_API friend bool operator !=(vec const& l, vec const& r) { bool s = false; for (int i = 0; i < N; ++i) s |= (l.c[i] != r.c[i]); return s; }
		
		SML_API friend bvec lessThan(vec const& l, vec const& r)      { bvec s; for (int i = 0; i < N; ++i) s.c[i] = (l.c[i] <  r.c[i]); return s; }
		SML_API friend bvec lessThanEqual(vec const& l, vec const& r) { bvec s; for (int i = 0; i < N; ++i) s.c[i] = (l.c[i] <= r.c[i]); return s; }
		SML_API friend bvec equal(vec const& l, vec const& r)         { bvec s; for (int i = 0; i < N; ++i) s.c[i] = (l.c[i] == r.c[i]); return s; }
		SML_API friend bvec notEqual(vec const& l, vec const& r)      { bvec s; for (int i = 0; i < N; ++i) s.c[i] = (l.c[i] != r.c[i]); return s; }
		
		template <class Op>
		SML_API friend vec cw(Op&& op, SML_POD_VEC const& l) { vec s; for (int i = 0; i < N; ++i) s.c[i] = op(l.c[i]); return s; }
		template <class Op>
		SML_API friend vec cw(Op&& op, vec const& l, vec const& r) { vec s; for (int i = 0; i < N; ++i) s.c[i] = op(l.c[i], r.c[i]); return s; }

#ifdef SML_MSVC_LEGACY
		SML_API operator vec&() { return *reinterpret_cast<vec*>(reinterpret_cast<T*>(this)); }
		SML_API operator vec const&() const { return *reinterpret_cast<vec const*>(reinterpret_cast<T const*>(this)); }
#endif
		SML_MAKE_COMPATIBILITY_CASTS()

#ifdef SML_MSVC_LEGACY
	};
	// vector (no-op default initialization)
	template <class T, int N>
	struct vec : pod_vec<T, N>
	{
#endif
#ifdef SML_MSVC_LEGACY
		SML_API vec() { }
#else
		SML_API vec() = default;
#endif
		
		template <class T2, int N2>
		SML_API explicit vec(SML_POD_VEC<T2, N2> const& v, T trail = T())
		{
			for (int i = 0; i < N2 && i < N; ++i)
				this->c[i] = T(v.c[i]);
			for (int i = N2; i < N; ++i)
				this->c[i] = trail;
		}

		template <class V>
		SML_API vec(V const& v, typename is_compatible_vec_type<T, N, V>::type *assert_compatible_vec = 0)
		{ *this = *reinterpret_cast<vec const*>(&reinterpret_cast<T const&>(v)); }
		
		template <int C>
		SML_API static vec base(T x = T(1), T zero = T())
		{ vec s; for (int i = 0; i < N; ++i) s.c[i] = (i == C) ? x : zero; return s; }
		SML_API static vec base(int C, T x = T(1), T zero = T())
		{ vec s; for (int i = 0; i < N; ++i) s.c[i] = (i == C) ? x : zero; return s; }
		
#ifdef SML_MSVC_WORKAROUNDS
		// MSVC++ treats operators specially and requires them to be re-declared here to be instantiated in all cases
		SML_API friend vec operator + (vec const& l, vec const& r);
		SML_API friend vec operator - (vec const& l, vec const& r);
		SML_API friend vec operator * (vec const& l, vec const& r);
		SML_API friend vec operator / (vec const& l, vec const& r);
		SML_API friend vec operator % (vec const& l, vec const& r);
		SML_API friend vec operator & (vec const& l, vec const& r);
		SML_API friend vec operator | (vec const& l, vec const& r);
		SML_API friend vec operator ^ (vec const& l, vec const& r);
		SML_API friend vec operator <<(vec const& l, vec const& r);
		SML_API friend vec operator >>(vec const& l, vec const& r);
		
		SML_API friend vec  operator -(pod_vec const& l);
		SML_API friend vec  operator ~(pod_vec const& l);
		SML_API friend typename vec::bvec operator !(pod_vec const& l);
		
		SML_API friend vec operator +(T l, vec const& r);
		SML_API friend vec operator -(T l, vec const& r);
		SML_API friend vec operator *(T l, vec const& r);
		SML_API friend vec operator /(T l, vec const& r);
		
		SML_API friend vec operator +(vec const& l, T r);
		SML_API friend vec operator -(vec const& l, T r);
		SML_API friend vec operator *(vec const& l, T r);
		SML_API friend vec operator /(vec const& l, T r);
#endif
	};

	#undef MAKE_COMPATIBILITY_CAST
	
	typedef vec<float, 1> vec1;
	typedef vec<float, 2> vec2;
	typedef vec<float, 3> vec3;
	typedef vec<float, 4> vec4;

	typedef vec<int, 1> ivec1;
	typedef vec<int, 2> ivec2;
	typedef vec<int, 3> ivec3;
	typedef vec<int, 4> ivec4;

	typedef vec<unsigned, 1> uvec1;
	typedef vec<unsigned, 2> uvec2;
	typedef vec<unsigned, 3> uvec3;
	typedef vec<unsigned, 4> uvec4;

	typedef vec<bool, 1> bvec1;
	typedef vec<bool, 2> bvec2;
	typedef vec<bool, 3> bvec3;
	typedef vec<bool, 4> bvec4;
	
	template <class T, int N>
	SML_API inline T length2(SML_POD_VEC<T, N> const& v) { return dot(v, v); }
	template <class T, int N>
	SML_API inline T length(SML_POD_VEC<T, N> const& v) { return sqrt(dot(v, v)); }
	template <class T, int N>
	SML_API inline vec<T, N> normalize(SML_POD_VEC<T, N> const& v) { return v / sqrt(dot(v, v)); }

	template <class T, int N>
	SML_API inline vec<T, N> reflect(vec<T, N> const& incident, vec<T, N> const& normal) { auto p = dot(incident, normal); return incident - (p + p) * normal; }
	template <class T, int N>
	SML_API inline T distance(vec<T, N> const& a, vec<T, N> const& b) { return length(a - b); }

	// matrix (no-op default initialization)
	template <class T, int R>
	struct mat<T, 0, R>;
	template <class T, int C>
	struct mat<T, C, 0>;
	template <class T>
	struct mat<T, 0, 0>;
	template <class T, int R>
	struct mat<T, 1, R>
	{
		typedef T component;
		static int const columns = 1;
		static int const rows = R;
		typedef vec<T, R> column;
		typedef vec<T, R - 1> col_dehom;

#ifdef SML_TRIVIAL_UNIONS
		SML_POD_VEC<T, R> a;
#else
		union
		{
			SML_POD_VEC<T, R> a;
			SML_POD_VEC<T, R> cls[columns];
			T e[columns][rows];
		};
#endif

		SML_API mat() { }
		SML_API explicit mat(T x) { this->a = column::template base<0>(x); }
		SML_API mat(column const& x) { this->a = x; }
		SML_API mat(col_dehom const& x) { this->a = column(x, T()); }

		template <class T2, int C2, int R2>
		SML_API explicit mat(mat<T2, C2, R2> const& v, typename enable_if<C2 >= columns && R2 >= R>::type* assert_size = nullptr)
		{ this->a = column(v.a); }

		typedef mat<bool, columns, R> bmat;
		typedef mat<T, columns, columns> mat_s;
		typedef mat<T, R, columns> mat_t;
		
		SML_API column& operator [](int i) { return (column&) ((T(&)[columns][rows]) a)[i]; }
		SML_API column  operator [](int i) const { return ((SML_POD_VEC<T, R>(&)[columns]) a)[i]; }
		
		SML_API mat& operator +=(mat const& r) { a += r.a; return *this; }
		SML_API mat& operator -=(mat const& r) { a -= r.a; return *this; }
		
		SML_API mat& operator *=(T r) { a *= r; return *this; }
		SML_API mat& operator /=(T r) { a /= r; return *this; }

		SML_API friend mat operator +(mat const& l, mat const& r) { mat s; s.a = l.a + r.a; return s; }
		SML_API friend mat operator -(mat const& l, mat const& r) { mat s; s.a = l.a - r.a; return s; }

		SML_API friend mat operator *(T l, mat const& r) { mat s; s.a = l * r.a; return s; }
		SML_API friend mat operator /(T l, mat const& r) { mat s; s.a = l / r.a; return s; }

		SML_API friend mat operator *(mat const& l, T r) { mat s; s.a = l.a * r; return s; }
		SML_API friend mat operator /(mat const& l, T r) { mat s; s.a = l.a / r; return s; }

		SML_API static mat from_transposed(mat_t const& l)
		{
			mat s;
			s.a = column::transpose(l.a.c + 0, stride_literal<mat_t::rows>());
			return s;
		}
		SML_API friend mat_t transpose(mat const& l) { return mat_t::from_transposed(l); }
		SML_API friend mat_s operator *(mat_t const& l, mat const& r)
		{
			mat_s s;
			mat l_t = transpose(l);
			s.a = typename mat_s::column( dot(l_t.a, r.a) );
			return s;
		}
		SML_API friend vec<T, columns> operator *(vec<T, R> const& l, mat const& r) { return vec<T, columns>( dot(l, r.a) ); }
		SML_API static vec<T, columns> transposed_transform(mat_t const& l, vec<T, R> const& r)
		{
			return vec<T, columns>(
				  column::dot_transposed(l.a.c + 0, stride_literal<mat_t::rows>(), r)
				); 
		}
		SML_API friend vec<T, R> operator *(mat const& l, vec<T, columns> const& r) { return mat_t::transposed_transform(l, r); }

		SML_API friend mat min(mat const& l, mat const& r) { mat s; s.a = min(l.a, r.a); return s; }
		SML_API friend mat max(mat const& l, mat const& r) { mat s; s.a = max(l.a, r.a); return s; }
		SML_API friend mat abs(mat const& l) { mat s; s.a = abs(l.a); return s; }

		SML_API friend T compMin(mat const& l) { return compMin(l.a); }
		SML_API friend T compMax(mat const& l) { return compMax(l.a); }

		SML_API friend bool operator ==(mat const& l, mat const& r) { return l.a == r.a; }
		SML_API friend bool operator !=(mat const& l, mat const& r) { return l.a != r.a; }
		
		SML_API friend bmat lessThan(mat const& l, mat const& r)      { bmat s; s.a = lessThan(l.a, r.a); return s; }
		SML_API friend bmat lessThanEqual(mat const& l, mat const& r) { bmat s; s.a = lessThanEqual(l.a, r.a); return s; }
		SML_API friend bmat equal(mat const& l, mat const& r)         { bmat s; s.a = equal(l.a, r.a); return s; }
		SML_API friend bmat notEqual(mat const& l, mat const& r)      { bmat s; s.a = notEqual(l.a, r.a); return s; }

		template <class Op>
		SML_API friend mat cw(Op&& op, mat const& l) { mat s; s.a = cw(op, l.a); return s; }
		template <class Op>
		SML_API friend mat cw(Op&& op, mat const& l, mat const& r) { mat s; s.a = cw(op, l.a, r.a); return s; }
	};
	template <class T, int R>
	struct mat<T, 2, R>
	{
		typedef T component;
		static int const columns = 2;
		static int const rows = R;
		typedef vec<T, R> column;
		typedef vec<T, R - 1> col_dehom;
		
#ifdef SML_TRIVIAL_UNIONS
		SML_POD_VEC<T, R> a, b;
#else
		union
		{
			struct { SML_POD_VEC<T, R> a, b; };
			SML_POD_VEC<T, R> cls[columns];
			T e[columns][rows];
		};
#endif
		
		SML_API mat() { }
		SML_API explicit mat(T x) { this->a = column::template base<0>(x); this->b = column::template base<1>(x); }
		SML_API mat(column const& x, column const& y)
		{ this->a = x; this->b = y; }
		SML_API mat(col_dehom const& x, col_dehom const& y)
		{ this->a = column(x, T()); this->b = column(y, T()); }

		template <class T2, int C2, int R2>
		SML_API explicit mat(mat<T2, C2, R2> const& v, typename enable_if<C2 >= columns && R2 >= R>::type* assert_size = nullptr)
		{ this->a = column(v.ca); this->b = column(v.b); }
		
		typedef mat<bool, columns, R> bmat;
		typedef mat<T, columns, columns> mat_s;
		typedef mat<T, R, columns> mat_t;
		
		SML_API column& operator [](int i) { return (column&) ((T(&)[columns][rows]) a)[i]; }
		SML_API column  operator [](int i) const { return ((SML_POD_VEC<T, R>(&)[columns]) a)[i]; }
		
		SML_API mat& operator +=(mat const& r) { a += r.a; b += r.b; return *this; }
		SML_API mat& operator -=(mat const& r) { a -= r.a; b -= r.b; return *this; }

		SML_API mat& operator *=(T r) { a *= r; b *= r; return *this; }
		SML_API mat& operator /=(T r) { a /= r; b /= r; return *this; }

		SML_API friend mat operator +(mat const& l, mat const& r) { mat s; s.a = l.a + r.a; s.b = l.b + r.b; return s; }
		SML_API friend mat operator -(mat const& l, mat const& r) { mat s; s.a = l.a - r.a; s.b = l.b - r.b; return s; }

		SML_API friend mat operator *(T l, mat const& r) { mat s; s.a = l * r.a; s.b = l * r.b; return s; }
		SML_API friend mat operator /(T l, mat const& r) { mat s; s.a = l / r.a; s.b = l / r.b; return s; }

		SML_API friend mat operator *(mat const& l, T r) { mat s; s.a = l.a * r; s.b = l.b * r; return s; }
		SML_API friend mat operator /(mat const& l, T r) { mat s; s.a = l.a / r; s.b = l.b / r; return s; }

		SML_API static mat from_transposed(mat_t const& l)
		{
			mat s;
			s.a = column::transpose(l.a.c + 0, stride_literal<mat_t::rows>());
			s.b = column::transpose(l.a.c + 1, stride_literal<mat_t::rows>());
			return s;
		}
		SML_API friend mat_t transpose(mat const& l) { return mat_t::from_transposed(l); }
		SML_API friend mat_s operator *(mat_t const& l, mat const& r)
		{
			mat_s s;
			mat l_t = transpose(l);
			s.a = typename mat_s::column( dot(l_t.a, r.a), dot(l_t.b, r.a) );
			s.b = typename mat_s::column( dot(l_t.a, r.b), dot(l_t.b, r.b) );
			return s;
		}
		SML_API friend vec<T, columns> operator *(vec<T, R> const& l, mat const& r) { return vec<T, columns>( dot(l, r.a), dot(l, r.b) ); }
		SML_API static vec<T, columns> transposed_transform(mat_t const& l, vec<T, R> const& r)
		{
			return vec<T, columns>(
				  column::dot_transposed(l.a.c + 0, stride_literal<mat_t::rows>(), r)
				, column::dot_transposed(l.a.c + 1, stride_literal<mat_t::rows>(), r)
				);
		}
		SML_API friend vec<T, R> operator *(mat const& l, vec<T, columns> const& r) { return mat_t::transposed_transform(l, r); }

		SML_API friend mat min(mat const& l, mat const& r) { mat s; s.a = min(l.a, r.a); s.b = min(l.b, r.b); return s; }
		SML_API friend mat max(mat const& l, mat const& r) { mat s; s.a = max(l.a, r.a); s.b = max(l.b, r.b); return s; }
		SML_API friend mat abs(mat const& l) { mat s; s.a = abs(l.a); s.b = abs(l.b); return s; }
		
		SML_API friend T compMin(mat const& l) { return min(compMin(l.a), compMin(l.b)); }
		SML_API friend T compMax(mat const& l) { return max(compMax(l.a), compMax(l.b)); }

		SML_API friend bool operator ==(mat const& l, mat const& r) { return l.a == r.a && l.b == r.b; }
		SML_API friend bool operator !=(mat const& l, mat const& r) { return l.a != r.a || l.b != r.b; }
		
		SML_API friend bmat lessThan(mat const& l, mat const& r)      { bmat s; s.a = lessThan(l.a, r.a); s.b = lessThan(l.b, r.b); return s; }
		SML_API friend bmat lessThanEqual(mat const& l, mat const& r) { bmat s; s.a = lessThanEqual(l.a, r.a); s.b = lessThanEqual(l.b, r.b); return s; }
		SML_API friend bmat equal(mat const& l, mat const& r)         { bmat s; s.a = equal(l.a, r.a); s.b = equal(l.b, r.b); return s; }
		SML_API friend bmat notEqual(mat const& l, mat const& r)      { bmat s; s.a = notEqual(l.a, r.a); s.b = notEqual(l.b, r.b); return s; }

		template <class Op>
		SML_API friend mat cw(Op&& op, mat const& l) { mat s; s.a = cw(op, l.a); s.b = cw(op, l.b); return s; }
		template <class Op>
		SML_API friend mat cw(Op&& op, mat const& l, mat const& r) { mat s; s.a = cw(op, l.a, r.a); s.b = cw(op, l.b, r.b); return s; }
	};
	template <class T, int R>
	struct mat<T, 3, R>
	{
		typedef T component;
		static int const columns = 3;
		static int const rows = R;
		typedef vec<T, R> column;
		typedef vec<T, R - 1> col_dehom;

#ifdef SML_TRIVIAL_UNIONS
		SML_POD_VEC<T, R> a, b, c;
#else
		union
		{
			struct { SML_POD_VEC<T, R> a, b, c; };
			SML_POD_VEC<T, R> cls[columns];
			T e[columns][rows];
		};
#endif
		
		SML_API mat() { }
		SML_API explicit mat(T x) { this->a = column::template base<0>(x); this->b = column::template base<1>(x); this->c = column::template base<2>(x); }
		SML_API mat(column const& x, column const& y, column const& z)
		{ this->a = x; this->b = y; this->c = z; }
		SML_API mat(col_dehom const& x, col_dehom const& y, col_dehom const& z)
		{ this->a = column(x, T()); this->b = column(y, T()); this->c = column(z, T()); }
		SML_API mat(mat<T, 2, R> const& m, column const& w)
		{ this->a = m.a; this->b = m.b; this->c = w; }
		SML_API mat(mat<T, 2, R - 1> const& m, col_dehom const& w)
		{ this->a = column(m.a, T()); this->b = column(m.b, T()); this->c = column(w, T(1)); }

		template <class T2, int C2, int R2>
		SML_API explicit mat(mat<T2, C2, R2> const& v, typename enable_if<C2 >= columns && R2 >= R>::type* assert_size = nullptr)
		{ this->a = column(v.a); this->b = column(v.b); this->c = column(v.c); }
		
		typedef mat<bool, columns, R> bmat;
		typedef mat<T, columns, columns> mat_s;
		typedef mat<T, R, columns> mat_t;
		
		SML_API column& operator [](int i) { return (column&) ((T(&)[columns][rows]) a)[i]; }
		SML_API column  operator [](int i) const { return ((SML_POD_VEC<T, R>(&)[columns]) a)[i]; }

		SML_API mat& operator +=(mat const& r) { a += r.a; b += r.b; c += r.c; return *this; }
		SML_API mat& operator -=(mat const& r) { a -= r.a; b -= r.b; c -= r.c; return *this; }
		
		SML_API mat& operator *=(T r) { a *= r; b *= r; c *= r; return *this; }
		SML_API mat& operator /=(T r) { a /= r; b /= r; c /= r; return *this; }

		SML_API friend mat operator + (mat const& l, mat const& r) { mat s; s.a = l.a +  r.a; s.b = l.b +  r.b; s.c = l.c +  r.c; return s; }
		SML_API friend mat operator - (mat const& l, mat const& r) { mat s; s.a = l.a -  r.a; s.b = l.b -  r.b; s.c = l.c -  r.c; return s; }
		
		SML_API friend mat operator *(T l, mat const& r) { mat s; s.a = l * r.a; s.b = l * r.b; s.c = l * r.c; return s; }
		SML_API friend mat operator /(T l, mat const& r) { mat s; s.a = l / r.a; s.b = l / r.b; s.c = l / r.c; return s; }
		
		SML_API friend mat operator *(mat const& l, T r) { mat s; s.a = l.a * r; s.b = l.b * r; s.c = l.c * r; return s; }
		SML_API friend mat operator /(mat const& l, T r) { mat s; s.a = l.a / r; s.b = l.b / r; s.c = l.c / r; return s; }
		
		SML_API static mat from_transposed(mat_t const& l)
		{
			mat s;
			s.a = column::transpose(l.a.c + 0, stride_literal<mat_t::rows>());
			s.b = column::transpose(l.a.c + 1, stride_literal<mat_t::rows>());
			s.c = column::transpose(l.a.c + 2, stride_literal<mat_t::rows>());
			return s;
		}
		SML_API friend mat_t transpose(mat const& l) { return mat_t::from_transposed(l); }
		SML_API friend mat_s operator *(mat_t const& l, mat const& r)
		{
			mat_s s;
			mat l_t = transpose(l);
			s.a = typename mat_s::column( dot(l_t.a, r.a), dot(l_t.b, r.a), dot(l_t.c, r.a) );
			s.b = typename mat_s::column( dot(l_t.a, r.b), dot(l_t.b, r.b), dot(l_t.c, r.b) );
			s.c = typename mat_s::column( dot(l_t.a, r.c), dot(l_t.b, r.c), dot(l_t.c, r.c) );
			return s;
		}
		SML_API friend vec<T, columns> operator *(vec<T, R> const& l, mat const& r) { return vec<T, columns>( dot(l, r.a), dot(l, r.b), dot(l, r.c) ); }
		SML_API static vec<T, columns> transposed_transform(mat_t const& l, vec<T, R> const& r)
		{
			return vec<T, columns>(
				  column::dot_transposed(l.a.c + 0, stride_literal<mat_t::rows>(), r)
				, column::dot_transposed(l.a.c + 1, stride_literal<mat_t::rows>(), r)
				, column::dot_transposed(l.a.c + 2, stride_literal<mat_t::rows>(), r)
				); 
		}
		SML_API friend vec<T, R> operator *(mat const& l, vec<T, columns> const& r) { return mat_t::transposed_transform(l, r); }

		SML_API friend mat min(mat const& l, mat const& r) { mat s; s.a = min(l.a, r.a); s.b = min(l.b, r.b); s.c = min(l.c, r.c); return s; }
		SML_API friend mat max(mat const& l, mat const& r) { mat s; s.a = max(l.a, r.a); s.b = max(l.b, r.b); s.c = max(l.c, r.c); return s; }
		SML_API friend mat abs(mat const& l) { mat s; s.a = abs(l.a); s.b = abs(l.b); s.c = abs(l.c); return s; }
		
		SML_API friend T compMin(mat const& l) { return min(min(compMin(l.a), compMin(l.b)), compMin(l.c)); }
		SML_API friend T compMax(mat const& l) { return max(max(compMax(l.a), compMax(l.b)), compMax(l.c)); }

		SML_API friend bool operator ==(mat const& l, mat const& r) { return l.a == r.a && l.b == r.b && l.c == r.c; }
		SML_API friend bool operator !=(mat const& l, mat const& r) { return l.a != r.a || l.b != r.b || l.c != r.c; }
		
		SML_API friend bmat lessThan(mat const& l, mat const& r)      { bmat s; s.a = lessThan(l.a, r.a); s.b = lessThan(l.b, r.b); s.c = lessThan(l.c, r.c); return s; }
		SML_API friend bmat lessThanEqual(mat const& l, mat const& r) { bmat s; s.a = lessThanEqual(l.a, r.a); s.b = lessThanEqual(l.b, r.b); s.c = lessThanEqual(l.c, r.c); return s; }
		SML_API friend bmat equal(mat const& l, mat const& r)         { bmat s; s.a = equal(l.a, r.a); s.b = equal(l.b, r.b); s.c = equal(l.c, r.c); return s; }
		SML_API friend bmat notEqual(mat const& l, mat const& r)      { bmat s; s.a = notEqual(l.a, r.a); s.b = notEqual(l.b, r.b); s.c = notEqual(l.c, r.c); return s; }

		template <class Op>
		SML_API friend mat cw(Op&& op, mat const& l) { mat s; s.a = cw(op, l.a); s.b = cw(op, l.b); s.c = cw(op, l.c); return s; }
		template <class Op>
		SML_API friend mat cw(Op&& op, mat const& l, mat const& r) { mat s; s.a = cw(op, l.a, r.a); s.b = cw(op, l.b, r.b); s.c = cw(op, l.c, r.c); return s; }
	};
	template <class T, int R>
	struct mat<T, 4, R>
	{
		typedef T component;
		static int const columns = 4;
		static int const rows = R;
		typedef vec<T, R> column;
		typedef vec<T, R - 1> col_dehom;

#ifdef SML_TRIVIAL_UNIONS
		SML_POD_VEC<T, R> a, b, c, d;
#else
		union
		{
			struct { SML_POD_VEC<T, R> a, b, c, d; };
			SML_POD_VEC<T, R> cls[columns];
			T e[columns][rows];
		};
#endif

		SML_API mat() { }
		SML_API explicit mat(T x) { this->a = column::template base<0>(x); this->b = column::template base<1>(x); this->c = column::template base<2>(x); this->d = column::template base<3>(x); }
		SML_API mat(column const& x, column const& y, column const& z, column const& w)
		{ this->a = x; this->b = y; this->c = z; this->d = w; }
		SML_API mat(col_dehom const& x, col_dehom const& y, col_dehom const& z, col_dehom const& w)
		{ this->a = column(x, T()); this->b = column(y, T()); this->c = column(z, T()); this->d = column(x, T(1)); }
		SML_API mat(mat<T, 3, R> const& m, column const& w)
		{ this->a = m.a; this->b = m.b; this->c = m.c; this->d = w; }
		SML_API mat(mat<T, 3, R - 1> const& m, col_dehom const& w)
		{ this->a = column(m.a, T()); this->b = column(m.b, T()); this->c = column(m.c, T()); this->d = column(w, T(1)); }

		template <class T2, int C2, int R2>
		SML_API explicit mat(mat<T2, C2, R2> const& v, typename enable_if<C2 >= columns && R2 >= R>::type* assert_size = nullptr)
		{ this->a = column(v.a); this->b = column(v.b); this->c = column(v.c); this->d = column(v.d); }
		
		typedef mat<bool, columns, R> bmat;
		typedef mat<T, columns, columns> mat_s;
		typedef mat<T, R, columns> mat_t;
		
		SML_API column& operator [](int i) { return (column&) ((T(&)[columns][rows]) a)[i]; }
		SML_API column  operator [](int i) const { return ((SML_POD_VEC<T, R>(&)[columns]) a)[i]; }
		
		SML_API mat& operator +=(mat const& r) { a += r.a; b += r.b; c += r.c; d += r.d; return *this; }
		SML_API mat& operator -=(mat const& r) { a -= r.a; b -= r.b; c -= r.c; d -= r.d; return *this; }

		SML_API mat& operator *=(T r) { a *= r; b *= r; c *= r; d *= r; return *this; }
		SML_API mat& operator /=(T r) { a /= r; b /= r; c /= r; d /= r; return *this; }
		
		SML_API friend mat operator + (mat const& l, mat const& r) { mat s; s.a = l.a +  r.a; s.b = l.b +  r.b; s.c = l.c +  r.c; s.d = l.d +  r.d; return s; }
		SML_API friend mat operator - (mat const& l, mat const& r) { mat s; s.a = l.a -  r.a; s.b = l.b -  r.b; s.c = l.c -  r.c; s.d = l.d -  r.d; return s; }
		
		SML_API friend mat operator *(T l, mat const& r) { mat s; s.a = l * r.a; s.b = l * r.b; s.c = l * r.c; s.d = l * r.d; return s; }
		SML_API friend mat operator /(T l, mat const& r) { mat s; s.a = l / r.a; s.b = l / r.b; s.c = l / r.c; s.d = l / r.d; return s; }
		
		SML_API friend mat operator *(mat const& l, T r) { mat s; s.a = l.a * r; s.b = l.b * r; s.c = l.c * r; s.d = l.d * r; return s; }
		SML_API friend mat operator /(mat const& l, T r) { mat s; s.a = l.a / r; s.b = l.b / r; s.c = l.c / r; s.d = l.d / r; return s; }
		
		SML_API static mat from_transposed(mat_t const& l)
		{
			mat s;
			s.a = column::transpose(l.a.c + 0, stride_literal<mat_t::rows>());
			s.b = column::transpose(l.a.c + 1, stride_literal<mat_t::rows>());
			s.c = column::transpose(l.a.c + 2, stride_literal<mat_t::rows>());
			s.d = column::transpose(l.a.c + 3, stride_literal<mat_t::rows>());
			return s;
		}
		SML_API friend mat_t transpose(mat const& l) { return mat_t::from_transposed(l); }
		SML_API friend mat_s operator *(mat_t const& l, mat const& r)
		{
			mat_s s;
			mat l_t = transpose(l);
			s.a = typename mat_s::column( dot(l_t.a, r.a), dot(l_t.b, r.a), dot(l_t.c, r.a), dot(l_t.d, r.a) );
			s.b = typename mat_s::column( dot(l_t.a, r.b), dot(l_t.b, r.b), dot(l_t.c, r.b), dot(l_t.d, r.b) );
			s.c = typename mat_s::column( dot(l_t.a, r.c), dot(l_t.b, r.c), dot(l_t.c, r.c), dot(l_t.d, r.c) );
			s.d = typename mat_s::column( dot(l_t.a, r.d), dot(l_t.b, r.d), dot(l_t.c, r.d), dot(l_t.d, r.d) );
			return s;
		}
		SML_API friend vec<T, columns> operator *(vec<T, R> const& l, mat const& r) { return vec<T, columns>( dot(l, r.a), dot(l, r.b), dot(l, r.c), dot(l, r.d) ); }
		SML_API static vec<T, columns> transposed_transform(mat_t const& l, vec<T, R> const& r)
		{
			return vec<T, columns>(
				  column::dot_transposed(l.a.c + 0, stride_literal<mat_t::rows>(), r)
				, column::dot_transposed(l.a.c + 1, stride_literal<mat_t::rows>(), r)
				, column::dot_transposed(l.a.c + 2, stride_literal<mat_t::rows>(), r)
				, column::dot_transposed(l.a.c + 3, stride_literal<mat_t::rows>(), r)
				); 
		}
		SML_API friend vec<T, R> operator *(mat const& l, vec<T, columns> const& r) { return mat_t::transposed_transform(l, r); }

		SML_API friend mat min(mat const& l, mat const& r) { mat s; s.a = min(l.a, r.a); s.b = min(l.b, r.b); s.c = min(l.c, r.c); s.d = min(l.d, r.d); return s; }
		SML_API friend mat max(mat const& l, mat const& r) { mat s; s.a = max(l.a, r.a); s.b = max(l.b, r.b); s.c = max(l.c, r.c); s.d = max(l.d, r.d); return s; }
		SML_API friend mat abs(mat const& l) { mat s; s.a = abs(l.a); s.b = abs(l.b); s.c = abs(l.c); s.d = abs(l.d); return s; }
		
		SML_API friend T compMin(mat const& l) { return min(min(compMin(l.a), compMin(l.b)), min(compMin(l.c), compMin(l.d))); }
		SML_API friend T compMax(mat const& l) { return max(max(compMax(l.a), compMax(l.b)), max(compMax(l.c), compMax(l.d))); }

		SML_API friend bool operator ==(mat const& l, mat const& r) { return l.a == r.a && l.b == r.b && l.c == r.c && l.d == r.d; }
		SML_API friend bool operator !=(mat const& l, mat const& r) { return l.a != r.a || l.b != r.b || l.c != r.c || l.d != r.d; }
		
		SML_API friend bmat lessThan(mat const& l, mat const& r)      { bmat s; s.a = lessThan(l.a, r.a); s.b = lessThan(l.b, r.b); s.c = lessThan(l.c, r.c); s.d = lessThan(l.d, r.d); return s; }
		SML_API friend bmat lessThanEqual(mat const& l, mat const& r) { bmat s; s.a = lessThanEqual(l.a, r.a); s.b = lessThanEqual(l.b, r.b); s.c = lessThanEqual(l.c, r.c); s.d = lessThanEqual(l.d, r.d); return s; }
		SML_API friend bmat equal(mat const& l, mat const& r)         { bmat s; s.a = equal(l.a, r.a); s.b = equal(l.b, r.b); s.c = equal(l.c, r.c); s.d = equal(l.d, r.d); return s; }
		SML_API friend bmat notEqual(mat const& l, mat const& r)      { bmat s; s.a = notEqual(l.a, r.a); s.b = notEqual(l.b, r.b); s.c = notEqual(l.c, r.c); s.d = notEqual(l.d, r.d); return s; }

		template <class Op>
		SML_API friend mat cw(Op&& op, mat const& l) { mat s; s.a = cw(op, l.a); s.b = cw(op, l.b); s.c = cw(op, l.c); s.d = cw(op, l.d); return s; }
		template <class Op>
		SML_API friend mat cw(Op&& op, mat const& l, mat const& r) { mat s; s.a = cw(op, l.a, r.a); s.b = cw(op, l.b, r.b); s.c = cw(op, l.c, r.c); s.d = cw(op, l.d, r.d); return s; }
	};
	template <class T, int C, int R>
	struct mat
	{
		typedef T component;
		static int const columns = C;
		static int const rows = R;
		typedef vec<T, R> column;
		
#ifdef SML_TRIVIAL_UNIONS
		SML_POD_VEC<T, R> cls[columns];
#else
		union
		{
			SML_POD_VEC<T, R> cls[columns];
			T e[columns][rows];
		};
#endif

		SML_API mat() { }
		SML_API explicit mat(T x) { for (int i = 0; i < C; ++i) cls[i] = column::base(i, x); }

		typedef mat<bool, C, R> bmat;
		typedef mat<T, C, C> mat_s;
		typedef mat<T, R, C> mat_t;
		
		SML_API column& operator [](int i) { return (column&) (T(&)[rows]) cls[i]; }
		SML_API column  operator [](int i) const { return cls[i]; }
		
		SML_API mat& operator +=(mat const& r) { for (int i = 0; i < C; ++i) cls[i] += r.cls[i]; return *this; }
		SML_API mat& operator -=(mat const& r) { for (int i = 0; i < C; ++i) cls[i] -= r.cls[i]; return *this; }
		
		SML_API mat& operator *=(T r) { for (int i = 0; i < C; ++i) cls[i] *= r; return *this; }
		SML_API mat& operator /=(T r) { for (int i = 0; i < C; ++i) cls[i] /= r; return *this; }
		
		SML_API friend mat operator +(mat const& l, mat const& r) { mat s; for (int i = 0; i < C; ++i) s.cls[i] = l.cls[i] + r.cls[i]; return s; }
		SML_API friend mat operator -(mat const& l, mat const& r) { mat s; for (int i = 0; i < C; ++i) s.cls[i] = l.cls[i] - r.cls[i]; return s; }
		
		SML_API friend mat operator *(T l, mat const& r) { mat s; for (int i = 0; i < C; ++i) s.cls[i] = l * r.cls[i]; return s; }
		SML_API friend mat operator /(T l, mat const& r) { mat s; for (int i = 0; i < C; ++i) s.cls[i] = l / r.cls[i]; return s; }

		SML_API friend mat operator *(mat const& l, T r) { mat s; for (int i = 0; i < C; ++i) s.cls[i] = l.cls[i] * r; return s; }
		SML_API friend mat operator /(mat const& l, T r) { mat s; for (int i = 0; i < C; ++i) s.cls[i] = l.cls[i] / r; return s; }
		
		SML_API static mat from_transposed(mat_t const& l)
		{
			mat s;
			for (int i = 0; i < C; ++i) 
				s.cls[i] = column::transpose(l.cls[0].c + i, stride_literal<mat_t::rows>());
			return s;
		}
		SML_API friend mat_t transpose(mat const& l) { return mat_t::from_transposed(l); }
		SML_API friend mat_s operator * (mat_t const& l, mat const& r)
		{
			mat_s s;
			for (int i = 0; i < C; ++i) 
				for (int j = 0; j < C; ++i) 
					s.cls[i].c[j] = column::dot_transposed(l.cls[0].c + j, stride_literal<mat_t::rows>(), r.cls[i]);
			return s;
		}
		SML_API friend vec<T, C> operator *(vec<T, R> const& l, mat const& r)
		{
			vec<T, C> s;
			for (int i = 0; i < C; ++i)
				s.c[i] = dot(l, r.cls[i]);
			return s;
		}
		SML_API static vec<T, C> transposed_transform(mat_t const& l, vec<T, R> const& r)
		{
			vec<T, C> s;
			for (int i = 0; i < C; ++i)
				s.c[i] = column::dot_transposed(l.cls[0].c + i, stride_literal<mat_t::rows>(), r);
			return s;
		}
		SML_API friend vec<T, R> operator *(mat const& l, vec<T, C> const& r) { return mat_t::transposed_transform(l, r); }

		SML_API friend mat min(mat const& l, mat const& r) { mat s; for (int i = 0; i < C; ++i) s.cls[i] = min(l.cls[i], r.cls[i]); return s; }
		SML_API friend mat max(mat const& l, mat const& r) { mat s; for (int i = 0; i < C; ++i) s.cls[i] = max(l.cls[i], r.cls[i]); return s; }
		SML_API friend mat abs(mat const& l) { mat s; for (int i = 0; i < C; ++i) s.cls[i] = abs(l.cls[i]); return s; }

		SML_API friend T compMin(mat const& l) { T s = compMin(l.cls[0]); for (int i = 1; i < C; ++i) s = min(compMin(l.cls[i]), s); return s; }
		SML_API friend T compMax(mat const& l) { T s = compMax(l.cls[0]); for (int i = 1; i < C; ++i) s = max(compMax(l.cls[i]), s); return s; }

		SML_API friend bool operator ==(mat const& l, mat const& r) { bool s = true;  for (int i = 0; i < C; ++i) s &= (l.cls[i] == r.cls[i]); return s; }
		SML_API friend bool operator !=(mat const& l, mat const& r) { bool s = false; for (int i = 0; i < C; ++i) s |= (l.cls[i] != r.cls[i]); return s; }
		
		SML_API friend bmat lessThan(mat const& l, mat const& r)      { bmat s; for (int i = 0; i < C; ++i) s.cls[i] = lessThan(l.cls[i], r.cls[i]); return s; }
		SML_API friend bmat lessThanEqual(mat const& l, mat const& r) { bmat s; for (int i = 0; i < C; ++i) s.cls[i] = lessThanEqual(l.cls[i], r.cls[i]); return s; }
		SML_API friend bmat equal(mat const& l, mat const& r)         { bmat s; for (int i = 0; i < C; ++i) s.cls[i] = equal(l.cls[i], r.cls[i]); return s; }
		SML_API friend bmat notEqual(mat const& l, mat const& r)      { bmat s; for (int i = 0; i < C; ++i) s.cls[i] = notEqual(l.cls[i], r.cls[i]); return s; }

		template <class Op>
		SML_API friend mat cw(Op&& op, mat const& l) { mat s; for (int i = 0; i < C; ++i) s.cls[i] = cw(op, l.cls[i]); return s; }
		template <class Op>
		SML_API friend mat cw(Op&& op, mat const& l, mat const& r) { mat s; for (int i = 0; i < C; ++i) s.cls[i] = cw(op, l.cls[i], r.cls[i]); return s; }
	};

	typedef mat<float, 1, 1> mat1, mat1x1;
	typedef mat<float, 2, 2> mat2, mat2x2;
	typedef mat<float, 3, 3> mat3, mat3x3;
	typedef mat<float, 4, 4> mat4, mat4x4;
	typedef mat<float, 3, 2> mat3x2; // 3 columns, 2 rows
	typedef mat<float, 2, 3> mat2x3; // 2 columns, 3 rows
	typedef mat<float, 4, 3> mat4x3; // 4 columns, 3 rows
	typedef mat<float, 3, 4> mat3x4; // 3 columns, 4 rows

	typedef mat<bool, 1, 1> bmat1, bmat1x1;
	typedef mat<bool, 2, 2> bmat2, bmat2x2;
	typedef mat<bool, 3, 3> bmat3, bmat3x3;
	typedef mat<bool, 4, 4> bmat4, bmat4x4;
	typedef mat<bool, 3, 2> bmat3x2; // 3 columns, 2 rows
	typedef mat<bool, 2, 3> bmat2x3; // 2 columns, 3 rows
	typedef mat<bool, 4, 3> bmat4x3; // 4 columns, 3 rows
	typedef mat<bool, 3, 4> bmat3x4; // 3 columns, 4 rows

	template <class T>
	SML_API mat<T, 2> inverse(mat<T, 2> const& m)
	{
		mat<T, 2> s;
		T one_over_determinant = T(1) / (m.a.c[0] * m.b.c[1] - m.b.c[0] * m.a.c[1]);
		s.a.c[0] = +m.b.c[1] * one_over_determinant;
		s.a.c[1] = -m.a.c[1] * one_over_determinant;
		s.b.c[0] = -m.b.c[0] * one_over_determinant;
		s.b.c[1] = +m.a.c[0] * one_over_determinant;
		return s;
	}

	template <class T>
	SML_API mat<T, 3> inverse(mat<T, 3> const& m)
	{
		mat<T, 3> s;
		T one_over_determinant = T(1) / (
			+ m.a[0] * (m.b.c[1] * m.c.c[2] - m.c.c[1] * m.b.c[2])
			- m.b[0] * (m.a.c[1] * m.c.c[2] - m.c.c[1] * m.a.c[2])
			+ m.c[0] * (m.a.c[1] * m.b.c[2] - m.b.c[1] * m.a.c[2])
			);
		s.a.c[0] = + (m.b.c[1] * m.c.c[2] - m.c.c[1] * m.b.c[2]) * one_over_determinant;
		s.b.c[0] = - (m.b.c[0] * m.c.c[2] - m.c.c[0] * m.b.c[2]) * one_over_determinant;
		s.c.c[0] = + (m.b.c[0] * m.c.c[1] - m.c.c[0] * m.b.c[1]) * one_over_determinant;
		s.a.c[1] = - (m.a.c[1] * m.c.c[2] - m.c.c[1] * m.a.c[2]) * one_over_determinant;
		s.b.c[1] = + (m.a.c[0] * m.c.c[2] - m.c.c[0] * m.a.c[2]) * one_over_determinant;
		s.c.c[1] = - (m.a.c[0] * m.c.c[1] - m.c.c[0] * m.a.c[1]) * one_over_determinant;
		s.a.c[2] = + (m.a.c[1] * m.b.c[2] - m.b.c[1] * m.a.c[2]) * one_over_determinant;
		s.b.c[2] = - (m.a.c[0] * m.b.c[2] - m.b.c[0] * m.a.c[2]) * one_over_determinant;
		s.c.c[2] = + (m.a.c[0] * m.b.c[1] - m.b.c[0] * m.a.c[1]) * one_over_determinant;
		return s;
	}

	template <class T>
	SML_API mat<T, 4> inverse(mat<T, 4> const& m)
	{
		T Coef00 = m.e[2][2] * m.e[3][3] - m.e[3][2] * m.e[2][3];
		T Coef02 = m.e[1][2] * m.e[3][3] - m.e[3][2] * m.e[1][3];
		T Coef03 = m.e[1][2] * m.e[2][3] - m.e[2][2] * m.e[1][3];

		T Coef04 = m.e[2][1] * m.e[3][3] - m.e[3][1] * m.e[2][3];
		T Coef06 = m.e[1][1] * m.e[3][3] - m.e[3][1] * m.e[1][3];
		T Coef07 = m.e[1][1] * m.e[2][3] - m.e[2][1] * m.e[1][3];

		T Coef08 = m.e[2][1] * m.e[3][2] - m.e[3][1] * m.e[2][2];
		T Coef10 = m.e[1][1] * m.e[3][2] - m.e[3][1] * m.e[1][2];
		T Coef11 = m.e[1][1] * m.e[2][2] - m.e[2][1] * m.e[1][2];

		T Coef12 = m.e[2][0] * m.e[3][3] - m.e[3][0] * m.e[2][3];
		T Coef14 = m.e[1][0] * m.e[3][3] - m.e[3][0] * m.e[1][3];
		T Coef15 = m.e[1][0] * m.e[2][3] - m.e[2][0] * m.e[1][3];

		T Coef16 = m.e[2][0] * m.e[3][2] - m.e[3][0] * m.e[2][2];
		T Coef18 = m.e[1][0] * m.e[3][2] - m.e[3][0] * m.e[1][2];
		T Coef19 = m.e[1][0] * m.e[2][2] - m.e[2][0] * m.e[1][2];

		T Coef20 = m.e[2][0] * m.e[3][1] - m.e[3][0] * m.e[2][1];
		T Coef22 = m.e[1][0] * m.e[3][1] - m.e[3][0] * m.e[1][1];
		T Coef23 = m.e[1][0] * m.e[2][1] - m.e[2][0] * m.e[1][1];

		vec<T, 4> Fac0(Coef00, Coef00, Coef02, Coef03);
		vec<T, 4> Fac1(Coef04, Coef04, Coef06, Coef07);
		vec<T, 4> Fac2(Coef08, Coef08, Coef10, Coef11);
		vec<T, 4> Fac3(Coef12, Coef12, Coef14, Coef15);
		vec<T, 4> Fac4(Coef16, Coef16, Coef18, Coef19);
		vec<T, 4> Fac5(Coef20, Coef20, Coef22, Coef23);

		vec<T, 4> Vec0(m.e[1][0], m.e[0][0], m.e[0][0], m.e[0][0]);
		vec<T, 4> Vec1(m.e[1][1], m.e[0][1], m.e[0][1], m.e[0][1]);
		vec<T, 4> Vec2(m.e[1][2], m.e[0][2], m.e[0][2], m.e[0][2]);
		vec<T, 4> Vec3(m.e[1][3], m.e[0][3], m.e[0][3], m.e[0][3]);

		vec<T, 4> SignA(+1, -1, +1, -1);
		vec<T, 4> SignB(-1, +1, -1, +1);

		mat<T, 4> s;
		s.a = (Vec1 * Fac0 - Vec2 * Fac1 + Vec3 * Fac2) * SignA;
		s.b = (Vec0 * Fac0 - Vec2 * Fac3 + Vec3 * Fac4) * SignB;
		s.c = (Vec0 * Fac1 - Vec1 * Fac3 + Vec3 * Fac5) * SignA;
		s.d = (Vec0 * Fac2 - Vec1 * Fac4 + Vec2 * Fac5) * SignB;

		vec<T, 4> Row0(s.e[0][0], s.e[1][0], s.e[2][0], s.e[3][0]);
		s *= static_cast<T>(1) / dot(m.cls[0], Row0);

		return s;
	}

	template <class T>
	SML_API inline mat<T, 3> orientation(T angle, typename id< vec<T, 3> >::t const& v)
	{
		mat<T, 3> s;

		T const cosA = cos(angle);
		T const sinA = sin(angle);

		vec<T, 3> axis(v);
		vec<T, 3> temp((T(1) - cosA) * axis);
	
		s.e[0][0] = cosA + temp.c[0] * axis.c[0];
		s.e[0][1] =    0 + temp.c[0] * axis.c[1] + sinA * axis.c[2];
		s.e[0][2] =    0 + temp.c[0] * axis.c[2] - sinA * axis.c[1];

		s.e[1][0] =    0 + temp.c[1] * axis.c[0] - sinA * axis.c[2];
		s.e[1][1] = cosA + temp.c[1] * axis.c[1];
		s.e[1][2] =    0 + temp.c[1] * axis.c[2] + sinA * axis.c[0];

		s.e[2][0] =    0 + temp.c[2] * axis.c[0] + sinA * axis.c[1];
		s.e[2][1] =    0 + temp.c[2] * axis.c[1] - sinA * axis.c[0];
		s.e[2][2] = cosA + temp.c[2] * axis.c[2];

		return s;
	}

	template <class T>
	SML_API inline mat<T, 4> rotate(T angle, typename id< vec<T, 3> >::t const& axis)
	{
		return mat<T, 4>( orientation(angle, axis), vec<T, 3>(T(0)) );
	}

	template <class T>
	SML_API inline mat<T, 4> scale(vec<T, 3> const& scale)
	{
		mat<T, 4> s;
		s.a = vec<T, 4>(T(scale.x), T(0), T(0), T(0));
		s.b = vec<T, 4>(T(0), T(scale.y), T(0), T(0));
		s.c = vec<T, 4>(T(0), T(0), T(scale.z), T(0));
		s.d = vec<T, 4>(T(0), T(0), T(0), T(1));
		return s;
	}

	template <class T>
	SML_API inline mat<T, 4> translate(vec<T, 3> const& offset)
	{
		mat<T, 4> s;
		s.a = vec<T, 4>(T(1), T(0), T(0), T(0));
		s.b = vec<T, 4>(T(0), T(1), T(0), T(0));
		s.c = vec<T, 4>(T(0), T(0), T(1), T(0));
		s.d = vec<T, 4>(offset, T(1));
		return s;
	}

	template <class T>
	SML_API inline mat<T, 4> transform(vec<T, 3> const& offset, typename id< vec<T, 3> >::t const& scale = T(1), typename id< mat<T, 3> >::t const& orientation = mat<T, 3>(T(1)))
	{
		mat<T, 4> s;
		s.a = vec<T, 4>(orientation.a * T(scale.x), T(0));
		s.b = vec<T, 4>(orientation.b * T(scale.y), T(0));
		s.c = vec<T, 4>(orientation.c * T(scale.z), T(0));
		s.d = vec<T, 4>(offset, T(1));
		return s;
	}
	
	template <class T>
	SML_API inline mat<T, 2> orientation(T angle)
	{
		mat<T, 2> s;

		T const cosA = cos(angle);
		T const sinA = sin(angle);

		s.e[0][0] = cosA;
		s.e[0][1] = sinA;

		s.e[1][0] = -sinA;
		s.e[1][1] = cosA;

		return s;
	}

	template <class T>
	SML_API inline mat<T, 3> rotate(T angle)
	{
		return mat<T, 3>( orientation(angle), vec<T, 3>(T(0)) );
	}
	
	template <class T>
	SML_API inline mat<T, 3> scale(vec<T, 2> const& scale)
	{
		mat<T, 3> s;
		s.a = vec<T, 3>(T(scale.x), T(0), T(0));
		s.b = vec<T, 3>(T(0), T(scale.y), T(0));
		s.c = vec<T, 3>(T(0), T(0), T(1));
		return s;
	}

	template <class T>
	SML_API inline mat<T, 3> translate(vec<T, 2> const& offset)
	{
		mat<T, 3> s;
		s.a = vec<T, 3>(T(1), T(0), T(0));
		s.b = vec<T, 3>(T(0), T(1), T(0));
		s.c = vec<T, 3>(offset, T(1));
		return s;
	}
	
	template <class T>
	SML_API inline mat<T, 3> transform(vec<T, 2> const& offset, typename id< vec<T, 2> >::t const& scale = T(1), typename id< mat<T, 2> >::t const& orientation = mat<T, 2>(T(1)))
	{
		mat<T, 3> s;
		s.a = vec<T, 3>(orientation.a * T(scale.x), T(0));
		s.b = vec<T, 3>(orientation.b * T(scale.y), T(0));
		s.c = vec<T, 3>(offset, T(1));
		return s;
	}

	template <class T>
	SML_API mat<T, 4> ortho(T left, T right, T bottom, T top, T zNear, T zFar)
	{
		mat<T, 4> s;
		s.a = vec<T, 4>(T(2) / (right - left), T(0), T(0), T(0));
		s.b = vec<T, 4>(T(0), T(2) / (top - bottom), T(0), T(0));
		s.c = vec<T, 4>(T(0), T(0), -T(2) / (zFar - zNear), T(0));
		s.d = vec<T, 4>(-(right + left) / (right - left), -(top + bottom) / (top - bottom), -(zFar + zNear) / (zFar - zNear), T(1));
		return s;
	}

	template <class T>
	SML_API mat<T, 4> perspective(T fovy, T aspect, T zNear, T zFar)
	{
		mat<T, 4> s;
		T tanHalfFovy = tan(fovy / T(2));
		s.a = vec<T, 4>(T(1) / (aspect * tanHalfFovy), T(0), T(0), T(0));
		s.b = vec<T, 4>(T(0), T(1) / tanHalfFovy, T(0), T(0));
		s.c = vec<T, 4>(T(0), T(0), -(zFar + zNear) / (zFar - zNear), -T(1));
		s.d = vec<T, 4>(T(0), T(0), -(T(2) * zFar * zNear) / (zFar - zNear), T(0));
		return s;
	}

} // namespace
