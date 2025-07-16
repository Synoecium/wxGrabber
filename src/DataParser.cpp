#include "../include/DataParser.h"
#include "../include/log.h"
#include <iostream>
#include <filesystem>

uint32_t DataParser::DataObject::nextUID = 0;

uint32_t DataParser::readBigEndian32(std::ifstream &file) {
    uint32_t value = 0;
    file.read(reinterpret_cast<char*>(&value), sizeof(value));
    return (value >> 24) | ((value >> 8) & 0x0000FF00) | ((value << 8) & 0x00FF0000) | (value << 24);
}

uint32_t DataParser::readBigEndian32(std::vector<uint8_t> &buffer, size_t& offset) {
    uint32_t value;
    value = (buffer[offset] << 24) | (buffer[offset + 1] << 16) | (buffer[offset + 2] << 8) | buffer[offset + 3];
    offset += 4;
    return value;
}

std::string DataParser::BigEndian32ToSring(uint32_t value) {
    std::string str;
    str.resize(4);
    int32_t InvertedValue = (value >> 24) | ((value >> 8) & 0x0000FF00) | ((value << 8) & 0x00FF0000) | (value << 24);
    std::memcpy(str.data(), &InvertedValue, 4);
    return str;
}

bool DataParser::ParseDataObjects(std::vector<uint8_t> &buffer, std::vector<std::shared_ptr<DataObject>> &objects) {
    size_t offset = 0;
    uint32_t objectCount = readBigEndian32(buffer, offset);

    objects.clear();
    objects.reserve(objectCount);

    for (uint32_t i = 0; i < objectCount; ++i) {
        DataObject obj;
        obj.propertyOrder.clear();  // Ensure property order vector is empty

        // Read properties
        while (true) {
            uint32_t propMagic = readBigEndian32(buffer, offset);
            if (propMagic != 'prop') 
            {
                offset -= 4;    // Go back to the start of the property
                break;          // Exit if no more properties
            }

            uint32_t propTypeID = readBigEndian32(buffer, offset);
            uint32_t propSize = readBigEndian32(buffer, offset);
            std::vector<char> propData(propSize);
            memcpy(propData.data(), buffer.data() + offset, propSize);
            offset += propSize;

            // Store property value and add to order list
            obj.properties[propTypeID] = std::string(propData.data(), propSize);
            obj.propertyOrder.push_back(propTypeID);  // Store order as properties are read
            if (propTypeID == 'NAME') {
                obj.name = obj.properties[propTypeID];
            }
        }

        // Read object type ID
        obj.typeID = static_cast<ObjectType>(readBigEndian32(buffer, offset));
        uint32_t compressedSize = readBigEndian32(buffer, offset);
        int32_t uncompressedSize = readBigEndian32(buffer, offset);     // can be negative, that means compressed

        // Create temporary buffer for object data
        std::vector<uint8_t> objData(compressedSize);
        memcpy(objData.data(), buffer.data() + offset, compressedSize);
        offset += compressedSize;

        // Process the object based on its type
        switch (obj.typeID) {
            case DAT_FILE: {
                // Parse nested objects recursively
                std::vector<std::shared_ptr<DataObject>> nestedObjects;
                if (ParseDataObjects(objData, nestedObjects)) {
                    obj.data = nestedObjects;
                } else {
                    obj.data = objData; // Store as raw data if parsing fails
                }
                break;
            }
            case DAT_BITMAP:
            case DAT_RLE_SPRITE:
            case DAT_C_SPRITE:
            case DAT_XC_SPRITE:
            case DAT_PALETTE: {
                // Try to parse as bitmap or palette
                BitmapData bmpData;
                if (BitmapData::parse(objData, obj.typeID, bmpData)) {
                    obj.data = std::move(bmpData);
                } else {
                    obj.data = objData; // Store as raw data if parsing fails
                }
                break;
            }
            case DAT_OGG: 
            case DAT_SAMP: 
            case DAT_MIDI: {
                // Try to parse as audio
                AudioData audioData;
                if (AudioData::parse(objData, obj.typeID, audioData)) {
                    obj.data = audioData;
                } else {
                    obj.data = objData; // Store as raw data if parsing fails
                }
                break;
            }
            case DAT_FLI: {
                // Try to parse as video (FLIC)
                VideoData videoData;
                if (VideoData::parse(objData, obj.typeID, videoData)) {
                    obj.data = videoData;
                } else {
                    obj.data = objData; // Store as raw data if parsing fails
                }
                break;
            }
            case DAT_FONT: {
                logDebug("Parsing font data");
                FontData fontData;
                if (FontData::parse(objData, obj.typeID, fontData)) {
                    logDebug("Font data parsed successfully");
                    obj.data = fontData;
                } else {
                    obj.data = objData; // Store as raw data if parsing fails
                }
                break;
            }
            case DAT_INFO: {
                obj.data = objData;
                break;
            }
            default:
                obj.data = objData;
                logError("Unknown object type: " + ConvertIDToString(obj.typeID));
        }

        objects.push_back(std::make_shared<DataObject>(std::move(obj)));
    }

    return true;
}

