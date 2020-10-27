
#include "pch.h"

// Header
#include "CharacterAtlas.hpp"

// FreeType
#include <ft2build.h>
#include <freetype\freetype.h>


using namespace nui;


ErrStack Font::addSize(uint32_t size)
{
	ErrStack err_stack;
	FT_Error err;

	uint32_t first_unicode = '!';
	uint32_t last_unicode = '~';
	uint32_t unicode_count = last_unicode - first_unicode + 1;

	FT_Face face = (FT_Face)face_ft;

	err = FT_Set_Pixel_Sizes(face, 0, size);
	if (err) {
		return ErrStack(code_location, "failed to set font face size");
	}

	FontSize& font_size = sizes.emplace_back();
	font_size.size = size;

	FT_Size_Metrics& metrics = face->size->metrics;
	font_size.ascender = metrics.ascender / 64;
	font_size.descender = (-metrics.descender) / 64;
	font_size.line_spacing = metrics.height / 64;
	font_size.chars.resize(unicode_count + 1);

	uint32_t i = 0;
	for (uint32_t unicode = first_unicode; unicode <= last_unicode; unicode++) {

		uint32_t glyph_idx = FT_Get_Char_Index(face, unicode);

		err = FT_Load_Glyph(face, glyph_idx, FT_LOAD_RENDER);
		if (err) {
			return ErrStack(code_location, "failed to load and render glyph");
		}

		auto& glyph = face->glyph;

		Character& chara = font_size.chars[i++];
		chara.unicode = unicode;
		chara.bitmap_left = glyph->bitmap_left;
		chara.bitmap_top = glyph->bitmap_top;
		chara.hori_bearing_X = glyph->metrics.horiBearingX / 64;
		chara.hori_bearing_Y = glyph->metrics.horiBearingY / 64;
		chara.advance_X = glyph->advance.x / 64;
		chara.advance_Y = glyph->advance.y / 64;

		bitmap.resize(glyph->bitmap.width * glyph->bitmap.rows);
		std::memcpy(bitmap.data(), glyph->bitmap.buffer, bitmap.size());

		TextureAtlas& tex_atlas = atlas->atlas;
		if (!tex_atlas.addBitmap(bitmap, glyph->bitmap.width, glyph->bitmap.rows, chara.zone)) {
			return ErrStack(code_location, "failed to find space to store character in atlas");
		}
	}

	// White Space
	{
		uint32_t space_unicode = 0x0020;
		uint32_t glyph_idx = FT_Get_Char_Index(face, space_unicode);

		err = FT_Load_Glyph(face, glyph_idx, FT_LOAD_RENDER);
		if (err) {
			return ErrStack(code_location, "failed to load and render glyph");
		}

		auto& glyph = face->glyph;

		Character& chara = font_size.chars[i];
		chara.unicode = space_unicode;
		chara.bitmap_left = glyph->bitmap_left;
		chara.bitmap_top = glyph->bitmap_top;
		chara.hori_bearing_X = glyph->metrics.horiBearingX / 64;
		chara.hori_bearing_Y = glyph->metrics.horiBearingY / 64;
		chara.advance_X = glyph->advance.x / 64;
		chara.advance_Y = glyph->advance.y / 64;

		chara.zone = nullptr;
	}

	return err_stack;
}

ErrStack CharacterAtlas::addFont(std::string path, Font*& r_font)
{
	ErrStack err_stack;
	FT_Error err;

	FT_Library free_type = (FT_Library)free_type_ft;

	if (free_type == nullptr) {
		err = FT_Init_FreeType(&free_type);
		if (err) {
			return ErrStack(code_location, "failed to initialize FreeType library");
		}
		this->free_type_ft = free_type;
	}

	Font& font = this->fonts.emplace_back();
	font.atlas = this;

	checkErrStack(io::readLocalFile(path, font.ttf_file), "failed to read font file");
	
	FT_Face face;
	err = FT_New_Memory_Face(free_type, font.ttf_file.data(), (uint32_t)font.ttf_file.size(), 0, &face);
	if (err) {
		return ErrStack(code_location, "failed to create font face");
	}
	
	font.face_ft = face;
	font.family_name = face->family_name;
	font.style_name = face->style_name;

	r_font = &font;

	return err_stack;
}
