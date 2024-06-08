#include "Disassembler.h"
#include <unordered_set>
#include <algorithm>
std::unordered_set<uint8_t> op_codes = { 0x68 , 0xB8 , 0xB9 , 0xBA , 0xBB,  0xBE, 0xBF, 0xC7, 0x8B, 0x8D };


bool Disassembler::ParseExe(ULONG_PTR start_text, ULONG_PTR end_text, ULONG_PTR start_data, ULONG_PTR end_data, ULONG_PTR start_rdata, ULONG_PTR end_rdata) {
  
	
	ZydisDecoder decoder;
	ZydisDecoderInit(&decoder, ZYDIS_MACHINE_MODE_LEGACY_32, ZYDIS_STACK_WIDTH_32);

	ZyanU32 runtime_address = (ZyanU32)start_text;
	ZyanUSize offset = 0;
	ZydisDecodedInstruction instruction;
	size_t length = end_text - start_text;
	ZydisDecodedOperand operands[ZYDIS_MAX_OPERAND_COUNT_VISIBLE];

	std::unordered_set<ULONG_PTR> pointers_;
	unsigned int instr_nb = 0;
	while (runtime_address < (ZyanU32)end_text)
	{
		ZyanStatus zs = ZYAN_SUCCESS(ZydisDecoderDecodeFull(&decoder, (const uint8_t*)(runtime_address), 0xFF,
			&instruction, operands, ZYDIS_MAX_OPERAND_COUNT_VISIBLE,
			ZYDIS_DFLAG_VISIBLE_OPERANDS_ONLY));
		
		if (zs == 1) {
			
			addrs.push_back(runtime_address);
			if (instruction.opcode == 0xE8) //a call
			{
				int fun_ptr = *((uint32_t*)(runtime_address + 1));
				functions.push_back(fun_ptr + runtime_address + 5);
			
			}
			else if (op_codes.count(instruction.opcode) > 0) //pointer to data
			{
				uint32_t ptr = -1, location = -1;
				if (instruction.opcode == 0xC7) {
					uint8_t variant_value = *((uint8_t*)(runtime_address + 1));
					if (variant_value == 0x05) { //Faire la C7 plus intelligemment un jour où j'ai pas la flemme, sans dec
						ptr = *((uint32_t*)(runtime_address + 2 + 4));
						location = runtime_address + 2 + 4;
					}
					else if (variant_value == 0x44) {
						ptr = *((uint32_t*)(runtime_address + 2 + 2));
						location = runtime_address + 2 + 2;
					}
					else {
						ptr = *((uint32_t*)(runtime_address + 2));
						location = runtime_address + 2;
					}
				}
				else if ((instruction.opcode == 0x8B)|| (instruction.opcode == 0x8D)) {
					ptr = *((uint32_t*)(runtime_address + instruction.length - 4));
					location = runtime_address + instruction.length - 4;
				}
				else {
					ptr = *((uint32_t*)(runtime_address + 1));
					location = runtime_address + 1;
				}
				
				if (ptr != -1) {

					if (((ptr > start_rdata) && (ptr < end_rdata)) || ((ptr > start_data) && (ptr < end_data))){
						ULONG_PTR end_sect = end_rdata;
						if ((ptr > start_data) && (ptr < end_data))
							end_sect = end_data;

						ULONG_PTR addr_ = ptr;
						uint8_t byte = *(uint8_t*)addr_;
						std::vector<uint8_t> bytes = {};
						//OK, raisonnement de merde ici mais je vois pas comment faire autrement :
						//1 - on va à priori traduire que de l'anglais, donc de l'UTF16 avec un second byte à 0
						//Je peux pas faire la diff entre UTF16 et UTF8 (à moins de savoir la fonction qui utilise le pointeur,
						//mais flemme; donc je propose de stocker comme UTF16 toute chaine de caractère de 1 seul caractère 
						// de toute façon un mot anglais de une lettre, bon.
						if (byte != 0)
							bytes.push_back(byte);
						addr_ = addr_ + 1;
						byte = *(uint8_t*)addr_;
						if (byte == 0) {
						 //=> de l'utf 16
							bytes.push_back(byte);
							addr_ = addr_ + 1;
							byte = *(uint8_t*)addr_;
							while (byte != 0){
								bytes.push_back(byte);
								addr_ = addr_ + 1;
								byte = *(uint8_t*)addr_;
								bytes.push_back(byte);
								addr_ = addr_ + 1;
								if (addr_ >= end_sect)
									break;
								byte = *(uint8_t*)addr_;

							} 
							bytes.push_back(0);
						}
						else {
							do {

								bytes.push_back(byte);
								addr_ = addr_ + 1;
								if (addr_ >= end_sect)
									break;
								byte = *(uint8_t*)addr_;

							} while (byte != 0);

						}
						bytes.push_back(0);
						this->pointers[ptr].push_back({location, addrs.size()-1});
						pointers_.emplace((ULONG_PTR) ptr);
						strings_location[bytes].emplace(ptr);
						
					}
				}
				
			}

			runtime_address += instruction.length;
			
		}
		else {

			runtime_address++;

		}
		instr_nb++;
	}
	std::sort(functions.begin(), functions.end()); 

	auto last = std::unique(functions.begin(), functions.end());
	functions.erase(last, functions.end());

	for (auto ptr : pointers_) {
		//Also here, often pointers of pointers are located inside arrays of pointers, so we might need to look at the following
		//pointers as well, and stop if they're outside rdata
		ULONG_PTR addr_ = *(uint32_t*)ptr;
		while ((addr_ >= start_rdata) && (addr_ <= end_rdata - 4)){
			
			uint8_t byte = *(uint8_t*)addr_;
			std::vector<uint8_t> bytes = {};
			while (byte != 0) {

				bytes.push_back(byte);
				addr_ = addr_ + 1;
				if (addr_ >= end_rdata)
					break;
				byte = *(uint8_t*)addr_;
			}
			addr_ = *(uint32_t*)ptr;
			bytes.push_back(0);
			strings_location[bytes].emplace(addr_);
			this->pointers[addr_].push_back({ptr, 0xFFFFFFFF});
			ptr = ptr + 4;
			addr_ = *(uint32_t*)ptr;
		}
			
	
	}

	return true;
}