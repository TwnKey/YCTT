

#include <unicode/ucnv.h>
#include <unicode/unistr.h>
#include <math.h>
#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <ft2build.h>
#include FT_FREETYPE_H
#include <windows.h>
#include "utf8.h"
#include <filesystem>
#include "Character.h"
#include "ArrayOperations.h"
#include <sstream>
#include <algorithm>
#include <map>
#include <unordered_set>

int char_height;
int size;
int total_number_of_chars;
size_t space_width;
std::string font;
std::string quiet = "true";
std::vector<Character> chrs;




uint32_t unicode_to_shiftjis(uint32_t chr) {
	// Convert Unicode code point to UTF-16
	UChar utf16_char = static_cast<UChar>(chr);

	// Convert UTF-16 to Shift JIS
	UErrorCode status = U_ZERO_ERROR;
	UConverter* conv = ucnv_open("Shift_JIS", &status);
	if (U_FAILURE(status)) {
		throw std::runtime_error("Failed to open Shift JIS converter");
	}

	// Allocate buffer for Shift JIS data
	char shiftjis_char[3]; // Shift JIS characters can be up to 2 bytes long, plus null termination

	// Convert UTF-16 to Shift JIS
	int32_t shiftjis_len = ucnv_fromUChars(conv, shiftjis_char, sizeof(shiftjis_char), &utf16_char, 1, &status);
	ucnv_close(conv);

	if (U_FAILURE(status)) {
		throw std::runtime_error("Failed to convert Unicode to Shift JIS");
	}

	// Extract the Shift JIS code point from the converted string
	uint32_t shiftjis_code_point = 0;
	for (int i = 0; i < shiftjis_len; ++i) {
		shiftjis_code_point = (shiftjis_code_point << 8) | (shiftjis_char[i] & 0xFF);
	}

	return shiftjis_code_point;
}




/*read_length reads an entire character inside the buffer in order to determine its length in bytes,
so that we know which part of the font file we need to replace by a new character*/
size_t calculateTotalSize(std::vector<unsigned char>& buffer, int start_address) {
	size_t total_size = 0;
	size_t header_size = 0x10;  // Assuming the header size is 0x10 (16 bytes)
	size_t offset = 0;

	// Initial parsing to find the size of total_info_section and total_row_section
	
	size_t total_row_section_size = 0;

	int line_number = readShort(buffer, start_address + 6);

	size_t total_info_section_size = line_number * 4;
	size_t current_idx = 0;
	int current_col = 0;
	size_t compteur_de_pixels = 0;
	for (size_t idx_row = 0; idx_row < line_number; idx_row++) {
		
		//first we get the count of empty bytes
		while (compteur_de_pixels < line_number) {
			size_t empty_bytes_count = buffer[start_address + header_size + total_info_section_size + current_idx];
			compteur_de_pixels += empty_bytes_count;
			current_idx++;
			if (compteur_de_pixels < line_number) {
				//il y a encore des pixels à lire, non noirs cette fois
				size_t non_empty_pixels_count = buffer[start_address + header_size + total_info_section_size + current_idx];
				size_t frac = non_empty_pixels_count / 2;
				size_t nb_bytes_to_skip = (non_empty_pixels_count % 2 == 0) ? frac : frac + 1;
				current_idx += nb_bytes_to_skip + 1;
				compteur_de_pixels += non_empty_pixels_count;
			}
			else {
				
				break;
				
			}
		}
		compteur_de_pixels = 0;
		
	}
	size_t row_section_size = current_idx;
	//row_section_size += 2;
	total_size = header_size + total_info_section_size + row_section_size;
	//size_t oui = readInt(buffer, start_address + total_size);
	
	//std::cout << std::hex << oui << " " << start_address + total_size << std::endl;

	return total_size;
}

/*get_addr_char finds the position in the font file where the specified character's drawing is located
code is the code used inside the font file, an internal encoding rule that follows the behaviour of "get_index" function given below*/
int get_addr_char(int code, std::vector<unsigned char> buffer, bool &success){
	
	int char_number = total_number_of_chars;
	
	if ((((code) * 4+2)>0)&& (((code) * 4+2)<buffer.size())) success = true;
	else {
		success = false; 
		return -1;
	}
	
	int corresponding_offset = readInt(buffer, (code)*4+2); 
	
	if (corresponding_offset == 0) { 
		success = false;
		return -1;
	}
	int addr = (char_number)*4+corresponding_offset+2;	
	
	if (((addr)>0)&& (addr<buffer.size())) success = true;
	else {
		success = false; 
		return -1;
	}
	return addr;
}

//https://unifoundry.com/japanese/index.html
//0x471FD4
uint32_t get_index(uint32_t sjis) {
	uint8_t lead_byte = sjis >> 8;
	uint8_t trail_byte = sjis & 0xFF;

	unsigned int u1, u2;
	if (lead_byte > 0xDF)
		u1 = (0xA0 - 0x81) * 0xBC + (lead_byte - 0xE0) * 0xBC;
	else if ((lead_byte > 0x80) && (lead_byte < 0xA0))
		u1 = (lead_byte - 0x81) * 0xBC;
	else
		return -1;
	if ((trail_byte > 0x7F) && (trail_byte < 0xFD))
		u2 = trail_byte - 1 - 0x40;
	else if ((trail_byte > 0x3F) && (trail_byte < 0x7F))
		u2 = trail_byte - 0x40;
	else
		return -1;

	uint32_t index = u1 + u2;

	return index;
}
uint32_t get_sjis_from_index(uint32_t index) {
	// Calculate row and cell numbers from the index
	uint32_t row = (index / 0xBC);
	uint32_t cell = (index % 0xBC);

	uint8_t lead_byte;
	uint8_t trail_byte;
	if (row > (0xA0 - 0x81))
		lead_byte = 0xE0 + (row - (0xA0 - 0x81));
	else 
		lead_byte = 0x81 + row;
	// Adjust cell byte for skipped ranges
	if ((cell > (0x7F - 0x40))){
		cell++;
	}
	cell += 0x40;

	return (lead_byte << 8) | cell;
}




