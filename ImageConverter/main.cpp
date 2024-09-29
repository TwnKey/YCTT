#include <iostream>
#include <string>
#include <filesystem>
#include "ImageConverter.h"

namespace fs = std::filesystem;

int main(int argc, char* argv[]) {
    // Check for the correct number of command line arguments
    if (argc < 3 || argc > 4) {
        std::cerr << "Usage: " << argv[0] << " <palette_file> <filename_or_folder> [-allp]" << std::endl;
        std::cerr << "If the optional '-allp' flag is provided, all palettes will be used for extraction or conversion." << std::endl;
        return 1;
    }

    bool useAllPalettes = false; // Flag to indicate whether to use all palettes
    std::string paletteFileName = argv[2]; // The first argument is the palette file
    std::string inputPath = argv[1]; // The second argument is the input path (file or folder)

    // Check if there's a third argument and if it matches the flag
    if (argc == 4 && std::string(argv[3]) == "-allp") {
        useAllPalettes = true; // Set flag to use all palettes
    }

    ImageConverter converter(paletteFileName); // Initialize the converter with the palette file

    if (useAllPalettes) {
        converter.selectPalette(-1, true);
    }
    else {
    
    
    }
    // Function to process individual files
    auto processFile = [&](const std::string& fileName) {
        fs::path filePath(fileName);
        std::string extension = filePath.extension().string();
        std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower); // Convert to lowercase
        converter.currentFileName = filePath.stem().string();
        
        // Determine file type based on extension
        if (extension == ".ski") {
            std::cout << converter.currentFileName << std::endl;
           if (converter.parseSKI(fileName)) {
           }
           else {
               std::cerr << "Failed to extract SKI from " << fileName << "." << std::endl;
           }

            
        }
        else if (extension == ".256") {
            std::cout << converter.currentFileName << std::endl;

            if (converter.convert(fileName)) {
            }
            else {
                std::cerr << "Failed to convert 256 file from " << fileName << "." << std::endl;
            }
        }
        else {
            std::cerr << "Unsupported file type: " << fileName << ". Only .ski and .256 files are supported." << std::endl;
        }

        if (!useAllPalettes) {
            converter.selectPalette(-1, false); // it will select a default palette
        }

        converter.dumpAllBMP();


    };

    // Check if the input path is a directory or a file
    fs::path inputPathObj(inputPath);
    if (fs::exists(inputPathObj)) {
        if (fs::is_directory(inputPathObj)) {
            for (const auto& entry : fs::directory_iterator(inputPathObj)) {
                if (entry.is_regular_file()) {
                    processFile(entry.path().string());
                }
            }
        }
        else if (fs::is_regular_file(inputPathObj)) {
            processFile(inputPath);
        }
        else {
            std::cerr << "Invalid input path: " << inputPath << std::endl;
            return 1;
        }
    }
    else {
        std::cerr << "Input path does not exist: " << inputPath << std::endl;
        return 1;
    }

    return 0;
}
