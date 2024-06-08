#pragma once

#include <vector>
#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include "Translation.h"

struct jump {

	uint32_t addr = -1;
	uint32_t id = -1;
	bool dest = false;
	size_t encoded_size = 1;
	
};

bool compare_addr(const jump& a, const jump& b);
bool compare_id(const jump& a, const jump& b);

struct pointer {

	uint32_t loc = -1;
	uint32_t dest = -1;
	uint32_t offset = 0;
	bool operator<(const pointer& a) const
	{
		return (offset + dest) < (a.offset + a.dest);
	}
};

class Parser
{

public:

	unsigned int internal_addr = 0;
	std::vector<uint8_t> content = {};
	std::vector<uint32_t> text_addrs = {};
	std::vector<Translation> TLs = {};
	std::vector<pointer> pointeurs = {};
	std::vector<jump> jumps = {};

	Parser(std::string path)
	{
		std::cout << "opening " << path << std::endl;
		std::ifstream file(path, std::ios::binary);
		std::vector<uint8_t> f = std::vector<uint8_t>((std::istreambuf_iterator<char>(file)),
			std::istreambuf_iterator<char>());
		this->content = f;
	
	};

	void decrypt();
	uint32_t read_u32();
	uint16_t read_u16();
	std::string read_str();
	uint32_t read_u32_at(uint32_t addr);
	std::string read_str_at(uint32_t addr);
	uint16_t read_u16_at(uint32_t addr);
	void extract_TL();
	void GetAllPtrsFromSection(int i);
	void AddTL();
	void AddJump();

};