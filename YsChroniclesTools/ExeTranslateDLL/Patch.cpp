#include "Patch.h"
#include "Utilities.h"
#include <fstream>
#include <iostream>
#include <winuser.h>
#include <sys/stat.h>
#include <Zydis/Zydis.h>
#include <unordered_set>
#include <algorithm>
#include <time.h>
#include <nlohmann/json.hpp>

#define COMPLETED 0x00444E45

DWORD  AllocationProtectText = PAGE_EXECUTE_READ;
DWORD  AllocationProtectRData = PAGE_READONLY;


bool Patch::GetSectionsData() {

	ULONG_PTR base = (ULONG_PTR) h;
	exe.base = base;

	int starting_point = base + read_int((void*)(base + 0x3C));
	
	int imageBase = read_int((void*) (starting_point + 0x34));

	this->timestamp = read_int((void*)(starting_point + 0x8));

	BYTE nbSections = read_byte((void*)(starting_point + 0x6));
	int image_size = read_int((void*)(starting_point + 0x50));
	exe.sectionAlignment = read_int((void*)(starting_point + 0x38));
	unsigned char sizeBytes[4];


	for (int idx_sec = 0; idx_sec < nbSections; idx_sec++) {
		int depart_header_section = starting_point + 0xF8 + 0x28 * (idx_sec);
		
		int section_raw_pointer = read_int((void*)(depart_header_section + 0x14));
		int raw_size = read_int((void*)(depart_header_section + 0x10));
		int section_virt_pointer = read_int((void*)(depart_header_section + 0xc));

		std::string name = std::string((const char *)depart_header_section);
		
		Section sect;
		sect.name = name;
		sect.PhysicalAddrStart = section_raw_pointer;
		sect.PhysicalAddrEnd = section_raw_pointer + raw_size;
		sect.VirtualAddrStart = base + section_virt_pointer;
		sect.VirtualAddrEnd = ((int)ceil(((float)base + section_virt_pointer + raw_size) / exe.sectionAlignment)) * exe.sectionAlignment;
		sects.push_back(sect);
		
	}

	int text_section = find_section_by_name(".text", sects);
	Section TextSection = sects[text_section];
	exe.start_text_section = TextSection.VirtualAddrStart;
	exe.end_text_section = TextSection.VirtualAddrEnd;

	int rdata_section = find_section_by_name(".rdata", sects);
	Section rDataSection = sects[rdata_section];
	exe.start_rdata_section = rDataSection.VirtualAddrStart;
	exe.end_rdata_section = rDataSection.VirtualAddrEnd;
	
	int data_section = find_section_by_name(".data", sects);
	Section DataSection = sects[data_section];
	exe.start_data_section = DataSection.VirtualAddrStart;
	exe.end_data_section = DataSection.VirtualAddrEnd;

	int rsrc_section = find_section_by_name(".rsrc", sects);
	Section RSRCSection = sects[rsrc_section];
	exe.start_rsrc_section = RSRCSection.VirtualAddrStart;
	exe.end_rsrc_section = RSRCSection.VirtualAddrEnd;
	
	return true;
}

bool Patch::CreateExtraSections() {

	size_rdata = exe.sectionAlignment * ceil((float)0x3001 / exe.sectionAlignment);
	size_text = 2 * exe.sectionAlignment;

	exe.extra_strings_section_start = (ULONG_PTR)VirtualAlloc(nullptr, size_rdata, MEM_COMMIT, PAGE_READONLY);
	exe.extra_rdata_section_start = (ULONG_PTR) VirtualAlloc(nullptr, size_rdata, MEM_COMMIT, PAGE_READONLY);
	exe.extra_text_section_start = (ULONG_PTR)VirtualAlloc(nullptr, size_text, MEM_COMMIT, PAGE_EXECUTE_READ);
	exe.extra_text_section_2_start = (ULONG_PTR)VirtualAlloc(nullptr, size_text, MEM_COMMIT, PAGE_EXECUTE_READ);

	exe.extra_text_section_2_end = exe.extra_text_section_2_start + size_text;
	exe.extra_text_section_end = exe.extra_text_section_start + size_text;
	exe.extra_rdata_section_end = exe.extra_rdata_section_start + size_rdata;
	exe.extra_strings_section_end = exe.extra_strings_section_start + size_rdata;

	next_available_addr_text = exe.extra_text_section_start;
	next_available_addr_text_2 = exe.extra_text_section_2_start;
	next_available_addr_rdata = exe.extra_rdata_section_start;

	section_map[(ULONG_PTR) h] = non_allocated;
	section_map[exe.extra_text_section_start] = extra_text1;
	section_map[exe.extra_text_section_2_start] = extra_text2;
	section_map[exe.extra_rdata_section_start] = extra_rdata1;
	section_map[exe.extra_strings_section_start] = extra_str;

	reverse_section_map[non_allocated] = (ULONG_PTR) h;
	reverse_section_map[extra_text1] = exe.extra_text_section_start;
	reverse_section_map[extra_text2] = exe.extra_text_section_2_start;
	reverse_section_map[extra_rdata1] = exe.extra_rdata_section_start;
	reverse_section_map[extra_str] = exe.extra_strings_section_start;

	sections_sorted.push_back((ULONG_PTR)h);
	sections_sorted.push_back(exe.extra_text_section_start);
	sections_sorted.push_back(exe.extra_text_section_2_start);
	sections_sorted.push_back(exe.extra_rdata_section_start);
	sections_sorted.push_back(exe.extra_strings_section_start);

	std::sort(sections_sorted.begin(), sections_sorted.end());

	
	return true;
}

