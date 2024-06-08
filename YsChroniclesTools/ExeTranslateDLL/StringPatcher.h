#pragma once
#include "Utilities.h"
#include "Disassembler.h"
#include "Console.h"
class StringPatcher
{

	public:
		exe_info exe;
		std::string TLFile_name;
		std::string Ref_name;
		std::vector<Translation> TLs;
		std::string data_folder;


		StringPatcher() = default;
		StringPatcher(exe_info exe, std::string TL_filename, std::string Ref_name, std::string data_folder) : exe(exe), TLFile_name(TL_filename), Ref_name(Ref_name), data_folder(data_folder) {
			populateCPTable(data_folder + "/text.ini");
		};
		void populateCPTable(const std::string& filename);
		bool GetRefData();
		bool ReadStringCSV(std::string file_name);
		bool FindTextEN(Translation& TL, int offset, std::ofstream& TranslationFile, int cnt);
		bool CreateTranslations(Console* console);
		bool find_data_from_addr(Translation& TL, int offset, std::ofstream& TranslationFile);
		
		bool GoThroughAllTL();
		bool AttemptTranslateExeStr();
		bool RecreateStringTable(Console* console);
		ULONG_PTR find_raw_text(ULONG_PTR start, ULONG_PTR end, Translation TL, std::ofstream& TranslationFile);
};

