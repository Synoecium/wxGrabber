#include "../include/lzss.h"
#include <unordered_map>

std::vector<uint8_t> LZSS::Compress(const std::vector<uint8_t>& inputBuffer) {
    if (inputBuffer.empty()) return {};

    std::vector<uint8_t> result;
    size_t dataSize = inputBuffer.size();
    
    // Reserve some space to avoid frequent reallocations
    result.reserve(dataSize); // Worst case: no compression
    
    uint8_t flags = 0;         // Flags byte: 0 for literal, 1 for offset/length pair
    uint8_t flagPos = 0;       // Position in flags byte
    size_t flagsIndex = 0;     // Position in output where current flags byte is stored
    
    // Add placeholder for the first flags byte
    result.push_back(0);
    
    // Dictionary for hash-based search with MIN_MATCH_LENGTH bytes prefixes. 
    // Key is the value of the first MIN_MATCH_LENGTH bytes, value is the position in the data.
    std::unordered_multimap<int32_t, int32_t> dictionary;
    // initial zeroes in dictionary
    for (int32_t i = 1; i <= MATCH_LENGTH_LIMIT; ++i) {
        int32_t data_pos = -i;
        // MIN_MATCH_LENGTH bytes prefix is 0 initially
        int32_t hash = CalculateHash(inputBuffer, data_pos);
        dictionary.insert({hash, data_pos});
    }
    
    int32_t pos = 0;
    
    while (pos < dataSize) {
        bool found_match = false;
        int32_t best_length = 0;
        int32_t best_offset = 0;
        // Only search if we have enough bytes ahead
        if (pos + MIN_MATCH_LENGTH <= dataSize) {
            int32_t hash = CalculateHash(inputBuffer, pos);
            
            auto range = dictionary.equal_range(hash);
            std::vector<std::unordered_multimap<int32_t, int32_t>::iterator> iterators_to_remove;
            // reverse loop
            //for (auto it = range.second; it != range.first; --it) {
            for (auto it = range.first; it != range.second; ++it) {
                int32_t offset = it->second;
                int32_t length = MIN_MATCH_LENGTH;
                if (offset < pos - RING_BUFFER_SIZE) {
                    iterators_to_remove.push_back(it);
                    continue;
                }

                // check if there are more matching bytes
                while (
                    pos + length < dataSize 
                    && inputBuffer[pos + length] == ((offset + length < 0) ? 0 : inputBuffer[offset + length])
                    && length < MATCH_LENGTH_LIMIT
                    ) {
                    length++;
                }
                // found a match
                if (length > best_length) {
                    found_match = true;
                    best_length = length;
                    best_offset = offset;
                }
            }
            for (auto it : iterators_to_remove) {
                dictionary.erase(it);
            }
        }
        
        if (found_match && best_length >= MIN_MATCH_LENGTH) {
            // write match to result
            int32_t window_pos = (INITIAL_POSITION + best_offset) % RING_BUFFER_SIZE;
            uint8_t byte1 = static_cast<uint8_t>(window_pos & 0xFF);
            uint8_t byte2 = static_cast<uint8_t>((window_pos >> 4) & 0xF0);
            byte2 |= (best_length - 3) & 0x0F;
            result.push_back(byte1);
            result.push_back(byte2);
            
            for (int32_t i = 0; i < best_length; ++i) {
                int32_t hash = CalculateHash(inputBuffer, pos + i);
                if (hash != -1) {
                    dictionary.insert({hash, pos + i});
                }
            }
            pos += best_length;
        }
        else {
            flags |= (1 << flagPos);
            // write literal to result
            result.push_back(inputBuffer[pos]);
            
            int32_t hash = CalculateHash(inputBuffer, pos);
            if (hash != -1) {
                dictionary.insert({hash, pos});
            }
            pos++;
        }
        
        // Update flag position
        flagPos++;
        
        // If we've filled all bits in the flags byte, store it and start a new one
        if (flagPos == FLAG_BYTE_BITS) {
            // Update the flags byte
            result[flagsIndex] = flags;
            
            // Reset flags
            flags = 0;
            flagPos = 0;
            
            // Add placeholder for the next flags byte if we're not at the end
            if (pos < dataSize) {
                flagsIndex = result.size();
                result.push_back(0);
            }
        }
    }
    
    // Handle the last flags byte if it's not full
    if (flagPos > 0) {
        result[flagsIndex] = flags;
    }
    
    return result;
}

std::vector<uint8_t> LZSS::Decompress(const std::vector<uint8_t>& inputBuffer) {
    if (inputBuffer.empty()) return {};
    
    std::vector<uint8_t> result;
    size_t pos = 0;
    
    // Reserve some space to avoid frequent reallocations
    result.reserve(inputBuffer.size() * 2); // Estimate: 2x expansion
    int32_t window_pos = INITIAL_POSITION;
    std::vector<uint8_t> window(RING_BUFFER_SIZE, 0);
    
    while (pos < inputBuffer.size()) {
        // Read the flags byte
        uint8_t flags = inputBuffer[pos++];
        
        // Process each bit in the flags byte
        for (uint8_t flagPos = 0; flagPos < FLAG_BYTE_BITS && pos < inputBuffer.size(); ++flagPos) {
            bool isBitSet = (flags & (1 << flagPos)) != 0;
            
            if (isBitSet) {
                // This is a literal - copy the byte
                if (pos >= inputBuffer.size()) {
                    //logError("Not enough data for a complete literal");
                    // Not enough data for a complete literal
                    break;
                }
                
                uint8_t byte = inputBuffer[pos++];
                result.push_back(byte);
                window[window_pos] = byte;
                window_pos = (window_pos + 1) % RING_BUFFER_SIZE;
            } else {
                // This is a match - read offset and length
                if (pos + 1 >= inputBuffer.size()) {
                    // Not enough data for a complete match
                    break;
                }
                
                uint8_t byte1 = inputBuffer[pos++];
                uint8_t byte2 = inputBuffer[pos++];
                
                size_t offset = byte1 | ((byte2 & 0xF0) << 4);
                size_t length = (byte2 & 0x0F) + MIN_MATCH_LENGTH;
                
                // Copy the matched bytes
                for (size_t i = 0; i < length; ++i) {
                    uint8_t byte = window[(offset + i) % RING_BUFFER_SIZE];
                    result.push_back(byte);
                    window[window_pos] = byte;
                    window_pos = (window_pos + 1) % RING_BUFFER_SIZE;
                }
            }
        }
    }
    
    return result;
}

int32_t LZSS::CalculateHash(const std::vector<uint8_t>& data, int32_t pos) {
    int32_t hash = 0;
    if (pos + MIN_MATCH_LENGTH > data.size())
    {
        return -1;
    }
    for (int32_t i = 0; i < MIN_MATCH_LENGTH; ++i)
    {
        uint8_t byte = (pos + i < 0) ? 0 : data[pos + i];
        hash |= (byte << (i * 8));
    }
    return hash;
}