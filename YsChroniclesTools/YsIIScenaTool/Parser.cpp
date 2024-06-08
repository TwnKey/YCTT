#include "Parser.h"
#include <fstream>
#include <algorithm>

bool compare_addr(const jump& a, const jump& b)
{
	return a.addr < b.addr;
}
bool compare_id(const jump& a, const jump& b)
{
	return a.id < b.id;
}


//Copied from Darkmet's text decryption
void Parser::decrypt()
{
	uint32_t key = read_u32();
	uint32_t count = read_u32();
	std::vector<uint8_t> out = {};
	unsigned long k = (unsigned long)key;
	for (int i = 8; i < content.size(); i++)
	{
		key = key * 0x3d09;
		uint8_t op = (uint8_t)(key >> 0x10);
		out.push_back((uint8_t)(content[i] - op));
	}
	this->content = out;
}


uint32_t Parser::read_u32() {
	uint32_t out;
	memcpy(&out, content.data() + internal_addr, sizeof(uint32_t));
	internal_addr += sizeof(uint32_t);
	return out;
}
uint16_t Parser::read_u16() {
	uint16_t out;
	memcpy(&out, content.data() + internal_addr, sizeof(uint16_t));
	internal_addr += sizeof(uint16_t);
	return out;
}
std::string Parser::read_str() {
	std::vector<uint8_t> temp = {};

	while (content[internal_addr] != 0) {
		temp.push_back(content[internal_addr]);
		internal_addr++;
	}
	std::string out(temp.begin(), temp.end());
	internal_addr++;
	return out;
}

uint32_t Parser::read_u32_at(uint32_t addr) {
	uint32_t out;
	memcpy(&out, content.data() + addr, sizeof(uint32_t));
	return out;


}
std::string Parser::read_str_at(uint32_t addr) {
	std::vector<uint8_t> temp = {};

	while (content[addr] != 0) {
		temp.push_back(content[addr]);
		addr++;
	}
	std::string out(temp.begin(), temp.end());
	return out;


}
uint16_t Parser::read_u16_at(uint32_t addr) {
	uint16_t out;
	memcpy(&out, content.data() + addr, sizeof(uint16_t));
	return out;
}

