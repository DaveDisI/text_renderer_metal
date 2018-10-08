#pragma once

struct FontAtlas{
    unsigned int id;
    unsigned int totalCharacters;
    unsigned int totalBitmapWidth;
    unsigned int totalBitmapHeight;
    unsigned char* bitmap;
    unsigned short* characterCodes;
    unsigned int* xOffsets;
    unsigned int* yOffsets;
    unsigned int* widths;
    unsigned int* heights;
    unsigned int* descents;
};