#include "ImageConverter.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#include <filesystem>
#include <iostream>
#include <fstream>

// Constructor implementation
ImageConverter::ImageConverter(const std::string& paletteFileName, const std::string& bitmapFileName)
    : palFileName(paletteFileName), bmpFileName(bitmapFileName) {
    // Load palettes if the palette file name is provided
    if (!palFileName.empty()) {
        loadPalettesFromPALfile();
    }
}

// Helper to strip the file extension
std::string ImageConverter::stripExtension(const std::string& filename) {
    size_t lastDot = filename.find_last_of(".");
    return (lastDot == std::string::npos) ? filename : filename.substr(0, lastDot);
}

// Helper function to extract the filename from a full path
std::string ImageConverter::getFileNameWithoutPath(const std::string& filepath) {
    std::filesystem::path pathObj(filepath);
    return pathObj.stem().string();
}

// Function to select the appropriate palette based on maxIndex
/* 3 possibilit¨¦s : 
1 - On mentionne DIRECTEMENT l'id de la palette dans la commande, donc pas d'ambiguit¨¦
2 - On mentionne allp : on s¨¦lectionnera toutes les palettes
3 - On mentionne rien : on prend une qui fit ¨¤ 100%, donc celle qui a le plus de couleurs */
int ImageConverter::selectPalette(int index, bool allP) {

    selectedPalettes.clear();
    if (allP) {

        selectedPalettes = PALfile;
        for (int i = 0; i < selectedPalettes.size(); i++)
            palettes_id.push_back(i);
        
    }
    else {
        if (index == -1) {
            int maxIndex = 0;
            for (int i = 0; i < PALfile.size(); ++i) {
                if (PALfile[i].size() > maxIndex) {
                    maxIndex = PALfile[i].size();
                    if (selectedPalettes.empty()) {
                        selectedPalettes.push_back(PALfile[i]); 
                        palettes_id.push_back(i);
                    }
                    else {
                        selectedPalettes[0] = PALfile[i];
                        palettes_id[0] = i;
                    }
                }
            }
        
        }
        else {
            selectedPalettes.push_back(PALfile[index]);
            palettes_id.push_back(index);
        }
    
    }
    
    return -1; // Error: No suitable palette found
}

// The existing createBMP method (unchanged)
void ImageConverter::createBMP(const frameData& frameData, std::string& outputFileName, std::vector<Color> palette) {
    const int headerSize = 54;  // BMP header size
    const int bitsPerPixel = 32; // Use 32 bits per pixel for transparency
    const int rowSize = (frameData.width * bitsPerPixel / 8 + 3) & (~3); // Row size must be a multiple of 4
    const int dataSize = rowSize * frameData.height; // Total size of pixel data
    const int fileSize = headerSize + dataSize; // Total file size

    // BMP header initialization
    uint8_t header[headerSize] = {
        'B', 'M',  // BMP signature
        static_cast<uint8_t>(fileSize), static_cast<uint8_t>(fileSize >> 8),
        static_cast<uint8_t>(fileSize >> 16), static_cast<uint8_t>(fileSize >> 24),
        0, 0, 0, 0, // Reserved
        headerSize, 0, 0, 0, // DIB header size
        40, 0, 0, 0, // DIB header size
        static_cast<uint8_t>(frameData.width), static_cast<uint8_t>(frameData.width >> 8),
        static_cast<uint8_t>(frameData.width >> 16), static_cast<uint8_t>(frameData.width >> 24),
        static_cast<uint8_t>(frameData.height), static_cast<uint8_t>(frameData.height >> 8),
        static_cast<uint8_t>(frameData.height >> 16), static_cast<uint8_t>(frameData.height >> 24),
        1, 0,  // Number of color planes
        static_cast<uint8_t>(bitsPerPixel), 0, // Bits per pixel
        0, 0, 0, 0, // Compression (0 = none)
        static_cast<uint8_t>(dataSize), static_cast<uint8_t>(dataSize >> 8),
        static_cast<uint8_t>(dataSize >> 16), static_cast<uint8_t>(dataSize >> 24), // Image size
        0, 0, 0, 0, // Horizontal resolution (pixels per meter, not important)
        0, 0, 0, 0, // Vertical resolution (pixels per meter, not important)
        0, 0, 0, 0, // Number of colors in palette (0 = 2^n)
        0, 0, 0, 0  // Important colors (0 = all)
    };

    std::ofstream bmpFile(outputFileName, std::ios::binary);
    if (!bmpFile.is_open()) {
        std::cerr << "Error: Unable to open BMP file " << outputFileName << std::endl;
        return;
    }

    bmpFile.write(reinterpret_cast<char*>(header), headerSize);

    for (int y = frameData.height - 1; y >= 0; --y) {
        for (int x = 0; x < frameData.width; ++x) {
            uint8_t index = frameData.data[y * frameData.width + x];
            if (index < palette.size()) {
                Color color = palette[index];
                // Write in ARGB format
                bmpFile.put(color.a); // Alpha: 0 (fully transparent) or 255 (fully opaque), modify as necessary
                bmpFile.put(color.b);
                bmpFile.put(color.g);
                bmpFile.put(color.r);
            }
            else {
                // Write transparent black for out-of-bounds index
                bmpFile.put(0); // Alpha
                bmpFile.put(0);
                bmpFile.put(0);
                bmpFile.put(0);
            }
        }

        // Padding to ensure row size is a multiple of 4
        for (int p = 0; p < rowSize - (frameData.width * (bitsPerPixel / 8)); ++p) {
            bmpFile.put(0);
        }
    }

    bmpFile.close();
}

