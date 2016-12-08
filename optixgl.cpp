#include "optix"
#include "ogl"

namespace optx
{
	// annoying ...
	namespace
	{
		GLint glTextureTarget(RTgltarget rtTarget)
		{
			switch (rtTarget)
			{
			case RT_TARGET_GL_TEXTURE_1D:
				return GL_TEXTURE_1D;
			case RT_TARGET_GL_TEXTURE_2D:
				return GL_TEXTURE_2D;
			case RT_TARGET_GL_TEXTURE_RECTANGLE:
				return GL_TEXTURE_RECTANGLE_ARB;
			case RT_TARGET_GL_TEXTURE_3D:
				return GL_TEXTURE_3D;
			case RT_TARGET_GL_TEXTURE_1D_ARRAY:
				return GL_TEXTURE_1D_ARRAY;
			case RT_TARGET_GL_TEXTURE_2D_ARRAY:
				return GL_TEXTURE_2D_ARRAY;
			case RT_TARGET_GL_TEXTURE_CUBE_MAP:
				return GL_TEXTURE_CUBE_MAP;
			case RT_TARGET_GL_TEXTURE_CUBE_MAP_ARRAY:
				return GL_TEXTURE_CUBE_MAP_ARRAY;
			default:
				return 0;
			}
		}

