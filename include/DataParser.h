#ifndef DATAPARSER_H
#define DATAPARSER_H

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstdint>
#include <cstring>
#include <variant>
#include <unordered_map>
#include <chrono>
#include <thread>
#include <map>
#include <memory>
#include <optional>
#include "BitmapData.h"
#include "CommonTypes.h"
#include "lzss.h"
#include "AudioData.h"
#include "VideoData.h"
#include "FontData.h"

// User-defined literal for converting 4-character codes to uint32_t
constexpr uint32_t operator""_u32(const char* str, size_t) {
    return (str[0] << 24) | (str[1] << 16) | (str[2] << 8) | str[3];
}

// Function declarations

class DataParser {
public:
    // Structure to hold property data
    using PropertyMap = std::unordered_map<uint32_t, std::string>;

    // Forward declaration for recursive variant
    struct DataObject;
    using NestedObjects = std::vector<std::shared_ptr<DataObject>>;
    using DataVariant = std::variant<std::vector<uint8_t>, BitmapData, AudioData, VideoData, NestedObjects, FontData>;

    // Structure to hold object data
    struct DataObject {
        ObjectType typeID;
        PropertyMap properties;
        DataVariant data;
        std::vector<uint32_t> propertyOrder;  // Store order of properties
        std::string name;
        uint32_t ui_id; // UI ID for the object
        static uint32_t nextUID;

        DataObject() : ui_id(nextUID++) {}

        // Helper methods for type checking
        bool isRawData() const { return std::holds_alternative<std::vector<uint8_t>>(data); }
        bool isBitmap() const { return std::holds_alternative<BitmapData>(data); }
        bool isNested() const { return std::holds_alternative<NestedObjects>(data); }
        bool isAudio() const { return std::holds_alternative<AudioData>(data); }
        bool isVideo() const { return std::holds_alternative<VideoData>(data); }
        bool isFont() const { return std::holds_alternative<FontData>(data); }
        // Helper methods for data access
        const std::vector<uint8_t>& getRawData() const { return std::get<std::vector<uint8_t>>(data); }
        const BitmapData& getBitmap() const { return std::get<BitmapData>(data); }
        BitmapData& getBitmap() { return std::get<BitmapData>(data); }
        NestedObjects& getNestedObjects() { return std::get<NestedObjects>(data); }
        const AudioData& getAudio() const { return std::get<AudioData>(data); }
        const VideoData& getVideo() const { return std::get<VideoData>(data); }
        const FontData& getFont() const { return std::get<FontData>(data); }
        // Helper method to get property value
        std::string getProperty(uint32_t propID) const {
            auto it = properties.find(propID);
            return it != properties.end() ? it->second : "";
        }

        // Helper method to set property value
        void setProperty(uint32_t propID, const std::string& value) {
            if (properties.find(propID) == properties.end()) {
                propertyOrder.push_back(propID);  // Add to order if new property
            }
            properties[propID] = value;
            if (propID == 'NAME') {
                name = value;
            }
        }

        // Helper method to set property value with string ID
        void setProperty(const std::string& propID, const std::string& value) {
            // Convert string ID to uint32_t (4 chars)
            uint32_t id = 0;
            size_t len = std::min(propID.length(), size_t(4));
            for (size_t i = 0; i < len; i++) {
                id = (id << 8) | static_cast<unsigned char>(propID[i]);
            }
            setProperty(id, value);
        }

        // Helper method to get ordered properties
        std::vector<std::pair<uint32_t, std::string>> getOrderedProperties() const {
            std::vector<std::pair<uint32_t, std::string>> result;
            result.reserve(propertyOrder.size());
            for (uint32_t propID : propertyOrder) {
                auto it = properties.find(propID);
                if (it != properties.end()) {
                    result.emplace_back(propID, it->second);
                }
            }
            return result;
        }