std::unordered_set<unsigned int> characters_used_by_english() {
	std::vector<uint32_t> chars = {};

	std::ifstream file("characters_kept.ini");

	if (!file.is_open()) {
		std::cout << "No 'characters_kept.ini' file found, so took the hardcoded list of 'official' characters to NOT replace." << std::endl;
		chars = { 0x8149,0x8168,0x8194,0x8190,0x8193,0x8195,0x8166,0x8169,0x816A,0x8196,0x817B,0x8143,0x817C,0x8144,0x815E,0x824F,0x8250,0x8251,0x8252,0x8253,0x8254,0x8255,0x8256,0x8257,0x8258,0x8146,0x8147,0x8183,0x8181,0x8184,0x8148,0x8197,0x8260,0x8261,0x8262,0x8263,0x8264,0x8265,0x8266,0x8267,0x8268,0x8269,0x826A,0x826B,0x826C,0x826D,0x826E,0x826F,0x8270,0x8271,0x8272,0x8273,0x8274,0x8275,0x8276,0x8277,0x8278,0x8279,0x816D,0x818F,0x816E,0x814F,0x8151,0x8140,0x8281,0x8282,0x8283,0x8284,0x8285,0x8286,0x8287,0x8288,0x8289,0x828A,0x828B,0x828C,0x828D,0x828E,0x828F,0x8290,0x8291,0x8292,0x8293,0x8294,0x8295,0x8296,0x8297,0x8298,0x8299,0x829A,0x816F,0x8162,0x8170,0x8160,0xd86d, 0x25, 0x9364, 0x65, 0x2e, 0x6e, 0x82ac, 0x76, 0x865a, 0x2c, 0xda4e, 0x6c, 0xcd5f, 0x74, 0x20, 0x825f, 0x21, 0xd992, 0x61, 0x32, 0xcbaf, 0x72, 0x6f, 0xd14a, 0x41, 0x33, 0x835e, 0x73, 0xcf48, 0x2d, 0x96df, 0x6d, 0x62, 0x82b7, 0x79, 0xd686, 0x30, 0xe1bf, 0x70, 0x94b5, 0x69, 0x23, 0x63, 0x64, 0x8f63, 0xcb97, 0xa, 0x8655, 0x77, 0x48, 0x75, 0x66, 0x27, 0x67, 0x68, 0x52, 0x49, 0x82c2, 0x78, 0x97ec, 0xcd4e, 0x8380, 0x31, 0x71, 0x8356, 0x6b, 0xcc50, 0x54, 0xd053, 0x43, 0x4e, 0x8174, 0x53, 0x44, 0xcd89, 0x56, 0x4f, 0xd7ae, 0x47, 0xd1bd, 0x46, 0x57, 0x3b, 0x4d, 0x4c, 0x45, 0x9446, 0x82b0, 0x7a, 0x4b, 0x42, 0xdc5d, 0x59, 0x50, 0x8d95, 0x6a, 0x8fe0, 0x55, 0x8346, 0x5b, 0x8790, 0x5d, 0x85b2, 0x35, 0x8366, 0x4a, 0x836f, 0x82c9, 0x8362, 0x834f, 0x86ba, 0x8381, 0xd64d, 0x835a, 0x815b, 0xb2, 0x8357, 0x82a2, 0xd34b, 0xd4, 0xd28e, 0x82ab, 0xd044, 0x96b2, 0x895e, 0x8ca9, 0x8561, 0x82cc, 0xce48, 0x8660, 0x8fea, 0xc3, 0xd94c, 0xba, 0x82f0, 0x959c, 0x856c, 0x8194, 0x82ea, 0x9843, 0x82bd, 0xd099, 0x8142, 0x8359, 0x28, 0x39, 0x29, 0x5a, 0xcc, 0xd2, 0x3a, 0x8365, 0x8ad6, 0x34, 0x36, 0x51, 0x3f, 0xdb86, 0x81f4, 0x2a, 0xd0, 0xcd4b, 0x8b41, 0xfcfc, 0xcd, 0x37, 0x38, 0xbb, 0x815c, 0x7e, 0x8374, 0x8340, 0x91d4, 0x834e, 0xcd5d, 0x8367, 0xcf65, 0x877d, 0xa6, 0xd047, 0xcbaa, 0xd844, 0xcd67, 0xd874, 0xbc, 0xcc44, 0xcf75, 0xc0, 0x8588, 0x9558, 0xdb7b, 0x85ba, 0xda69, 0x7, 0xd960, 0xb8, 0x93ae, 0x837e, 0x13, 0xe, 0xc9, 0x8642, 0xd24e, 0x8173, 0x96f2, 0xd3, 0xd7, 0xb3, 0x10, 0x8a9b, 0x15, 0x11, 0xd948, 0x1f, 0x834c, 0x8383, 0x8345, 0x14, 0x16, 0x8770, 0x3d, 0x838c, 0xc6, 0xce56, 0x97d5, 0xd8, 0x8fd5, 0x8c82, 0x9755, 0xd6, 0xce58, 0xce9f, 0x94ad, 0x865d, 0x866f, 0xca, 0xcf63, 0xd76b, 0xd0ba, 0x17, 0xccbb, 0x834b, 0xb6, 0x8ebf, 0xcd5a, 0xb9, 0xd9, 0x8342, 0x8ad2, 0x8ef4, 0xa2, 0x8347, 0x8375, 0xd8ac, 0xc4, 0x1c, 0x94b1, 0x1d, 0xbd, 0xd451, 0xda43, 0xb7, 0xd57b, 0x9768, 0x8354, 0xd259, 0x8382, 0x836a, 0xf, 0x8378, 0x8c80, 0xd769, 0x875c, 0xaf, 0xd756, 0xcf, 0xbe, 0xdd, 0xd36f, 0xcd47, 0xc, 0x838d, 0xb0, 0x836d, 0x8350, 0xcbb8, 0x8b40, 0xad, 0x837c, 0xda, 0xcb6d, 0xa4, 0xdb, 0x8560, 0x8e8f, 0x4, 0xcf7d, 0x8376, 0xd46f, 0x8641, 0xd177, 0x8349, 0x8384, 0x9841, 0xd96d, 0x9846, 0x8353, 0x6, 0x835c, 0x834d, 0xce76, 0x90a8, 0xd972, 0x2f, 0xd173, 0xcd50, 0xd0a3, 0xac, 0xbf, 0xd251, 0x837a, 0x82b8, 0xd7a3, 0x82d4, 0x8cec, 0x8f9d, 0x97c3, 0xd360, 0xa1, 0xc8, 0x84bd, 0x8370, 0x82cf, 0x8360, 0x8352, 0xc5, 0x82ae, 0x8387, 0xcb, 0x82b4, 0x9862, 0xdc, 0x90e0, 0xd45c, 0xc2, 0x1, 0x19, 0x18, 0x8, 0x87a7, 0x825c, 0xd249, 0x12, 0x90fc, 0x95cf, 0xb4, 0x819a, 0xa5, 0xc1, 0xb5, 0x2, 0x8c76, 0xc7, 0xd7ac, 0xd042, 0x92b7, 0xd059, 0x8841, 0x8fb8, 0xcc4e, 0x8466, 0x9066, 0xcf61, 0xd6ba, 0xcf79, 0xd086, 0xcc7f, 0xae, 0x8b43, 0x85aa, 0x8dd7, 0xd5, 0x859d, 0x82b2, 0xd450, 0x9557, 0xcf49, 0xa7, 0xce9d, 0x865e, 0xd65f, 0x8ad4, 0x88d9, 0xb1, 0xcf7a, 0x8158, 0x914e, 0xb, 0xd766, 0xd95e, 0xaa, 0xcb67, 0xce, 0xd192, 0x82ba, 0xcc59, 0x89ea, 0xce7c, 0x8468, 0x83b5, 0x8648, 0x884f, 0x8bad, 0x1b, 0xce5a, 0x8c57, 0x8971, 0x837b, 0xcd68, 0xa8, 0xd57c, 0xd38b, 0xa3, 0x94f2, 0x8386, 0x8465, 0x8ccb, 0xd579, 0x8e45, 0x838f, 0x58 };
	}
	else {
		std::string str;
		std::size_t idx = -1;
		while (std::getline(file, str)) {

			int code = 0;
			std::vector<unsigned int> utf32line;
			utf8::utf8to32(str.begin(), str.end(), std::back_inserter(utf32line));
			if (!utf32line.empty()) {
				code = utf32line[0];
				unsigned int sjis = unicode_to_shiftjis(code);
				chars.push_back(sjis);
			}
		}
		file.close();
	}
	
	std::unordered_set<unsigned int> used;
	for (auto c : chars) {
		uint8_t leading_byte = (c & 0xFF00) >> 8;
		if (leading_byte == 0)
			used.emplace(c);
		else {
			if (get_index(c) != -1) {
				used.emplace(c);
			}
		}
	}
	return used;

}