std::string DataParser::ConvertIDToString(const uint32_t id) {
    return BigEndian32ToSring(id);
}

std::string DataParser::ConvertIDToHexString(const uint32_t id) {
    std::stringstream ss;
    ss << std::hex << id;
    return ss.str();
}

std::vector<uint8_t> DataParser::ReadPackfile(const std::string& inputFilename, const std::string& password) {
    std::ifstream inputFile(inputFilename, std::ios::binary);
    if (!inputFile) {
        logError("Error opening input file!");
        return {};
    }

    // Get file size
    inputFile.seekg(0, std::ios::end);
    size_t fileSize = inputFile.tellg();
    inputFile.seekg(0, std::ios::beg);

    if (fileSize < 4) {
        logError("File too small to be valid!");
        return {};
    }

    // Read entire file into buffer
    std::vector<uint8_t> inputBuffer(fileSize);
    inputFile.read(reinterpret_cast<char*>(inputBuffer.data()), fileSize);
    
    if (inputFile.gcount() != fileSize) {
        logError("Error reading file!");
        return {};
    }

    // Read magic number
    uint32_t magic = (inputBuffer[0] << 24) | (inputBuffer[1] << 16) | 
                    (inputBuffer[2] << 8) | inputBuffer[3];

    if (!password.empty()) {
        magic = encrypt_id(magic, password, true);
        encryptBuffer(inputBuffer, password, 4);
        logDebug("Encrypted buffer with password: " + password);
    }

    // Check if the data is compressed
    std::vector<uint8_t> decompressedData;
    if (magic == F_PACK_MAGIC) {
        // Compressed data - decompress starting after magic number
        decompressedData = LZSS::Decompress(std::vector<uint8_t>(inputBuffer.begin() + 4, inputBuffer.end()));
    } else if (magic == F_NOPACK_MAGIC) {
        // Uncompressed data - return everything after magic number
        decompressedData.assign(inputBuffer.begin() + 4, inputBuffer.end());
    } else {
        logError("Invalid packfile magic number!");
        return {};
    }

    // Check DAT magic number
    if (decompressedData.size() < 4) {
        logError("Decompressed data too small!");
        return {};
    }

    magic = (decompressedData[0] << 24) | (decompressedData[1] << 16) | 
            (decompressedData[2] << 8) | decompressedData[3];
    logDebug("Magic number: " + ConvertIDToString(magic) + " hex value: " + ConvertIDToHexString(magic));
    if (magic != DAT_MAGIC) {
        logError("Invalid DAT magic number!");
        return {};
    }

    // Return data after DAT magic
    return std::vector<uint8_t>(decompressedData.begin() + 4, decompressedData.end());
}

