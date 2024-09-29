#include "ImageConverter.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#include <filesystem> // For extracting filename from path


#include "ImageConverter.h"
#include <vector>
#include <fstream>

std::string ImageConverter::stripExtension(const std::string& filename) {
    size_t lastDot = filename.find_last_of(".");
    if (lastDot == std::string::npos) {
        return filename; // No extension found, return the original filename
    }
    return filename.substr(0, lastDot); // Strip the extension
}

int ImageConverter::selectPalette(int maxIndex) {
    // Iterate through all the palettes and find one that has enough colors
    for (int i = 0; i < palettes.size(); ++i) {
        if (palettes[i].size() > maxIndex) {
            return i; // Return the index of the first suitable palette
        }
    }

    // If no palette is found that can handle maxIndex, return an error code
    return -1; // Error: No suitable palette found
}
void ImageConverter::createBMP(const std::vector<uint8_t>& frameData, const std::string& outputFileName, int width, int height) {
    // Find the maximum index value in frameData
    int maxIndex = 0;
    for (uint8_t index : frameData) {
        if (index > maxIndex) {
            maxIndex = index;
        }
    }

    // Select an appropriate palette
    int paletteIndex = selectPalette(maxIndex);
    if (paletteIndex == -1) {
        std::cerr << "Error: No suitable palette found." << std::endl;
        return;
    }

    // Get the selected palette
    const auto& selectedPalette = palettes[paletteIndex];

    // BMP file header size
    const int headerSize = 54; // 14 bytes for file header + 40 bytes for info header
    const int rowSize = (width * 3 + 3) & (~3); // Ensure row size is a multiple of 4
    const int dataSize = rowSize * height; // Total size of pixel data
    const int fileSize = headerSize + dataSize; // Total file size

    // Create the BMP header
    uint8_t header[headerSize] = {
        'B', 'M', // Signature
        static_cast<uint8_t>(fileSize), static_cast<uint8_t>(fileSize >> 8), static_cast<uint8_t>(fileSize >> 16), static_cast<uint8_t>(fileSize >> 24), // File size
        0, 0, 0, 0, // Reserved
        headerSize, 0, 0, 0, // Offset to pixel data
        40, 0, 0, 0, // Size of info header
        static_cast<uint8_t>(width), static_cast<uint8_t>(width >> 8), static_cast<uint8_t>(width >> 16), static_cast<uint8_t>(width >> 24), // Width
        static_cast<uint8_t>(height), static_cast<uint8_t>(height >> 8), static_cast<uint8_t>(height >> 16), static_cast<uint8_t>(height >> 24), // Height
        1, 0, // Color planes
        24, 0, // Bits per pixel
        0, 0, 0, 0, // Compression
        static_cast<uint8_t>(dataSize), static_cast<uint8_t>(dataSize >> 8), static_cast<uint8_t>(dataSize >> 16), static_cast<uint8_t>(dataSize >> 24), // Size of raw bitmap data
        0, 0, 0, 0, // Horizontal resolution
        0, 0, 0, 0, // Vertical resolution
        0, 0, 0, 0, // Number of colors
        0, 0, 0, 0  // Important colors
    };

    // Open the output BMP file
    std::ofstream bmpFile(outputFileName, std::ios::binary);
    if (!bmpFile.is_open()) {
        std::cerr << "Error: Unable to open BMP file " << outputFileName << std::endl;
        return;
    }

    // Write the BMP header
    bmpFile.write(reinterpret_cast<char*>(header), headerSize);

    // Write pixel data
    for (int y = height - 1; y >= 0; --y) { // BMP stores pixels bottom to top
        for (int x = 0; x < width; ++x) {
            // Get the pixel index
            uint8_t index = frameData[y * width + x];

            // Check if the index is within the bounds of the selected palette
            if (index < selectedPalette.size()) {
                // Map the index to the corresponding RGB value using the selected palette
                Color color = selectedPalette[index];
                bmpFile.put(color.b); // Blue
                bmpFile.put(color.g); // Green
                bmpFile.put(color.r); // Red
            }
            else {
                // Handle out-of-bounds index (default to black or any other color)
                bmpFile.put(0); // Blue
                bmpFile.put(0); // Green
                bmpFile.put(0); // Red
            }
        }

        // Row padding
        for (int p = 0; p < rowSize - (width * 3); ++p) {
            bmpFile.put(0); // Pad the row with zeroes
        }
    }

    bmpFile.close();
}


