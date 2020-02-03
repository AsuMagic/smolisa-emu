#include "smol/emu/framebuffer/framebuffer.hpp"

#include "smol/common/masks.hpp"

#include <fmt/core.h>

auto FrameBuffer::get_char(std::size_t x, std::size_t y) const -> FrameBuffer::Char
{
	const std::size_t base_address = (x + y * width) * sizeof_fb_char;

	return {.code                = m_character_data.at(base_address),
			.palette_front_entry = char((m_character_data.at(base_address + 1) & masks::lower_nibble) >> 0),
			.palette_back_entry  = char((m_character_data.at(base_address + 1) & masks::upper_nibble) >> 4)};
}

void FrameBuffer::set_palette_entry(std::size_t index, FrameBuffer::PaletteEntry entry)
{
	const std::size_t base_address = index * sizeof_palette_entry;

	m_palette_data[base_address]     = entry.r;
	m_palette_data[base_address + 1] = entry.g;
	m_palette_data[base_address + 2] = entry.b;
}

auto FrameBuffer::get_palette_entry(std::size_t index) const -> FrameBuffer::PaletteEntry
{
	const std::size_t base_address = index * sizeof_palette_entry;

	return {.r = Byte(m_palette_data[base_address]),
			.g = Byte(m_palette_data[base_address + 1]),
			.b = Byte(m_palette_data[base_address + 2])};
}

FrameBuffer::FrameBuffer(FrameBufferConfig config) :
	m_character_data(width * height * sizeof_fb_char), m_palette_data(width * height * sizeof_palette_entry)
{
	// TODO: error checks

	m_font.loadFromFile(std::string{config.font_path});

	m_glyph_width  = m_font.getSize().x / 256;
	m_glyph_height = m_font.getSize().y;

	m_image.create(unsigned(width * m_glyph_width), unsigned(height * m_glyph_height));

	m_window.create(sf::VideoMode{m_image.getSize().x, m_image.getSize().y}, "smolisa framebuffer");
	m_window.setVerticalSyncEnabled(true);
	m_window.setFramerateLimit(30);

	clear();
	rebuild();
	display();
}

void FrameBuffer::clear()
{
	std::fill(m_palette_data.begin(), m_palette_data.end(), 0); // Default all palettes to black
	set_palette_entry(1, {255, 255, 255});
	set_palette_entry(2, {127, 0, 0});

	for (std::size_t y = 0; y < height; ++y)
	{
		for (std::size_t x = 0; x < width; ++x)
		{
			std::size_t offset              = (x + y * width) * sizeof_fb_char;
			m_character_data.at(offset)     = '\0';
			m_character_data.at(offset + 1) = 0b0000'0001;
		}
	}
}

void FrameBuffer::rebuild()
{
	for (std::size_t y = 0; y < height; ++y)
	{
		for (std::size_t x = 0; x < width; ++x)
		{
			update_char(get_char(x, y), x, y);
		}
	}
}

auto FrameBuffer::display() -> bool
{
	for (sf::Event ev{}; m_window.pollEvent(ev);)
	{
		switch (ev.type)
		{
		case sf::Event::Closed:
		{
			m_window.close();
			return false;
		}

		default: break;
		}
	}

	m_window.clear();

	sf::Texture texture;
	texture.loadFromImage(m_image);

	sf::Sprite sprite;
	sprite.setTexture(texture);

	m_window.draw(sprite);
	m_window.display();

	return m_window.isOpen();
}

void FrameBuffer::update_char(Char c, std::size_t x, std::size_t y)
{
	const auto [br, bg, bb] = get_palette_entry(c.palette_back_entry);
	const auto [fr, fg, fb] = get_palette_entry(c.palette_front_entry);

	const sf::Color back_color{br, bg, bb};
	const sf::Color front_color{fr, fg, fb};

	for (std::size_t image_y = y * m_glyph_height; image_y < (y + 1) * m_glyph_height; ++image_y)
	{
		for (std::size_t image_x = x * m_glyph_width; image_x < (x + 1) * m_glyph_width; ++image_x)
		{
			const std::size_t glyph_base_x = (c.code % 128) * m_glyph_width;
			const std::size_t glyph_base_y = (c.code / 128) * m_glyph_height;

			const std::size_t glyph_x = glyph_base_x + (image_x % m_glyph_width);
			const std::size_t glyph_y = glyph_base_y + (image_y % m_glyph_height);

			const bool set = m_font.getPixel(unsigned(glyph_x), unsigned(glyph_y)).r > 127;

			m_image.setPixel(unsigned(image_x), unsigned(image_y), set ? front_color : back_color);
		}
	}
}

void FrameBuffer::display_simple_string(std::string_view s, std::size_t origin_x, std::size_t origin_y)
{
	std::size_t x = origin_x;
	std::size_t y = origin_y;

	for (char i : s)
	{
		// Jump on newlines
		if (i == '\n')
		{
			++y;
			x = origin_x;
			continue;
		}

		// Loop back to the left
		if (x >= width)
		{
			++y;
			x = 0;
		}

		// Loop back to the top... meh
		if (y >= height)
		{
			y = 0;
		}

		const auto addr = Addr((x + y * width) * sizeof_fb_char);
		set_byte(addr, i);
		set_byte(addr + 1, 0b0010'0001); // Alert color

		++x;
	}
}

auto FrameBuffer::byte_region(Addr addr) -> FrameBuffer::Region
{
	if (addr >= pixel_data_address && addr <= pixel_data_end_address)
	{
		return Region::FrameData;
	}

	if (addr >= palette_address && addr <= palette_end_address)
	{
		return Region::PaletteData;
	}

	if (addr == vsync_wait_address)
	{
		return Region::VsyncWait;
	}

	return Region::Invalid;
}

auto FrameBuffer::set_byte(Addr addr, Byte byte) -> bool
{
	switch (byte_region(addr))
	{
	case Region::FrameData:
	{
		m_character_data[addr - pixel_data_address] = byte;

		const std::size_t pixel_index = (addr - pixel_data_address) / sizeof_fb_char;
		const std::size_t x           = pixel_index % width;
		const std::size_t y           = pixel_index / width;
		update_char(get_char(x, y), x, y);

		return true;
	}

	case Region::PaletteData:
	{
		m_palette_data[addr - palette_address] = byte;
		rebuild();
		return true;
	}

	case Region::VsyncWait:
	{
		display();
		return true;
	}

	default:
	case Region::Invalid: return false;
	}
}

auto FrameBuffer::get_byte(Addr addr) const -> std::optional<Byte>
{
	switch (byte_region(addr))
	{
	case Region::FrameData: return m_character_data[addr - pixel_data_address];
	case Region::PaletteData: return m_palette_data[addr - palette_address];
	case Region::VsyncWait:
	case Region::Invalid:
	default: return std::nullopt;
	}
}
