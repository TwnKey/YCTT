#include <iostream>
#include <vector>
#include <fstream>
#include <algorithm>
#include <set>
#include <filesystem>
#include "ImageConverter.h"
namespace fs = std::filesystem;  // Alias for the filesystem namespace

int main(int argc, char* argv[])
{
    if (argc == 3) {
        std::string firstArg = argv[1];
        std::string secondArg = argv[2];

        // Check if the first argument is a directory
        if (fs::is_directory(firstArg) && secondArg.substr(secondArg.find_last_of(".") + 1) == "PAL") {
            // Handle image conversion for all .SKI and .256 files in the directory
            std::string directoryPath = firstArg;
            std::string palFilePath = secondArg;

            // Iterate over all files in the directory
            for (const auto& entry : fs::directory_iterator(directoryPath)) {
                std::string filePath = entry.path().string();
                std::string extension = entry.path().extension().string();

                if (extension == ".SKI" || extension == ".256") {
                    //std::cout << "Processing: " << filePath << std::endl;
                    ImageConverter converter(palFilePath, "");  // Initialize with the PAL file for palette data
                    // First, read the palette
                    if (!converter.readPalette()) {
                        std::cerr << "Error: Failed to read the PAL file " << palFilePath << std::endl;
                        return 1;
                    }

                    // Now parse the SKI or 256 file based on the extension
                    if (extension == ".SKI") {
                        if (!converter.parseSKI(filePath)) {
                            std::cerr << "Error: Failed to parse SKI file " << filePath << std::endl;
                            return 1;
                        }
                    }
                    else if (extension == ".256") {
                        if (!converter.convert(filePath)) {
                            std::cerr << "Error: Failed to convert 256 file " << filePath << std::endl;
                            return 1;
                        }
                    }

                    // Create the PNG based on the output and the palette
                    if (converter.getPalettes().empty()) {
                        std::cerr << "Error: No palettes loaded." << std::endl;
                        return 1;
                    }

                    //std::cout << "Processed file: " << filePath << std::endl;
                }
            }
        }
        // Handle single image conversion for one .256 or .SKI file and one .PAL file
        else if ((firstArg.substr(firstArg.find_last_of(".")) == ".256" || firstArg.substr(firstArg.find_last_of(".")) == ".SKI") &&
            secondArg.substr(secondArg.find_last_of(".") + 1) == "PAL") {
            std::string filePath = firstArg;
            std::string palFilePath = secondArg;

            ImageConverter converter(palFilePath, filePath);
            if (firstArg.substr(firstArg.find_last_of(".")) == ".SKI") {
                // Handle SKI file
                if (!converter.readPalette()) {
                    std::cerr << "Error: Failed to read the PAL file " << palFilePath << std::endl;
                    return 1;
                }
                if (!converter.parseSKI(filePath)) {
                    std::cerr << "Error: Failed to parse SKI file " << filePath << std::endl;
                    return 1;
                }
            }
            else if (firstArg.substr(firstArg.find_last_of(".")) == ".256") {
                // Handle 256 file
                if (!converter.convert(filePath)) {
                    return 1;
                }
            }

            std::cout << "PNG file created successfully for: " << filePath << std::endl;
        }
        else {
            std::cerr << "Error: Invalid arguments provided." << std::endl;
            return 1;
        }
    }
    else {
        std::cerr << "Error: Expected 2 arguments (file path and palette file)." << std::endl;
        return 1;
    }
}