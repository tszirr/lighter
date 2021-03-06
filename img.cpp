#include "img"
#include "FreeImage.h"
#include <algorithm>

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

struct FreeImageDeleter
{
	void operator ()(FIMEMORY* mem) { if (mem) FreeImage_CloseMemory(mem); }
	void operator ()(FIBITMAP* bmp) { if (bmp) FreeImage_Unload(bmp); }
};
typedef stdx::unique_handle< FIMEMORY, FreeImageDeleter> mem_handle;
typedef stdx::unique_handle< FIBITMAP, FreeImageDeleter> bmp_handle;

struct copy_channel_identity
{
	template <class T>
	T operator ()(T val) const { return val; }

	template <class T> struct is_identity;
};
template <class T> struct copy_channel_identity::is_identity { static bool const value = false; };
template <> struct copy_channel_identity::is_identity<copy_channel_identity> { static bool const value = true; };

template <class D, class S, class Transform>
inline void copy_channels(D* destBytes, unsigned destChannels, S const* srcBytes, unsigned srcChannels, unsigned srcPitch, glm::uvec2 dim, D zero, D one, Transform&& transform, unsigned destPitch = 0)
{
	auto destLine = destBytes;
	auto srcLine = srcBytes;
	auto copyChannels = stdx::min_value(destChannels, srcChannels);
	auto skipChannels = srcChannels - copyChannels;

	for (unsigned y = 0; y < dim.y; ++y)
	{
		auto srcByte = srcLine;
		auto destByte = destLine;

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
			srcByte += skipChannels;
		}

		srcLine = (S*) ((char*) srcLine + srcPitch);

		if (destPitch)
			destLine = (D*) ((char*) srcLine + destPitch);
		else
			destLine = destByte;
	}
}

bmp_handle load_bmp(stdx::data_range_param<const char> imageData, int flags = 0)
{
	// Load image header
	mem_handle fifStream( FreeImage_OpenMemory((BYTE*)(imageData.data()), static_cast<DWORD>(imageData.size())) );

	auto fifType = FreeImage_GetFileTypeFromMemory(fifStream);
	if (fifType == FIF_UNKNOWN) throwx(img_error("Unknown or unidentifiable file format (FreeImage)"));

	bmp_handle bmp( FreeImage_LoadFromMemory(fifType, fifStream, flags) );
	if (!bmp.valid()) throwx(img_error("Unsupported image type or format (FreeImage)"));

	return bmp;
}

FREE_IMAGE_TYPE free_image_type(ImageType::T destElementType, unsigned destChannels)
{
	switch (destElementType)
	{
	case ImageType::int8bpp: return FIT_BITMAP;
	case ImageType::int16bpp:
		if (destChannels == 1)  return FIT_UINT16;
		else if (destChannels == 3)  return FIT_RGB16;
		else if (destChannels == 4)  return FIT_RGBA16;
	case ImageType::float32bpp:
		if (destChannels == 1)  return FIT_FLOAT;
		else if (destChannels == 3)  return FIT_RGBF;
		else if (destChannels == 4)  return FIT_RGBAF;
	}
	return FIT_UNKNOWN;
}

unsigned bits_per_pixel(ImageType::T destElementType, unsigned destChannels)
{
	switch (destElementType)
	{
	case ImageType::int8bpp: return destChannels * 8;
	case ImageType::int16bpp: return destChannels * 16;
	case ImageType::float32bpp: return destChannels * 32;
	}
	return 0;
}

