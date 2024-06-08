#ifndef UTILITIES_H
#define UTILITIES_H
#define ANYTHING 0xFFFF

#include <windows.h>
#include <cstdint>
#include <vector>
#include <string>
#include <iostream>
#include <unordered_map>

extern std::unordered_map<uint32_t, uint32_t> CPTable;

struct Section {
	unsigned long VirtualAddrStart;
	unsigned long PhysicalAddrStart;
	unsigned long VirtualAddrEnd;
	unsigned long PhysicalAddrEnd;
	std::string name;

	std::vector<BYTE> ToBytes() {
		std::vector<BYTE> result;
		return result;
	}
};
inline bool exists(const std::string& name) {
	struct stat buffer;
	return (stat(name.c_str(), &buffer) == 0);
}

bool check_at_index_byte(const uint8_t* text, size_t text_size, std::vector<BYTE> pattern);
std::vector<BYTE> string_to_bytes(std::string str, std::string encoding, std::string hard);
std::vector<BYTE> number_to_bytes(int c, int nb_bytes); 
std::vector<BYTE> ReadFileLine(std::string text_line, std::string encoding, std::string wide);
int find_section_by_name(std::string name, std::vector<Section> sects);

bool check_at_index(const uint8_t* text, size_t text_size, std::vector<uint16_t> pattern);
ULONG_PTR find_chunk(ULONG_PTR addr, ULONG_PTR end, std::vector<uint16_t> pattern, int number);
ULONG_PTR find_bytes(ULONG_PTR addr, ULONG_PTR end, std::vector<BYTE> pattern, int number, bool isInstr);
bool RewriteMemory(ULONG_PTR addr, std::vector<BYTE> BytesToWrite);
bool RewriteMemoryEx(ULONG_PTR addr, std::vector<BYTE> BytesToWrite);
int read_int(void* addr);
BYTE read_byte(void* addr);
std::vector<BYTE> read_array_of_bytes(void* addr, size_t size);
void replace_all_substr_occurrence(std::string& str, std::string to_replace, std::string replacement);
bool compare_bytes(void* addr, std::vector<BYTE> bytes);

struct Translation {
	Translation(std::string str_en, std::string str_fr, ULONG_PTR addr, std::string encoding, std::string hard) {

		this->english_str = str_en;
		this->french_str = str_fr;
		this->addr = addr;
		this->encoding = encoding;
		this->hard = hard;
		std::vector<BYTE> english_bytes = ReadFileLine(english_str, encoding, hard);
		std::vector<BYTE> french_bytes = ReadFileLine(french_str, encoding, hard);

		if (french_bytes.size() == 0) french_bytes = english_bytes;
		english = english_bytes;
		french = french_bytes;

	}
	std::string english_str;
	std::string french_str;
	std::vector<BYTE> english;
	std::vector<BYTE> french;
	std::string encoding;
	ULONG_PTR addr;
	std::string hard;
};


#endif