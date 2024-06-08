#pragma once
#include <string>
#include <vector>
class Translation
{
public:
	uint32_t addr;
	std::string original;
	std::string translation;
	Translation(uint32_t addr, std::string orig, std::string tl) : addr(addr), original(orig), translation(tl) {}
};



void WriteTLstoFile(std::vector<Translation> tls);