std::pair<bool, bool> DataParser::LoadPackfile(const std::string& inputFilename, std::vector<std::shared_ptr<DataObject>> &objects, const std::string& password) {
    // Check if file exists and get magic number
    std::ifstream inputFile(inputFilename, std::ios::binary);
    if (!inputFile) {
        logError("Failed to open file: " + inputFilename);
        return {false, false};
    }

    // Read magic number
    uint32_t magic;
    inputFile.read(reinterpret_cast<char*>(&magic), sizeof(magic));
    inputFile.close();

    // Convert to big endian
    magic = (magic >> 24) | ((magic >> 8) & 0x0000FF00) | ((magic << 8) & 0x00FF0000) | (magic << 24);
    logDebug("Magic number: " + ConvertIDToString(magic) + " hex value: " + ConvertIDToHexString(magic));
    if (!password.empty()) {
        std::stringstream ss;
        for (size_t i = 0; i < password.length(); i++) {
            ss << std::hex << static_cast<uint32_t>(password[i]);
        }
        std::string hexPassword = ss.str();
        logDebug("Decrypting magic number with password: " + password + " hex password: " + hexPassword);
        // decrypt the magic number
        uint32_t newMagic = encrypt_id(magic, password, true);
        logDebug("Decrypted magic number (new format): " + ConvertIDToString(newMagic) + " hex value: " + ConvertIDToHexString(newMagic));
        magic = newMagic;
    }

    bool isCompressed = (magic == F_PACK_MAGIC);

    // Read the packfile
    auto buffer = ReadPackfile(inputFilename, password);
    if (buffer.empty()) {
        logError("Failed to read packfile: " + inputFilename);
        return {false, isCompressed};
    }

    // Parse the objects
    if (!ParseDataObjects(buffer, objects)) {
        logError("Failed to parse objects from packfile: " + inputFilename);
        return {false, isCompressed};
    }

    return {true, isCompressed};
}

void DataParser::writeBigEndian32(std::vector<uint8_t>& buffer, uint32_t value) {
    buffer.push_back(static_cast<uint8_t>((value >> 24) & 0xFF));
    buffer.push_back(static_cast<uint8_t>((value >> 16) & 0xFF));
    buffer.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
    buffer.push_back(static_cast<uint8_t>(value & 0xFF));
}

bool DataParser::WritePackfile(const std::string& outputFilename, const std::vector<uint8_t>& objectsBuffer, bool useCompression, const std::string& password) {
    try {
        std::vector<uint8_t> fileBuffer;

        // Create a new buffer for the file
        fileBuffer.reserve(objectsBuffer.size() + 4); // Reserve space
        
        // Write the magic number which depends on the compression flag
        uint32_t magic = useCompression ? F_PACK_MAGIC : F_NOPACK_MAGIC;
        
        // Encrypt the magic number if password is provided
        if (!password.empty()) {
            magic = encrypt_id(magic, password, true);
            logDebug("Encrypting magic number with password: " + password);
        }
        
        writeBigEndian32(fileBuffer, magic);

        // Compress the data if requested
        std::vector<uint8_t> dataToWrite;
        if (useCompression) {
            // The DecompressData function expects the compressed data directly after the magic number
            dataToWrite = LZSS::Compress(objectsBuffer);
        } else {
            // For uncompressed data, just use the buffer
            dataToWrite = objectsBuffer;
        }
        
        // Encrypt the data if password is provided
        if (!password.empty()) {
            encryptBuffer(dataToWrite, password, 0);
            logDebug("Encrypted data buffer with password: " + password);
        }
        
        fileBuffer.insert(fileBuffer.end(), dataToWrite.begin(), dataToWrite.end());

        // Write buffer to file
        std::ofstream outFile(outputFilename, std::ios::binary);
        if (!outFile) {
            logError("Failed to open output file: " + outputFilename);
            return false;
        }
        
        outFile.write(reinterpret_cast<const char*>(fileBuffer.data()), fileBuffer.size());
        return outFile.good();
    } catch (const std::exception& e) {
        logError("Error while writing file: " + std::string(e.what()));
        return false;
    }
}