/*build_characters_list creates a list of characters to either replace or add inside the font
the replaced characters are hardcoded inside the function, and the added characters are
specified inside the config_font.ini file*/
void build_characters_list(std::vector<uint8_t> buffer_font_file){
	//here we're going to create a character list;
	//we'll get the utf32 from the symbol written in the file
	//A, B, C ... regular characters will be hardcoded here since they're hardcoded in the game executable
	bool success = true;
	chrs.push_back(Character(get_addr_char(get_index(0x8140), buffer_font_file, success),0x20,0x8140));
	chrs.push_back(Character(get_addr_char(get_index(0x8149), buffer_font_file, success),0x21,0x8149));
	chrs.push_back(Character(get_addr_char(get_index(0x8168), buffer_font_file, success),0x22,0x8168));
	chrs.push_back(Character(get_addr_char(get_index(0x8194), buffer_font_file, success),0x23,0x8194));
	chrs.push_back(Character(get_addr_char(get_index(0x8190), buffer_font_file, success),0x24,0x8190));
	chrs.push_back(Character(get_addr_char(get_index(0x8193), buffer_font_file, success),0x25,0x8193));
	chrs.push_back(Character(get_addr_char(get_index(0x8195), buffer_font_file, success),0x26,0x8195));
	chrs.push_back(Character(get_addr_char(get_index(0x8166), buffer_font_file, success),0x27,0x8166));
	chrs.push_back(Character(get_addr_char(get_index(0x8169), buffer_font_file, success),0x28,0x8169));
	chrs.push_back(Character(get_addr_char(get_index(0x816A), buffer_font_file, success),0x29,0x816A));
	chrs.push_back(Character(get_addr_char(get_index(0x8196), buffer_font_file, success),0x2A,0x8196));
	chrs.push_back(Character(get_addr_char(get_index(0x817B), buffer_font_file, success),0x2B,0x817B));
	chrs.push_back(Character(get_addr_char(get_index(0x8143), buffer_font_file, success),0x2C,0x8143));
	chrs.push_back(Character(get_addr_char(get_index(0x817C), buffer_font_file, success),0x2D,0x817C));
	chrs.push_back(Character(get_addr_char(get_index(0x8144), buffer_font_file, success),0x2E,0x8144));
	chrs.push_back(Character(get_addr_char(get_index(0x815E), buffer_font_file, success),0x2F,0x815E));
	chrs.push_back(Character(get_addr_char(get_index(0x824F), buffer_font_file, success),0x30,0x824F));
	chrs.push_back(Character(get_addr_char(get_index(0x8250), buffer_font_file, success),0x31,0x8250));
	chrs.push_back(Character(get_addr_char(get_index(0x8251), buffer_font_file, success),0x32,0x8251));
	chrs.push_back(Character(get_addr_char(get_index(0x8252), buffer_font_file, success),0x33,0x8252));
	chrs.push_back(Character(get_addr_char(get_index(0x8253), buffer_font_file, success),0x34,0x8253));
	chrs.push_back(Character(get_addr_char(get_index(0x8254), buffer_font_file, success),0x35,0x8254));
	chrs.push_back(Character(get_addr_char(get_index(0x8255), buffer_font_file, success),0x36,0x8255));
	chrs.push_back(Character(get_addr_char(get_index(0x8256), buffer_font_file, success),0x37,0x8256));
	chrs.push_back(Character(get_addr_char(get_index(0x8257), buffer_font_file, success),0x38,0x8257));
	chrs.push_back(Character(get_addr_char(get_index(0x8258), buffer_font_file, success),0x39,0x8258));
	chrs.push_back(Character(get_addr_char(get_index(0x8146), buffer_font_file, success),0x3A,0x8146));
	chrs.push_back(Character(get_addr_char(get_index(0x8147), buffer_font_file, success),0x3B,0x8147));
	chrs.push_back(Character(get_addr_char(get_index(0x8183), buffer_font_file, success),0x3C,0x8183));
	chrs.push_back(Character(get_addr_char(get_index(0x8181), buffer_font_file, success),0x3D,0x8181));
	chrs.push_back(Character(get_addr_char(get_index(0x8184), buffer_font_file, success),0x3E,0x8184));
	chrs.push_back(Character(get_addr_char(get_index(0x8148), buffer_font_file, success),0x3F,0x8148));
	chrs.push_back(Character(get_addr_char(get_index(0x8197), buffer_font_file, success),0x40,0x8197));
	chrs.push_back(Character(get_addr_char(get_index(0x8260), buffer_font_file, success),0x41,0x8260));
	chrs.push_back(Character(get_addr_char(get_index(0x8261), buffer_font_file, success),0x42,0x8261));
	chrs.push_back(Character(get_addr_char(get_index(0x8262), buffer_font_file, success),0x43,0x8262));
	chrs.push_back(Character(get_addr_char(get_index(0x8263), buffer_font_file, success),0x44,0x8263));
	chrs.push_back(Character(get_addr_char(get_index(0x8264), buffer_font_file, success),0x45,0x8264));
	chrs.push_back(Character(get_addr_char(get_index(0x8265), buffer_font_file, success),0x46,0x8265));
	chrs.push_back(Character(get_addr_char(get_index(0x8266), buffer_font_file, success),0x47,0x8266));
	chrs.push_back(Character(get_addr_char(get_index(0x8267), buffer_font_file, success),0x48,0x8267));
	chrs.push_back(Character(get_addr_char(get_index(0x8268), buffer_font_file, success),0x49,0x8268));
	chrs.push_back(Character(get_addr_char(get_index(0x8269), buffer_font_file, success),0x4A,0x8269));
	chrs.push_back(Character(get_addr_char(get_index(0x826A), buffer_font_file, success),0x4B,0x826A));
	chrs.push_back(Character(get_addr_char(get_index(0x826B), buffer_font_file, success),0x4C,0x826B));
	chrs.push_back(Character(get_addr_char(get_index(0x826C), buffer_font_file, success),0x4D,0x826C));
	chrs.push_back(Character(get_addr_char(get_index(0x826D), buffer_font_file, success),0x4E,0x826D));
	chrs.push_back(Character(get_addr_char(get_index(0x826E), buffer_font_file, success),0x4F,0x826E));
	chrs.push_back(Character(get_addr_char(get_index(0x826F), buffer_font_file, success),0x50,0x826F));
	chrs.push_back(Character(get_addr_char(get_index(0x8270), buffer_font_file, success),0x51,0x8270));
	chrs.push_back(Character(get_addr_char(get_index(0x8271), buffer_font_file, success),0x52,0x8271));
	chrs.push_back(Character(get_addr_char(get_index(0x8272), buffer_font_file, success),0x53,0x8272));
	chrs.push_back(Character(get_addr_char(get_index(0x8273), buffer_font_file, success),0x54,0x8273));
	chrs.push_back(Character(get_addr_char(get_index(0x8274), buffer_font_file, success),0x55,0x8274));
	chrs.push_back(Character(get_addr_char(get_index(0x8275), buffer_font_file, success),0x56,0x8275));
	chrs.push_back(Character(get_addr_char(get_index(0x8276), buffer_font_file, success),0x57,0x8276));
	chrs.push_back(Character(get_addr_char(get_index(0x8277), buffer_font_file, success),0x58,0x8277));
	chrs.push_back(Character(get_addr_char(get_index(0x8278), buffer_font_file, success),0x59,0x8278));
	chrs.push_back(Character(get_addr_char(get_index(0x8279), buffer_font_file, success),0x5A,0x8279));
	chrs.push_back(Character(get_addr_char(get_index(0x816D), buffer_font_file, success),0x5B,0x816D));
	chrs.push_back(Character(get_addr_char(get_index(0x818F), buffer_font_file, success),0x5C,0x818F));
	chrs.push_back(Character(get_addr_char(get_index(0x816E), buffer_font_file, success),0x5D,0x816E));
	chrs.push_back(Character(get_addr_char(get_index(0x814F), buffer_font_file, success),0x5E,0x814F));
	chrs.push_back(Character(get_addr_char(get_index(0x8151), buffer_font_file, success),0x5F,0x8151));
	chrs.push_back(Character(get_addr_char(get_index(0x8140), buffer_font_file, success),0x60,0x8140));
	chrs.push_back(Character(get_addr_char(get_index(0x8281), buffer_font_file, success),0x61,0x8281));
	chrs.push_back(Character(get_addr_char(get_index(0x8282), buffer_font_file, success),0x62,0x8282));
	chrs.push_back(Character(get_addr_char(get_index(0x8283), buffer_font_file, success),0x63,0x8283));
	chrs.push_back(Character(get_addr_char(get_index(0x8284), buffer_font_file, success),0x64,0x8284));
	chrs.push_back(Character(get_addr_char(get_index(0x8285), buffer_font_file, success),0x65,0x8285));
	chrs.push_back(Character(get_addr_char(get_index(0x8286), buffer_font_file, success),0x66,0x8286));
	chrs.push_back(Character(get_addr_char(get_index(0x8287), buffer_font_file, success),0x67,0x8287));
	chrs.push_back(Character(get_addr_char(get_index(0x8288), buffer_font_file, success),0x68,0x8288));
	chrs.push_back(Character(get_addr_char(get_index(0x8289), buffer_font_file, success),0x69,0x8289));
	chrs.push_back(Character(get_addr_char(get_index(0x828A), buffer_font_file, success),0x6A,0x828A));
	chrs.push_back(Character(get_addr_char(get_index(0x828B), buffer_font_file, success),0x6B,0x828B));
	chrs.push_back(Character(get_addr_char(get_index(0x828C), buffer_font_file, success),0x6C,0x828C));
	chrs.push_back(Character(get_addr_char(get_index(0x828D), buffer_font_file, success),0x6D,0x828D));
	chrs.push_back(Character(get_addr_char(get_index(0x828E), buffer_font_file, success),0x6E,0x828E));
	chrs.push_back(Character(get_addr_char(get_index(0x828F), buffer_font_file, success),0x6F,0x828F));
	chrs.push_back(Character(get_addr_char(get_index(0x8290), buffer_font_file, success),0x70,0x8290));
	chrs.push_back(Character(get_addr_char(get_index(0x8291), buffer_font_file, success),0x71,0x8291));
	chrs.push_back(Character(get_addr_char(get_index(0x8292), buffer_font_file, success),0x72,0x8292));
	chrs.push_back(Character(get_addr_char(get_index(0x8293), buffer_font_file, success),0x73,0x8293));
	chrs.push_back(Character(get_addr_char(get_index(0x8294), buffer_font_file, success),0x74,0x8294));
	chrs.push_back(Character(get_addr_char(get_index(0x8295), buffer_font_file, success),0x75,0x8295));
	chrs.push_back(Character(get_addr_char(get_index(0x8296), buffer_font_file, success),0x76,0x8296));
	chrs.push_back(Character(get_addr_char(get_index(0x8297), buffer_font_file, success),0x77,0x8297));
	chrs.push_back(Character(get_addr_char(get_index(0x8298), buffer_font_file, success),0x78,0x8298));
	chrs.push_back(Character(get_addr_char(get_index(0x8299), buffer_font_file, success),0x79,0x8299));
	chrs.push_back(Character(get_addr_char(get_index(0x829A), buffer_font_file, success),0x7A,0x829A));
	chrs.push_back(Character(get_addr_char(get_index(0x816F), buffer_font_file, success),0x7B,0x816F));
	chrs.push_back(Character(get_addr_char(get_index(0x8162), buffer_font_file, success),0x7C,0x8162));
	chrs.push_back(Character(get_addr_char(get_index(0x8170), buffer_font_file, success),0x7D,0x8170));
	chrs.push_back(Character(get_addr_char(get_index(0x8160), buffer_font_file, success),0x7E,0x8160));
	
	std::ifstream file("config_font.ini");
	std::string str;
	std::size_t idx = -1;
	while (std::getline(file, str)) {
        size_t idx;
        if ((idx = str.find("CharacterSize")) != std::string::npos) {
            std::string size_str = str.substr(str.find("=") + 1);
            size = std::stoi(size_str);
        } else if ((idx = str.find("MaxHeightFromOrigin")) != std::string::npos) {
            std::string height_str = str.substr(str.find("=") + 1);
            char_height = std::stoi(height_str);
        } else if ((idx = str.find("Font")) != std::string::npos) {
            font = str.substr(str.find("=") + 1);
        } 
		else if ((idx = str.find("SpaceWidth")) != std::string::npos) {
			std::string space_str = str.substr(str.find("=") + 1);
			space_width = std::stoi(space_str);
		}
		else if ((idx = str.find("Quiet")) != std::string::npos) {
            quiet = str.substr(str.find("=") + 1);
        } else {
            // We expect the .ini file to be encoded in UTF-8, but the FreeType Library needs UTF-32 codes
            // to find the drawing of the character, therefore we need the UTF-32 code.
            int code = 0;
            std::vector<unsigned int> utf32line;
            utf8::utf8to32(str.begin(), str.end(), std::back_inserter(utf32line));
            if (!utf32line.empty()) {
                code = utf32line[0];
				unsigned int sjis = unicode_to_shiftjis(code);
                chrs.push_back(Character(-1, code, sjis));
            }
        }
    }
	std::cout << "Character size: " << std::hex << size << std::endl;
	std::cout << "Max height from origin: " << std::hex << char_height << std::endl;
	std::cout << "Font chosen: " << std::hex << font << std::endl;
	std::cout << "List of characters built!" << std::endl;
	file.close();
	
}


