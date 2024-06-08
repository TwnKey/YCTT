#include "StringPatcher.h"
#include <fstream>
#include <boost/tokenizer.hpp>

#include <fstream>
#include <sstream>

std::unordered_map<uint32_t, uint32_t> CPTable;

bool StringPatcher::GetRefData() {
	ReadStringCSV(Ref_name);
	
	return true;
}

void StringPatcher::populateCPTable(const std::string& filename) {
	std::ifstream infile(filename);
	if (!infile) {
		std::cerr << "Unable to open file: " << filename << std::endl;
		return;
	}

	std::string line;
	while (std::getline(infile, line)) {
		std::istringstream iss(line);
		int key, value;
		if (iss >> key >> value) {
			CPTable[key] = value;//key = utf32 value = sjis
		}
	}
	infile.close();
}

bool StringPatcher::CreateTranslations(Console * console) {
	if (exe.disasm.pointers.empty())
		exe.disasm.ParseExe(exe.start_text_section, exe.end_text_section, exe.start_data_section, exe.end_data_section, exe.start_rdata_section, exe.end_rdata_section);

	std::ofstream TranslationFile;
	TranslationFile.open(TLFile_name);

	GetRefData();
	int completed_tl = 0;


	for (Translation TL : TLs) {
		bool success = FindTextEN(TL, 0, TranslationFile, completed_tl);
		completed_tl++;
		float progress = (float)completed_tl / TLs.size();
		console->Update(progress);

	}

	TranslationFile << "#COMPLETED";
	TranslationFile.close();
	return true;
}

bool StringPatcher::GoThroughAllTL() {

	DWORD dwProtect, dwProtect2;
	BYTE* french_location = (BYTE*)exe.extra_strings_section_start;

	if (VirtualProtect((LPVOID)exe.extra_strings_section_start, exe.extra_strings_section_end - exe.extra_strings_section_start, PAGE_EXECUTE_READWRITE, &dwProtect)) {

		for (int idx_TL = 0; idx_TL < TLs.size(); ++idx_TL) {

			if (TLs[idx_TL].addr == -1) {
				return false;
			}
			else {
				if ((TLs[idx_TL].english_str != "POINTER")) { 
					
					if (compare_bytes((void*)TLs[idx_TL].addr, TLs[idx_TL].english)){
						int nb_bytes = TLs[idx_TL].french.size();
						std::copy(TLs[idx_TL].french.begin(), TLs[idx_TL].french.end(), french_location);

						std::vector<BYTE> bytes_pointer = number_to_bytes((ULONG_PTR)french_location, 4);
						bool is_there_a_pointer1 = true;
						bool is_there_a_pointer2 = true;
						int cnt = 1;

						if (!((idx_TL + cnt) >= TLs.size())) {
							bool there_is_a_pointer = (TLs[idx_TL + cnt].english_str == "POINTER");
							if (there_is_a_pointer) {
								do {
									if (TLs[idx_TL].hard.compare("FORCED") != 0)
										RewriteMemoryEx(TLs[idx_TL + cnt].addr, bytes_pointer);
									cnt++;
									if (((idx_TL + cnt) >= TLs.size())) break;
									there_is_a_pointer = (TLs[idx_TL + cnt].english_str == "POINTER");
								} while (there_is_a_pointer);

							}
							else is_there_a_pointer1 = false;
						}
						else is_there_a_pointer1 = false;
						
						if (TLs[idx_TL].hard.compare("FORCED") == 0) {
							//fprintf(stdout, "HARD REPLACE... at addr %x\n", TLs[idx_TL].addr);
							RewriteMemoryEx(TLs[idx_TL].addr, TLs[idx_TL].french);

						}
						idx_TL += (cnt - 1);
						french_location += nb_bytes;
					}
					else {

						fprintf(stdout, "FAIL... at addr %x\n", TLs[idx_TL].addr);

						for (int i = 0; i < TLs[idx_TL].english.size(); i++) {
							fprintf(stdout, "%x ", (int)TLs[idx_TL].english[i]);
						}
						fprintf(stdout, "\n");
						std::cout << "The bytes didnt match! " << std::endl;

					}
				}
			}
		}
		VirtualProtect((LPVOID)exe.extra_strings_section_start, exe.extra_strings_section_end - exe.extra_strings_section_start, dwProtect, &dwProtect2);
	}
	return true;
}