bool DataParser::SavePackfile(const std::string& outputFilename, const std::vector<std::shared_ptr<DataObject>>& objects, bool createBackup, bool useCompression, const std::string& password) {
    // Create backup if requested
    if (createBackup && std::filesystem::exists(outputFilename)) {
        std::string backupPath = outputFilename + ".bak";
        try {
            logInfo("Creating backup: " + backupPath);
            if (std::filesystem::exists(backupPath)) {
                logInfo("Removing existing backup file");
                std::filesystem::remove(backupPath);
            }
            std::filesystem::copy_file(outputFilename, backupPath, std::filesystem::copy_options::overwrite_existing);
            logInfo("Backup created successfully");
        } catch (const std::filesystem::filesystem_error& e) {
            logError("Failed to create backup: " + std::string(e.what()));
            // Continue with save even if backup fails
        }
    }

    // Get the serialized data objects and write DAT magic number
    std::vector<uint8_t> buffer;
    writeBigEndian32(buffer, DAT_MAGIC);
    auto serializedObjects = SerializeDataObjects(objects);
    buffer.insert(buffer.end(), serializedObjects.begin(), serializedObjects.end());

    // Write packfile with or without compression and password
    return WritePackfile(outputFilename, buffer, useCompression, password);
}

std::vector<uint8_t> DataParser::SerializeDataObjects(const std::vector<std::shared_ptr<DataObject>>& objects) {
    // Create a buffer for the file header
    std::vector<uint8_t> buffer;
    buffer.reserve(1024 * 1024); // 1MB initial reservation

    // Write DAT magic number
    //writeBigEndian32(buffer, DAT_MAGIC);

    // Write object count
    uint32_t count = static_cast<uint32_t>(objects.size());
    writeBigEndian32(buffer, count);
    
    // Serialize objects directly into the buffer
    for (const auto& obj : objects) {
        // Write properties with 'prop' magic number for each property in order
        for (const auto& [propId, value] : obj->getOrderedProperties()) {
            // Write property magic
            writeBigEndian32(buffer, 'prop');
            
            // Write property ID
            writeBigEndian32(buffer, propId);
            
            // Write property value size
            uint32_t propSize = static_cast<uint32_t>(value.size());
            writeBigEndian32(buffer, propSize);
            
            // Write property value
            buffer.insert(buffer.end(), value.begin(), value.end());
        }
        
        // Write object type
        writeBigEndian32(buffer, obj->typeID);
        
        // Get data buffer based on object type
        std::vector<uint8_t> dataBuffer;
        
        if (obj->isRawData()) {
            // For raw data, just use the raw data directly
            dataBuffer = obj->getRawData();
        } else if (obj->isBitmap()) {
            // For bitmap data, use the serialize method
            const auto& bmpData = obj->getBitmap();
            dataBuffer = bmpData.serialize();
        } else if (obj->isAudio()) {
            // For audio data, use the serialize method
            const auto& audioData = obj->getAudio();
            dataBuffer = audioData.serialize();
        } else if (obj->isVideo()) {
            // For video data, use the serialize method
            const auto& videoData = obj->getVideo();
            dataBuffer = videoData.serialize();
        } else if (obj->isFont()) {
            // For font data, use the serialize method
            const auto& fontData = obj->getFont();
            dataBuffer = fontData.serialize();
        } else if (obj->isNested()) {
            // For nested objects, recursively serialize them
            const auto& nestedObjects = obj->getNestedObjects();
            dataBuffer = SerializeDataObjects(nestedObjects);
        }
        
        // Write data size (twice for consistency) and data
        uint32_t dataSize = static_cast<uint32_t>(dataBuffer.size());
        writeBigEndian32(buffer, dataSize);
        writeBigEndian32(buffer, dataSize);
        buffer.insert(buffer.end(), dataBuffer.begin(), dataBuffer.end());
    }
    
    return buffer;
}

