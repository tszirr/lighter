#pragma once

#undef rtDeclareVariable
// declare actual variable first to allow for extern "C" declaration (prevents stupid constructor code gen erros)
#define rtDeclareVariable(type, name, semantic, annotation)    \
  __device__ type name; \
  namespace rti_internal_typeinfo { \
    __device__ ::rti_internal_typeinfo::rti_typeinfo name = { ::rti_internal_typeinfo::_OPTIX_VARIABLE, sizeof(type)}; \
  } \
  namespace rti_internal_typename { \
    __device__ char name[] = #type; \
  } \
  namespace rti_internal_semantic { \
    __device__ char name[] = #semantic; \
  } \
  namespace rti_internal_annotation { \
    __device__ char name[] = #annotation; \
  }
