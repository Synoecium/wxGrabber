#include "../include/GrabberInfo.h"
#include "../include/log.h"
#include <algorithm>
#include <fstream>
#include <sstream>

GrabberInfo::GrabberInfo()
    : m_xGrid(16)
    , m_yGrid(16)
    , m_backup(true)
    , m_dither(true)
    , m_name("GrabberInfo")
    , m_pack(CompressionMode::None)
    , m_relativeFilenames(false)
    , m_sort(false)
    , m_transparency(false)
    , m_griddleXGrid(64)
    , m_griddleYGrid(64)
    , m_griddleMode(true)  // true = grid
    , m_griddleEmpties(false)
    , m_griddleAutocrop(false)
    , m_griddleType(0)  // bitmap
    , m_griddleColor(0xffffff)
{
}

bool GrabberInfo::LoadSettings(const std::string& filename)
{
    std::ifstream file(filename);
    if (!file.is_open()) {
        logWarning("Could not open settings file: " + filename);
        return false;
    }

    std::string line;
    bool inGrabberSection = false;

    while (std::getline(file, line)) {
        // Remove comments (but not in griddle_color value)
        size_t commentPos = std::string::npos;
        size_t equalPos = line.find('=');
        if (equalPos != std::string::npos) {
            std::string key = line.substr(0, equalPos);
            // Only look for comments before the equals sign
            commentPos = key.find('#');
            if (commentPos == std::string::npos) {
                commentPos = key.find(';');
            }
        } else {
            // No equals sign, check whole line for comments
            commentPos = line.find('#');
            if (commentPos == std::string::npos) {
                commentPos = line.find(';');
            }
        }
        if (commentPos != std::string::npos) {
            line = line.substr(0, commentPos);
        }

        // Trim whitespace safely
        size_t first = line.find_first_not_of(" \t\r\n");
        size_t last = line.find_last_not_of(" \t\r\n");
        if (first == std::string::npos) { // Line is all whitespace
            continue;
        }
        line = line.substr(first, last - first + 1);

        if (line.empty()) continue;

        if (line == "[grabber]") {
            inGrabberSection = true;
            continue;
        }

        if (!inGrabberSection) continue;

        size_t pos = line.find('=');
        if (pos == std::string::npos) continue;

        std::string key = line.substr(0, pos);
        std::string value = line.substr(pos + 1);

        // Trim whitespace safely
        first = key.find_first_not_of(" \t\r\n");
        last = key.find_last_not_of(" \t\r\n");
        if (first != std::string::npos) { // Not all whitespace
            key = key.substr(first, last - first + 1);
        } else {
            continue; // Skip if key is all whitespace
        }

        first = value.find_first_not_of(" \t\r\n");
        last = value.find_last_not_of(" \t\r\n");
        if (first != std::string::npos) { // Not all whitespace
            value = value.substr(first, last - first + 1);
        } else {
            value = ""; // Empty value
        }

        if (key == "xgrid") {
            try {
                m_xGrid = std::stoi(value);
            } catch (const std::exception&) {
                logWarning("Invalid xgrid value in settings file");
            }
        }
        else if (key == "ygrid") {
            try {
                m_yGrid = std::stoi(value);
            } catch (const std::exception&) {
                logWarning("Invalid ygrid value in settings file");
            }
        }
        else if (key == "backups") {
            m_backup = (value == "y");
        }
        else if (key == "index") {
            m_index = (value == "y");
        }
        else if (key == "sort") {
            m_sort = (value == "y");
        }
        else if (key == "relative") {
            m_relativeFilenames = (value == "y");
        }
        else if (key == "dither") {
            m_dither = (value == "y");
        }
        else if (key == "transparency") {
            m_transparency = (value == "y");
        }
        else if (key == "griddle_xgrid") {
            m_griddleXGrid = std::stoi(value);
        }
        else if (key == "griddle_ygrid") {
            m_griddleYGrid = std::stoi(value);
        }
        else if (key == "griddle_mode") {
            // Check for either "grid" or "col255"
            m_griddleMode = (value == "grid");
            if (value != "grid" && value != "col255") {
                logWarning("Invalid griddle_mode value (should be 'grid' or 'col255'): " + value);
            }
        }
        else if (key == "griddle_empties") {
            m_griddleEmpties = (value == "y");
        }
        else if (key == "griddle_autocrop") {
            m_griddleAutocrop = (value == "y");
        }
        else if (key == "griddle_type") {
            m_griddleType = std::stoi(value);
        }
        else if (key == "griddle_color") {
            // Check if value starts with '#' and has exactly 6 hex digits
            if (value.length() == 7 && value[0] == '#') {
                std::string hexColor = value.substr(1); // Skip the '#'
                // Verify all characters are valid hex digits
                bool validHex = std::all_of(hexColor.begin(), hexColor.end(), 
                    [](char c) { return std::isxdigit(c); });
                
                if (validHex) {
                    try {
                        // Parse the hex string to an integer
                        m_griddleColor = std::stoul(hexColor, nullptr, 16);
                    } catch (const std::exception&) {
                        logWarning("Failed to parse griddle_color value: " + value);
                    }
                } else {
                    logWarning("Invalid hex digits in griddle_color value: " + value);
                }
            } else {
                logWarning("Invalid griddle_color format (should be #RRGGBB): " + value);
            }
        }
        else if (key == "password") {
            m_password = value;
        }
        // Shell association: store any unknown key as a shell association
        else {
            // Trim trailing whitespace from key
            size_t end = key.find_last_not_of(" \t\r\n");
            std::string trimmedKey = (end == std::string::npos) ? key : key.substr(0, end + 1);
            m_shellAssociations[trimmedKey] = value;
        }
    }

    return true;
}

