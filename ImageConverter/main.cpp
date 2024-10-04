#include <iostream>
#include <string>
#include <filesystem>
#include <regex>
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

    // Function to process BMP files for insertion
    auto processInsertFile = [&](const std::string& fileName, const std::string& baseName) {
        fs::path filePath(fileName);
        std::string fileNameStr = filePath.filename().string(); // Get only the filename
        std::string extension = filePath.extension().string();
        std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower); // Convert to lowercase

        // Regular expression to extract frame and palette numbers from the base filename
        std::regex bmpRegex(R"((.*)_frame_(\d+)_palette_(\d+)\.bmp)");
        std::smatch match;


        // Check if the filename matches the expected pattern
        if (std::regex_match(fileNameStr, match, bmpRegex)) {
            std::string ignore = match[1].str();
            int frameNumber = std::stoi(match[2].str());
            int paletteNumber = std::stoi(match[3].str());

            std::cout << "Processing BMP file: " << fileNameStr
                << " (Base Name: " << baseName
                << ", Frame: " << frameNumber
                << ", Palette: " << paletteNumber
                << ")" << std::endl;

            // Insert BMP file as 256 or SKI format based on the user-specified format
            if (insertFormat == "256") {
                std::cout << "Inserting BMP (Frame " << frameNumber
                    << ") into 256 format." << std::endl;
                // You would add your insertion code here
            }
            else if (insertFormat == "ski") {
                std::cout << "Inserting BMP (Frame " << frameNumber
                    << ") into SKI format." << std::endl;
                // You would add your insertion code here
            }
        }
        else {
            std::cerr << "Filename format invalid for insertion: " << fileName
                << ". Expected format: frame_X_palette_Y.bmp" << std::endl;
        }
    };

    // Function to process individual files for extraction
    auto processExtractFile = [&](const std::string& fileName) {
        fs::path filePath(fileName);
        std::string extension = filePath.extension().string();
        std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower); // Convert to lowercase
        converter.currentFileName = filePath.stem().string();

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
    };

    // Check if the input path is a directory or a file
    fs::path inputPathObj(inputPath);
    if (fs::exists(inputPathObj)) {
        if (fs::is_directory(inputPathObj)) {
            // Extract the folder name as the base name for insertion
            std::string baseName = inputPathObj.stem().string();

            if (isExtracting) {
                // Iterate through files in the folder for extraction
                for (const auto& entry : fs::directory_iterator(inputPathObj)) {
                    if (entry.is_regular_file()) {
                        processExtractFile(entry.path().string());
                    }
                }
            }
            else if (isInserting) {
                // Iterate through BMP files in the folder for insertion
                for (const auto& entry : fs::directory_iterator(inputPathObj)) {
                    if (entry.is_regular_file()) {
                        std::string extension = entry.path().extension().string();
                        std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
                        if (extension == ".bmp") {
                            processInsertFile(entry.path().string(), baseName);
                        }
                    }
                }
            }
        }
        else if (fs::is_regular_file(inputPathObj)) {
            if (isExtracting) {
                processExtractFile(inputPath);
            }
            else {
                std::cerr << "For insertion, the input must be a folder containing BMP files." << std::endl;
                return 1;
            }
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
