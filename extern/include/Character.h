#ifndef CHARACTER
#define CHARACTER

struct Character {
	int addr_in_file = -1;
	int utf32 = -1; 
	int sjis = -1;

	Character(int addr, int code, int sjis_code){
		addr_in_file = addr;
		utf32 = code;
		sjis = sjis_code;
	}  
bool operator<(const Character& rhs) const
    {
        if (addr_in_file < rhs.addr_in_file)
        {
           return true;
        }
        else return false;
    }
	
} ;
std::ostream& operator<<(std::ostream& os, const Character& chr)
{
    return os << (wchar_t) chr.utf32 << " " << chr.sjis << '\n';
}

#endif