namespace fs = std::filesystem;
ImageConverter::ImageConverter(const std::string& paletteFileName, const std::string& bitmapFileName)
    : palFileName(paletteFileName), bmpFileName(bitmapFileName) {}

uint16_t readShort(std::ifstream& file) {
    uint16_t value;
    file.read(reinterpret_cast<char*>(&value), sizeof(value));
    return value;
}

// Helper function to read 32-bit integer (little endian)
uint32_t readInt(std::ifstream& file) {
    uint32_t value;
    file.read(reinterpret_cast<char*>(&value), sizeof(value));
    return value;
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
        size_t actual_pos = frameAddresses[frameIndex] + frameAddresses.size() * 4 + 6;
        skiFile.seekg(actual_pos);  // Seek to the current frame's address

        // Read width and height for this frame
        uint16_t width, height;
        skiFile.read(reinterpret_cast<char*>(&width), sizeof(uint16_t));
        skiFile.read(reinterpret_cast<char*>(&height), sizeof(uint16_t));
        uint16_t sth1, sth2;
        skiFile.read(reinterpret_cast<char*>(&sth1), sizeof(uint16_t));
        skiFile.read(reinterpret_cast<char*>(&sth2), sizeof(uint16_t));
        uint32_t flags;
        skiFile.read(reinterpret_cast<char*>(&flags), sizeof(uint32_t));

        uint16_t numPairs = height;  // Number of rows corresponds to height
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
        std::vector<uint8_t> frameData(width * height, 0);  // Store frame data
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
                if (c >= width) {
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
                        frameData[outputIndex] = value;  // Store it at the calculated position
                    }
                }

                // Update the index_start for the next insertion
                index_start += d;  // Move index_start by d bytes
                remainingB -= 2;  // Decrease remaining by 2 for c and d, and by d for the data bytes
            }
            current_index += width;  // Move to the next row
        }

        // Output the frame data as a BMP
        std::string outputBmpFileName = outputDir.string() + "/frame_" + std::to_string(frameIndex) + ".bmp";
        createBMP(frameData, outputBmpFileName, width, height); // Create BMP instead of PNG
    }

    skiFile.close();
    return true;
}

// Helper function to strip the extension and get the filename without it


// Helper function to extract the filename from a full path
std::string getFileNameWithoutPath(const std::string& filepath) {
    std::filesystem::path pathObj(filepath);
    return pathObj.stem().string(); // Get filename without path and extension
}

bool ImageConverter::readPalette() {
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
            palFile.read(reinterpret_cast<char*>(&r), 1);
            palFile.read(reinterpret_cast<char*>(&g), 1);
            palFile.read(reinterpret_cast<char*>(&b), 1);
            palFile.read(reinterpret_cast<char*>(&a), 1);

            // Store the color (ignoring alpha for now)
            palette.push_back({ r, g, b }); // Store RGB values
        }
        palettes.push_back(palette); // Add the palette to the collection
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

    // Read the .256 file header (assuming width and height are stored)
    inputFile.read(reinterpret_cast<char*>(&width), sizeof(width));
    inputFile.read(reinterpret_cast<char*>(&height), sizeof(height));

    // Prepare to read color indices
    size_t totalPixels = width * height;
    indices.resize(totalPixels);  // Adjust based on width and height

    // Read pixel indices from the .256 file
    inputFile.read(reinterpret_cast<char*>(indices.data()), totalPixels);

    // Ensure palettes are available (assume readPalette was called earlier)
    if (palettes.empty()) {
        std::cerr << "Error: No palettes available. Ensure readPalette() was called before convert()." << std::endl;
        inputFile.close();
        return false;
    }

    // Select an appropriate palette based on the max index in frameData
    int maxIndex = *std::max_element(indices.begin(), indices.end());
    int paletteIndex = selectPalette(maxIndex);
    if (paletteIndex == -1) {
        std::cerr << "Error: No suitable palette found." << std::endl;
        inputFile.close();
        return false;
    }

    // Generate the output BMP filename in the working directory
    std::string outputBmpFileName = stripExtension(FileName) + ".bmp"; // Save in current directory

    // Reuse createBMP to generate BMP from the indices
    createBMP(indices, outputBmpFileName, width, height);

    inputFile.close();
    return true;  // Return true if parsing and conversion is successful
}