bool GrabberInfo::SaveSettings(const std::string& filename) const
{
    std::ofstream file(filename);
    if (!file.is_open()) {
        logWarning("Could not create settings file: " + filename);
        return false;
    }

    file << "[grabber]\n";
    file << "xgrid = " << m_xGrid << "\n";
    file << "ygrid = " << m_yGrid << "\n";
    file << "backups = " << (m_backup ? "y" : "n") << "\n";
    file << "index = " << (m_index ? "y" : "n") << "\n";
    file << "sort = " << (m_sort ? "y" : "n") << "\n";
    file << "relative = " << (m_relativeFilenames ? "y" : "n") << "\n";
    file << "dither = " << (m_dither ? "y" : "n") << "\n";
    file << "transparency = " << (m_transparency ? "y" : "n") << "\n";
    file << "griddle_xgrid = " << m_griddleXGrid << "\n";
    file << "griddle_ygrid = " << m_griddleYGrid << "\n";
    file << "griddle_mode = " << (m_griddleMode ? "grid" : "col255") << "\n";
    file << "griddle_empties = " << (m_griddleEmpties ? "y" : "n") << "\n";
    file << "griddle_autocrop = " << (m_griddleAutocrop ? "y" : "n") << "\n";
    file << "griddle_type = " << m_griddleType << "\n";

    // Save griddle color in hex format #RRGGBB
    std::stringstream colorStream;
    colorStream << "#" << std::hex << std::setfill('0') << std::setw(6) << m_griddleColor;
    file << "griddle_color = " << colorStream.str() << "\n";

    // Save password only if not empty
    if (!m_password.empty()) {
        file << "password = " << m_password << "\n";
    }

    // Save shell associations
    for (const auto& pair : m_shellAssociations) {
        file << pair.first << "=" << pair.second << "\n";
    }

    return true;
}

void GrabberInfo::ParseDataObject(std::vector<std::shared_ptr<DataParser::DataObject>>& objects)
{
    // Find info object (should be the last one)
    if (objects.empty() || objects.back()->typeID != 'info') {
        logInfo("No info object found, using default values");
        return;
    }

    const auto& infoObj = objects.back();

    // Parse grid values
    std::string xgrd = infoObj->getProperty('XGRD');
    std::string ygrd = infoObj->getProperty('YGRD');
    
    // Convert to numbers if values are valid
    if (!xgrd.empty()) {
        try {
            m_xGrid = std::stoi(xgrd);
        } catch (const std::exception&) {
            logWarning("Invalid XGRD value in info object: " + xgrd);
        }
    }
    
    if (!ygrd.empty()) {
        try {
            m_yGrid = std::stoi(ygrd);
        } catch (const std::exception&) {
            logWarning("Invalid YGRD value in info object: " + ygrd);
        }
    }

    // Parse boolean properties (convert "y"/"n" strings to bool)
    m_backup = infoObj->getProperty('BACK') == "y";
    m_dither = infoObj->getProperty('DITH') == "y";
    m_relativeFilenames = infoObj->getProperty('RELF') == "y";
    m_sort = infoObj->getProperty('SORT') == "y";
    m_transparency = infoObj->getProperty('TRAN') == "y";

    // Parse compression mode
    std::string pack = infoObj->getProperty('PACK');
    if (!pack.empty()) {
        try {
            int packValue = std::stoi(pack);
            switch (packValue) {
                case 0:
                    m_pack = CompressionMode::None;
                    break;
                case 1:
                    m_pack = CompressionMode::Individual;
                    break;
                case 2:
                    m_pack = CompressionMode::Global;
                    break;
                default:
                    logWarning("Invalid PACK value in info object: " + pack + ", defaulting to None");
                    m_pack = CompressionMode::None;
            }
        } catch (const std::exception&) {
            logWarning("Invalid PACK value in info object: " + pack + ", defaulting to None");
            m_pack = CompressionMode::None;
        }
    }

    // Parse string properties
    m_name = infoObj->getProperty('NAME');

    // Remove info object from objects
    objects.pop_back();
}

DataParser::DataObject GrabberInfo::SerializeToDataObject() const
{
    // Create new info object
    DataParser::DataObject infoObj;
    infoObj.typeID = ObjectType::DAT_INFO;

    // Set grid properties
    infoObj.setProperty("XGRD", std::to_string(m_xGrid));
    infoObj.setProperty("YGRD", std::to_string(m_yGrid));

    // Set boolean properties
    infoObj.setProperty("BACK", m_backup ? "y" : "n");
    infoObj.setProperty("DITH", m_dither ? "y" : "n");
    infoObj.setProperty("RELF", m_relativeFilenames ? "y" : "n");
    infoObj.setProperty("SORT", m_sort ? "y" : "n");
    infoObj.setProperty("TRAN", m_transparency ? "y" : "n");

    // Set compression mode
    infoObj.setProperty("PACK", std::to_string(static_cast<int>(m_pack)));

    // Set string properties
    if (!m_name.empty()) {
        infoObj.setProperty("NAME", m_name);
    }

    // Set data buffer with grabber identifier string
    std::string grabberStr = "For internal use by the grabber";
    infoObj.data = std::vector<uint8_t>(grabberStr.begin(), grabberStr.end());

    return infoObj;
}

std::string GrabberInfo::GetShellCommandForType(const std::string& type) const {
    // Trim trailing whitespace from type
    size_t end = type.find_last_not_of(" \t\r\n");
    std::string trimmedType = (end == std::string::npos) ? type : type.substr(0, end + 1);
    auto it = m_shellAssociations.find(trimmedType);
    if (it != m_shellAssociations.end()) {
        return it->second;
    }
    return std::string();
} 