#include "Utilities.h"

#include <iostream>
#include <unordered_set>

#include <unicode/ucnv.h>
#include <unicode/unistr.h>



int find_section_by_name(std::string name, std::vector<Section> sects) {
	for (size_t idx_sect = 0; idx_sect < sects.size(); idx_sect++) {
		if (sects[idx_sect].name == name) {
			return idx_sect;
		}
	}
	return -1;

}

std::vector<BYTE> GetBytesFromCP(int cp) {
	std::vector<BYTE> result;
	for (int i = 3; i >= 0; i--) {
		unsigned int mask = (0xff << (8 * i));
		
		unsigned int masked = (cp & mask);
		BYTE res = (BYTE)(masked >> (8 * i));
		result.push_back(res);
	}
	
	
	return result;
	

}

bool compare_bytes(void* addr, std::vector<BYTE> bytes) {
	bool result = true;
	BYTE* current_addr = static_cast<BYTE*>(addr);
	for (auto b : bytes) {
		
		BYTE exe = *(current_addr);
		//fprintf(stdout, "%x %x\n", exe, b);
		result = result && (b == exe);
		current_addr++;
	}
	return result;
}


void findAndReplaceAll(std::string& data, std::string toSearch, std::string replaceStr)
{
	// Get the first occurrence
	size_t pos = data.find(toSearch);
	// Repeat till end is reached
	while (pos != std::string::npos)
	{
		
		// Replace this occurrence of Sub String
		data.replace(pos, toSearch.size(), replaceStr);
		// Get the next occurrence from the current position
		pos = data.find(toSearch, pos + replaceStr.size());
	}
}
bool FixStr(std::string &str) {
	findAndReplaceAll(str, "\\n", "\x0a");
	findAndReplaceAll(str, "\\t", "\x09");
	findAndReplaceAll(str, "\\\\", "\\");
	findAndReplaceAll(str, "\\'", "'");
	return true;
}

inline bool ends_with(std::string const& value, std::string const& ending)
{
	if (ending.size() > value.size()) return false;
	return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
}


std::u32string utf8_to_utf32(const std::string& utf8) {
	std::u32string utf32;
	for (size_t i = 0; i < utf8.size();) {
		uint32_t cp = 0;
		unsigned char ch = utf8[i];
		if (ch < 0x80) {
			cp = ch;
			i += 1;
		}
		else if ((ch & 0xE0) == 0xC0) {
			cp = ch & 0x1F;
			cp = (cp << 6) | (utf8[i + 1] & 0x3F);
			i += 2;
		}
		else if ((ch & 0xF0) == 0xE0) {
			cp = ch & 0x0F;
			cp = (cp << 6) | (utf8[i + 1] & 0x3F);
			cp = (cp << 6) | (utf8[i + 2] & 0x3F);
			i += 3;
		}
		else if ((ch & 0xF8) == 0xF0) {
			cp = ch & 0x07;
			cp = (cp << 6) | (utf8[i + 1] & 0x3F);
			cp = (cp << 6) | (utf8[i + 2] & 0x3F);
			cp = (cp << 6) | (utf8[i + 3] & 0x3F);
			i += 4;
		}
		utf32.push_back(cp);
	}
	return utf32;
}

std::vector<BYTE> utf32_to_shiftjis(uint32_t chr) {
	std::vector<BYTE> bytes_to_add;
	if (chr < 0x80)
		bytes_to_add.push_back(static_cast<BYTE>(chr));
	else {
		// If the character exists in CPTable, use it
		if (CPTable.count(chr) != 0) {
			uint32_t sjis = CPTable[chr];
			bytes_to_add.push_back((sjis >> 8) & 0xFF);
			bytes_to_add.push_back(sjis & 0xFF);
		}
		else {
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
			for (int i = 0; i < shiftjis_len; ++i) {
				bytes_to_add.push_back(static_cast<BYTE>(shiftjis_char[i]));
			}
		}
	}
	
	return bytes_to_add;
}

std::vector<BYTE> string_to_bytes(std::string str, std::string encoding, std::string hard) {
	std::vector<BYTE> bytes;
	size_t nb_bytes = 1;  // Default to 1 byte per character for UTF-8
	bool custom_characters = false;
	std::string actual_encoding = encoding;

	if (ends_with(encoding, "_custom")) {
		custom_characters = true;
		actual_encoding = encoding.substr(0, encoding.size() - 7);
	}
	bool wide = actual_encoding == "utf16";

	if (wide) {
		nb_bytes = 2;
	}

	// Convert UTF-8 to UTF-32 manually
	std::u32string utf32_str = utf8_to_utf32(str);

	for (char32_t chr : utf32_str) {
		std::vector<BYTE> bytes_to_add = utf32_to_shiftjis(chr);
		
		while (bytes_to_add.size() < nb_bytes) {
			bytes_to_add.insert(bytes_to_add.begin(), 0);
		}
		bytes.insert(bytes.end(), bytes_to_add.begin(), bytes_to_add.end());
	}

	bytes.push_back(0);
	if (wide) {
		bytes.push_back(0);
	}
	return bytes;
}


