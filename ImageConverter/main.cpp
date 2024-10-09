#include <iostream>
#include <string>
#include <filesystem>
#include <regex>
#include <vector>
#include <sstream>
#include "ImageConverter.h"

namespace fs = std::filesystem;

int main(int argc, char* argv[]) {
    // Check for the correct number of command-line arguments
    if (argc < 4 || argc > 6) {
        std::cerr << "Usage: " << argv[0] << " <palette_file> <filename_or_folder> [-e|-i256|-iski] [-allp] [-p<palette_number>] [-ps<id1,id2,...>]" << std::endl;
        std::cerr << "Options:\n";
        std::cerr << "  -e               Extract pictures\n";
        std::cerr << "  -i256            Insert pictures into 256 format\n";
        std::cerr << "  -iski            Insert pictures into SKI format\n";
        std::cerr << "  -allp            Use all palettes for extraction (overrides -p)\n";
        std::cerr << "  -p<palette_num>  Specify a palette number to use (ignored if -allp is provided)\n";
        std::cerr << "  -ps<id1,id2,...> Concatenate and use multiple palettes in the given order for SKI files" << std::endl;
        return 1;
    }

    bool useAllPalettes = false;  // Flag to indicate whether to use all palettes
    bool isExtracting = false;    // Flag to indicate extracting (-e)
    bool isInserting = false;     // Flag to indicate inserting
    std::string insertFormat = ""; // Format for insertion ("256" or "ski")
    int selectedPalette = -1;     // Default palette value (-1 means no palette specified)
    std::vector<int> paletteIDs;  // List of palette IDs for SKI files

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
        else if (arg.substr(0, 3) == "-ps") {
            // Parse a comma-separated list of palette IDs
            std::string paletteList = arg.substr(3);
            std::stringstream ss(paletteList);
            std::string id;
            while (std::getline(ss, id, ',')) {
                try {
                    paletteIDs.push_back(std::stoi(id));
                }
                catch (std::invalid_argument&) {
                    std::cerr << "Invalid palette ID in list: " << id << std::endl;
                    return 1;
                }
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
    else if (!paletteIDs.empty()) {
        // Concatenate palettes for SKI processing
        std::cout << "Concatenating palettes: ";
        for (int id : paletteIDs) {
            std::cout << id << " ";
        }
        std::cout << std::endl;
        converter.concatenatePalettes(paletteIDs); // Use concatenated palettes
    }
    else if ((isExtracting || isInserting) && insertFormat == "ski" && paletteIDs.empty()) {
        // This case applies only if we are dealing with SKI files and no palettes are provided
        paletteIDs = { 0xD9, 0x02, 0x01, 0x2A, 0x55 }; // Default palette IDs for SKI
        std::cout << "No palette IDs provided for SKI extraction/insertion. Defaulting to palettes 0xD9, 0x02, 0x01, 0x2A, 0x55." << std::endl;
        converter.concatenatePalettes(paletteIDs); // Use default palettes for SKI
    }
    else {
        // Default palette selection if neither '-allp' nor '-pXXX' nor '-ps' is provided
        converter.selectPalette(-1, false); // Select default palette
    }

    // Function to process BMP files for insertion
    auto processInsertFile = [&](const std::string& fileName, const std::string& baseName) {
        fs::path filePath(fileName);
        std::string fileNameStr = filePath.filename().string(); // Get only the filename
        std::string extension = filePath.extension().string();
        std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower); // Convert to lowercase

        // Regular expression to extract frame and palette numbers from the base filename
        std::regex bmpRegex(R"(^(.*)_frame_(\d+)_palette_(\d+)_position_(\d+)\.bmp)");
        std::smatch match;

        // Check if the filename matches the expected pattern
        if (std::regex_match(fileNameStr, match, bmpRegex)) {
            std::string base = match[1].str();
            int frameNumber = std::stoi(match[2].str());
            int paletteNumber = std::stoi(match[3].str());
            int positionNumber = std::stoi(match[4].str()); // Extract position
            

            converter.selectPalette(paletteNumber, false);
            std::cout << "Processing BMP file: " << fileNameStr
                << " (Base Name: " << base
                << ", Frame: " << frameNumber
                << ", Position: " << positionNumber
                << ", Palette: " << paletteNumber
                << ")" << std::endl;
            // Insert BMP file as 256 or SKI format based on the user-specified format
            if (insertFormat == "256") {
                std::cout << "Inserting BMP (Frame " << frameNumber
                    << ", Position " << positionNumber
                    << ") into 256 format." << std::endl;
                converter.BMPto256(filePath.string());
            }
            // Add more insertion formats if needed
        }
        else {
            std::cerr << "Filename format invalid for insertion: " << fileName
                << ". Expected format: <base>_frame_<X>_palette_<Y>_position_<Z>.bmp" << std::endl;
        }
    };
    auto processInsertSKIFolder = [&](const std::string& folderPath) {
        std::cout << "Inserting BMP files into SKI format from folder: " << folderPath << std::endl;
        converter.BMPtoSKI(folderPath); // Pass the folder path to BMPtoSKI function
    };
    // Function to process individual files for extraction
    auto processExtractFile = [&](const std::string& fileName) {
        fs::path filePath(fileName);
        std::string extension = filePath.extension().string();
        std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower); // Convert to lowercase
        converter.currentFileName = filePath.stem().string();
        converter.framesData.clear();
        bool skifile = false;
        if (extension == ".ski") {
            paletteIDs = { 0xD9, 0x02, 0x01, 0x2A, 0x55 }; // Default palette IDs for SKI
            std::cout << "No palette IDs provided for SKI extraction/insertion. Defaulting to palettes 0xD9, 0x02, 0x01, 0x2A, 0x55." << std::endl;
            converter.concatenatePalettes(paletteIDs); // Use default palettes for SKI

            skifile = true;
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
        converter.dumpAllBMP(skifile);
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
                if (insertFormat == "256") {
                    // Iterate through BMP files in the folder for 256 insertion
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
                else if (insertFormat == "ski") {
                    // Process the entire folder for SKI insertion
                    processInsertSKIFolder(inputPathObj.string());
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