/*create_bitmap creates a vector of bytes corresponding to the drawing of the chosen character,
written in the Ys I font format*/
std::vector<unsigned char> create_bitmap(FT_Bitmap*  bitmap,
             FT_Int      x,
             FT_Int      y){
	int i,j,p,q;
	FT_Int  x_max = x + bitmap->width;
	FT_Int  y_max = y + bitmap->rows;
	int origin = y;
	int lowest_y =  bitmap->rows; 
	int highest_y = 0;
	uint8_t column_number = x_max - x; //1 here to put some space for the next char
	uint8_t line_number = y_max - y;
	
	uint8_t max_dim = column_number > line_number ? column_number : line_number;
	

	uint8_t base_line_y = char_height+1;
	uint8_t offset_y = base_line_y-y;
	uint8_t offset_before_next_chr = column_number+2;
	/*The following line is a policy to manage the spaces between characters, there is probably a smarter way to do it*/
	if (column_number<5) offset_before_next_chr = column_number+3;
	//we also prevent a negative y position
	if ((offset_y>0x7F)) offset_y = 0;
	std::vector<unsigned char> header = {};
	size_t col_number_in_bitmap = column_number;
	column_number = line_number;

	header.push_back(0);
	header.push_back(0);
	header.push_back(0);
	header.push_back(0);
	header.push_back(offset_before_next_chr);
	header.push_back(0);
	header.push_back(max_dim);
	header.push_back(0);
	header.push_back(0);
	header.push_back(0);
	header.push_back(offset_y);
	header.push_back(0);
	header.push_back(0);
	header.push_back(0);
	header.push_back(0);
	header.push_back(0);
	
	//first, we build the bitmap :
	std::vector<std::vector<unsigned char>> image(max_dim, std::vector<unsigned char>(max_dim, 0));
	//map is first initialized
	for ( j = 0; j< max_dim; j++){
	  for ( i = 0; i< max_dim; i++){
		  image[j][i] = 0;
		  
	  }
	}
	
	for (int j = origin < 0 ? 0 : origin; j < lowest_y; j++) {
		for (int i = 0; i < max_dim; i++) {

			if (!(i >= col_number_in_bitmap) )
			{
				if (j < max_dim) {
					image[j][i] = (((bitmap->buffer[j * col_number_in_bitmap + i]) & 0xF0) >> 4);
				}
			}
			
		}
	}

	// Iterate over rows from origin - 1 to highest_y (inclusive)
	for (int j = origin < 0 ? 0 : origin-1; j >= highest_y; j--) {
		for (int i = 0; i < max_dim; i++) {
			if (!(i >= col_number_in_bitmap))
			{
				if (j >= 0 && j < max_dim) {
					image[j][i] = (((bitmap->buffer[j * col_number_in_bitmap + i]) & 0xF0) >> 4);
				}
			}
		}
	}
	//display image :
	
	if (quiet == "false"){
		for ( j = 0; j< max_dim; j++){
			for ( i = 0; i< max_dim; i++){
				if (image[j][i]>0x0) printf("%x",image[j][i]);
				else printf(" ");
			  
		  }
		  printf("\n");
		}
	}
	//our bitmap is done, now we need to translate it to the font format.
	
	
	std::vector<unsigned char> total;
	std::vector<unsigned char> total_row_section;
	std::vector<unsigned char> total_info_section;
	int activate = 0;
	int offset = 0;
	int counter = 0;
	for (int idx_row = 0; idx_row < max_dim; idx_row++) {
		int idx_col = 0;
		int nb_subpack = 1;
		std::vector<unsigned char> total_row;

		while (idx_col < max_dim) {
			std::vector<unsigned char> subpack;

			// Skip empty pixels
			while (idx_col < max_dim && image[idx_row][idx_col] == 0) {
				idx_col++;
			}

			// Calculate number of empty pixels
			int nb_empty_pixels = idx_col - counter;
			subpack.push_back(static_cast<unsigned char>(nb_empty_pixels));

			if (idx_col == max_dim) {
				total_row.insert(total_row.end(), subpack.begin(), subpack.end());
				break;
			}
			else {
				nb_subpack += 2;
			}

			// Process non-empty pixels
			counter = idx_col;
			int activate = 0;
			unsigned char current_pixel_byte = 0;

			while (idx_col < max_dim && image[idx_row][idx_col] != 0) {
				if (activate == 1) {
					activate = 0;
					current_pixel_byte = current_pixel_byte + (image[idx_row][idx_col] << 4);
					subpack.push_back(static_cast<unsigned char>(current_pixel_byte));
				}
				else {
					activate = 1;
					current_pixel_byte = image[idx_row][idx_col];
				}
				idx_col++;
			}

			if (activate == 1) {
				subpack.push_back(static_cast<unsigned char>(current_pixel_byte));
			}

			// Insert number of pixels after empty pixels
			int nb_pixels = idx_col - counter;
			subpack.insert(subpack.begin() + 1, static_cast<unsigned char>(nb_pixels));

			counter = idx_col;
			total_row.insert(total_row.end(), subpack.begin(), subpack.end());
		}
		
		// Update total_info_section and total_row_section
		total_info_section.push_back(static_cast<unsigned char>(offset));
		total_info_section.push_back(0);
		total_info_section.push_back(static_cast<unsigned char>(nb_subpack));
		total_info_section.push_back(0);
		offset = offset + total_row.size();
		counter = 0;
		total_row_section.insert(total_row_section.end(), total_row.begin(), total_row.end());
	}
	total_row_section.push_back((unsigned char) 0x00);
	total_row_section.push_back((unsigned char) 0x00);
	total_row_section.push_back((unsigned char) 0x00);
	total_row_section.push_back((unsigned char) 0x00);
	
	
	total.insert(total.end(), header.begin(), header.end());
	
	total.insert(total.end(), total_info_section.begin(), total_info_section.end());
	total.insert(total.end(), total_row_section.begin(), total_row_section.end());
	
	return total;
}
/*draw_character calls create_bitmap for the character specified by UTF32 (character_code)*/
std::vector<unsigned char> draw_character(int character_code, int *length, int target_height, FT_Library library, FT_Face face, FT_GlyphSlot  slot){
	
    FT_Load_Glyph(face, character_code, FT_LOAD_RENDER);
	int distance_top_to_origin = slot->bitmap_top;
		

	std::vector<unsigned char> letter_drawing = create_bitmap(&slot->bitmap,
             slot->bitmap_left,
             slot->bitmap_top);

	return letter_drawing;
}

