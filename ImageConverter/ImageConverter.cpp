#include "ImageConverter.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#include <filesystem> // For extracting filename from path
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

        // Output the frame data as a PNG
        std::string outputPngFileName = outputDir.string() + "/frame_" + std::to_string(frameIndex) + ".png";
        createPNG(frameData, outputPngFileName, width, height); // Assuming createPNG handles the image creation

        //std::cout << "Parsed and saved frame " << frameIndex << " as PNG: " << outputPngFileName << std::endl;
    }

    skiFile.close();
    return true;
}



// Helper function to strip the extension and get the filename without it
std::string stripExtension(const std::string& filename) {
    size_t lastDot = filename.find_last_of(".");
    if (lastDot == std::string::npos) {
        return filename; // No extension found, return the original filename
    }
    return filename.substr(0, lastDot); // Strip the extension
}

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
void ImageConverter::createPNG(const std::vector<uint8_t>& frameData, const std::string& outputFileName, int width, int height) {
    if (frameData.size() == 0) {
        std::ofstream file(outputFileName);
        file.close();
        return;
    
    }
    uint8_t maxIndex = *std::max_element(frameData.begin(), frameData.end());
    std::vector<Color> palette;

    // Find the appropriate palette based on the max index
    for (auto p : palettes) {
        if (p.size() > maxIndex) {
            palette = p;
            break;
        }
    }

    if (palette.empty()) {
        std::cerr << "Error: Palette is empty, cannot create PNG." << std::endl;
        return;
    }

    // Create an RGB image from the indexed data (assuming 3 channels: R, G, B)
    std::vector<uint8_t> rgbImage(width * height * 3);

    for (int i = 0; i < frameData.size(); i++) {
        uint8_t index = frameData[i];

        // Handle palette lookup. Assuming the 'index' directly corresponds to a color in the palette.
        if (index >= palette.size()) {
            std::cerr << "Error: Invalid palette index at pixel " << i << std::endl;
            continue;
        }

        // Fill the RGB image with the palette color corresponding to the pixel index
        rgbImage[i * 3 + 0] = palette[index].r;  // Red
        rgbImage[i * 3 + 1] = palette[index].g;  // Green
        rgbImage[i * 3 + 2] = palette[index].b;  // Blue
    }

    // Write the main image to a PNG file using stbi_write_png
    if (stbi_write_png(outputFileName.c_str(), width, height, 3, rgbImage.data(), width * 3) == 0) {
        std::cerr << "Error: Failed to write PNG file " << outputFileName << std::endl;
    }
    else {
        //std::cout << "PNG file created successfully: " << outputFileName << std::endl;
    }

    // Now let's create a PNG to show the palette
    std::string paletteFileName = outputFileName.substr(0, outputFileName.find_last_of(".")) + "_palette.png";

    // Determine the dimensions for the palette image. For simplicity, let's make a single row.
    int paletteWidth = static_cast<int>(palette.size());
    int paletteHeight = 20;  // Fixed height for the palette display

    std::vector<uint8_t> paletteImage(paletteWidth * paletteHeight * 3);

    // Fill the palette image
    for (int y = 0; y < paletteHeight; ++y) {
        for (int x = 0; x < paletteWidth; ++x) {
            if (x < palette.size()) {
                const Color& color = palette[x];
                int index = (y * paletteWidth + x) * 3;
                paletteImage[index + 0] = color.r;  // Red
                paletteImage[index + 1] = color.g;  // Green
                paletteImage[index + 2] = color.b;  // Blue
            }
        }
    }

    // Write the palette image to a PNG file
    if (stbi_write_png(paletteFileName.c_str(), paletteWidth, paletteHeight, 3, paletteImage.data(), paletteWidth * 3) == 0) {
        std::cerr << "Error: Failed to write palette PNG file " << paletteFileName << std::endl;
    }
    else {
        //std::cout << "Palette PNG file created successfully: " << paletteFileName << std::endl;
    }
}
bool ImageConverter::convert(const std::string& FileName) {
    // Open the .256 file for reading
    std::ifstream inputFile(FileName, std::ios::binary);
    if (!inputFile.is_open()) {
        std::cerr << "Error: Could not open .256 file " << bmpFileName << std::endl;
        return false;
    }

    // Read the .256 file header (assumed structure)
    // Adjust based on your actual file format
    inputFile.read(reinterpret_cast<char*>(&width), sizeof(width));
    inputFile.read(reinterpret_cast<char*>(&height), sizeof(height));

    // Prepare to read color indices
    size_t totalPixels = width * height;
    indices.resize(totalPixels);  // Adjust based on width and height

    // Read pixel indices from the .256 file
    inputFile.read(reinterpret_cast<char*>(indices.data()), totalPixels);

    // Create an RGB image from the indexed data (assuming 3 channels: R, G, B)
    std::vector<uint8_t> rgbImage(totalPixels * 3);

    // Read the palette
    readPalette();
    std::vector<Color> palette;
    uint8_t maxIndex = *std::max_element(indices.begin(), indices.end());
    // Find the appropriate palette based on the max index
    for (auto p : palettes) {
        if (p.size() > maxIndex) {
            palette = p;
            break;
        }
    }

    // Fill the RGB image with the palette colors corresponding to the pixel indices
    for (size_t i = 0; i < totalPixels; ++i) {
        uint8_t index = indices[i];

        // Check if the index is valid
        if (index < palette.size()) {
            rgbImage[i * 3 + 0] = palette[index].r;  // Red
            rgbImage[i * 3 + 1] = palette[index].g;  // Green
            rgbImage[i * 3 + 1] = palette[index].b;  // Blue
        }
        else {
            std::cerr << "Error: Invalid palette index at pixel " << i << std::endl;
            // Fill with a default color (e.g., black) for invalid indices
            rgbImage[i * 3 + 0] = 0;  // Red
            rgbImage[i * 3 + 1] = 0;  // Green
            rgbImage[i * 3 + 2] = 0;  // Blue
        }
    }

    // Generate the output PNG filename in the working directory
    std::string outputPngFileName = fs::path(FileName).stem().string() + ".png"; // Save in current directory

    // Write the image to a PNG file using stbi_write_png
    if (stbi_write_png(outputPngFileName.c_str(), width, height, 3, rgbImage.data(), width * 3) == 0) {
        std::cerr << "Error: Failed to write PNG file " << outputPngFileName << std::endl;
        inputFile.close();
        return false;
    }
    else {
        //std::cout << "PNG file created successfully in the working directory: " << outputPngFileName << std::endl;
    }

    inputFile.close();
    return true;  // Return true if parsing and conversion is successful
}