bool ImageConverter::parseSKI(const std::string& skiFileName) {
    // Extract the base name (without extension) of the SKI file
    std::filesystem::path skiPath(skiFileName);
    std::string baseName = skiPath.stem().string();  // Gets the file name without extension

    // Create a directory with the base name of the SKI file in the current working directory
    std::filesystem::path outputDir = std::filesystem::current_path() / baseName;  // Create in the working directory
    if (!std::filesystem::exists(outputDir)) {
        std::filesystem::create_directory(outputDir);
    }

    std::ifstream skiFile(skiFileName, std::ios::binary);
    if (!skiFile.is_open()) {
        std::cerr << "Error: Unable to open SKI file " << skiFileName << std::endl;
        return false;
    }

    // Read the number of frames
    uint16_t numFrames;
    skiFile.read(reinterpret_cast<char*>(&numFrames), sizeof(uint16_t));

    // Read frame addresses
    std::vector<uint32_t> frameAddresses(numFrames);
    for (uint32_t i = 0; i < numFrames; ++i) {
        skiFile.read(reinterpret_cast<char*>(&frameAddresses[i]), sizeof(uint32_t));
    }

    // Loop through each frame
    for (uint32_t frameIndex = 0; frameIndex < numFrames; ++frameIndex) {
        frameData currentfd;
        size_t actual_pos = frameAddresses[frameIndex] + frameAddresses.size() * 4 + 6;
        skiFile.seekg(actual_pos);  // Seek to the current frame's address

        skiFile.read(reinterpret_cast<char*>(&currentfd.width), sizeof(uint16_t));
        skiFile.read(reinterpret_cast<char*>(&currentfd.height), sizeof(uint16_t));

        uint16_t sth1, sth2;
        skiFile.read(reinterpret_cast<char*>(&sth1), sizeof(uint16_t));
        skiFile.read(reinterpret_cast<char*>(&sth2), sizeof(uint16_t));
        uint32_t flags;
        skiFile.read(reinterpret_cast<char*>(&flags), sizeof(uint32_t));

        uint16_t numPairs = currentfd.height;  // Number of rows corresponds to height
        currentfd.data.resize(currentfd.width * currentfd.height);
        std::vector<std::pair<uint32_t, uint32_t>> abPairs;
        if ((flags >> 24) == 2)
            skiFile.seekg(actual_pos + 12);  // Seek to the current frame's address
        else if ((flags >> 24) == 1) {
            std::cout << "Format not supported!!!!!!!!!!!!!!!!!!!!!!!!!!!! " << skiFileName << std::endl;
            return false;
        }

        // Read (a, b) pairs for this frame
        for (uint16_t i = 0; i < numPairs; ++i) {
            int a = 0, b = 0;
            skiFile.read(reinterpret_cast<char*>(&a), sizeof(uint32_t));  // Offset for the row
            skiFile.read(reinterpret_cast<char*>(&b), sizeof(uint32_t));  // Count of bytes for the row
            abPairs.emplace_back(a, b);
        }

        // Calculate offset for the second section
        std::streamoff secondSectionOffset = actual_pos + 12 + static_cast<std::streamoff>(numPairs * 8);

        // Resize the output vector to the correct size (width * height)
        std::vector<uint8_t> frameData(currentfd.width * currentfd.height, 0);  // Store frame data
        int current_index = 0;

        // Process each (a, b) pair for rows in this frame
        for (const auto& [a, b] : abPairs) {
            std::streamoff offset = secondSectionOffset + static_cast<std::streamoff>(a);  // Calculate the offset for the row
            skiFile.seekg(offset);  // Seek to the row's offset

            int remainingB = b;  // Remaining bytes to process for this row
            int index_start = current_index;

            // Process each row's data
            while ((remainingB > 0) && ((remainingB - 1) > 0)) {
                uint8_t c, d;
                skiFile.read(reinterpret_cast<char*>(&c), 1);  // Read byte c (position)
                skiFile.read(reinterpret_cast<char*>(&d), 1);  // Read byte d (number of following bytes)
                index_start += c;

                // Ensure that the position c is within the bounds of the output vector
                if (c >= currentfd.width) {
                    std::cerr << "Warning: Position c (" << static_cast<int>(c) << ") is out of bounds for the row." << std::endl;
                    break;  // Exit the loop if position is out of bounds
                }

                // Read d bytes directly into the frameData at position c
                for (uint8_t i = 0; i < d; ++i) {
                    uint8_t value;
                    skiFile.read(reinterpret_cast<char*>(&value), 1);  // Read the next byte into value

                    // Calculate the index to insert into the frameData vector
                    int outputIndex = index_start + i;
                    if (outputIndex < frameData.size()) {
                        currentfd.data[outputIndex] = value;  // Store it at the calculated position
                    }
                }

                // Update the index_start for the next insertion
                index_start += d;  // Move index_start by d bytes
                remainingB -= 2;  // Decrease remaining by 2 for c and d, and by d for the data bytes
            }
            current_index += currentfd.width;  // Move to the next row
        }
        framesData.push_back(currentfd);
        // Output the frame data as a BMP
    }

    skiFile.close();
    return true;
}