bool DataParser::DataObject::update(std::string &ErrorMessage, bool ForceUpdate, std::vector<uint8_t>* currentPalette, bool useDithering) {
    // Check if object has ORIG property
    auto origIt = properties.find('ORIG');
    if (origIt == properties.end()) {
        // form ErrorMessage like this: "<name> has no origin data - skipping"
        ErrorMessage = name + " has no origin data - skipping";
        return false;
    }

    // Get the original file path
    std::string origPath = origIt->second;

    // Check if file exists
    if (!std::filesystem::exists(origPath)) {
        // form ErrorMessage like this: "<name>: <path> not found - skipping"
        ErrorMessage = name + ": " + origPath + " not found - skipping";
        return false;
    }

    // check if the file is a valid file
    {
        std::ifstream origFile(origPath, std::ios::binary);
        if (!origFile.good()) {
            ErrorMessage = name + ": " + origPath + " could not be opened - skipping";
            return false;
        }
    }

    // check this type
    if (isBitmap()) {
        // import the bitmap data from the file
        BitmapData bitmap;
        if (!bitmap.importFromFile(origPath, currentPalette, useDithering)) {
            ErrorMessage = name + ": " + origPath + " is not a valid bitmap - skipping";
            return false;
        }
        if (!ForceUpdate) {
            // compare the bitmap data with the data
            if (bitmap == std::get<BitmapData>(data)) {
                ErrorMessage = name + ": " + origPath + " is identical - skipping";
                return false;
            }
        }
        // update the bitmap data
        std::get<BitmapData>(data) = bitmap;
    }
    else if (isAudio()) {
        AudioData audiodata;
        audiodata.typeID = std::get<AudioData>(data).typeID;
        if (!audiodata.importFromFile(origPath)) {
            ErrorMessage = name + ": " + origPath + " is not a valid audio - skipping";
            return false;
        }
        if (!ForceUpdate) {
            if (audiodata == std::get<AudioData>(data)) {
                ErrorMessage = name + ": " + origPath + " is identical - skipping";
                return false;
            }
        }
        // update the audio data
        std::get<AudioData>(data) = audiodata;
    }
    else if (isVideo()) {
        VideoData videoData;
        videoData.typeID = std::get<VideoData>(data).typeID;
        if (!videoData.importFromFile(origPath)) {
            ErrorMessage = name + ": " + origPath + " is not a valid video - skipping";
            return false;
        }
        if (!ForceUpdate) {
            if (videoData == std::get<VideoData>(data)) {
                ErrorMessage = name + ": " + origPath + " is identical - skipping";
                return false;
            }
        }
        // update the video data
        std::get<VideoData>(data) = videoData;
    }
    else if (isFont()) {
        FontData fontData = std::get<FontData>(data);
        if (!fontData.importFromFile(origPath)) {
            ErrorMessage = name + ": " + origPath + " is not a valid font - skipping";
            return false;
        }
        if (!ForceUpdate) {
            if (fontData == std::get<FontData>(data)) {
                ErrorMessage = name + ": " + origPath + " is identical - skipping";
                return false;
            }
        }
        // update the font data
        std::get<FontData>(data) = fontData;
    }
    else if (isNested()) {
        // Read the original file
        std::vector<uint8_t> origFile = ReadPackfile(origPath);
        std::vector<std::shared_ptr<DataObject>> fileObjects;
        if(!ParseDataObjects(origFile, fileObjects)) {
            ErrorMessage = name + ": " + origPath + " is not a valid data file - skipping";
            return false;
        }
        std::vector<std::shared_ptr<DataObject>> nestedObjects = std::get<NestedObjects>(data);
        if (!ForceUpdate) {
            //compare the fileObjects with the nested objects by size and data
            bool isIdentical = true;
            if (fileObjects.size() == nestedObjects.size()) {                
                for (size_t i = 0; i < fileObjects.size(); ++i) {
                    if (*fileObjects[i] != *nestedObjects[i]) {
                        isIdentical = false;
                        break;
                    }
                }
            }
            if (isIdentical) {
                ErrorMessage = name + ": " + origPath + " is identical - skipping";
                return false;
            }
        }
        // set the nested objects to the fileObjects
        data = fileObjects;
    }
    else if (isRawData()) {
        // Read the original file
        std::vector<uint8_t> fileData(std::filesystem::file_size(origPath));
        std::ifstream origFile(origPath, std::ios::binary);
        origFile.read(reinterpret_cast<char*>(fileData.data()), fileData.size());
        origFile.close();
        if (!ForceUpdate) {
            if (memcmp(fileData.data(), std::get<std::vector<uint8_t>>(data).data(), fileData.size()) == 0) {
                ErrorMessage = name + ": " + origPath + " is identical - skipping";
                return false;
            }
        }
        // Update the raw data
        data = fileData;
    }
    else {
        ErrorMessage = name + ": " + origPath + " is not a valid data object - skipping";
        return false;
    }

    // Set the date to the current date with format MM-DD-YYYY, HH:MM
    updateDateProperty();

    return true;
}