		RTformat optixFormatFromGl(GLint glFormat, size_t& elementSize, size_t& count, GLenum& glComponents, GLenum& glType)
		{
			switch (glFormat) {
			case GL_R8_SNORM:
			case GL_R8I:
				return (count = 1, elementSize = 1, glComponents = GL_RED, glType = GL_BYTE, RT_FORMAT_BYTE);
			case GL_R8:
			case GL_R8UI:
				return (count = 1, elementSize = 1, glComponents = GL_RED, glType = GL_UNSIGNED_BYTE, RT_FORMAT_UNSIGNED_BYTE);
			case GL_R16_SNORM:
			case GL_R16I:
				return (count = 1, elementSize = 2, glComponents = GL_RED, glType = GL_SHORT, RT_FORMAT_SHORT);
			case GL_R16:
			case GL_R16UI:
				return (count = 1, elementSize = 2, glComponents = GL_RED, glType = GL_UNSIGNED_SHORT, RT_FORMAT_UNSIGNED_SHORT);
			case GL_R32I:
				return (count = 1, elementSize = 4, glComponents = GL_RED, glType = GL_INT, RT_FORMAT_INT);
			case GL_R32UI:
				return (count = 1, elementSize = 4, glComponents = GL_RED, glType = GL_UNSIGNED_INT, RT_FORMAT_UNSIGNED_INT);

			case GL_RG8_SNORM:
			case GL_RG8I:
				return (count = 2, elementSize = 1, glComponents = GL_RG, glType = GL_BYTE, RT_FORMAT_BYTE2);
			case GL_RG8:
			case GL_RG8UI:
				return (count = 2, elementSize = 1, glComponents = GL_RG, glType = GL_UNSIGNED_BYTE, RT_FORMAT_UNSIGNED_BYTE2);
			case GL_RG16_SNORM:
			case GL_RG16I:
				return (count = 2, elementSize = 2, glComponents = GL_RG, glType = GL_SHORT, RT_FORMAT_SHORT2);
			case GL_RG16:
			case GL_RG16UI:
				return (count = 2, elementSize = 2, glComponents = GL_RG, glType = GL_UNSIGNED_SHORT, RT_FORMAT_UNSIGNED_SHORT2);
			case GL_RG32I:
				return (count = 2, elementSize = 4, glComponents = GL_RG, glType = GL_INT, RT_FORMAT_INT2);
			case GL_RG32UI:
				return (count = 2, elementSize = 4, glComponents = GL_RG, glType = GL_UNSIGNED_INT, RT_FORMAT_UNSIGNED_INT2);

			case GL_RGB8_SNORM:
			case GL_RGB8I:
				return (count = 4, elementSize = 1, glComponents = GL_RGBA, glType = GL_BYTE, RT_FORMAT_BYTE4);
			case GL_RGB8:
			case GL_SRGB8:
			case GL_RGB8UI:
				return (count = 4, elementSize = 1, glComponents = GL_RGBA, glType = GL_UNSIGNED_BYTE, RT_FORMAT_UNSIGNED_BYTE4);
			case GL_RGB16_SNORM:
			case GL_RGB16I:
				return (count = 4, elementSize = 2, glComponents = GL_RGBA, glType = GL_SHORT, RT_FORMAT_SHORT4);
			case GL_RGB16:
			case GL_RGB16UI:
				return (count = 4, elementSize = 2, glComponents = GL_RGBA, glType = GL_UNSIGNED_SHORT, RT_FORMAT_UNSIGNED_SHORT4);
			case GL_RGB32I:
				return (count = 4, elementSize = 4, glComponents = GL_RGBA, glType = GL_INT, RT_FORMAT_INT4);
			case GL_RGB32UI:
				return (count = 4, elementSize = 4, glComponents = GL_RGBA, glType = GL_UNSIGNED_INT, RT_FORMAT_UNSIGNED_INT4);

			case GL_RGBA8_SNORM:
			case GL_RGBA8I:
				return (count = 4, elementSize = 1, glComponents = GL_RGBA, glType = GL_BYTE, RT_FORMAT_BYTE4);
			case GL_RGBA8:
			case GL_SRGB8_ALPHA8:
			case GL_RGBA8UI:
				return (count = 4, elementSize = 1, glComponents = GL_RGBA, glType = GL_UNSIGNED_BYTE, RT_FORMAT_UNSIGNED_BYTE4);
			case GL_RGBA16_SNORM:
			case GL_RGBA16I:
				return (count = 4, elementSize = 2, glComponents = GL_RGBA, glType = GL_SHORT, RT_FORMAT_SHORT4);
			case GL_RGBA16:
			case GL_RGBA16UI:
				return (count = 4, elementSize = 2, glComponents = GL_RGBA, glType = GL_UNSIGNED_SHORT, RT_FORMAT_UNSIGNED_SHORT4);
			case GL_RGBA32I:
				return (count = 4, elementSize = 4, glComponents = GL_RGBA, glType = GL_INT, RT_FORMAT_INT4);
			case GL_RGBA32UI:
				return (count = 4, elementSize = 4, glComponents = GL_RGBA, glType = GL_UNSIGNED_INT, RT_FORMAT_UNSIGNED_INT4);

			case GL_R16F:
				return (count = 1, elementSize = 2, glComponents = GL_RED, glType = GL_FLOAT16_NV, RT_FORMAT_HALF);
			case GL_RG16F:
				return (count = 2, elementSize = 2, glComponents = GL_RG, glType = GL_FLOAT16_NV, RT_FORMAT_HALF2);
			case GL_RGBA16F:
				return (count = 4, elementSize = 2, glComponents = GL_RGBA, glType = GL_FLOAT16_NV, RT_FORMAT_HALF4);

			case GL_R32F:
				return (count = 1, elementSize = 4, glComponents = GL_RED, glType = GL_FLOAT, RT_FORMAT_FLOAT);
			case GL_RG32F:
				return (count = 2, elementSize = 4, glComponents = GL_RG, glType = GL_FLOAT, RT_FORMAT_FLOAT2);
			case GL_RGBA32F:
				return (count = 4, elementSize = 4, glComponents = GL_RGBA, glType = GL_FLOAT, RT_FORMAT_FLOAT4);
			}
			return RT_FORMAT_UNKNOWN;
		}

