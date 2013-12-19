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

template <class Float, class Int>
inline void float_to_int(Float const* fl, Float const* fle, Int* i, unsigned maxInt)
{
	Float scale = Float(maxInt) + Float(1.0f);

	for (; fl < fle; ++fl)
		*i++ = (Int) std::min( (unsigned) (*fl * scale), maxInt );
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
	unsigned srcChannels = desc.channels;

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
			auto pitch = FreeImage_GetPitch(bmp);

			if (bytesPerPixel / 1 == desc.channels && pitch == desc.dim.x * bytesPerPixel)
				memcpy(destBytes, FreeImage_GetBits(bmp), numPixels * bytesPerPixel);
			else
			{
				// todo: required GL data format?
			}

			// todo: copy to channels
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
			auto pitch = FreeImage_GetPitch(bmp);
			
			if (bytesPerPixel / 2 == desc.channels && pitch == desc.dim.x * bytesPerPixel)
				memcpy(destBytes, FreeImage_GetBits(bmp), numPixels * bytesPerPixel);
			else
			{
				// todo: copy to channels
			}
		}
		break;
	case ImageType::float32bpp:
		if (imgType == FIT_FLOAT || imgType == FIT_RGBF || imgType == FIT_RGBAF)
		{
			auto bytesPerPixel = FreeImage_GetBPP(bmp) / 8;
			auto pitch = FreeImage_GetPitch(bmp);

			if (bytesPerPixel / 4 == desc.channels && pitch == desc.dim.x * bytesPerPixel)
				memcpy(destBytes, FreeImage_GetBits(bmp), numPixels * bytesPerPixel);
			else
			{
				// todo: copy to channels
			}
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

			// todo: 8 bit to float & copy to channels
		}
		else if (imgType == FIT_UINT16 || imgType == FIT_INT16 || imgType == FIT_RGB16 || imgType == FIT_RGBA16)
		{
			// todo: 16 bit to float & copy to channels
		}
		break;

	default:
		throwx(img_error("Unsupported image format"));
	}
}

} // namespace