std::map<int, std::vector<unsigned char>> parseFontFile(const std::string& filename, std::vector<int> & available_slots) {
	std::ifstream file(filename, std::ios::in | std::ios::binary);
	if (!file) {
		throw std::runtime_error("Could not open file");
	}

	std::vector<unsigned char> buffer((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
	file.close();

	int num_chars = readInt(buffer, 0) & 0xFFFF;
	std::map<int, std::vector<unsigned char>> char_map;
	unsigned int start = 2 + (num_chars + 1) * 4;
	size_t tot = 0;
	std::unordered_set<unsigned int> already_used = characters_used_by_english();

	size_t nb_available_slots = 0;
	for (int i = 0; i < num_chars; ++i) {
		int addr_offset = 2 + i * 4;
		int char_addr = readInt(buffer, addr_offset) - 4;

		int sjis_code = get_sjis_from_index(i);

		if ((char_addr == -4) && (i > 0)) { 
			already_used.emplace(sjis_code);
			available_slots.push_back(i); 
		}
		else if (!(already_used.count(sjis_code))){
			already_used.emplace(sjis_code);
		}
		size_t sz = calculateTotalSize(buffer, start + char_addr);
		tot = tot + sz;

		std::vector<unsigned char> char_data(buffer.begin() + start + char_addr, buffer.begin() + start + char_addr + sz);
		char_data.push_back(0);
		char_data.push_back(0);
		char_data.push_back(0);
		char_data.push_back(0);
		
		//std::cout << std::hex << sjis_code << " " << i << " " << get_sjis_from_index(get_index(0x9168)) << " " << get_index(sjis_code) << std::endl;
		int index = get_index(sjis_code);
		char_map[i] = char_data;
		
	}
	for (int i = 0; i < 8464; ++i) {
		int sjis_code = get_sjis_from_index(i);
		int reindex = get_index(sjis_code);
		

		if ((reindex != -1) && (!(already_used.count(sjis_code)))) {

			available_slots.push_back(i);
		}
		else {
		}
	}
	std::cout << std::dec << "number of available slots in the font file: " << available_slots.size() << std::endl;
	return char_map;
}

bool compare(int a, int b) {
	return a > b; // Sort in descending order
}


int main( int     argc,
      char**  argv )
{
	
	
	FT_Library    library;
	FT_Face       face;
	FT_GlyphSlot  slot;
	FT_Error      error;
	space_width = 10;

	std::string font_file(argv[1]);
	std::string base_filename = font_file.substr(font_file.find_last_of("/\\") + 1);
	

	std::string Output = std::filesystem::current_path().string()+"\\outputs\\";
	std::string full_output_path = Output+base_filename;
	std::string output_ini = Output+"\\text.ini";
	
	const char * Outputcstr = Output.c_str();
	const char * full_output_pathcstr = full_output_path.c_str();
	const char * output_inicstr = output_ini.c_str();
	

	//std::cout << std::hex << get_index(0x8281) << std::endl; //0xFC
	//std::cout << std::hex << get_index(0xE540) << std::endl; //0x1A70
	//std::cout << std::hex << get_index(0x82D6) << std::endl; //0x151
	//std::cout << std::hex << get_index(0xE69A) << std::endl; //0x1B85
	//std::cout << std::hex << get_sjis_from_index(get_index(0x8281)) << std::endl;
	//std::cout << std::hex << get_sjis_from_index(get_index(0xE540)) << std::endl;
	//std::cout << std::hex << get_sjis_from_index(get_index(0x82D6)) << std::endl;
	//std::cout << std::hex << get_sjis_from_index(get_index(0xE69A)) << std::endl;



	std::ifstream original_font_file(argv[1], std::ios::in | std::ios::binary);
	std::vector<uint8_t> buffer_font_file((std::istreambuf_iterator<char>(original_font_file)), std::istreambuf_iterator<char>());
	original_font_file.close();
	

	build_characters_list(buffer_font_file);
	error = FT_Init_FreeType(&library);
	std::stringstream ss;
	std::vector<int> available_slots;
	std::map<int, std::vector<unsigned char>> char_map = parseFontFile(argv[1], available_slots);

	std::sort(available_slots.begin(), available_slots.end(), compare);

	std::cout << "Successfully parsed " << char_map.size() << " characters." << std::endl;
	int successful_characters = 0, inserted_characters = 0;
	auto size_index_array_it = char_map.rbegin();

	// Get the highest key
	int size_index_array = char_map.size();
	int addr_end_index_array = 4 * size_index_array + 2;


	for (auto it = chrs.begin(); it != chrs.end(); it++) {
		
		bool success = true;

		error = FT_New_Face(library, font.c_str(), 0, &face);
		error = FT_Set_Char_Size(face, size * 64, 0, 44, 44);

		slot = face->glyph;


		int code = FT_Get_Char_Index(face, it->utf32);
		int index = get_index(it->sjis);
		if (code != 0) {
			successful_characters++;
			if ((char_map.count(index) == 0) || ((index == 0) && (char_map.count(index) > 0))) { //The character is new, we need to add it to the font file
				inserted_characters++;
				int original_length = 0;
				int length;
				std::vector<unsigned char> letter = draw_character(code, &length, char_height, library, face, slot);
				if (available_slots.empty()) {
					std::cout << "All the available character slots were used, none is available, stopping here." << std::endl;
					break;
				}
				int available_slot = available_slots.back(); 
				available_slots.pop_back();

				addr_end_index_array = 4 * available_slot + 2;


				char_map[available_slot] = letter;
				it->sjis = get_sjis_from_index(available_slot);
				
				ss << *it;
			}
			else {
				int length;
				std::vector<unsigned char> letter = draw_character(code, &length, char_height, library, face, slot);
				it->sjis = get_sjis_from_index(index);
				
				/*for (auto b : letter) {
					std::cout << std::hex << int(b) << " ";
				}*/
				ss << *it;
				char_map[index] = letter;
			}
		}
		else {
			std::cout << "The TTF file doesn't include this character (UTF32 = " << std::hex << it->utf32 << ")" << std::endl;
		}
		FT_Done_Face(face);

	}

	if (char_map.count(get_index(0x8140)))
		char_map[get_index(0x8140)][4] = (unsigned char) space_width;

	// Create the output file
	

	std::vector<unsigned char> addresses;
	std::vector<unsigned char> letters;
	int current_position = 0;
	int current_index_addr = 0;
	for (const auto& entry : char_map) {

		

		std::vector<unsigned char> byte_Array = intToByteArray(current_position);
		addresses.insert(addresses.end(), byte_Array.begin(), byte_Array.end());
		letters.insert(letters.end(), entry.second.begin(), entry.second.end());
		current_position += entry.second.size();
		
		current_index_addr = entry.first + 1;
	}

	std::vector<unsigned char> output_data;
	// Write size_index_array in little-endian format
	for (int i = 0; i < 2; ++i) {
		output_data.push_back(((addresses.size()/4) >> (i * 8)) & 0xFF);
	}

	// Write the letters
	output_data.insert(output_data.end(), addresses.begin(), addresses.end());
	output_data.insert(output_data.end(), letters.begin(), letters.end());

	// Write the output data to the file
	std::ofstream writeFontFile;
	writeFontFile.open(full_output_pathcstr, std::ios::out | std::ios::binary);
	writeFontFile.write((const char*)&output_data[0], output_data.size());
	writeFontFile.close();

	// Write the INI file
	std::ofstream writeIniFile;
	writeIniFile.open(output_inicstr, std::ios::out);
	writeIniFile << ss.rdbuf();
	writeIniFile.close();

	std::cout << "Successfully wrote the output files." << std::endl;
	return 0;


}