		void transferGLParameters(RTtexturesampler ref, GLenum glTarget, unsigned glid)
		{
			// Fix up read format
			{
				int internalFormat = GL_RGBA8;
				THROW_OPENGL_VERROR(glGetTextureLevelParameterivEXT(glid, glTarget, 0, GL_TEXTURE_INTERNAL_FORMAT, &internalFormat), "glGetTextureParameterivEXT(FORMAT)");
				auto readMode = RT_TEXTURE_READ_ELEMENT_TYPE;
				if (internalFormat == GL_SRGB || internalFormat == GL_SRGB8 || internalFormat == GL_SRGB8_ALPHA8_EXT)
					readMode = RT_TEXTURE_READ_NORMALIZED_FLOAT_SRGB;
				else if (internalFormat == GL_R || internalFormat == GL_RG || internalFormat == GL_RGB || internalFormat == GL_RGBA
					|| internalFormat == GL_R8 || internalFormat == GL_RG8 || internalFormat == GL_RGB8 || internalFormat == GL_RGBA8
					|| internalFormat == GL_R16 || internalFormat == GL_RG16 || internalFormat == GL_RGB16 || internalFormat == GL_RGBA16
					|| internalFormat == GL_R8_SNORM || internalFormat == GL_RG8_SNORM || internalFormat == GL_RGB8_SNORM || internalFormat == GL_RGBA8_SNORM
					|| internalFormat == GL_R16_SNORM || internalFormat == GL_RG16_SNORM || internalFormat == GL_RGB16_SNORM || internalFormat == GL_RGBA16_SNORM
					|| internalFormat == GL_RGB10 || internalFormat == GL_RGB10_A2)
					readMode = RT_TEXTURE_READ_NORMALIZED_FLOAT;
				THROW_OPTIX_ERROR(rtTextureSamplerSetReadMode(ref, readMode), "rtTextureSamplerSetReadMode");
			}

			// Fix up wrapping
			GLenum wrapParams[] = { GL_TEXTURE_WRAP_S, GL_TEXTURE_WRAP_T, GL_TEXTURE_WRAP_R };
			int numWrapDim = (glTarget == GL_TEXTURE_3D) ? 3 : 2;
			for (int i = 0; i < numWrapDim; ++i) {
				int glWrapMode = GL_REPEAT;
				THROW_OPENGL_VERROR(glGetTextureParameterivEXT(glid, glTarget, wrapParams[i], &glWrapMode), "glGetTextureParameterivEXT(WRAP)");
				auto wrapMode = RT_WRAP_REPEAT;
				if (glWrapMode == GL_CLAMP_TO_EDGE || glWrapMode == GL_CLAMP) wrapMode = RT_WRAP_CLAMP_TO_EDGE;
				else if (glWrapMode == GL_CLAMP_TO_BORDER) wrapMode = RT_WRAP_CLAMP_TO_BORDER;
				else if (glWrapMode == GL_MIRRORED_REPEAT) wrapMode = RT_WRAP_MIRROR;
				THROW_OPTIX_ERROR(rtTextureSamplerSetWrapMode(ref, unsigned(i), wrapMode), "rtTextureSamplerSetWrapMode");
			}

			// Fix up filtering
			{
				auto glMinFilterMode = GL_LINEAR_MIPMAP_LINEAR, glMagFilterMode = GL_LINEAR;
				THROW_OPENGL_VERROR(glGetTextureParameterivEXT(glid, glTarget, GL_TEXTURE_MIN_FILTER, &glMinFilterMode), "glGetTextureParameterivEXT(MIN)");
				THROW_OPENGL_VERROR(glGetTextureParameterivEXT(glid, glTarget, GL_TEXTURE_MAG_FILTER, &glMagFilterMode), "glGetTextureParameterivEXT(MAG)");
				auto magFilter = (glMagFilterMode == GL_NEAREST) ? RT_FILTER_NEAREST : RT_FILTER_LINEAR;
				auto minFilter = (glMinFilterMode == GL_NEAREST_MIPMAP_NEAREST || glMinFilterMode == GL_NEAREST_MIPMAP_LINEAR) ? RT_FILTER_NEAREST : RT_FILTER_LINEAR;
				auto mipFilter = (glMinFilterMode == GL_NEAREST_MIPMAP_NEAREST || glMinFilterMode == GL_LINEAR_MIPMAP_NEAREST) ? RT_FILTER_NEAREST : RT_FILTER_LINEAR;
				THROW_OPTIX_ERROR(rtTextureSamplerSetFilteringModes(ref, minFilter, magFilter, mipFilter), "rtTextureSamplerSetFilteringModes");
			}
		}

	} // namespace

	InteropSampler InteropSamplerRef::from(RTcontext context, unsigned glid, RTgltarget target)
	{
		owned r = nullptr;
		THROW_OPTIX_ERROR_PRINT(rtTextureSamplerCreateFromGLImage(context, glid, target, &r.ref), "rtTextureSamplerCreateFromGLImage");

		// Fix up
		THROW_OPTIX_ERROR_PRINT(rtTextureSamplerSetIndexingMode(r.ref, RT_TEXTURE_INDEX_NORMALIZED_COORDINATES), "rtTextureSamplerSetIndexingMode");
		if (auto glTarget = glTextureTarget(target)) {
			transferGLParameters(r.ref, glTarget, glid);
		}

		return r;
	}