void Parser::GetAllPtrsFromSection(int i) {
	uint32_t origaddr = this->internal_addr;
	uint32_t addr_sect_i_start = read_u32_at(0xC + 0x4 * (i)) + 8;
	size_t count_sect_i = read_u16_at(addr_sect_i_start - 6);
	uint32_t addr_sect_i_ptr_end = addr_sect_i_start + 4 * count_sect_i;

	pointer ptr_sect = { 0xC + 0x4 * (i), addr_sect_i_start - 8, 0};
	this->pointeurs.push_back(ptr_sect);

	this->internal_addr = addr_sect_i_start;
	for (unsigned int j = 0; j < count_sect_i; j++) {
		uint32_t loc = this->internal_addr;
		uint32_t dest = read_u32();
		pointer ptr = { loc, dest, addr_sect_i_ptr_end };
		this->pointeurs.push_back(ptr);
	}
	this->internal_addr = origaddr;
}
void Parser::extract_TL() {

	GetAllPtrsFromSection(0);
	GetAllPtrsFromSection(1);
	GetAllPtrsFromSection(2);
	GetAllPtrsFromSection(3);
	GetAllPtrsFromSection(4);
	GetAllPtrsFromSection(5);

	uint32_t addr_sect_2_start = read_u32_at(0xC + 0x4 * 1) + 8;
	size_t count_sect_2 = read_u16_at(addr_sect_2_start - 6);
	uint32_t addr_sect_2_ptr_end = addr_sect_2_start + 4 * count_sect_2;
	uint32_t addr_sect_2_end = read_u32_at(0xC + 0x4 * 2);
	
	uint32_t addr_sect_5_start = read_u32_at(0xC + 0x4 * 5) + 8;
	size_t count_sect_5 = read_u16_at(addr_sect_5_start - 6);
	uint32_t addr_sect_5_ptr_end = addr_sect_5_start + 4 * count_sect_5;
	uint32_t addr_sect_5_end = content.size();

	uint32_t ptr_addr = addr_sect_2_start;

	
	size_t id = 0; //tableau de fonctions à 0x4fad30, lecture de l'op code à 4636FE
	std::vector<uint8_t> op_codes = {};
	//while (ptr_addr < addr_sect_2_ptr_end) {
		this->internal_addr = read_u32_at(ptr_addr) + addr_sect_2_ptr_end;
		//"C:\Users\Administrator\Downloads\googletrad.csv" 
		ptr_addr += 4;
		uint8_t op_code = content[this->internal_addr];
		uint8_t cntt;
		uint16_t wd;
		uint32_t orig_addr;
		std::string text;
		//std::cout << "new function " << std::hex << this->internal_addr << std::endl;
		while (true) { 
			
			this->internal_addr++;
			switch (op_code) {
			case 0x04:
				AddJump();
				break;
			case 0x02:
			case 0x03:
			case 0x05:
			case 0x06:
			case 0x07:
			case 0x08:
			case 0x0A:
			case 0x15:
			case 0x17:
			case 0x18:
			case 0x21:
			case 0x23:
			case 0x3E:
			case 0x62:
			case 0x63:
			case 0x64:
			case 0x67:
			case 0x6C:
			case 0x79:
			case 0x7A:
			case 0x83:
			case 0xA9:
			case 0xD5:
			case 0xD7:
				this->internal_addr += 2;
				break;
			case 0x09:
			case 0x0C:
			case 0x0D:
			case 0x0E:
			case 0x10:
			case 0x14:
			case 0x1A:
			case 0x1E:
			case 0x16:
			case 0x19:
			case 0x1C:
			case 0x37:

			case 0x4F:
			case 0x40:
			case 0x41:
			case 0x42:
			case 0x43:
			case 0x44:
			case 0x45:
			case 0x46:
			case 0x47:
			case 0x48:
			case 0x49:
			case 0x4A:
			case 0x4B:
			case 0x56:
			case 0x5E:
			case 0x5F:
			case 0x6B:
			case 0x73:
			case 0x74:
			case 0xC7:
			case 0xC8:
			case 0xD1:
			case 0x01:
				break;
			case 0x38:
			case 0x39:
			case 0x4C:
			case 0x4D:
			case 0x8F:
			case 0xA3:
			case 0xB0:
			case 0xB6:
			case 0x9E:
			case 0x9F:
			case 0xA0:
			case 0xAC:
			case 0xB1:
			case 0xB5:
			case 0xB8:
			case 0xBD:
			case 0xBE:
			case 0xC0:
			case 0xD2:
			case 0xD3:
			case 0xD6:
				this->internal_addr += 3;
				break;
			case 0xB2:
				this->internal_addr += 13;
				break;
			case 0x3A:
			case 0x3D:
			case 0xB7:
			case 0xC1:
				this->internal_addr += 1;
				break;
			case 0xBB:
				this->internal_addr += 5;
				break;
			case 0x51:

				AddTL();
				
				break;
			case 0x11:
			case 0x13:
			case 0x55:
			case 0x60:
			case 0x61:
			case 0x6F:
			case 0xB4:
			case 0xD0:
				cntt = content[this->internal_addr++];
				for (unsigned int i = 0; i < cntt; i++)
					this->internal_addr += 2;
				break;
			case 0xB3:
				this->internal_addr++;
				cntt = content[this->internal_addr++];
				for (unsigned int i = 0; i < cntt; i++)
					this->internal_addr += 2;
				break;
			case 0x8E:
			case 0x5D:

				cntt = content[this->internal_addr++] - 1;
				this->internal_addr += 2;
				for (unsigned int i = 0; i < cntt; i++)
					this->internal_addr += 2;
				break;
			case 0x0F:
			case 0x12:
			case 0x20:
			case 0x34:
			case 0x3F:
			case 0x57:
			case 0x5B:
			case 0x5C:
			case 0x69:
			case 0x6A:
			case 0x70:
			case 0x71:
			case 0x75:
			case 0x77:
			case 0x78:
			case 0x7B:
			case 0x7C:
			case 0x7E:
			case 0x7F:
			case 0x84:
			case 0x87:
			case 0x88:
			case 0x89:
			case 0x8A:
			case 0x8B:
			case 0x8C:
			case 0x8D:
			case 0x91:
			case 0x92:
			case 0x94:

			case 0xA2:
			case 0xA6:
			case 0xC9:
			case 0xCA:
			case 0xD9:
				this->internal_addr += 4;
				break;
			case 0x98:
			case 0x99:
			case 0xB9:
				this->internal_addr += 5;
				break;
			case 0x22:
			case 0x2B:
			case 0x31:
			case 0x50:
			case 0x35:
			case 0x68:
			case 0x72:
			case 0x90:
			case 0x93:
			case 0x97:
			case 0x9B:
			case 0xA4:
			case 0xDA:
				this->internal_addr += 6;
				break;
			case 0x95:
			case 0xAB:
			case 0xCB:
			case 0xCC:
			case 0xD4:
				this->internal_addr += 8;
				break;
			case 0x25:
			case 0x26:
			case 0x27:
			case 0x53:
			case 0x80:
				this->internal_addr += 10;
				break;
			case 0x81:
			case 0xAA:
				this->internal_addr += 12;
				break;
			case 0x9D:
				this->internal_addr += 14;
				break;

			case 0x58:
			case 0x59:
				this->internal_addr += 16;
				break;
			case 0xCD:
				this->internal_addr += 18;
				break;
			case 0xCE:
				this->internal_addr += 20;
				break;
			case 0xC3:
				this->internal_addr += 28;
				break;
			case 0x7D:
				cntt = (content[this->internal_addr++] - 3 >> 1) + 1;
				this->internal_addr += 4;
				for (unsigned int i = 0; i < cntt; i++)
					this->internal_addr += 4;
				break;
			case 0x9A:
				this->internal_addr += 3;
				text = read_str();
				break;
			case 0xDB:
				AddTL();
				break;
			case 0x52:
				this->internal_addr += 2;
				AddTL();
				this->internal_addr += 2;
				break;
			case 0x76:
				cntt = content[this->internal_addr++];
				for (unsigned int i = 0; i < cntt;) {
					wd = read_u16();
					if (wd == 0) {
						this->internal_addr += 4;
						i += 3;
					}
					else {
						i += 2;
						this->internal_addr += 2;

						if (wd == 3) {
							this->internal_addr += 4;
							i += 2;
						}
						else if (wd == 2) {
							this->internal_addr += 4;
							i += 2;
						}
						else {

						}

					}
				}

				break;
			case 0xC2:
				this->internal_addr += 32;
				break;
			default:
				if (op_code > 220)
				{
				}
				else {
					std::sort(op_codes.begin(), op_codes.end());
					op_codes.erase(std::unique(op_codes.begin(), op_codes.end()), op_codes.end());
					float prog = 100 * (float)op_codes.size() / 220;
					throw(std::exception("unknown op code"));
				}
				{}
			}

			op_codes.push_back(op_code);
			if (this->internal_addr >= addr_sect_2_end) break;
			op_code = content[this->internal_addr];
		}
		//id++;

	//}
	ptr_addr = addr_sect_5_start;
	while (ptr_addr < addr_sect_5_ptr_end) {

		this->internal_addr = read_u32_at(ptr_addr) + addr_sect_5_ptr_end;
		ptr_addr += 4;
		uint32_t raw_bytes = read_u32_at(this->internal_addr);
		size_t bytes_count_str = raw_bytes & 0xFF;
		uint16_t type = (raw_bytes & 0xFFFF0000) >> 16;
		uint32_t offset = 0;
		if ((type & 0x4000) == 0) {
		}
		else {
			offset = offset + 2;
		}
		if ((type & 0x8000) == 0) {

		}
		else {
			offset = offset + 2;
		}
		if ((type & 0x2000) == 0) {
		}
		else {
			offset = offset + 2;
		}
		if ((type & 0x1000) != 0) {
			offset = offset + 4;
		}
		this->internal_addr = this->internal_addr + offset + 4;
		AddTL();
		//TLs.push_back(Translation(this->internal_addr + offset + 4, text, ""));

	}
	

	std::sort(this->pointeurs.begin(), this->pointeurs.end());
	std::sort(this->jumps.begin(), this->jumps.end(), compare_addr);

}


void Parser::AddTL() {

	

	uint32_t orig_addr = this->internal_addr;
	std::string text = read_str();

	TLs.push_back(Translation(orig_addr, text, ""));
	text_addrs.push_back(orig_addr);

}
void Parser::AddJump() {

	uint32_t from_addr = this->internal_addr;
	uint16_t wd = this->read_u16();
	uint32_t jp_id = this->jumps.size();
	this->jumps.push_back({ from_addr, jp_id, false, 2 });
	this->jumps.push_back({ from_addr + wd, jp_id, true, 2 });

}
