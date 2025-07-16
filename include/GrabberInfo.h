#pragma once

#include "DataParser.h"
#include <string>
#include <vector>
#include <unordered_map>

class GrabberInfo {
public:
    // Compression modes
    enum class CompressionMode {
        None = 0,
        Individual = 1,
        Global = 2
    };

    GrabberInfo();

    // Parse info object from data objects array and remove it
    void ParseDataObject(std::vector<std::shared_ptr<DataParser::DataObject>>& objects);

    // Create info object and add it to data objects array
    //void SerializeDataObject(std::vector<DataParser::DataObject>& objects) const;
    DataParser::DataObject SerializeToDataObject() const;

    // Load settings from .cfg file
    bool LoadSettings(const std::string& filename);

    // Save settings to .cfg file
    bool SaveSettings(const std::string& filename) const;

    // Getters
    int GetXGrid() const { return m_xGrid; }
    int GetYGrid() const { return m_yGrid; }
    bool GetBackup() const { return m_backup; }
    bool GetDither() const { return m_dither; }
    const std::string& GetName() const { return m_name; }
    CompressionMode GetPack() const { return m_pack; }
    bool GetRelativeFilenames() const { return m_relativeFilenames; }
    bool GetSort() const { return m_sort; }
    bool GetTransparency() const { return m_transparency; }
    int GetGriddleXGrid() const { return m_griddleXGrid; }
    int GetGriddleYGrid() const { return m_griddleYGrid; }
    bool GetGriddleMode() const { return m_griddleMode; }
    bool GetGriddleEmpties() const { return m_griddleEmpties; }
    bool GetGriddleAutocrop() const { return m_griddleAutocrop; }
    int GetGriddleType() const { return m_griddleType; }
    uint32_t GetGriddleColor() const { return m_griddleColor; }
    const std::string& GetPassword() const { return m_password; }

    // Setters
    void SetXGrid(int value) { m_xGrid = value; }
    void SetYGrid(int value) { m_yGrid = value; }
    void SetBackup(bool value) { m_backup = value; }
    void SetDither(bool value) { m_dither = value; }
    void SetName(const std::string& value) { m_name = value; }
    void SetPack(CompressionMode value) { m_pack = value; }
    void SetRelativeFilenames(bool value) { m_relativeFilenames = value; }
    void SetSort(bool value) { m_sort = value; }
    void SetTransparency(bool value) { m_transparency = value; }
    void SetGriddleXGrid(int value) { m_griddleXGrid = value; }
    void SetGriddleYGrid(int value) { m_griddleYGrid = value; }
    void SetGriddleMode(bool value) { m_griddleMode = value; }
    void SetGriddleEmpties(bool value) { m_griddleEmpties = value; }
    void SetGriddleAutocrop(bool value) { m_griddleAutocrop = value; }
    void SetGriddleType(int value) { m_griddleType = value; }
    void SetGriddleColor(uint32_t value) { m_griddleColor = value; }
    void SetPassword(const std::string& value) { m_password = value; }

    // Shell association lookup
    std::string GetShellCommandForType(const std::string& type) const;

private:
    // Grid settings
    int m_xGrid;
    int m_yGrid;

    // Griddle settings
    int m_griddleXGrid;
    int m_griddleYGrid;
    bool m_griddleMode;     // true = grid, false = col255
    bool m_griddleEmpties;  // Skip empties
    bool m_griddleAutocrop; // Autocrop
    int m_griddleType;      // Type index (0-3)
    uint32_t m_griddleColor;     // Color value

    // Options
    bool m_backup;           // BACK - Backup datafiles
    bool m_dither;          // DITH - Dither images
    bool m_index;  // Added for indexing objects
    bool m_relativeFilenames; // RELF - Store relative filenames
    bool m_sort;            // SORT - Sort objects
    bool m_transparency;    // TRAN - Preserve transparency
    std::string m_name;     // NAME - Datafile name
    CompressionMode m_pack; // PACK - Compression mode (0=None, 1=Individual, 2=Global)
    std::string m_password; // PASSWORD - Password for encryption

    // Shell associations
    std::unordered_map<std::string, std::string> m_shellAssociations;
}; 