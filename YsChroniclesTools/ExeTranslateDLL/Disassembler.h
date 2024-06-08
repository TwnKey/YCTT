#pragma once
#include <vector>
#include <Zydis/Zydis.h>
#include <windows.h>
#include <unordered_map>
#include <unordered_set>
struct VectorHasher {
	int operator()(const std::vector<uint8_t>& V) const {
		int hash = V.size();
		for (auto& i : V) {
			hash ^= i + 0x9e3779b9 + (hash << 6) + (hash >> 2);
		}
		return hash;
	}
};

struct ref {

	ULONG_PTR ref_loc;
	unsigned int instruction_nb;
	ref(ULONG_PTR loc, unsigned int nb) : ref_loc(loc), instruction_nb(nb) {}
};

class Disassembler
{
	public:
		Disassembler() = default;
		std::vector<ULONG_PTR> addrs;

		std::vector<ULONG_PTR> functions;
		std::unordered_map<ULONG_PTR, std::vector<ref>> pointers; //Stores pointers and their references
		std::unordered_map<std::vector<uint8_t>, std::unordered_set<ULONG_PTR>, VectorHasher> strings_location; //Stores location for strings in exe
		bool ParseExe(ULONG_PTR start_text, ULONG_PTR end_text, ULONG_PTR start_data, ULONG_PTR end_data, ULONG_PTR start_rdata, ULONG_PTR end_rdata);
	
};


struct exe_info {

	Disassembler disasm;

	size_t sectionAlignment;
	ULONG_PTR base;

	ULONG_PTR start_text_section;
	ULONG_PTR end_text_section;
	ULONG_PTR start_rdata_section;
	ULONG_PTR end_rdata_section;
	ULONG_PTR start_data_section;
	ULONG_PTR end_data_section;
	ULONG_PTR start_rsrc_section;
	ULONG_PTR end_rsrc_section;
	ULONG_PTR extra_strings_section_start;
	ULONG_PTR extra_strings_section_end;

	ULONG_PTR extra_text_section_start = NULL;
	ULONG_PTR extra_text_section_end;

	ULONG_PTR extra_text_section_2_start = NULL;
	ULONG_PTR extra_text_section_2_end;


	ULONG_PTR extra_rdata_section_start = NULL;
	ULONG_PTR extra_rdata_section_end;

};
