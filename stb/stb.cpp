#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_IMAGE_RESIZE_IMPLEMENTATION

#include "stb.h"

namespace sc
{
	namespace stb
	{
#pragma region Image Read
		int stbi_sc_io_read(void* user, char* data, int size)
		{
			Stream* stream = (sc::Stream*)user;

			return static_cast<int>(stream->read(data, size));
		};

		void stbi_sc_io_skip(void* user, int n)
		{
			Stream* stream = (sc::Stream*)user;

			stream->seek(n, sc::Seek::Add);
		};

		int stbi_sc_io_eof(void* user)
		{
			Stream* stream = (sc::Stream*)user;

			return stream->position() == stream->length() ? -1 : 0;
		};

		void load_image(Stream& stream, RawImage** image)
		{
			int width, height, channels;

			// Image Info
			{
				size_t position = stream.position();
				bool result = stbi_info_from_callbacks(&stbi_sc_io_callback, (void*)&stream, &width, &height, &channels);

				if (!result)
				{
					throw StbLoadingException(stbi_failure_reason());
				}

				stream.seek(position);
			}

			Image::BasePixelType type = Image::BasePixelType::RGBA;
			Image::PixelDepth depth = Image::PixelDepth::RGB8;

			switch (channels)
			{
			case 1:
				type = Image::BasePixelType::L;
				depth = Image::PixelDepth::LUMINANCE8;
				break;

			case 2:
				type = Image::BasePixelType::LA;
				depth = Image::PixelDepth::LUMINANCE8_ALPHA8;
				break;

			case 3:
				type = Image::BasePixelType::RGB;
				depth = Image::PixelDepth::RGB8;
				break;

			case 4:
				type = Image::BasePixelType::RGBA;
				depth = Image::PixelDepth::RGBA8;
				break;

			default:
				break;
			}

			RawImage* image_buffer = new RawImage(
				static_cast<uint16_t>(width), static_cast<uint16_t>(height),
				type, depth
			);

			// Image Loading
			{
				stbi__context ctx;
				{
					ctx.io = stbi_sc_io_callback;
					ctx.io_user_data = (void*)&stream;
					ctx.buflen = image_buffer->data_length();
					ctx.read_from_callbacks = 1;
					ctx.callback_already_read = 0;
					ctx.img_buffer = ctx.img_buffer_original = image_buffer->data();
					ctx.img_buffer_original_end = image_buffer->data() + ctx.buflen;
				}

				uint8_t* data = stbi__load_and_postprocess_8bit(&ctx, &width, &height, &channels, 0);

				if (data == NULL)
				{
					throw StbLoadingException(stbi_failure_reason());
				};
			}

			*image = image_buffer;
		}
#pragma endregion

#pragma region Image Write
		int stbi_sc_io_write(void* user, void* data, int size)
		{
			Stream* stream = (Stream*)user;
			return static_cast<int>(stream->write(data, size));
		}

		void write_image(RawImage& image, ImageFormat format, Stream& output)
		{
			uint8_t* buffer = nullptr;

			uint8_t channels = 0;

			Image::BasePixelType source_type = image.base_type();
			switch (source_type)
			{
			case Image::BasePixelType::RGBA:
				channels = 4;
				break;
			case Image::BasePixelType::RGB:
				channels = 3;
				break;
			case Image::BasePixelType::LA:
				channels = 2;
				break;
			case Image::BasePixelType::L:
				channels = 1;
				break;
			default:
				break;
			}

			Image::PixelDepth destination_depth = Image::PixelDepth::RGBA8;
			if (source_type == Image::BasePixelType::RGBA)
			{
				destination_depth = Image::PixelDepth::RGBA8;
			}
			else if (source_type == Image::BasePixelType::RGB)
			{
				destination_depth = Image::PixelDepth::RGB8;
			}
			else if (source_type == Image::BasePixelType::LA)
			{
				destination_depth = Image::PixelDepth::LUMINANCE8_ALPHA8;
			}
			else if (source_type == Image::BasePixelType::L)
			{
				destination_depth = Image::PixelDepth::LUMINANCE8;
			}

			if (image.depth() != destination_depth)
			{
				buffer = sc::memalloc(Image::calculate_image_length(image.width(), image.height(), destination_depth));

				Image::remap(image.data(), buffer,
					image.width(), image.height(),
					image.depth(), destination_depth
				);
			}

			int result = 0;
			switch (format)
			{
			case ImageFormat::PNG:
				result = stbi_write_png_to_func((stbi_write_func*)&stbi_sc_io_write, (void*)&output, image.width(), image.height(), channels, buffer ? buffer : image.data(), 0);
				break;
			case sc::stb::ImageFormat::BMP:
				result = stbi_write_bmp_to_func((stbi_write_func*)&stbi_sc_io_write, (void*)&output, image.width(), image.height(), channels, buffer ? buffer : image.data());
				break;
			case sc::stb::ImageFormat::TGA:
				result = stbi_write_tga_to_func((stbi_write_func*)&stbi_sc_io_write, (void*)&output, image.width(), image.height(), channels, buffer ? buffer : image.data());
				break;
			case sc::stb::ImageFormat::JPEG:
				result = stbi_write_jpg_to_func((stbi_write_func*)&stbi_sc_io_write, (void*)&output, image.width(), image.height(), channels, buffer ? buffer : image.data());
				break;
			default:
				break;
			}

			if (buffer)
			{
				free(buffer);
			}

			if (result == 0)
			{
				throw StbWritingException();
			}
		}

		void write_image(RawImage& image, std::string extension, Stream& output)
		{
			ImageFormat format = ImageFormat::PNG;

			if (extension.empty())
			{
				extension = ".png";
			}

			if (extension.at(0) == '.')
			{
				extension = std::string(extension.begin() + 1, extension.end());
			}

			if (extension == "png")
			{
				format = ImageFormat::PNG;
			}

			if (extension == "jpg" || extension == "jpeg")
			{
				format = ImageFormat::JPEG;
			}

			if (extension == "tga")
			{
				format = ImageFormat::TGA;
			}

			if (extension == "bmp")
			{
				format = ImageFormat::BMP;
			}

			write_image(image, format, output);
		}

#pragma endregion
	}
}