//Will read the PAL file
bool ImageConverter::loadPalettesFromPALfile() {
    std::ifstream palFile(palFileName, std::ios::binary);
    if (!palFile.is_open()) {
        std::cerr << "Error: Unable to open palette file " << palFileName << std::endl;
        return false;
    }

    char header[16]; // Adjusting to read the entire header
    palFile.read(header, sizeof(header));
    if (strncmp(header, "PAL ", 4) != 0) {
        std::cerr << "Error: Invalid .PAL file format." << std::endl;
        return false;
    }

    // Read the color palettes
    while (true) {
        uint32_t colorListSize;
        palFile.read(reinterpret_cast<char*>(&colorListSize), sizeof(colorListSize)); // Read size of color list
        if (palFile.eof() || colorListSize == 0) {
            break; // Exit if end of file or size is 0
        }

        std::vector<Color> palette; // Temporary palette for current list

        // Read colors in the current list
        for (uint32_t i = 0; i < colorListSize; ++i) {
            uint8_t a, r, g, b; // Using ARGB format
            palFile.read(reinterpret_cast<char*>(&b), 1);
            palFile.read(reinterpret_cast<char*>(&g), 1);
            palFile.read(reinterpret_cast<char*>(&r), 1);
            palFile.read(reinterpret_cast<char*>(&a), 1);

            // Store the color (ignoring alpha for now)
            palette.push_back({ r, g, b, a }); // Store RGB values
        }
        PALfile.push_back(palette); // Add the palette to the collection
    }

    palFile.close();
    return true;
}
bool ImageConverter::convert(const std::string& FileName) {
    // Open the .256 file for reading
    std::ifstream inputFile(FileName, std::ios::binary);
    if (!inputFile.is_open()) {
        std::cerr << "Error: Could not open .256 file " << FileName << std::endl;
        return false;
    }
    frameData currentfd;
    // Read the .256 file header (assuming width and height are stored)
    
    inputFile.read(reinterpret_cast<char*>(&currentfd.width), sizeof(currentfd.width));
    inputFile.read(reinterpret_cast<char*>(&currentfd.height), sizeof(currentfd.height));
    // Prepare to read color indices
    size_t totalPixels = currentfd.width * currentfd.height;

    // Read pixel indices from the .256 file
    currentfd.data.resize(totalPixels);
    inputFile.read(reinterpret_cast<char*>(currentfd.data.data()), totalPixels);

    framesData.push_back(currentfd);

    inputFile.close();
    return true;  // Return true if parsing and conversion is successful
}