	Sampler InteropSamplerRef::clone(RTcontext context, unsigned glid, RTgltarget optixTarget, Buffer& buffer, unsigned bufferType)
	{
		auto r = Buffer::create(context, bufferType);
		
		auto glTarget = glTextureTarget(optixTarget);
		assert(glTarget);

		unsigned numLevels = 0;
		while (true) {
			GLint width = 0;
			glGetTextureLevelParameterivEXT(glid, glTarget, numLevels, GL_TEXTURE_WIDTH, &width);
			if (width > 0)
				++numLevels;
			else
				break;
		}

		assert(numLevels > 0);
		numLevels = 1;

		GLint width = 0, height = 0, depth = 0;
		THROW_OPENGL_VERROR(glGetTextureLevelParameterivEXT(glid, glTarget, 0, GL_TEXTURE_WIDTH, &width), "glGetTextureLevelParameterivEXT(WIDTH)");
		THROW_OPENGL_VERROR(glGetTextureLevelParameterivEXT(glid, glTarget, 0, GL_TEXTURE_HEIGHT, &height), "glGetTextureLevelParameterivEXT(HEIGHT)");
		THROW_OPENGL_VERROR(glGetTextureLevelParameterivEXT(glid, glTarget, 0, GL_TEXTURE_DEPTH, &depth), "glGetTextureLevelParameterivEXT(DEPTH)");
		bool is3D = (depth > 1); // todo: might need to be refined with layered textures, cube maps
		if (is3D) {
			THROW_OPTIX_ERROR_PRINT(rtBufferSetSize3D(r, width, height, depth), "rtBufferSetSize3D");
		} else {
			THROW_OPTIX_ERROR_PRINT(rtBufferSetSize2D(r, width, height), "rtBufferSetSize2D");
		}
		THROW_OPTIX_ERROR_PRINT(rtBufferSetMipLevelCount(r, numLevels), "rtBufferSetMipLevelCount");

		GLint internalFormat = 0;
		THROW_OPENGL_VERROR(glGetTextureLevelParameterivEXT(glid, glTarget, 0, GL_TEXTURE_INTERNAL_FORMAT, &internalFormat), "glGetTextureParameterivEXT(FORMAT)");
		size_t elementSize = 0, dimension = 0;
		GLenum glComponents, glType;
		auto optixFormat = optixFormatFromGl(internalFormat, elementSize, dimension, glComponents, glType);
		if (optixFormat == RT_FORMAT_UNKNOWN)
			THROW_OPTIX_ERROR_PRINT(RT_ERROR_INVALID_IMAGE, "RT_ERROR_INVALID_IMAGE");
		THROW_OPTIX_ERROR_PRINT(rtBufferSetFormat(r, optixFormat), "rtBufferSetFormat");

		for (unsigned level = 0; level < numLevels; ++level) {
			GLint width = 0, height = 0, depth = 0;
			THROW_OPENGL_VERROR(glGetTextureLevelParameterivEXT(glid, glTarget, level, GL_TEXTURE_WIDTH, &width), "glGetTextureLevelParameterivEXT(WIDTH)");
			THROW_OPENGL_VERROR(glGetTextureLevelParameterivEXT(glid, glTarget, level, GL_TEXTURE_HEIGHT, &height), "glGetTextureLevelParameterivEXT(HEIGHT)");
			THROW_OPENGL_VERROR(glGetTextureLevelParameterivEXT(glid, glTarget, level, GL_TEXTURE_DEPTH, &depth), "glGetTextureLevelParameterivEXT(DEPTH)");

			void* dest = nullptr;
			THROW_OPTIX_ERROR_PRINT(rtBufferMapEx(r, RT_BUFFER_MAP_WRITE, level, nullptr, &dest), "rtBufferMapEx");
			glGetTextureImageEXT(glid, glTarget, level, glComponents, glType, dest);
			rtBufferUnmapEx(r, level);

			THROW_OPENGL_LASTERROR("glTextureSubImage#DEXT");
		}

		auto r2 = Sampler::create(context);
		THROW_OPTIX_ERROR_PRINT(rtTextureSamplerSetBuffer(r2, 0, 0, r), "rtTextureSamplerSetBuffer");
		transferGLParameters(r2, glTarget, glid);

		buffer = std::move(r);
		return r2;
	}

} // namespace