ULONG_PTR StringPatcher::find_raw_text(ULONG_PTR start, ULONG_PTR end, Translation TL, std::ofstream& TranslationFile) {
	ULONG_PTR addr_text = 0;
	ULONG_PTR current_addr = start;
	bool keep_looking = true;
	while (addr_text != -1){
		do {
			addr_text = find_bytes(current_addr, end, TL.english, 1, false);
			if (this->exe.disasm.strings_location.count(TL.english) > 0)
			{

				if (this->exe.disasm.strings_location[TL.english].count(addr_text) > 0) {
					//We found a reference, so it will be dealt in the above branch, not in this one
					current_addr = addr_text + 1;
					keep_looking = true;
				}

				else {
					current_addr = addr_text + 1;
					keep_looking = false;
				}
			}
			else {
				if (addr_text != -1) {

					uint8_t previous_byte = *(uint8_t*)(addr_text - 1);
					if (previous_byte == 0) {
						keep_looking = false;
					}
					current_addr = addr_text + 1;
				}
				else
					keep_looking = false;
			}


		} while (keep_looking);

		bool success = (addr_text != -1);
		if (success) { //On a trouvé le texte

			replace_all_substr_occurrence(TL.english_str, "\\", "\\\\"); //telling the parser that a \ is not an escape character 
			replace_all_substr_occurrence(TL.french_str, "\\", "\\\\");
			replace_all_substr_occurrence(TL.english_str, "\"", "\\\""); //telling the parser to ignore \"
			replace_all_substr_occurrence(TL.french_str, "\"", "\\\"");

			TranslationFile << "\"" << TL.english_str << "\",\"" << TL.french_str << "\"," << addr_text - exe.base << "," << "\"" << TL.hard << "\"" << "," << "\"" << TL.encoding << "\"" << std::endl;

			//We know where the text is, but it should not be used directly anywhere, so we look through the entirety of the data section to see if we can find a reference somewhere
			std::vector<BYTE> byteArrayAddr_data = number_to_bytes((int)addr_text, 4);
			ULONG_PTR addr_used = find_bytes(exe.start_data_section, exe.end_data_section, byteArrayAddr_data, 1, false);
			bool success_data = (addr_used != -1);
			if (success_data) {
				TranslationFile << "\"POINTER\"," << "\"\"," << addr_used - exe.base << ",\"\"" << ",\"\"" << std::endl;
			}
			addr_used = find_bytes(exe.start_rdata_section, exe.end_rdata_section, byteArrayAddr_data, 1, false);
			bool success_rdata = (addr_used != -1);
			if (success_rdata) {
				TranslationFile << "\"POINTER\"," << "\"\"," << addr_used - exe.base << ",\"\"" << ",\"\"" << std::endl;
			}
		}
	}
	return addr_text;


}


bool StringPatcher::AttemptTranslateExeStr() {

	bool read_csv = ReadStringCSV(TLFile_name);
	if (read_csv) {

		bool success = GoThroughAllTL();
		if (!success) {
			
			return false;
		}
		else 
			return true;
	}
	else 
		return false;
	
}

bool StringPatcher::RecreateStringTable(Console* console) {
	fprintf(stdout, "Text not found. Recreating the string table...\n");

	CreateTranslations(console);
	ReadStringCSV(TLFile_name);
	GoThroughAllTL();

	return true;
}

bool StringPatcher::FindTextEN(Translation& TL, int offset, std::ofstream& TranslationFile, int cnt) {
	bool success = find_data_from_addr(TL, offset, TranslationFile);
	
	return true;
}