bmp_handle make_bmp(stdx::data_range_param<const char> imageData, unsigned width, unsigned height, ImageType::T destElementType, unsigned destChannels, int flags = 0)
{
	bmp_handle bmp( FreeImage_AllocateT(free_image_type(destElementType, destChannels), width, height, bits_per_pixel(destElementType, destChannels)) );
	if (!bmp.valid()) throwx(img_error("Unsupported image type or format (FreeImage)"));

	// copy to FI bmp
	{
		auto bytes = FreeImage_GetBits(bmp);
		auto pitch = FreeImage_GetPitch(bmp);
		auto srcBytesPerLine = imageData.size() / height;

		if (pitch * height == imageData.size())
			memcpy(bytes, imageData.data(), imageData.size());
		else if (srcBytesPerLine < pitch && srcBytesPerLine * height == imageData.size()) {
			for (auto srcLine = imageData.first; srcLine != imageData.last; srcLine += srcBytesPerLine, bytes += pitch)
				memcpy(bytes, srcLine, srcBytesPerLine);
		}
		else
			throwx(img_error("Invalid pitch"));
	}

	return bmp;
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

ImageDesc load_image_desc(stdx::data_range_param<const char> imageData)
{
	prepareFreeImage();

	// Load image header
	auto bmp = load_bmp(imageData, FIF_LOAD_NOPIXELS);

	return image_desc(bmp);
}

void load_image(void* destImage, ImageType::T destElementType, unsigned destChannels, void* (*allocate)(ImageDesc const& desc, void* image),
				stdx::data_range_param<const char> imageData)
{
	prepareFreeImage();

	// Load image
	auto bmp = load_bmp(imageData);

	// Read header
	auto destDesc = image_desc(bmp);
	unsigned numPixels = destDesc.dim.x * destDesc.dim.y * destDesc.dim.z;

	// Fix up desc
	destDesc.channels = destChannels;

	// Allocate dest image
	char* destBytes = (char*) allocate(destDesc, destImage);

	auto colType = FreeImage_GetColorType(bmp);
	auto imgType = FreeImage_GetImageType(bmp);

	// Convert
	switch(destElementType)
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
			auto channels = bytesPerPixel / 1;
			auto pitch = FreeImage_GetPitch(bmp);
			auto bits = FreeImage_GetBits(bmp);

			if (channels == destDesc.channels && pitch == destDesc.dim.x * bytesPerPixel)
				memcpy(destBytes, bits, numPixels * bytesPerPixel);
			else
				copy_channels(
					reinterpret_cast<unsigned char*>(destBytes), destDesc.channels,
					reinterpret_cast<unsigned char const*>(bits), channels, pitch,
					glm::uvec2(destDesc.dim), (unsigned char)(0x00), (unsigned char)(0xff), copy_channel_identity());
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
			auto channels = bytesPerPixel / 2;
			auto pitch = FreeImage_GetPitch(bmp);
			auto bits = FreeImage_GetBits(bmp);

			if (channels == destDesc.channels && pitch == destDesc.dim.x * bytesPerPixel)
				memcpy(destBytes, bits, numPixels * bytesPerPixel);
			else
				copy_channels(
					reinterpret_cast<unsigned short*>(destBytes), destDesc.channels,
					reinterpret_cast<unsigned short const*>(bits), channels, pitch,
					glm::uvec2(destDesc.dim), (unsigned short)(0x0000), (unsigned short)(0xffff), copy_channel_identity());
		}
		break;
	case ImageType::float32bpp:
		if (imgType == FIT_FLOAT || imgType == FIT_RGBF || imgType == FIT_RGBAF)
		{
			auto bytesPerPixel = FreeImage_GetBPP(bmp) / 8;
			auto channels = bytesPerPixel / 4;
			auto pitch = FreeImage_GetPitch(bmp);
			auto bits = FreeImage_GetBits(bmp);

			if (channels == destDesc.channels && pitch == destDesc.dim.x * bytesPerPixel)
				memcpy(destBytes, bits, numPixels * bytesPerPixel);
			else
				copy_channels(
					reinterpret_cast<float*>(destBytes), destDesc.channels,
					reinterpret_cast<float const*>(bits), channels, pitch,
					glm::uvec2(destDesc.dim), 0.0f, 1.0f, copy_channel_identity());
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
			auto channels = bytesPerPixel / 1;
			auto pitch = FreeImage_GetPitch(bmp);
			auto bits = FreeImage_GetBits(bmp);

			copy_channels(
				reinterpret_cast<float*>(destBytes), destDesc.channels,
				reinterpret_cast<unsigned char const*>(bits), channels, pitch,
				glm::uvec2(destDesc.dim), 0.0f, 1.0f, [](unsigned char val){ return float(val) / float(0xff); });
		}
		else if (imgType == FIT_UINT16 || imgType == FIT_INT16 || imgType == FIT_RGB16 || imgType == FIT_RGBA16)
		{
			auto bytesPerPixel = FreeImage_GetBPP(bmp) / 8;
			auto channels = bytesPerPixel / 2;
			auto pitch = FreeImage_GetPitch(bmp);
			auto bits = FreeImage_GetBits(bmp);

			copy_channels(
				reinterpret_cast<float*>(destBytes), destDesc.channels,
				reinterpret_cast<unsigned short const*>(bits), channels, pitch,
				glm::uvec2(destDesc.dim), 0.0f, 1.0f, [](unsigned short val){ return float(val) / float(0xffff); });
		}
		break;

	default:
		throwx(img_error("Unsupported image format"));
	}
}