        // Helper method to clear a property
        void clearProperty(uint32_t propID) {
            properties.erase(propID);
            auto it = std::find(propertyOrder.begin(), propertyOrder.end(), propID);
            if (it != propertyOrder.end()) {
                propertyOrder.erase(it);
            }
        }

        // Update object data from ORIG property file path if it exists
        bool update(std::string &ErrorMessage, bool ForceUpdate = false, std::vector<uint8_t>* currentPalette = nullptr, bool useDithering = false);

        // Equality operator to compare two DataObjects
        bool operator==(const DataObject& other) const;
        bool operator!=(const DataObject& other) const {
            return !(*this == other);
        }

        void updateDateProperty();
    };

    static bool ParseDataObjects(std::vector<uint8_t> &buffer, std::vector<std::shared_ptr<DataObject>> &objects);
    static std::vector<uint8_t> ReadPackfile(const std::string& inputFilename, const std::string& password = "");
    static std::string ConvertIDToString(const uint32_t id);
    static std::string ConvertIDToHexString(const uint32_t id);
    // Convenience function to load and parse a packfile in one step
    // Returns pair<bool, bool> where first bool indicates success and second bool indicates if compression was used
    static std::pair<bool, bool> LoadPackfile(const std::string& inputFilename, std::vector<std::shared_ptr<DataObject>> &objects, const std::string& password = "");
    static bool SavePackfile(const std::string& outputFilename, const std::vector<std::shared_ptr<DataObject>>& objects, bool createBackup = true, bool useCompression = false, const std::string& password = "");
    static bool WritePackfile(const std::string& outputFilename, const std::vector<uint8_t>& objectsBuffer, bool useCompression = false, const std::string& password = "");
    // Serialize data objects (dat_magic + objects) without compression
    static std::vector<uint8_t> SerializeDataObjects(const std::vector<std::shared_ptr<DataObject>>& objects);
    // Create a DataObject from a palette vector
    // palette: Input vector containing 256 RGB colors (768 bytes)
    // dataObject: Output DataObject to initialize
    // Returns true if successful, false if palette size is invalid
    static bool createFromPalette(const std::vector<uint8_t>& palette, DataObject& dataObject);

    // Create a sample object with a bitmap containing "Hi!" text
    // typeID: The type ID for the object (e.g. DAT_BITMAP, DAT_RLE_SPRITE, etc.)
    // Returns a DataObject with a sample bitmap
    static DataObject createSampleObject(ObjectType typeID);

    // Encrypt/decrypt an ID using a password
    // x: The ID to encrypt/decrypt
    // password: The password string to use for encryption
    // new_format: If true, applies additional XOR with 42
    // Returns the encrypted/decrypted ID
    static uint32_t encrypt_id(uint32_t x, const std::string& password, bool new_format = false);

    // Encrypt/decrypt a buffer in-place using a password
    // buffer: The buffer to encrypt/decrypt (modified in-place)
    // password: The password string to use for encryption
    // startPos: Starting position in the buffer from which to begin encryption (default: 0)
    // The encryption works by XORing each byte with the password bytes, cycling through the password
    static void encryptBuffer(std::vector<uint8_t>& buffer, const std::string& password, size_t startPos = 0);

private:
    static const uint32_t F_PACK_MAGIC = 0x736c6821;       // Allegro Generic Packfile (compressed)
    static const uint32_t F_NOPACK_MAGIC = 0x736c682e;     // Allegro Generic Packfile (uncompressed)
    static const uint32_t DAT_MAGIC = 0x414c4c2e;          // Allegro DAT magic

    static uint32_t readBigEndian32(std::ifstream &file);
    static uint32_t readBigEndian32(std::vector<uint8_t> &buffer, size_t& offset);
    static std::string BigEndian32ToSring(uint32_t value);
    static void writeBigEndian32(std::vector<uint8_t>& buffer, uint32_t value);
};
#endif // DATAPARSER_H

