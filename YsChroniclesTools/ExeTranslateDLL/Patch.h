#ifndef PATCH_H
#define PATCH_H
#include "Utilities.h"
#include "Console.h"
#include "Disassembler.h"
#include <keystone/keystone.h>
#include <unordered_map>
#include "StringPatcher.h"

enum section { non_allocated = 0, extra_rdata1 = 1, extra_text1 = 2, extra_text2 = 3, extra_str = 4};


class Patch {
	
	public:
		Patch(HMODULE h, LPSTR name) {

			this->filename = name;
			this->h = h;

			GetSectionsData();
			CreateExtraSections();
			
			char drive[_MAX_DRIVE];
			char dir[_MAX_DIR];
			char fname[_MAX_FNAME];
			char ext[_MAX_FNAME];

			_splitpath_s(name, drive, dir, fname, ext);

			this->game_folder = std::string(drive) + std::string(dir);
			this->data_folder = std::string(drive) + std::string(dir) + "translation";
			this->TLFile_name = data_folder + "\\" + std::string(fname) + ".csv";

			this->Ref_name = data_folder + "\\" + "strings.csv";

			this->versions_json = data_folder + "\\" + "versions.json";
			
		}
		~Patch() {
			
			VirtualFree((LPVOID)exe.extra_text_section_start, 0, MEM_RELEASE);
			VirtualFree((LPVOID)exe.extra_rdata_section_start, 0, MEM_RELEASE);
			VirtualFree((LPVOID)exe.extra_strings_section_start, 0, MEM_RELEASE);
			VirtualFree((LPVOID)exe.extra_text_section_2_start, 0, MEM_RELEASE);
		}
		
		bool GetSectionsData();
		bool CreateExtraSections();
		
		bool Patching();
		bool CheckVersion();
		bool CheckGame();

		FILE * fout;
		HMODULE h;
		ULONG_PTR VirtualToRaw(ULONG_PTR virtualAddr);
		ULONG_PTR RawToVirtual(ULONG_PTR rawAddr);

		StringPatcher sp;
		Console * c;
		
		std::vector<Section> sects;

		exe_info exe;

		size_t size_text;
		size_t size_rdata;

		
		ULONG_PTR next_available_addr_text;
		ULONG_PTR next_available_addr_text_2;
		ULONG_PTR next_available_addr_rdata;

		LPSTR filename;
		std::string TLFile_name;
		std::string Ref_name;
		std::string data_folder;
		std::string game_folder;
		std::string versions_json;

		uint32_t timestamp;
		
		std::unordered_map<ULONG_PTR, section> section_map;
		std::unordered_map<section, ULONG_PTR> reverse_section_map;

		std::vector<ULONG_PTR> sections_sorted;
		ks_engine* ks;
};

#endif