void save_image(void* image, char const* filename, void* (*allocate)(size_t size, void* image),
	stdx::data_range_param<const char> imageData, ImageType::T elementType, ImageDesc const& desc)
{
	prepareFreeImage();

	auto fiformat = FreeImage_GetFIFFromFilename(filename);
	if (fiformat == FIF_UNKNOWN)
		throwx(img_error("Unsupported image format"));
	
	// Convert to free image
	auto bmp = make_bmp(imageData, desc.dim.x, desc.dim.y, elementType, desc.channels);

	bool result = false;

	if (!image || !allocate)
		result = FreeImage_Save(fiformat, bmp, filename) != 0;
	else {
		FreeImageIO fio;
		struct IOData {
			void* image;
			void* (*allocate)(size_t size, void* image);
			void* memory;
			size_t offset;
			size_t waterMark;
		} ioData = { image, allocate, nullptr, 0, 0 };
		fio.write_proc = [](void *buffer, unsigned size, unsigned count, fi_handle handle) -> unsigned {
			IOData& ioData = *(IOData*) handle;
			auto byteCount = size * count;
			auto endOffset = ioData.offset + byteCount;
			if (endOffset > ioData.waterMark) {
				ioData.memory = ioData.allocate(endOffset, ioData.image);
				ioData.waterMark = endOffset;
			}
			memcpy(ioData.memory, buffer,  byteCount);
			ioData.offset = endOffset;
			return byteCount;
		};
		fio.seek_proc = [](fi_handle handle, long offset, int start) -> int {
			IOData& ioData = *(IOData*) handle;
			if (start == SEEK_END)
				ioData.offset = ioData.waterMark + offset;
			else if (start == SEEK_SET)
				ioData.offset = offset;
			else
				ioData.offset += offset;
			return 0;
		};
		fio.tell_proc = [](fi_handle handle) -> long {
			IOData& ioData = *(IOData*) handle;
			return (long) ioData.offset;
		};
		result = FreeImage_SaveToHandle(fiformat, bmp, &fio, (fi_handle) &ioData) != 0;
	}

	if (!result)
		throwx(img_error("Error saving image"));
}


namespace
{
	#pragma pack(push, 1)
	struct targa_header
	{
		char id_length;
		char color_map;
		char image_type;

		struct palette
		{
			unsigned short first_entry_index;
			unsigned short length;
			unsigned char entry_size;
		} palette;

		struct image
		{
			unsigned short x_origin;
			unsigned short y_origin;
			unsigned short width;
			unsigned short height;
			unsigned char bpp;
			unsigned char descriptor;
		} image;
	};
	#pragma pack(pop)

} // namespace

size_t get_targa_size_8bpc(size_t width, size_t height, size_t channels)
{
	return sizeof(targa_header) + width * height * channels;
}

void* write_targa_header_8bpc(void* writeTo, size_t width, size_t height, size_t channels)
{
	targa_header header = { 0 };
	header.image_type = 2; // uncompressed pixel array
	header.image.width = static_cast<unsigned short>(width);
	header.image.height = static_cast<unsigned short>(height);
	header.image.bpp = static_cast<unsigned char>(8 * channels);
	header.image.descriptor = 0; // no attribute bits, origin bottom left
	memcpy(writeTo, &header, sizeof(header));
	return static_cast<char*>(writeTo) + sizeof(header);
}

} // namepsace