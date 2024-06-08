// YsIIScenaTool.cpp : Ce fichier contient la fonction 'main'. L'exécution du programme commence et se termine à cet endroit.
//

#include <iostream>
#include <vector>
#include <fstream>
#include <algorithm>
#include "Parser.h"
#include "Writer.h"
int main(int argc, char* argv[])
{
	if (argc == 2) {
		std::string path = argv[1];
		
		Parser p(path);
		p.decrypt();
		p.extract_TL();
		WriteTLstoFile(p.TLs);
	}
	else if (argc == 3) {
		std::string csv_path = argv[1];
		std::string original_file = argv[2];
		Writer w(csv_path);
		w.InsertTL(original_file);
		w.encrypt();
		w.WriteBinaryFile();
	
	}
}