ULONG_PTR Patch::VirtualToRaw(ULONG_PTR virtualAddr) {
	for (size_t i = 0; i < sects.size() - 1; i++) {

		if ((virtualAddr >= sects[i].VirtualAddrStart) && (virtualAddr < sects[i + 1].VirtualAddrStart)) {
			int offset = virtualAddr - sects[i].VirtualAddrStart;

			return sects[i].PhysicalAddrStart + offset;
		}
	}
	return -1;
}
ULONG_PTR Patch::RawToVirtual(ULONG_PTR rawAddr) {
	for (size_t i = 0; i < sects.size() - 1; i++) {
		if ((rawAddr >= sects[i].PhysicalAddrStart) && (rawAddr < sects[i + 1].PhysicalAddrStart)) {
			int offset = rawAddr - sects[i].PhysicalAddrStart;
			return sects[i].VirtualAddrStart + offset;
		}
	}
	return -1;
}

bool Patch::CheckGame() {

	std::ifstream f(this->versions_json);
	if (f.is_open()) {
		nlohmann::json data = nlohmann::json::parse(f);
		std::string name = data["gamefile"];

		f.close();

		if (name.compare(this->filename) == 0) return true;
		else return false;
	}
	else {
		nlohmann::json data;
		data["exe_timestamp"] = -1;
		data["current_patch_id"] = "0.0.0";
		data["gamefile"] = this->filename;
		std::ofstream outputFile(this->versions_json);

		// Check if the file stream is open
		if (outputFile.is_open()) {
			// Write the JSON object to the file with an indentation of 2 spaces
			outputFile << std::setw(2) << data << std::endl;

			// Close the file stream
			outputFile.close();
		}
	}
}

bool Patch::CheckVersion() {

	std::unordered_set<uint32_t> valid_versions = {};
	bool version_changed = false;

	std::ifstream version_file_in;
	int latest_timestamp = -1;

	std::ifstream f(this->versions_json);
	if (f.is_open()) {
		nlohmann::json data = nlohmann::json::parse(f);
		latest_timestamp = data["exe_timestamp"];
		f.close();
	}

	std::ifstream strver(this->versions_json);
	if (strver.is_open())
	{
		nlohmann::json data = nlohmann::json::parse(strver);
		latest_timestamp = data["exe_timestamp"];
		strver.close();
	}
	else {
		std::string error_msg = "Missing " + this->versions_json;
		MessageBoxA(
			NULL,
			error_msg.c_str(),
			"Error",
			MB_ICONWARNING | MB_ABORTRETRYIGNORE
		);
		exit(EXIT_FAILURE);

	}
	strver.close();

	if (latest_timestamp != this->timestamp) {

		std::ifstream old(this->versions_json);
		//on a changé de version, il faut tout regénérer.
		nlohmann::json data = nlohmann::json::parse(old);

		data["exe_timestamp"] = this->timestamp;
		version_changed = true;
		old.close();

		std::ofstream new_one(this->versions_json);
		// Check if the file stream is open
		if (new_one.is_open()) {
			// Write the JSON object to the file with an indentation of 2 spaces
			new_one << std::setw(2) << data << std::endl;

			// Close the file stream
			new_one.close();
		}
	}
	
	return version_changed;

}

bool Patch::Patching() {

	bool correct_exe = CheckGame();
	if (correct_exe) {
		bool version_changed = CheckVersion();

		if (version_changed) {

			std::remove(this->TLFile_name.c_str());
		}

		sp = StringPatcher(exe, TLFile_name, Ref_name, data_folder);
		bool success_strings = sp.AttemptTranslateExeStr();

		if (!success_strings) {
			c = new Console();

			std::remove(this->TLFile_name.c_str());

			sp.RecreateStringTable(c);

			this->exe = sp.exe;

			MessageBoxW(
				NULL,
				L"Translation updated, please restart the game.",
				L"OK",
				MB_ICONWARNING
			);
			exit(EXIT_SUCCESS);
		}
		delete c;

		return true;
	}
	else return false;
}




