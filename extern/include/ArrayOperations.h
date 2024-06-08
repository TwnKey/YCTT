#ifndef ARRAY_OPERATIONS
#define ARRAY_OPERATIONS

std::vector<unsigned char> intToByteArray(int k){
	std::vector<unsigned char> res(4);
	res[0] =  k & 0x000000ff;
	res[1] = (k & 0x0000ff00) >> 8;
	res[2] = (k & 0x00ff0000) >> 16;
	res[3] = (k) >> 24;
	return res;
}
int vectorToInt(std::vector<unsigned char> nb){
	int result = 0;
	int cnt = 0;
	for (std::vector<unsigned char>::const_iterator i = nb.begin(); i != nb.end(); ++i){
		result = result + (((*i) & 0x000000ff) << (cnt*8));
		cnt++;
		}
	return result;
}
void applyChange(int startAddr, std::vector<unsigned char> byteToWrite,  int byteToErase, std::vector<unsigned char> &font_file){

	font_file.erase(font_file.begin() + startAddr, font_file.begin() + startAddr + byteToErase);
	
	font_file.insert(font_file.begin() + startAddr , byteToWrite.begin(), byteToWrite.end());
	
}
std::vector<unsigned char> getsubvector(std::vector<unsigned char> content, int pos_a, int pos_b){
	
	std::vector<unsigned char>::const_iterator first = content.begin() + pos_a;
	std::vector<unsigned char>::const_iterator last = content.begin() + pos_b;
	std::vector<unsigned char> newVec(first, last);
	return newVec;
}


int readInt(std::vector<unsigned char> &buffer, int pos){
	return vectorToInt(getsubvector(buffer, pos, pos+4));
}
int readShort(std::vector<unsigned char> &buffer, int pos){
	return vectorToInt(getsubvector(buffer, pos, pos+2));
}

#endif