bool StringPatcher::find_data_from_addr(Translation& TL, int offset, std::ofstream& TranslationFile)
{

	bool success = this->exe.disasm.strings_location.count(TL.english) > 0;
	if (TL.hard.compare("FORCED") == 0) success = false;

	if (success) {
		for (auto loc : this->exe.disasm.strings_location[TL.english]) {
			

			replace_all_substr_occurrence(TL.english_str, "\\", "\\\\"); //telling the parser that a \ is not an escape character 
			replace_all_substr_occurrence(TL.french_str, "\\", "\\\\");
			replace_all_substr_occurrence(TL.english_str, "\"", "\\\""); //telling the parser to ignore \"
			replace_all_substr_occurrence(TL.french_str, "\"", "\\\"");

			TranslationFile << "\"" << TL.english_str << "\",\"" << TL.french_str << "\"," << loc - exe.base << "," << "\"" << TL.hard << "\"" << "," << "\"" << TL.encoding << "\"" << std::endl;

			if (exe.disasm.pointers.count(loc) > 0)
			{
				for (ref r : exe.disasm.pointers[loc]) {
					//Soit c'est un string, soit un pointeur vers le string
					/*for (auto b : TL.english) {
						fprintf(stdout, "%x ", b);
					}
					fprintf(stdout, "\n %x\n", loc);*/
					if (check_at_index_byte((const uint8_t*)loc, TL.english.size(), TL.english)) {

						TranslationFile << "\"POINTER\"," << "\"\"," << r.ref_loc - exe.base << ",\"\"" << ",\"\"" << std::endl;
					}

					else {
						if (r.ref_loc < exe.end_rsrc_section - 4) {
							uint32_t addr_2 = *(uint32_t*)r.ref_loc;
							if ((addr_2 >= exe.start_rdata_section) && (addr_2 <= exe.end_rdata_section - 4)) {
								if (check_at_index_byte((const uint8_t*)addr_2, TL.english.size(), TL.english))
									TranslationFile << "\"POINTER\"," << "\"\"," << addr_2 - exe.base << ",\"\"" << ",\"\"" << std::endl;

							}
						}
					}

				}
			}
		}
	}
	else {
		//The string might exist, but is not accessed directly (could be in an array of strings) we will search for it directly
		//Also it could exist as an operand in the text section, but could also identically exist somewhere else where it's inacessible,
		//Yeah, I didn't think it was likely to ever happen, but yeahhhhhhhhhhhh

		ULONG_PTR addr_text = -1;

		bool keep_looking = true;
		/*OK the reasoning here, which is flawed, is: if a string was not found after the disassembling then it is not used directly,
		but rather accessed with an offset, so it's likely in an array of strings of fixed length. It is also not the first
		element of the array that would have been caught by disassembling, it's one of the remaining ones, therefore there is a string
		before it, and that string "should" end with \x0 (again it could not since the length might be hardcoded, and it would suck.)*/
		
		addr_text = find_raw_text(exe.start_rdata_section, exe.end_rdata_section, TL, TranslationFile);
		
		addr_text = find_raw_text(exe.start_data_section, exe.end_data_section, TL, TranslationFile);
			
		addr_text = find_raw_text(exe.start_rsrc_section, exe.end_rsrc_section, TL, TranslationFile);
			

		
	}
}



bool StringPatcher::ReadStringCSV(std::string file_name) {
	using namespace std;
	using namespace boost;


	typedef tokenizer< escaped_list_separator<char> > Tokenizer;
	TLs.clear();
	std::ifstream file(file_name);
	std::string str, line;
	if (!file.is_open()) {
		return false;
	}
	int row_cnt = 0;
	bool inside_quotes(false);
	size_t last_quote(0);

	if (file.peek() == EOF) 
			return false;
	while (std::getline(file, str)) {

		if (file.peek() == EOF) {
			if (str.compare("#COMPLETED") == 0)
				return true;
			else
				return false;


		}

		row_cnt++;
		
		// --- deal with line breaks in quoted strings

		last_quote = str.find_first_of('"');
		while (last_quote != std::string::npos)
		{
			if (last_quote > 0) {
				if (str.at(last_quote - 1) != '\\')

				{
					inside_quotes = !inside_quotes;
				}
			}
			else inside_quotes = !inside_quotes;

			last_quote = str.find_first_of('"', last_quote + 1);
		}


		line.append(str);

		if (inside_quotes)
		{
			line.append("\n");
			continue;
		}

		escaped_list_separator<char> sep("\\", ",", "\"");

		Tokenizer tok(line, sep);


		int start = 0;
		auto it = tok.begin();

		std::string en = (*it++);
		std::string fr = (*it++);
		std::string addr_str = (*it++);
		std::string hard = (*it++);
		std::string encoding = (*it++);


		int addr_without_offset = std::atoi(addr_str.c_str());
		int base = exe.base;
		int full_addr = addr_without_offset + base;
		
		ULONG_PTR addr = (ULONG_PTR)full_addr;
		Translation TL(en, fr, addr, encoding, hard);
		TLs.push_back(TL);

		line.clear();

		
	}
}