bool DataParser::DataObject::operator==(const DataObject& other) const {
    if (typeID != other.typeID) return false;
    if (data.index() != other.data.index()) return false;
    if (properties.size() != other.properties.size()) return false;
    if (propertyOrder.size() != other.propertyOrder.size()) return false;
    if (name != other.name) return false;

    // Compare property order
    for (size_t i = 0; i < propertyOrder.size(); i++) {
        if (propertyOrder[i] != other.propertyOrder[i]) {
            return false;
        }
    }

    // Compare properties
    for (const auto& [key, value] : properties) {
        auto it = other.properties.find(key);
        if (it == other.properties.end() || it->second != value) {
            return false;
        }
    }

    // Compare data
    if (isRawData()) {
        if (std::get<std::vector<uint8_t>>(data) != std::get<std::vector<uint8_t>>(other.data)) {
            return false;
        }
    }
    else if (isBitmap()) {  
        if (std::get<BitmapData>(data) != std::get<BitmapData>(other.data)) {
            return false;
        }
    }
    else if (isAudio()) {
        if (std::get<AudioData>(data) != std::get<AudioData>(other.data)) {
            return false;
        }
    }
    else if (isFont()) {
        if (std::get<FontData>(data) != std::get<FontData>(other.data)) {
            return false;
        }
    }
    else if (isNested()) {
        if (std::get<NestedObjects>(data) != std::get<NestedObjects>(other.data)) {
            return false;
        }
    }
    else {
        return false;   // implement for new types
    }

    return true;
}

void DataParser::DataObject::updateDateProperty() {
    wxDateTime now = wxDateTime::Now();
    std::string dateString = now.Format("%m-%d-%Y, %H:%M").ToStdString();
    if (dateString[0] == '0') {
        dateString = dateString.substr(1);
    }
    setProperty('DATE', dateString);
}

bool DataParser::createFromPalette(const std::vector<uint8_t>& palette, DataObject& dataObject) {
    // Create a BitmapData object from the palette
    BitmapData bitmap;
    if (!BitmapData::createFromPalette(palette, bitmap)) {
        return false;
    }

    // Initialize the DataObject
    dataObject.typeID = ObjectType::DAT_PALETTE;  // Set as palette type
    dataObject.setProperty('NAME', "Palette");  // Set default name
    dataObject.data = bitmap;  // Set the bitmap data

    // Update the date property
    dataObject.updateDateProperty();

    return true;
}

