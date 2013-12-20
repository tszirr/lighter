#include "img"
#define FREEIMAGE_LIB
#include "FreeImage.h"

namespace img
{

namespace
{

struct FreeImage
{
	FreeImage() { FreeImage_Initialise(); }
	~FreeImage() { FreeImage_DeInitialise(); }
};

FreeImage& prepareFreeImage()
{
	static FreeImage fi;
	return fi;
}

template <class Handle>
struct FreeImageDeleter
{
	typedef Handle pointer;
	void operator ()(FIBITMAP* bmp) { if (bmp) FreeImage_Unload(bmp); }
};
typedef stdx::unique_handle< FIBITMAP*, FreeImageDeleter<FIBITMAP*> > bmp_handle;

struct copy_channel_identity
{
	template <class T>
	T operator ()(T val) const { return val; }

	template <class T> struct is_identity { static bool const value = false; };
	template <> struct is_identity<copy_channel_identity> { static bool const value = true; };
};

template <class D, class S, class Transform>
inline void copy_channels(D* destBytes, unsigned destChannels, S const* srcBytes, unsigned srcChannels, unsigned srcPitch, glm::uvec2 dim, D zero, D one, Transform&& transform)
{
	auto destByte = destBytes;
	auto srcLine = srcBytes;
	auto copyChannels = std::min(destChannels, srcChannels);

	for (unsigned y = 0; y < dim.y; ++y)
	{
		auto srcByte = srcLine;

		for (unsigned x = 0; x < dim.x; ++x)
		{
			unsigned c = 0;
			for (; c < copyChannels; ++c)
			{
				if (copy_channel_identity::is_identity<Transform>::value)
					// debug fast path
					*destByte++ = *srcByte++;
				else
					*destByte++ = transform(*srcByte++);
			}
			for (; c < destChannels; ++c)
				*destByte++ = ((c & 3) == 3) ? one : zero;
		}

		srcByte += srcPitch;
	}
}

} // namespace

ImageDesc image_desc(FIBITMAP* bmp)
{
	ImageDesc desc;
	desc.dim = glm::uvec3(FreeImage_GetWidth(bmp), FreeImage_GetHeight(bmp), 1);
	switch (FreeImage_GetColorType(bmp))
	{
	case FIC_MINISBLACK:
	case FIC_MINISWHITE:
		desc.channels = 1;
		break;
	default:
		desc.channels = 3;
		break;
	}
	if (FreeImage_IsTransparent(bmp))
		++desc.channels;
	return desc;
}

ImageDesc load_image_desc(const char* imageData, size_t imageDataLen)
{
	prepareFreeImage();

	// Load image header
	FIMEMORY fifStream = { (void*) imageData };
	bmp_handle bmp( FreeImage_LoadFromMemory(FIF_UNKNOWN, &fifStream, FIF_LOAD_NOPIXELS) );

	return image_desc(bmp);
}

void load_image(void* image, ImageType::T elementType, unsigned channels, void* (*allocate)(ImageDesc const& desc, void* image), const char* imageData, size_t imageDataLen)
{
	prepareFreeImage();

	// Load image
	FIMEMORY fifStream = { (void*) imageData };
	bmp_handle bmp( FreeImage_LoadFromMemory(FIF_UNKNOWN, &fifStream, 0) );
	if (!bmp.valid()) throwx(img_error("Unsupported image type or format"));

	// Read header
	auto desc = image_desc(bmp);

	// Allocate dest image
	desc.channels = channels;
	unsigned numPixels = desc.dim.x * desc.dim.y * desc.dim.z;
	char* destBytes = (char*) allocate(desc, image);

	auto colType = FreeImage_GetColorType(bmp);
	auto imgType = FreeImage_GetImageType(bmp);

	// Convert
	switch(elementType)
	{
	case ImageType::int8bpp:
		{
			// convert to rgb or rgba bitmap
			if (imgType != FIT_BITMAP || colType != FIC_RGB && colType != FIC_RGBALPHA)
			{
				if (imgType == FIT_UINT16 || imgType == FIT_INT16)
					bmp.reset( FreeImage_ConvertToStandardType(bmp) );
				else
					bmp.reset( FreeImage_ConvertTo32Bits(bmp) );
			}

			// check if potential conversions successful
			if (!bmp.valid()) throwx(img_error("Unsupported image format conversion"));
			imgType = FreeImage_GetImageType(bmp);
			colType = FreeImage_GetColorType(bmp);
			
			// may assume FIC_RGB or FIC_RGBA here
			assert(imgType == FIT_BITMAP && (colType == FIC_RGB || colType == FIC_RGBALPHA));

			auto bytesPerPixel = FreeImage_GetBPP(bmp) / 8;
			auto srcChannels = bytesPerPixel / 1;
			auto pitch = FreeImage_GetPitch(bmp);
			auto bits = FreeImage_GetBits(bmp);

			if (srcChannels == desc.channels && pitch == desc.dim.x * bytesPerPixel)
				memcpy(destBytes, bits, numPixels * bytesPerPixel);
			else
				copy_channels(
					reinterpret_cast<unsigned char*>(destBytes), desc.channels,
					reinterpret_cast<unsigned char const*>(bits), srcChannels, pitch,
					glm::uvec2(desc.dim), (unsigned char)(0x00), (unsigned char)(0xff), copy_channel_identity());
		}
		break;
	case ImageType::int16bpp:
		{
			// convert to int16
			if (imgType != FIT_UINT16 && imgType != FIT_INT16 && imgType != FIT_RGB16 && imgType != FIT_RGBA16)
				bmp.reset( FreeImage_ConvertToRGB16(bmp) );

			// check if potential conversions successful
			if (!bmp.valid()) throwx(img_error("Unsupported image format conversion"));
			imgType = FreeImage_GetImageType(bmp);
			colType = FreeImage_GetColorType(bmp);
			
			assert(imgType == FIT_UINT16 || imgType == FIT_INT16 || imgType == FIT_RGB16 || imgType == FIT_RGBA16);
			
			auto bytesPerPixel = FreeImage_GetBPP(bmp) / 8;
			auto srcChannels = bytesPerPixel / 2;
			auto pitch = FreeImage_GetPitch(bmp);
			auto bits = FreeImage_GetBits(bmp);

			if (srcChannels == desc.channels && pitch == desc.dim.x * bytesPerPixel)
				memcpy(destBytes, bits, numPixels * bytesPerPixel);
			else
				copy_channels(
					reinterpret_cast<unsigned short*>(destBytes), desc.channels,
					reinterpret_cast<unsigned short const*>(bits), srcChannels, pitch,
					glm::uvec2(desc.dim), (unsigned short)(0x0000), (unsigned short)(0xffff), copy_channel_identity());
		}
		break;
	case ImageType::float32bpp:
		if (imgType == FIT_FLOAT || imgType == FIT_RGBF || imgType == FIT_RGBAF)
		{
			auto bytesPerPixel = FreeImage_GetBPP(bmp) / 8;
			auto srcChannels = bytesPerPixel / 4;
			auto pitch = FreeImage_GetPitch(bmp);
			auto bits = FreeImage_GetBits(bmp);

			if (srcChannels == desc.channels && pitch == desc.dim.x * bytesPerPixel)
				memcpy(destBytes, bits, numPixels * bytesPerPixel);
			else
				copy_channels(
					reinterpret_cast<float*>(destBytes), desc.channels,
					reinterpret_cast<float const*>(bits), srcChannels, pitch,
					glm::uvec2(desc.dim), 0.0f, 1.0f, copy_channel_identity());
		}
		else if (imgType == FIT_BITMAP)
		{
			if (colType != FIC_RGB && colType != FIC_RGBALPHA)
				bmp.reset( FreeImage_ConvertTo32Bits(bmp) );

			// check if potential conversions successful
			if (!bmp.valid()) throwx(img_error("Unsupported image format conversion"));
			imgType = FreeImage_GetImageType(bmp);
			colType = FreeImage_GetColorType(bmp);

			// may assume FIC_RGB or FIC_RGBA here
			assert(imgType == FIT_BITMAP && (colType == FIC_RGB || colType == FIC_RGBALPHA));

			auto bytesPerPixel = FreeImage_GetBPP(bmp) / 8;
			auto srcChannels = bytesPerPixel / 1;
			auto pitch = FreeImage_GetPitch(bmp);
			auto bits = FreeImage_GetBits(bmp);

			copy_channels(
				reinterpret_cast<float*>(destBytes), desc.channels,
				reinterpret_cast<unsigned char const*>(bits), srcChannels, pitch,
				glm::uvec2(desc.dim), 0.0f, 1.0f, [](unsigned char val){ return float(val) / float(0xff); });
		}
		else if (imgType == FIT_UINT16 || imgType == FIT_INT16 || imgType == FIT_RGB16 || imgType == FIT_RGBA16)
		{
			auto bytesPerPixel = FreeImage_GetBPP(bmp) / 8;
			auto srcChannels = bytesPerPixel / 2;
			auto pitch = FreeImage_GetPitch(bmp);
			auto bits = FreeImage_GetBits(bmp);

			copy_channels(
				reinterpret_cast<float*>(destBytes), desc.channels,
				reinterpret_cast<unsigned short const*>(bits), srcChannels, pitch,
				glm::uvec2(desc.dim), 0.0f, 1.0f, [](unsigned short val){ return float(val) / float(0xffff); });
		}
		break;

	default:
		throwx(img_error("Unsupported image format"));
	}
}

} // namespace