std::vector<BYTE> number_to_bytes(int c, int nb_bytes) {
	std::vector<BYTE> res;
	for (int i = 0; i < nb_bytes; i++) {
		res.push_back((c & (0xff << (8 * i))) >> (8 * i));
	}
	return res;

}
void replace_all_substr_occurrence(std::string& str, std::string to_replace, std::string replacement) {
	std::string::size_type n = 0;
	while ((n = str.find(to_replace, n)) != std::string::npos)
	{
		str.replace(n, to_replace.size(), replacement);
		n += replacement.size();
	}

}

std::vector<BYTE> ReadFileLine(std::string text_line, std::string encoding, std::string hard){
	std::vector<BYTE> result;
	if (text_line.empty()) return result;

	
	if ((text_line.at(0) == ' ') && (text_line.at(1) == 'L')) {
		result = string_to_bytes(text_line.substr(2, text_line.length()), encoding, hard); //skip the first L, the first " and the last "
	}
	else {
		result = string_to_bytes(text_line, encoding, hard);
	}
	return result;
}

bool check_at_index(const uint8_t* text, size_t text_size, std::vector<uint16_t> pattern)
{
	size_t pattern_index = 0;

	for (size_t i = 0; i < text_size; i++)
	{
		if (pattern[pattern_index] == ANYTHING)
		{
			pattern_index++;
			continue;
		}

		if (text[i] != pattern[pattern_index]) return false;

		pattern_index++;
	}

	return true;
}

bool check_at_index_byte(const uint8_t* text, size_t text_size, std::vector<BYTE> pattern)
{
	size_t pattern_index = 0;
	
	for (size_t i = 0; i < text_size; i++)
	{
		
		if (text[i] != pattern[pattern_index]) return false;

		pattern_index++;
	}

	return true;
}

ULONG_PTR find_chunk(ULONG_PTR addr, ULONG_PTR end, std::vector<uint16_t> pattern, int number)
{
	int size = pattern.size();
	int cnt = 1;
	ULONG_PTR addr_patt = addr;
	for (addr_patt = addr; addr_patt < end - size; addr_patt++) {
		if (check_at_index((const uint8_t*)addr_patt, size, pattern)) {
			if (cnt == number) {
				return addr_patt;
			}
			cnt++;
		}
	}
	return -1;
}
ULONG_PTR find_bytes(ULONG_PTR addr, ULONG_PTR end, std::vector<BYTE> pattern, int number, bool isInstr) {
	int size = pattern.size();
	int cnt = 1;
	uint8_t* addr_patt = (uint8_t*)addr;

	for (addr_patt = (uint8_t*)addr; addr_patt < (uint8_t*)end - size; addr_patt++) {

		
		if (check_at_index_byte((const uint8_t*)addr_patt, size, pattern)) {
			if (cnt == number) {
				return (ULONG_PTR)addr_patt;
			}
			cnt++;
		}
		
	}
	return -1;
}

bool RewriteMemoryEx(ULONG_PTR addr, std::vector<BYTE> BytesToWrite) {
	DWORD dwProtect, dwProtect2;
	if (VirtualProtect((LPVOID)addr, BytesToWrite.size(), PAGE_EXECUTE_READWRITE, &dwProtect)) {
		unsigned char * addr_bytes_to_write = BytesToWrite.data();
		memcpy((void*)addr, addr_bytes_to_write, BytesToWrite.size());
		VirtualProtect((LPVOID) addr, BytesToWrite.size(), dwProtect, &dwProtect2);
		return true;
	}
	else {
		return false;
	}
}
bool RewriteMemory(ULONG_PTR addr, std::vector<BYTE> BytesToWrite) {
	unsigned char* addr_bytes_to_write = BytesToWrite.data();
	memcpy((void*)addr, addr_bytes_to_write, BytesToWrite.size());
		
	return true;
	
}

int read_int(void * addr) {
	int res;
	memcpy(&res, addr, 4);
	return res;
}


BYTE read_byte(void* addr) {
	BYTE res;
	memcpy(&res, addr, 1);
	return res;
}

std::vector<BYTE> read_array_of_bytes(void* addr, size_t size) {
	std::vector<BYTE> res;
	for (size_t idx_b = 0; idx_b < size; idx_b++) {
		res.push_back(read_byte((void *) ((ULONG_PTR)addr + idx_b)));
	}
	return res;
}