DataParser::DataObject DataParser::createSampleObject(ObjectType typeID) {
    // Create a new DataObject
    DataObject obj;
    obj.typeID = typeID;
    
    // Set data based on type
    if (typeID == DAT_FILE) {
        // For DAT files, create an empty nested objects vector
        obj.data = NestedObjects();
    } else if (typeID == DAT_FLI) {
        // For FLI animations, create a sample video
        obj.data = VideoData::createSampleFLI();
    } else if (typeID == DAT_SAMP) {
        // For audio samples, create a sample audio
        obj.data = AudioData::createSampleAudio();
    } else if (typeID == DAT_OGG) {
        // For OGG files, create a sample OGG
        obj.data = AudioData::createSampleOGG();
    } else if (typeID == DAT_MIDI) {
        // For MIDI files, create a sample MIDI
        obj.data = AudioData::createSampleMIDI();
    } else if (typeID == DAT_FONT) {
        // For font objects, create an empty FontData
        obj.data = FontData();
    } else if (typeID == DAT_BITMAP || typeID == DAT_RLE_SPRITE || typeID == DAT_C_SPRITE || typeID == DAT_XC_SPRITE || typeID == DAT_PALETTE) {
        // For bitmap types, create a sample bitmap
        obj.data = BitmapData::createSampleBitmap(typeID);
    } else {
        // For unknown types (including DAT_DATA), default to binary data
        obj.data = std::vector<uint8_t>();
    }
    
    // Set a default name based on the type
    std::string name;
    switch (typeID) {
        case DAT_FILE:
            name = "Data File";
            break;
        case DAT_BITMAP:
            name = "Bitmap";
            break;
        case DAT_RLE_SPRITE:
            name = "RLE Sprite";
            break;
        case DAT_C_SPRITE:
            name = "Compiled Sprite";
            break;
        case DAT_XC_SPRITE:
            name = "X-Compiled Sprite";
            break;
        case DAT_FLI:
            name = "FLI Animation";
            break;
        case DAT_SAMP:
            name = "Audio Sample";
            break;
        case DAT_MIDI:
            name = "MIDI File";
            break;
        case DAT_OGG:
            name = "OGG Audio";
            break;
        case DAT_FONT:
            name = "Font";
            break;
        case DAT_DATA:
            name = "Binary Data";
            break;
        default:
            name = "Binary Data";  // Default to binary data for unknown types
    }
    obj.setProperty('NAME', name);
    
    // Update the date property
    obj.updateDateProperty();
    
    return obj;
}

uint32_t DataParser::encrypt_id(uint32_t x, const std::string& password, bool new_format) {
    uint32_t mask = 0;
    
    if (!password.empty()) {
        // Create mask from password characters
        for (size_t i = 0; i < password.length(); i++) {
            mask ^= (static_cast<uint32_t>(password[i]) << ((i & 3) * 8));
        }

        /*// Apply additional mask pattern
        for (int i = 0, pos = 0; i < 4; i++) {
            mask ^= (static_cast<uint32_t>(password[pos++]) << (24 - i * 8));
            if (pos >= static_cast<int>(password.length())) {
                pos = 0;
            }
        }*/

        // Apply new format XOR if requested
        if (new_format) {
            mask ^= 42;
        }
    }
    logDebug("Encrypting ID: " + ConvertIDToHexString(x) + " with mask: " + ConvertIDToHexString(mask));

    return x ^ mask;
}

void DataParser::encryptBuffer(std::vector<uint8_t>& buffer, const std::string& password, size_t startPos) {
    if (password.empty()) {
        return; // No password, no encryption
    }
    
    if (startPos >= buffer.size()) {
        return; // Start position is beyond buffer size
    }
    
    const size_t passwordLength = password.length();
    size_t passwordIndex = 0;
    if (passwordLength > 4) {
        passwordIndex = 4;
    }
    
    for (size_t i = startPos; i < buffer.size(); ++i) {
        buffer[i] ^= static_cast<uint8_t>(password[passwordIndex]);
        passwordIndex = (passwordIndex + 1) % passwordLength;
    }
}