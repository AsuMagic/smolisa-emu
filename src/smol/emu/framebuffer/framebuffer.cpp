#include "framebuffer.hpp"

#include <fmt/core.h>

FrameBuffer::Char FrameBuffer::get_char(std::size_t x, std::size_t y) const
{
	const std::size_t base_address = (x + y * width) * sizeof_fb_char;

	Char c;
	c.code                = m_character_data.at(base_address);
	c.palette_front_entry = (m_character_data.at(base_address + 1) & 0b0000'1111) >> 0; // TODO: use masks::
	c.palette_back_entry  = (m_character_data.at(base_address + 1) & 0b1111'0000) >> 4;
	return c;
}

FrameBuffer::PaletteEntry FrameBuffer::get_palette_entry(std::size_t i) const
{
	const std::size_t base_address = i * sizeof_palette_entry;

	PaletteEntry entry;
	entry.r = m_palette_data[base_address];
	entry.g = m_palette_data[base_address + 1];
	entry.b = m_palette_data[base_address + 2];
	return entry;
}

FrameBuffer::FrameBuffer(FrameBufferConfig config) :
	m_character_data(width * height * sizeof_fb_char), m_palette_data(width * height * sizeof_palette_entry)
{
	// TODO: error checks

	m_font.loadFromFile(std::string{config.font_path});

	m_glyph_width  = m_font.getSize().x / 128;
	m_glyph_height = m_font.getSize().y / 2;

	m_image.create(width * m_glyph_width, height * m_glyph_height);

	m_window.create(sf::VideoMode{m_image.getSize().x, m_image.getSize().y}, "smolisa framebuffer");
	m_window.setVerticalSyncEnabled(true);
	m_window.setFramerateLimit(30);

	clear();
	rebuild();
	display();
}

void FrameBuffer::clear()
{
	std::fill(m_palette_data.begin(), m_palette_data.end(), 0);            // Default all palettes to black
	m_palette_data[3] = m_palette_data[4] = m_palette_data[5] = char(255); // palette[1] = white
	m_palette_data[6]                                         = char(127); // palette[2] = red

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

bool FrameBuffer::display()
{
	for (sf::Event ev; m_window.pollEvent(ev);)
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
			std::size_t glyph_base_x = (c.code % 128) * m_glyph_width, glyph_base_y = (c.code / 128) * m_glyph_height;
			std::size_t glyph_x = glyph_base_x + (image_x % m_glyph_width),
						glyph_y = glyph_base_y + (image_y % m_glyph_height);

			const bool set = m_font.getPixel(glyph_x, glyph_y).r > 127;

			m_image.setPixel(image_x, image_y, set ? front_color : back_color);
		}
	}
}

void FrameBuffer::display_simple_string(std::string_view s, std::size_t origin_x, std::size_t origin_y)
{
	std::size_t x = origin_x, y = origin_y;

	for (std::size_t i = 0; i < s.size(); ++i)
	{
		// Jump on newlines
		if (s[i] == '\n')
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

		const Addr addr = (x + y * width) * sizeof_fb_char;
		set_byte(addr, s[i]);
		set_byte(addr + 1, 0b0010'0001); // Alert color

		++x;
	}
}

FrameBuffer::Region FrameBuffer::byte_region(Addr addr) const
{
	if (addr >= 0x0000 && addr <= 0x0F9F)
	{
		return Region::FrameData;
	}

	if (addr >= 0x0FA0 && addr <= 0x0FCF)
	{
		return Region::PaletteData;
	}

	if (addr == 0x0FD0)
	{
		return Region::VsyncWait;
	}

	return Region::Invalid;
}

bool FrameBuffer::set_byte(Addr addr, Byte byte)
{
	switch (byte_region(addr))
	{
	case Region::FrameData:
	{
		m_character_data[addr - base_address] = byte;

		const std::size_t pixel_index = (addr - base_address) / sizeof_fb_char;
		const std::size_t x = pixel_index % width, y = pixel_index / width;
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

std::optional<Byte> FrameBuffer::get_byte(Addr addr) const
{
	switch (byte_region(addr))
	{
	case Region::FrameData: return m_character_data[addr - base_address];
	case Region::PaletteData: return m_palette_data[addr - palette_address];
	case Region::VsyncWait:
	case Region::Invalid: return std::nullopt;
	}
}
