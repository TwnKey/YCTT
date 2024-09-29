#ifndef IMAGE_CONVERTER_H
#define IMAGE_CONVERTER_H

#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <utility>
#include <cstdint>

struct Color {
    uint8_t r, g, b; // RGB color structure
};

class ImageConverter {
public:
    ImageConverter(const std::string& paletteFileName = "", const std::string& bitmapFileName = "");
    bool parseSKI(const std::string& skiFileName);
    void createBMP(const std::vector<uint8_t>& frameData, const std::string& outputFileName, int width, int height);
    bool readPalette(); // Method to read the palette
    bool convert(const std::string& FileName); // Method for conversion logic

    const std::vector<std::vector<Color>>& getPalettes() const { return palettes; } // Getter for palettes
    int selectPalette(int maxIndex); // Method to select the appropriate palette

private:
    std::vector<std::vector<Color>> palettes; // Store multiple palettes
    std::vector<uint8_t> indices;
    std::vector<uint8_t> output;
    std::string palFileName;
    std::string bmpFileName;
    int width = 0;
    int height = 0;

    std::string stripExtension(const std::string& filename);
    std::string getFileNameWithoutPath(const std::string& filepath);
};

#endif // IMAGE_CONVERTER_H
