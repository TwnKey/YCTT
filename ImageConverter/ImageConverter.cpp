#include "ImageConverter.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#include <filesystem>
#include <iostream>
#include <fstream>
#include <filesystem>
namespace fs = std::filesystem;
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

void ImageConverter::concatenatePalettes(const std::vector<int>& idsToUse) {
    selectedPalettes.clear();  // Clear any previously selected palettes
    palettes_id.clear();       // Clear palette IDs list as well

    // Create a new palette by concatenating colors from the selected palettes
    std::vector<Color> concatenatedPalette;

    for (int id : idsToUse) {
        if (id >= 0 && id < PALfile.size()) {
            const std::vector<Color>& currentPalette = PALfile[id];  // Get the current palette

            // Concatenate the current palette to the new one
            concatenatedPalette.insert(concatenatedPalette.end(), currentPalette.begin(), currentPalette.end());

            palettes_id.push_back(id);  // Record the palette ID
            std::cout << "Added palette " << id << " for concatenation." << std::endl;
        }
        else {
            std::cerr << "Invalid palette ID: " << id << ". Skipping." << std::endl;
        }
    }

    // Now, `concatenatedPalette` contains the merged colors from the selected palettes
    if (!concatenatedPalette.empty()) {
        selectedPalettes.push_back(concatenatedPalette);  // Add the concatenated palette as the only palette
        std::cout << "Concatenated palette created with " << concatenatedPalette.size() << " colors." << std::endl;
    }
    else {
        std::cerr << "No valid palettes selected for concatenation." << std::endl;
    }
}

