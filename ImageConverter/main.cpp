#include <iostream>
#include <string>
#include <filesystem>
#include <algorithm>
#include "ImageConverter.h"

namespace fs = std::filesystem;

int main(int argc, char* argv[]) {
    // Check for the correct number of command-line arguments
    if (argc < 4 || argc > 6) {
        std::cerr << "Usage: " << argv[0] << " <palette_file> <filename_or_folder> [-e|-i256|-iski] [-allp] [-p<palette_number>]" << std::endl;
        std::cerr << "Options:\n";
        std::cerr << "  -e               Extract pictures\n";
        std::cerr << "  -i256            Insert pictures into 256 format\n";
        std::cerr << "  -iski            Insert pictures into SKI format\n";
        std::cerr << "  -allp            Use all palettes for extraction (overrides -p)\n";
        std::cerr << "  -p<palette_num>  Specify a palette number to use (ignored if -allp is provided)" << std::endl;
        return 1;
    }

    bool useAllPalettes = false;  // Flag to indicate whether to use all palettes
    bool isExtracting = false;    // Flag to indicate extracting (-e)
    bool isInserting = false;     // Flag to indicate inserting
    std::string insertFormat = ""; // Format for insertion ("256" or "ski")
    int selectedPalette = -1;     // Default palette value (-1 means no palette specified)

    std::string inputPath = argv[2];  // The second argument is the input path (file or folder)
    std::string paletteFileName = argv[1]; // The first argument is the palette file

    // Parse command-line arguments
    for (int i = 3; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "-e") {
            isExtracting = true;  // Set flag to extract
        }
        else if (arg == "-i256") {
            isInserting = true;
            insertFormat = "256";  // Set to insert into 256 format
        }
        else if (arg == "-iski") {
            isInserting = true;
            insertFormat = "ski";  // Set to insert into SKI format
        }
        else if (arg == "-allp") {
            useAllPalettes = true; // Set flag to use all palettes (for extraction)
            isExtracting = true;   // If -allp is provided, it's for extraction
        }
        else if (arg.substr(0, 2) == "-p") {
            try {
                selectedPalette = std::stoi(arg.substr(2)); // Extract the palette number from -pXXX
            }
            catch (std::invalid_argument&) {
                std::cerr << "Invalid palette number: " << arg.substr(2) << std::endl;
                return 1;
            }
        }
        else {
            std::cerr << "Unknown option: " << arg << std::endl;
            return 1;
        }
    }

    // Ensure the user selects either extraction or insertion
    if (!isExtracting && !isInserting) {
        std::cerr << "You must specify either -e for extraction or -i256/-iski for insertion." << std::endl;
        return 1;
    }

    // Ensure the user specifies the insertion format if inserting
    if (isInserting && insertFormat.empty()) {
        std::cerr << "You must specify the insertion format using -i256 or -iski." << std::endl;
        return 1;
    }

    ImageConverter converter(paletteFileName); // Initialize the converter with the palette file

    // If '-allp' is specified, extract using all palettes.
    if (useAllPalettes) {
        converter.selectPalette(-1, true); // Select all palettes for extraction
    }
    // If '-pXXX' is specified, use that specific palette number
    else if (selectedPalette != -1) {
        converter.selectPalette(selectedPalette, false); // Select the specified palette
    }
    else {
        // Default palette selection if neither '-allp' nor '-pXXX' is provided
        converter.selectPalette(-1, false); // Select default palette
    }

    // Function to process individual files
    auto processFile = [&](const std::string& fileName) {
        fs::path filePath(fileName);
        std::string extension = filePath.extension().string();
        std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower); // Convert to lowercase
        converter.currentFileName = filePath.stem().string();

        // For extraction, handle .ski and .256 files
        if (isExtracting) {
            if (extension == ".ski") {
                std::cout << converter.currentFileName << std::endl;
                if (converter.parseSKI(fileName)) {
                    std::cout << "Extracted SKI from " << fileName << "." << std::endl;
                }
                else {
                    std::cerr << "Failed to extract SKI from " << fileName << "." << std::endl;
                }
            }
            else if (extension == ".256") {
                std::cout << converter.currentFileName << std::endl;
                if (converter.convert(fileName)) {
                    std::cout << "Converted 256 file from " << fileName << "." << std::endl;
                }
                else {
                    std::cerr << "Failed to convert 256 file from " << fileName << "." << std::endl;
                }
            }
            else {
                std::cerr << "Unsupported file type: " << fileName << ". Only .ski and .256 files are supported for extraction." << std::endl;
            }
            converter.dumpAllBMP();
        }

        // For insertion, only handle .bmp files
        else if (isInserting) {
            if (extension == ".bmp") {
                std::cout << "Inserting " << fileName << " as " << insertFormat << " format." << std::endl;
                if (insertFormat == "256") {
                    // Implement BMP to 256 conversion/insertion logic here
                    std::cout << "Inserting BMP into 256 format (not yet implemented)." << std::endl;
                    // You would add your insertion code here
                }
                else if (insertFormat == "ski") {
                    // Implement BMP to SKI conversion/insertion logic here
                    std::cout << "Inserting BMP into SKI format (not yet implemented)." << std::endl;
                    // You would add your insertion code here
                }
            }
            else {
                std::cerr << "Unsupported file type for insertion: " << fileName << ". Only .bmp files are supported for insertion." << std::endl;
            }
        }
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
