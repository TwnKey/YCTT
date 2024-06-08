#pragma once
#include <string>
#include <vector>
#include "Translation.h"
class Writer
{
	public:
		void encrypt();
		std::vector<uint8_t> content = {};
		std::vector<Translation> TLs = {};

		Writer(std::string csv_path) {
			ReadCSV(csv_path);
	
		}
		void ReadCSV(std::string path);
		void ReplaceIntAtIndex(uint32_t addr, uint32_t i);
		void ReplaceShortAtIndex(uint32_t addr, uint16_t s);
		void InsertTL(std::string path_original_file);
		void WriteBinaryFile();
};