// The existing createBMP method (unchanged)
void ImageConverter::createBMP(const frameData& frameData, std::string& outputFileName, std::vector<Color> palette, bool skifile) {
    const int headerSize = 14; // BMP header size (14 bytes)
    const int dibHeaderSize = 124; // DIB header size (BITMAPV5HEADER = 124 bytes)
    const int bitsPerPixel = 32; // Using 32 bits per pixel
    const int rowSize = ((frameData.width * bitsPerPixel + 31) / 32) * 4; // Row size must be a multiple of 4
    const int dataSize = rowSize * frameData.height; // Total size of pixel data
    const int fileSize = headerSize + dibHeaderSize + dataSize; // Total file size

    // BMP header initialization
    uint8_t header[headerSize] = {
        'B', 'M',  // BMP signature
        static_cast<uint8_t>(fileSize & 0xFF),
        static_cast<uint8_t>((fileSize >> 8) & 0xFF),
        static_cast<uint8_t>((fileSize >> 16) & 0xFF),
        static_cast<uint8_t>((fileSize >> 24) & 0xFF), // File size
        0, 0,      // Reserved1
        0, 0,      // Reserved2
        static_cast<uint8_t>(headerSize + dibHeaderSize), 0, 0, 0 // Offset to pixel data
    };

    // DIB header initialization
    uint8_t dibHeader[dibHeaderSize] = {
        static_cast<uint8_t>(dibHeaderSize & 0xFF),
        static_cast<uint8_t>((dibHeaderSize >> 8) & 0xFF),
        static_cast<uint8_t>((dibHeaderSize >> 16) & 0xFF),
        static_cast<uint8_t>((dibHeaderSize >> 24) & 0xFF), // Size of DIB header
        static_cast<uint8_t>(frameData.width & 0xFF),
        static_cast<uint8_t>((frameData.width >> 8) & 0xFF),
        static_cast<uint8_t>((frameData.width >> 16) & 0xFF),
        static_cast<uint8_t>((frameData.width >> 24) & 0xFF), // Width
        static_cast<uint8_t>(frameData.height & 0xFF),
        static_cast<uint8_t>((frameData.height >> 8) & 0xFF),
        static_cast<uint8_t>((frameData.height >> 16) & 0xFF),
        static_cast<uint8_t>((frameData.height >> 24) & 0xFF), // Height
        1, 0,  // Color planes
        static_cast<uint8_t>(bitsPerPixel), 0, // Bits per pixel
        03, 0, 0, 0, // Compression (BI_RGB = 0)
        static_cast<uint8_t>(dataSize & 0xFF), // Image size (0 for uncompressed)
        static_cast<uint8_t>((dataSize >> 8) & 0xFF),
        static_cast<uint8_t>((dataSize >> 16) & 0xFF),
        static_cast<uint8_t>((dataSize >> 24) & 0xFF), // Image size//24
        0, 0, 0, 0, // Horizontal resolution
        0, 0, 0, 0, // Vertical resolution
        0, 0, 0, 0, // Colors in palette
        0, 0, 0, 0, // Important colors
        0, 0, 0xFF, 0, // Important colors
        0, 0xFF, 0, 0, // Important colors//48
        0xFF,0,0,0,
        0x0,0x0,0,0xFF,
        0x20, 0x6E, 0x69, 0x57, // Color space type (bV5CSType)//60
        0, 0, 0, 0, // Endpoints (bV5Endpoints)
        0, 0, 0, 0, // Gamma Red (bV5GammaRed)
        0, 0, 0, 0, // Gamma Green (bV5GammaGreen)
        0, 0, 0, 0, // Gamma Blue (bV5GammaBlue)
        0, 0, 0, 0, // Intent (bV5Intent)//80
        0, 0, 0, 0, // Profile data (bV5ProfileData)
        0, 0, 0, 0, // Profile size (bV5ProfileSize)
        0, 0, 0, 0x00,  // Reserved (bV5Reserved)
        0, 0, 0, 0x00,  // Reserved (bV5Reserved)
        0, 0, 0, 0x00,  // Reserved (bV5Reserved)
        0, 0, 0, 0x00,  // Reserved (bV5Reserved)
        0, 0, 0, 0x00,  // Reserved (bV5Reserved)
        0, 0, 0, 0x00,  // Reserved (bV5Reserved)
        0, 0, 0, 0x00,  // Reserved (bV5Reserved)
        0, 0, 0, 0x00,  // Reserved (bV5Reserved)
        0, 0, 0, 0x00  // Reserved (bV5Reserved)
    };

    // Output the regular frameData.data BMP
    std::ofstream bmpFile(outputFileName, std::ios::binary);
    if (!bmpFile.is_open()) {
        std::cerr << "Error: Unable to open BMP file " << outputFileName << std::endl;
        return;
    }

    bmpFile.write(reinterpret_cast<char*>(header), headerSize);
    bmpFile.write(reinterpret_cast<char*>(dibHeader), dibHeaderSize);
    for (int y = frameData.height - 1; y >= 0; --y) {
        for (int x = 0; x < frameData.width; ++x) {
            int offset = (0x100 - palette.size());
            int index_file = frameData.data[y * frameData.width + x];
            uint8_t index;
            if (skifile)
                index = frameData.data[y * frameData.width + x];
            else
                index = frameData.data[y * frameData.width + x] + offset;
            if (index < palette.size()) {
                Color color = palette[index];
                // Write in ARGB format
                bmpFile.put(color.b); // Red
                bmpFile.put(color.g); // Green
                bmpFile.put(color.r); // Blue
                if ((skifile) && (index == 0))
                    bmpFile.put(0x0); // Alpha
                else
                    bmpFile.put(0xFF); // Alpha
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

    // If special_pixels is not empty, output the special pixel BMP
    if (!frameData.special_pixels.empty()) {
        std::string specialFileName = outputFileName.substr(0, outputFileName.find_last_of('.')) + "_special.bmp";
        std::ofstream specialBmpFile(specialFileName, std::ios::binary);
        if (!specialBmpFile.is_open()) {
            std::cerr << "Error: Unable to open special BMP file " << specialFileName << std::endl;
            return;
        }

        specialBmpFile.write(reinterpret_cast<char*>(header), headerSize);
        specialBmpFile.write(reinterpret_cast<char*>(dibHeader), dibHeaderSize);
        for (int y = frameData.height - 1; y >= 0; --y) {
            for (int x = 0; x < frameData.width; ++x) {
                uint8_t pixelValue = frameData.special_pixels[y * frameData.width + x];

                uint8_t red = 0, green = 0, blue = 0, alpha = 0;
                
                if (pixelValue >= 0x00 && pixelValue <= 0x0F) {
                    red = 255;
                    alpha = (pixelValue == 0x00) ? 0 : (pixelValue * 255 / 0x0F);
                }
                else if (pixelValue >= 0x10 && pixelValue <= 0x1F) {
                    green = 255;
                    alpha = ((pixelValue - 0x10) == 0x00) ? 0 : ((pixelValue - 0x10) * 255 / 0x0F);
                }
                else if (pixelValue >= 0x20 && pixelValue <= 0x2F) {
                    blue = 255;
                    alpha = ((pixelValue - 0x20) == 0x00) ? 0 : ((pixelValue - 0x20) * 255 / 0x0F);
                }

                // Write in ARGB format
                specialBmpFile.put(blue);//blue
                specialBmpFile.put(green);
                specialBmpFile.put(red);
                specialBmpFile.put(alpha);
            }

            // Padding to ensure row size is a multiple of 4
            for (int p = 0; p < rowSize - (frameData.width * (bitsPerPixel / 8)); ++p) {
                specialBmpFile.put(0);
            }
        }

        specialBmpFile.close();
    }
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
        currentfd.data.resize(currentfd.width * currentfd.height, 0);  // Initialize frameData.data with zeros
        currentfd.special_pixels.resize(currentfd.width * currentfd.height, 0);  // Initialize special_pixels with zeros

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

        int current_index = 0;  // Keep track of where we are in the frame data

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
                    if (outputIndex < currentfd.data.size()) {
                        if (value >= 0x30) {
                            
                            currentfd.data[outputIndex] = value;  // If value >= 0x30, store it in data
                        }
                        else {
                            
                            currentfd.special_pixels[outputIndex] = value;  // Otherwise, store it in special_pixels
                        }
                    }
                }

                // Update the index_start for the next insertion
                index_start += d;  // Move index_start by d bytes
                remainingB -= 2;  // Decrease remaining by 2 for c and d, and by d for the data bytes
            }
            current_index += currentfd.width;  // Move to the next row
        }

        framesData.push_back(currentfd);  // Add the processed frame to the framesData
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
        //std::cout << std::hex << PALfile.size() << " " << palFile.tellg() << std::endl;
        // Read colors in the current list
        for (uint32_t i = 0; i < colorListSize; ++i) {
            
            uint8_t a, r, g, b; // Using ARGB format
            palFile.read(reinterpret_cast<char*>(&r), 1);
            palFile.read(reinterpret_cast<char*>(&g), 1);
            palFile.read(reinterpret_cast<char*>(&b), 1);
            palFile.read(reinterpret_cast<char*>(&a), 1);
            if (a != 0)
                std::cout << std::hex << (int)a << std::endl;
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


void ImageConverter::dumpAllBMP(bool skifile) {
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
        //writePaletteAsBMP(singlePalette, paletteOutputName);
    }

    // Existing logic for dumping all BMP frames
    for (int ip = 0; ip < selectedPalettes.size(); ip++) {
        
        if (selectedPalettes.size() == 1) {

            for (auto fd : framesData) {
                std::string output_name = (outputDir / (currentFileName + "_frame_" + std::to_string(frame_data_cnt) + "_palette_" + std::to_string(ip) + ".bmp")).string();
                createBMP(fd, output_name, selectedPalettes[ip], skifile); // Assuming createBMP still handles frame data.
                frame_data_cnt++;
            }


        }
        else {
            std::string output_name = (outputDir / (currentFileName + "_frame_" + std::to_string(frame_data_cnt) + "_palette_" + std::to_string(ip) + ".bmp")).string();
            createBMP(framesData[0], output_name, selectedPalettes[ip],skifile); // Assuming createBMP still handles frame data.


        }
        
        
        
    }
}

const int BMP_HEADER_SIZE = 54; // BMP file header is 54 bytes

// Helper function to read a 4-byte integer from a binary stream
uint32_t readUint32(std::ifstream& stream) {
    uint8_t buffer[4];
    stream.read(reinterpret_cast<char*>(buffer), 4);
    return (buffer[3] << 24) | (buffer[2] << 16) | (buffer[1] << 8) | buffer[0];
}

// Helper function to read a 2-byte integer from a binary stream
uint16_t readUint16(std::ifstream& stream) {
    uint8_t buffer[2];
    stream.read(reinterpret_cast<char*>(buffer), 2);
    return (buffer[1] << 8) | buffer[0];
}

float colorDistance(const Color c1, const Color c2) {
    return std::sqrt(
        std::pow(c1.r - c2.r, 2) +
        std::pow(c1.g - c2.g, 2) +
        std::pow(c1.b - c2.b, 2) +
        std::pow(c1.a - c2.a, 2)
    );
}

// Function to find the closest color in the palette
int findClosestPaletteIndex(const Color& bmpColor, const std::vector<Color>& selectedPalette) {
    int closestIndex = 0;
    float minDistance = std::numeric_limits<float>::max();

    for (int i = 0; i < selectedPalette.size(); ++i) {
        float dist = colorDistance(bmpColor, selectedPalette[i]);
        if (dist < minDistance) {
            minDistance = dist;
            closestIndex = i;
        }
    }

    return closestIndex;
}
void ImageConverter::BMPto256(std::string BMPfile) {

    for (auto p : selectedPalettes) {
        std::ifstream bmpStream(BMPfile, std::ios::binary);

        if (!bmpStream.is_open()) {
            std::cerr << "Failed to open BMP file: " << BMPfile << std::endl;
            return;
        }

        // Skip to the width and height of the BMP file (located at offset 18 and 22 respectively)
        bmpStream.seekg(18, std::ios::beg);

        // Read the width and height of the BMP (both are 32-bit integers)
        uint32_t width = readUint32(bmpStream);
        uint32_t height = readUint32(bmpStream);

        // Determine if the BMP has an alpha channel (assume 32-bit if present)
        uint16_t bitsPerPixel;
        bmpStream.seekg(28, std::ios::beg);  // Bits per pixel field
        bmpStream.read(reinterpret_cast<char*>(&bitsPerPixel), sizeof(bitsPerPixel));
        bool hasAlpha = (bitsPerPixel == 32);  // BMP is 32-bit if it has an alpha channel

        // Skip the rest of the BMP header to reach pixel data (54 bytes for BMP header)
        bmpStream.seekg(BMP_HEADER_SIZE, std::ios::beg);

        // The BMP data is typically stored in rows that are padded to 4-byte boundaries (for RGB, BGR, or RGBA)
        int bytesPerPixel = hasAlpha ? 4 : 3;  // 4 bytes per pixel for RGBA, 3 bytes for RGB
        int rowPadding = (4 - (width * bytesPerPixel % 4)) % 4;  // Each row is padded to a multiple of 4 bytes

        // Output .256 file
        std::string outputFileName = BMPfile.substr(0, BMPfile.find_last_of('.')) + ".256";
        std::ofstream out256(outputFileName, std::ios::binary);
        if (!out256.is_open()) {
            std::cerr << "Failed to create .256 file: " << outputFileName << std::endl;
            return;
        }

        // Write width and height as 32-bit little-endian integers to the .256 file
        out256.put(static_cast<uint8_t>(width & 0xFF));
        out256.put(static_cast<uint8_t>((width >> 8) & 0xFF));
        out256.put(static_cast<uint8_t>((width >> 16) & 0xFF));
        out256.put(static_cast<uint8_t>((width >> 24) & 0xFF));

        out256.put(static_cast<uint8_t>(height & 0xFF));
        out256.put(static_cast<uint8_t>((height >> 8) & 0xFF));
        out256.put(static_cast<uint8_t>((height >> 16) & 0xFF));
        out256.put(static_cast<uint8_t>((height >> 24) & 0xFF));

        // Process BMP pixel data, reading from the last row (bottom) to the first row (top)
        for (int row = height - 1; row >= 0; --row) {  // Start from the last row
            bmpStream.seekg(BMP_HEADER_SIZE + (row * (width * bytesPerPixel + rowPadding)), std::ios::beg);

            for (int col = 0; col < width; ++col) {
                // Read BMP pixel data (BGR or BGRA)
                uint8_t r, g, b, a = 255;  // Alpha defaults to 255 if not present

                bmpStream.read(reinterpret_cast<char*>(&b), 1);  // BMP stores as BGR (or BGRA)
                bmpStream.read(reinterpret_cast<char*>(&g), 1);
                bmpStream.read(reinterpret_cast<char*>(&r), 1);
                if (hasAlpha) {
                    bmpStream.read(reinterpret_cast<char*>(&a), 1);  // Read alpha channel if present
                }

                Color bmpColor(r, g, b, a);

                // Find the closest palette index for the current BMP pixel
                int closestIndex = findClosestPaletteIndex(bmpColor, p);

                // Write the closest palette index with offset to the .256 file
                uint8_t finalIndex = static_cast<uint8_t>(closestIndex + (0x100 - p.size()));
                out256.put(finalIndex);
            }
        }

        // Close both files
        bmpStream.close();
        out256.close();

        std::cout << "Converted " << BMPfile << " to " << outputFileName << " using selected palette." << std::endl;
    }
}
// Process the special BMP pixel
uint8_t processSpecialPixel(const Color& pixel) {
    uint8_t transparency = static_cast<uint8_t>((pixel.a / 255.0f) * 15); // Transparency level (0 to 15)

    if (pixel.r > 0 && pixel.g == 0 && pixel.b == 0) {
        return 0x00 + transparency; // Red channel
    }
    else if (pixel.g > 0 && pixel.r == 0 && pixel.b == 0) {
        return 0x10 + transparency; // Green channel
    }
    else if (pixel.b > 0 && pixel.r == 0 && pixel.g == 0) {
        return 0x20 + transparency; // Blue channel
    }
    else if (transparency == 0)
        return 0x00; // Fully transparent, no overlay
    else {
        if (pixel.r > pixel.g && pixel.r > pixel.b)
            return 0x00 + transparency; // Red channel
        else if (pixel.g > pixel.r && pixel.g > pixel.b)
            return 0x10 + transparency; // Red channel
        else if (pixel.b > pixel.r && pixel.b > pixel.g)
            return 0x20 + transparency; // Red channel
        else 
            return 0x00;
    }
}
void ImageConverter::insertBMPPairIntoSKI(const std::string& normalBMPPath, const std::string& specialBMPPath) {
    // Open the normal BMP file
    std::ifstream normalStream(normalBMPPath, std::ios::binary);
    if (!normalStream.is_open()) {
        std::cerr << "Failed to open normal BMP file: " << normalBMPPath << std::endl;
        return;
    }

    // Open the special BMP file
    std::ifstream specialStream(specialBMPPath, std::ios::binary);
    if (!specialStream.is_open()) {
        std::cerr << "Failed to open special BMP file: " << specialBMPPath << std::endl;
        normalStream.close();
        return;
    }

    // Read the width and height of both BMPs (assume both are the same size)
    normalStream.seekg(18, std::ios::beg);
    uint32_t width = readUint32(normalStream);
    uint32_t height = readUint32(normalStream);

    specialStream.seekg(18, std::ios::beg);
    uint32_t specialWidth = readUint32(specialStream);
    uint32_t specialHeight = readUint32(specialStream);

    if (width != specialWidth || height != specialHeight) {
        std::cerr << "Error: Normal and special BMP dimensions do not match." << std::endl;
        normalStream.close();
        specialStream.close();
        return;
    }

    // Check bits per pixel for both images
    uint16_t bitsPerPixelNormal, bitsPerPixelSpecial;
    normalStream.seekg(28, std::ios::beg);
    normalStream.read(reinterpret_cast<char*>(&bitsPerPixelNormal), sizeof(bitsPerPixelNormal));
    bool normalHasAlpha = (bitsPerPixelNormal == 32); // BMP is 32-bit if it has an alpha channel

    specialStream.seekg(28, std::ios::beg);
    specialStream.read(reinterpret_cast<char*>(&bitsPerPixelSpecial), sizeof(bitsPerPixelSpecial));
    bool specialHasAlpha = (bitsPerPixelSpecial == 32);

    // Skip to pixel data for both BMPs
    normalStream.seekg(0xA, std::ios::beg);
    specialStream.seekg(0xA, std::ios::beg);
    unsigned int offset_start_data;
    unsigned int offset_start_data_special;
    normalStream.read(reinterpret_cast<char*>(&offset_start_data), sizeof(offset_start_data));
    specialStream.read(reinterpret_cast<char*>(&offset_start_data_special), sizeof(offset_start_data_special));

    normalStream.seekg(offset_start_data, std::ios::beg);
    specialStream.seekg(offset_start_data_special, std::ios::beg);

    int bytesPerPixelNormal = normalHasAlpha ? 4 : 3; // 4 bytes per pixel for RGBA, 3 for RGB
    int bytesPerPixelSpecial = specialHasAlpha ? 4 : 3;
    int rowPaddingNormal = (4 - (width * bytesPerPixelNormal % 4)) % 4; // Row padding to 4-byte boundary
    int rowPaddingSpecial = (4 - (width * bytesPerPixelSpecial % 4)) % 4;

    // Output vector for merged image
    frameData mergedImage;
    mergedImage.width = width;
    mergedImage.height = height;
    mergedImage.data = std::vector<uint8_t>(width * height, 0);
    
    // Process pixel data row by row (from bottom to top for BMP format)
    for (int row = height - 1; row >= 0; --row) {
        
        unsigned int normal_index = offset_start_data + (row * (width * bytesPerPixelNormal + rowPaddingNormal));
        unsigned int special_index = offset_start_data_special + (row * (width * bytesPerPixelSpecial + rowPaddingSpecial));

        normalStream.seekg(normal_index, std::ios::beg);
        specialStream.seekg(special_index, std::ios::beg);
        //std::cout << std::hex << " starting normal at " << normal_index << " " << special_index << std::endl;
        for (int col = 0; col < width; ++col) {
            // Normal image pixel (read RGB or RGBA)
            uint8_t rNormal, gNormal, bNormal, aNormal = 255;
            normalStream.read(reinterpret_cast<char*>(&bNormal), 1); // BMP stores BGR(A)
            normalStream.read(reinterpret_cast<char*>(&gNormal), 1);
            normalStream.read(reinterpret_cast<char*>(&rNormal), 1);
            if (normalHasAlpha) {
                normalStream.read(reinterpret_cast<char*>(&aNormal), 1);
            }
            Color normalPixel(rNormal, gNormal, bNormal, aNormal);

            // Special image pixel (read RGB or RGBA)
            uint8_t rSpecial, gSpecial, bSpecial, aSpecial = 255;
            specialStream.read(reinterpret_cast<char*>(&bSpecial), 1);
            specialStream.read(reinterpret_cast<char*>(&gSpecial), 1);
            specialStream.read(reinterpret_cast<char*>(&rSpecial), 1);
            if (specialHasAlpha) {
                specialStream.read(reinterpret_cast<char*>(&aSpecial), 1);
            }
            
            Color specialPixel(rSpecial, gSpecial, bSpecial, aSpecial);

            // Convert normal pixel to palette index
            int paletteIndex = findClosestPaletteIndex(normalPixel, selectedPalettes[0]);
            mergedImage.data[row * width + col] = static_cast<uint8_t>(paletteIndex);

            // Overlay special image pixel if applicable
            uint8_t specialValue = processSpecialPixel(specialPixel);
            if ((specialValue & 0x0F) != 0) {
                mergedImage.data[row * width + col] = specialValue;
            }
        }
    }

    // Close the streams
    normalStream.close();
    specialStream.close();

    // Store the merged image for further processing
    framesData.push_back(mergedImage); // Store frame's data for SKI repacking
}
void ImageConverter::repackIntoSKI(const std::string& outputSKIPath) {
    std::ofstream skiFile(outputSKIPath + ".SKI", std::ios::binary);
    if (!skiFile.is_open()) {
        std::cerr << "Error: Unable to create SKI file " << outputSKIPath << std::endl;
        return;
    }

    // Number of frames (write in 4 bytes)
    uint32_t numFrames = static_cast<uint16_t>(framesData.size());
    skiFile.write(reinterpret_cast<char*>(&numFrames), sizeof(uint16_t));

    // Placeholder for frame addresses (we'll come back to fill this later)
    std::vector<uint32_t> frameAddresses(numFrames, 0);
    uint32_t addressTableOffset = static_cast<uint32_t>(skiFile.tellp());
    skiFile.write(reinterpret_cast<char*>(frameAddresses.data()), sizeof(uint32_t) * numFrames);
    
    for (size_t frameIndex = 0; frameIndex < framesData.size(); ++frameIndex) {
        // Save the current position to point to the frame header
        std::streamoff frameHeaderOffset = skiFile.tellp();

        const auto& frame = framesData[frameIndex];
        uint16_t width = frame.width;
        uint16_t height = frame.height;

        // Frame starts here: update the frameAddresses to point to the frame header
        frameAddresses[frameIndex] = static_cast<uint32_t>(frameHeaderOffset - (addressTableOffset + numFrames * sizeof(uint32_t)));

        // Write frame header: {0, width, height, 0, 0, 0, 0}
        uint32_t zero32 = 0;
        uint16_t zero16 = 0x0;
        uint16_t position = 0; // ¨¤ r¨¦cup depuis le nom du fichier, ne pas oublier!!!
        skiFile.write(reinterpret_cast<char*>(&zero32), sizeof(uint32_t));  // 32-bit 0
        skiFile.write(reinterpret_cast<char*>(&width), sizeof(uint16_t));   // width
        skiFile.write(reinterpret_cast<char*>(&height), sizeof(uint16_t));  // height
        skiFile.write(reinterpret_cast<char*>(&position), sizeof(uint16_t));  // 16-bit 0
        for (int i = 0; i < 2; ++i) {
            skiFile.write(reinterpret_cast<char*>(&zero16), sizeof(uint16_t));  // 2 zeros
        }
        uint16_t idkwhatthatis = 0x200;
        skiFile.write(reinterpret_cast<char*>(&idkwhatthatis), sizeof(uint16_t));
        // Reserve space for the row index array (two 32-bit values per row)
        std::vector<std::pair<uint32_t, uint32_t>> rowIndexArray(height);
        for (int i = 0; i < rowIndexArray.size(); i++)
            rowIndexArray[i].first = -1;

        std::streamoff rowIndexOffset = skiFile.tellp();
        skiFile.seekp(rowIndexOffset + height * sizeof(uint32_t) * 2);  // Reserve space for row index array

        // Part 3: Compressed row data
        unsigned int addr_compressed_row = 0;
        
        std::streamoff rowDataOffset = skiFile.tellp();
        for (int row = 0; row < height; ++row) {
            std::streamoff rowStartPos = skiFile.tellp(); // Mark the start of the compressed row
            rowIndexArray[row].second = 0;
            bool rowIsEmpty = true;  // Track if the row is full of zeros
            int col = 0;
            unsigned int zeros = 0;
            uint32_t compressedSize = 0;
            int startColumn = 0;
            while (col < width) {
                // Find the next pack of non-zero pixels
                size_t nb_zeros = 0;
                while (col < width && frame.data[(height - 1 - row) * width + col] == 0) {
                    ++col;  // Skip zero pixels
                    nb_zeros++;
                }

                if (col >= width) {
                    // No more non-zero pixels in this row
                    break;
                }
                else {
                    unsigned int remaining_nb_zeros = nb_zeros;
                    while (remaining_nb_zeros > 0xFF) {
                        size_t nb_ff_in_width = (int)(width / 0xFF);
                        size_t remaining = width % 0xFF;
                        skiFile.put(0xFF);
                        skiFile.put(0x00);
                        remaining_nb_zeros -= 0xFF;
                        rowIndexArray[row].second += 2;
                        if (rowIndexArray[row].first == -1) {
                            rowIndexArray[row].first = addr_compressed_row;
                            rowIndexArray[row].second = 2;
                        }
                        else {
                            // Accumulate the total size if there are multiple packs
                            rowIndexArray[row].second += 2;
                        }
                        addr_compressed_row += compressedSize + 2;
                    }
                    startColumn += remaining_nb_zeros;
                
                }

                rowIsEmpty = false;  // This row contains non-zero data

                // Start of a new non-zero pack
                
                std::vector<uint8_t> compressedRowData;
                unsigned int remaining_bytes = 0;
                // Count non-zero pixels and collect the data
                while (col < width && frame.data[(height - 1 - row) * width + col] != 0) {
                    compressedRowData.push_back(frame.data[(height - 1 - row) * width + col]);
                    ++col;
                    remaining_bytes++;
                    if (compressedRowData.size() == 0xFF) {
                        remaining_bytes -= 0xFF;
                        int pixelCount = static_cast<int>(compressedRowData.size());
                        // Write the compressed row data for this pack
                        skiFile.put(static_cast<uint8_t>(startColumn));  // Starting column
                        skiFile.put(static_cast<uint8_t>(pixelCount));   // Number of non-zero pixels
                        startColumn = 0;
                        skiFile.write(reinterpret_cast<char*>(compressedRowData.data()), pixelCount);
                        // The offset and size for this pack
                        std::streamoff currentPos = skiFile.tellp();
                        uint32_t rowOffset = static_cast<uint32_t>(currentPos - rowStartPos);  // Offset from the start of this row's data
                        compressedSize = static_cast<uint32_t>(pixelCount);  // Include 2 bytes for start column and count

                        // If it's the first pack in this row, initialize rowIndexArray[row] with this info
                        if (rowIndexArray[row].first == -1) {
                            rowIndexArray[row].first = addr_compressed_row;
                            rowIndexArray[row].second = 2;
                        }
                        else {
                            // Accumulate the total size if there are multiple packs
                            rowIndexArray[row].second += 2;
                        }
                        addr_compressed_row += compressedSize + 2;
                        compressedRowData.clear();
                    
                    }
                }

                int pixelCount = static_cast<int>(compressedRowData.size());

               
                // Write the compressed row data for this pack
                skiFile.put(static_cast<uint8_t>(startColumn));  // Starting column
                skiFile.put(static_cast<uint8_t>(pixelCount));   // Number of non-zero pixels
                startColumn = 0;
                //startColumn += compressedRowData.size();
                skiFile.write(reinterpret_cast<char*>(compressedRowData.data()), pixelCount);

                // The offset and size for this pack
                std::streamoff currentPos = skiFile.tellp();
                compressedSize = static_cast<uint32_t>(pixelCount); 

                // If it's the first pack in this row, initialize rowIndexArray[row] with this info
                if (rowIndexArray[row].first == -1) {
                    rowIndexArray[row].first = addr_compressed_row;
                    rowIndexArray[row].second = 2;
                }
                else {
                    // Accumulate the total size if there are multiple packs
                    rowIndexArray[row].second += 2;
                }
                addr_compressed_row += compressedSize + 2;
            }

            // Handle empty rows by writing the 0xFF 0x00 sequence
            if (rowIsEmpty) {
                size_t nb_ff_in_width = (int)(width / 0xFF);
                size_t remaining =  width % 0xFF;
                for (int i = 0; i < nb_ff_in_width; i++) {
                    skiFile.put(0xFF);
                    skiFile.put(0x00);
                    rowIndexArray[row].second+=2;
                    compressedSize += 2;
                }
                skiFile.put(remaining);  // 0xFF indicates a full zero row
                compressedSize++;
                // Record the offset and size for this empty row

                rowIndexArray[row].first = addr_compressed_row;  // Save the offset for this row
                
                addr_compressed_row += compressedSize;
            }
            rowIndexArray[row].second++;

            
        }
        
        // Write the row index array
        std::streamoff endOfRowData = skiFile.tellp();
        skiFile.seekp(rowIndexOffset);  // Move to the reserved space for the row index array
        for (const auto& rowPair : rowIndexArray) {
            skiFile.write(reinterpret_cast<const char*>(&rowPair.first), sizeof(uint32_t));  // Row offset
            skiFile.write(reinterpret_cast<const char*>(&rowPair.second), sizeof(uint32_t)); // Total compressed size
        }
        skiFile.seekp(endOfRowData);  // Return to the end of the row data section
        unsigned int padding = skiFile.tellp() % 4;
        for (int i = 0; i < padding; i++) {
            skiFile.put(0xcc);
        }
    }

    // Go back and write the frame addresses relative to the end of the address table
    skiFile.seekp(addressTableOffset);
    skiFile.write(reinterpret_cast<char*>(frameAddresses.data()), sizeof(uint32_t) * numFrames);

    skiFile.close();
    std::cout << "Successfully repacked into SKI file: " << outputSKIPath << std::endl;
}




void ImageConverter::BMPtoSKI(const std::string& folderPath) {
    fs::path folder(folderPath);
    std::vector<std::string> normalFiles;
    std::vector<std::string> specialFiles;

    // Iterate through files and separate normal and special BMP files
    for (const auto& entry : fs::directory_iterator(folder)) {
        if (entry.is_regular_file()) {
            std::string fileName = entry.path().filename().string();
            std::string extension = entry.path().extension().string();
            std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);

            if (extension == ".bmp") {
                if (fileName.find("_special.bmp") != std::string::npos) {
                    specialFiles.push_back(entry.path().string()); // Special BMP files
                }
                else {
                    normalFiles.push_back(entry.path().string());  // Normal BMP files
                }
            }
        }
    }

    // Sort files by name to match corresponding normal and special pairs
    std::sort(normalFiles.begin(), normalFiles.end());
    std::sort(specialFiles.begin(), specialFiles.end());

    if (normalFiles.size() != specialFiles.size()) {
        std::cerr << "Mismatch between normal and special BMP file counts." << std::endl;
        return;
    }

    // Iterate through normal and special BMP files and convert each pair
    for (size_t i = 0; i < normalFiles.size(); ++i) {
        std::string normalFile = normalFiles[i];
        std::string specialFile = specialFiles[i];

        std::cout << "Processing SKI frame: Normal BMP = " << normalFile
            << ", Special BMP = " << specialFile << std::endl;

        // Logic to insert paired BMP files into SKI format
        insertBMPPairIntoSKI(normalFile, specialFile); // This function should handle pairing
    }

    // Once all BMP files are processed, repack them into the SKI archive
    repackIntoSKI(folderPath); // Logic to repack everything into the SKI archive
}

