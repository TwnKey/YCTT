#ifndef IMAGE_CONVERTER_H
#define IMAGE_CONVERTER_H

#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <utility>
#include <cstdint>
struct frameData {
    int width = 0;
    int height = 0;
    std::vector<uint8_t> data;

};
struct Color {
    uint8_t r, g, b, a; // RGB color structure
};

class ImageConverter {
public:
    ImageConverter(const std::string& paletteFileName = "", const std::string& bitmapFileName = "");

    bool parseSKI(const std::string& skiFileName); // Parse SKI files and extract using one palette
    bool loadPalettesFromPALfile();
    void createBMP(const frameData& frameData, std::string& outputFileName, std::vector<Color> palette);
    void dumpAllBMP();
    bool readPalette(); // Method to read the palette
    bool convert(const std::string& FileName); // Convert a .256 file using one palette

    const std::vector<std::vector<Color>>& getPalettes() const { return selectedPalettes; } // Getter for palettes
    int selectPalette(int index, bool allP);

    std::string currentFileName;

private:
    std::vector<std::vector<Color>> selectedPalettes;
    std::vector<unsigned int> palettes_id;
    std::vector<uint8_t> indices;
    std::vector<uint8_t> output;
    std::string palFileName;
    std::string bmpFileName;
    std::vector<std::vector<Color>> PALfile;
    std::vector <frameData> framesData;
    std::string stripExtension(const std::string& filename);
    std::string getFileNameWithoutPath(const std::string& filepath);
    
};


#endif // IMAGE_CONVERTER_H
