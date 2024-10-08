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
    std::vector<uint8_t> special_pixels;

};
struct Color {
    uint8_t r, g, b, a; // RGB color structure
};

class ImageConverter {
public:
    ImageConverter(const std::string& paletteFileName = "", const std::string& bitmapFileName = "");
    bool parseSKI(const std::string& skiFileName); // Parse SKI files and extract using one palette
    void repackIntoSKI(const std::string& outputSKIPath);
    bool loadPalettesFromPALfile();
    void createBMP(const frameData& frameData, std::string& outputFileName, std::vector<Color> palette, bool skifile);
    void dumpAllBMP(bool skifile);
    bool readPalette(); // Method to read the palette
    bool convert(const std::string& FileName); // Convert a .256 file using one palette
    void insertBMPPairIntoSKI(const std::string& normalBMPPath, const std::string& specialBMPPath);
    void BMPtoSKI(const std::string& folderPath);
    void BMPto256(std::string BMPfile);
    const std::vector<std::vector<Color>>& getPalettes() const { return selectedPalettes; } // Getter for palettes
    int selectPalette(int index, bool allP);
    void concatenatePalettes(const std::vector<int>& paletteIDs);
    std::string currentFileName;
    std::vector <frameData> framesData;
private:
    std::vector<std::vector<Color>> selectedPalettes;
    std::vector<unsigned int> palettes_id;
    std::vector<uint8_t> indices;
    std::vector<uint8_t> output;
    std::string palFileName;
    std::string bmpFileName;
    std::vector<std::vector<Color>> PALfile;
    
    std::string stripExtension(const std::string& filename);
    std::string getFileNameWithoutPath(const std::string& filepath);
    
};


#endif // IMAGE_CONVERTER_H
