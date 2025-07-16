#pragma once

#include <cstdint>
#include <wx/string.h>

const wxString GrabberVersion = "1.0.0";
const wxString GrabberName = "wxGrabber";

// Object Types
enum ObjectType {
    DAT_FILE = 'FILE',
    DAT_DATA = 'DATA',
    DAT_BITMAP = 'BMP ',
    DAT_RLE_SPRITE = 'RLE ',
    DAT_C_SPRITE = 'CMP ',
    DAT_XC_SPRITE = 'XCMP',
    DAT_INFO = 'info',
    DAT_OGG = 'OGG ',  // Added OGG object type
    DAT_SAMP = 'SAMP', // Digital sound sample
    DAT_MIDI = 'MIDI', // MIDI file type
    DAT_PALETTE = 'PAL ', // Palette object type
    DAT_FONT = 'FONT', // Font object
    DAT_PATCH = 'PAT ', // Gravis patch (MIDI instrument)
    DAT_FLI = 'FLIC', // FLI or FLC animation
    // DAT_SAMPLE is an alias for DAT_SAMP, not needed if DAT_SAMP is present
    // Add other object types as needed...
};

#define rgb(r,g,b) ((r) << 16 | (g) << 8 | (b))
#define red(c) ((c) >> 16 & 0xFF)
#define green(c) ((c) >> 8 & 0xFF)
#define blue(c) ((c) & 0xFF)

const uint32_t SEPARATOR_COLOR = rgb(255, 255, 0);
const uint32_t TRANSPARENT_COLOR = rgb(255, 0, 255);
const uint32_t BOUNDARY_COLOR = rgb(0, 255, 255);
const uint32_t SEPARATOR_COLOR_INDEX = 255;
const uint32_t TRANSPARENT_COLOR_INDEX = 0;
const uint32_t BOUNDARY_COLOR_INDEX = 255;
const uint16_t TRANSPARENT_COLOR_16 = 0xf81f;
const uint16_t SEPARATOR_COLOR_16 = 0xffe0;
const uint16_t TRANSPARENT_COLOR_15 = 0xf83f;
const uint16_t REFERENCE_COLOR_1516 = 0xf85f;