void writePaletteAsBMP(const std::vector<Color>& palette, const std::string& outputFileName) {
    const int width = static_cast<int>(palette.size());
    const int height = 1;

    // Calculate the row size (each pixel is 4 bytes for ARGB)
    int rowSize = (width * 4 + 3) & (~3); // Align to the nearest multiple of 4
    int dataSize = rowSize * height;
    int fileSize = 54 + dataSize; // 54 bytes for the header

    // BMP header for 32-bit ARGB
    uint8_t header[54] = {
        'B', 'M',               // Signature
        static_cast<uint8_t>(fileSize), static_cast<uint8_t>(fileSize >> 8),
        static_cast<uint8_t>(fileSize >> 16), static_cast<uint8_t>(fileSize >> 24),
        0, 0, 0, 0,            // Reserved1 & Reserved2
        54, 0, 0, 0,          // Offset to start of pixel data
        40, 0, 0, 0,          // Header size
        static_cast<uint8_t>(width), static_cast<uint8_t>(width >> 8),
        static_cast<uint8_t>(width >> 16), static_cast<uint8_t>(width >> 24),
        static_cast<uint8_t>(height), static_cast<uint8_t>(height >> 8),
        static_cast<uint8_t>(height >> 16), static_cast<uint8_t>(height >> 24),
        1, 0,                 // Planes
        32, 0,                // Bits per pixel (32-bit)
        0, 0, 0, 0,          // Compression
        static_cast<uint8_t>(dataSize), static_cast<uint8_t>(dataSize >> 8),
        static_cast<uint8_t>(dataSize >> 16), static_cast<uint8_t>(dataSize >> 24),
        0, 0, 0, 0,          // Pixels per meter (not used)
        0, 0, 0, 0,          // Colors in color table (0 for 32-bit)
        0, 0, 0, 0           // Important color count (0 means all)
    };

    // Open BMP file for writing
    std::ofstream bmpFile(outputFileName, std::ios::binary);
    if (!bmpFile.is_open()) {
        std::cerr << "Error: Unable to open BMP file " << outputFileName << std::endl;
        return;
    }

    // Write the header
    bmpFile.write(reinterpret_cast<char*>(header), sizeof(header));

    // Write pixel data
    for (const auto& color : palette) {
        // Write the color in ARGB format
        bmpFile.put(color.a); // Alpha: 255 (fully opaque), change to 0 for fully transparent
        bmpFile.put(color.b);
        bmpFile.put(color.g);
        bmpFile.put(color.r);
    }

    // Fill the remaining bytes to align to row size
    for (int p = 0; p < rowSize - (width * 4); ++p) {
        bmpFile.put(0);
    }

    // Close the file
    bmpFile.close();
}


void ImageConverter::dumpAllBMP() {
    std::filesystem::path outputDir = std::filesystem::current_path() / this->currentFileName;

    // Check if the directory exists, and create it if it doesn't
    if (!std::filesystem::exists(outputDir)) {
        std::filesystem::create_directory(outputDir);
    }

    int frame_data_cnt = 0;

    // Check if there's only one selected palette
    if (selectedPalettes.size() == 1) {
        const auto& singlePalette = selectedPalettes[0];

        // Create a BMP file for the palette using the dedicated function
        std::string paletteOutputName = (outputDir / (currentFileName + "_palette_colors.bmp")).string();
        writePaletteAsBMP(singlePalette, paletteOutputName);
    }

    // Existing logic for dumping all BMP frames
    for (int ip = 0; ip < selectedPalettes.size(); ip++) {
        if (selectedPalettes.size() == 1) {

            for (auto fd : framesData) {
                std::string output_name = (outputDir / (currentFileName + "_frame_" + std::to_string(frame_data_cnt) + "_palette_" + std::to_string(ip) + ".bmp")).string();
                createBMP(fd, output_name, selectedPalettes[ip]); // Assuming createBMP still handles frame data.
                frame_data_cnt++;
            }

            
        }
        else {
            std::string output_name = (outputDir / (currentFileName + "_frame_" + std::to_string(frame_data_cnt) + "_palette_" + std::to_string(ip) + ".bmp")).string();
            createBMP(framesData[0], output_name, selectedPalettes[ip]); // Assuming createBMP still handles frame data.

        
        }
        
    }
}
