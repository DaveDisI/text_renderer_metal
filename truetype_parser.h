#pragma once

#include <stdio.h>

#define Fixed unsigned int
#define SWAP16(V) V >> 8 | V << 8
#define SWAP32(V) ((V >> 24) & 0xff) | ((V << 8) & 0xff0000) | ((V >> 8) & 0xff00) | ((V << 24) & 0xff000000)
#define SWAP64(V) ((V >> 56) & 0xff) | ((V << 56) & 0xff00000000000000) | \
                  ((V >> 40) & 0xff00) | ((V < 40) & 0xff000000000000) | \
                  ((V >> 24) & 0xff0000) | ((V < 24) & 0xff000000) | \
                  ((V >> 8) & 0xff000000) | ( (V < 8) & 0xff0000)

#define strToInt(str) (str[0] << 24 | str[1] << 16 | str[2] << 8 | str[3])

#pragma pack(push, 1)
struct OffsetSubtable{
    unsigned int scalarType;
    unsigned short numTables;
    unsigned short searchRange;
    unsigned short entrySelector;
    unsigned short rangeShift;
};

struct Table{
    unsigned int tag;
    unsigned int checkSum;
    unsigned int offset;
    unsigned int length;
};

struct TableDirectory{
    unsigned short numTables;
    Table* tables;
};

struct HeadTable{
    Fixed version;
    Fixed fontRevision;
    unsigned int checkSumAdjustment;
    unsigned int magicNumber;
    unsigned short flags;
    unsigned short unitsPerEm;
    long created;
    long modified;
    short xMin;
    short yMin;
    short xMax;
    short yMax;
    unsigned short macStyle; 
    unsigned short lowestRecPPEM;
    short fontDirectionHint;
    short indexToLocFont;
    short glyphDataFormat;
};

struct CmapIndex{
    unsigned short version;
    unsigned short numberSubtables;
    unsigned short platformID;
    unsigned short platformSpecificID;
    unsigned int offset;
};

struct CmapSubtable{
    unsigned short format;
    unsigned short length;
    unsigned short language;
    unsigned short segCountX2;
    unsigned short searchRange;
    unsigned short entrySelector;
    unsigned short rangeShift;
};

struct Cmap{
    CmapIndex cmapIndex;
    CmapSubtable subtable;
};

struct HheaTable{
    Fixed version;
    short ascent;
    short descent;
    short lineGap;
    unsigned short advanceWidthMax;
    short minLeftSideBearing;
    short minRightSideBearing;
    short xMaxExtent;
    short caretSlopeRise;
    short caretSlopeRun;
    short caretOffset;
    unsigned long reserved;
    short metricDataFormat;
    unsigned short numOfLongHorMetrics;
};

struct MaxpTable{
    unsigned int version;
    unsigned short numGlyphs;
    unsigned short maxPoints;
    unsigned short maxContours;
    unsigned short maxComponentPoints;
    unsigned short maxComponentContours;
    unsigned short maxZones;
    unsigned short maxTwilightPoints;
    unsigned short maxStorage;
    unsigned short maxFunctionDefs;
    unsigned short maxInstructionDefs;
    unsigned short maxStackElements;
    unsigned short maxSizeOfInstructions;
    unsigned short maxComponentElements;
    unsigned short maxComponentDepth;
};

struct FileGlyph {
    short numberOfContours;
    short xMin;
    short yMin;
    short xMax;
    short yMax;
};
#pragma pack(pop)

struct GlyphPoint {
    short x;
    short y;
    bool onCurve;
};

struct GlyphShape {
    unsigned short numContours;
    unsigned short totalPoints;
    unsigned short* contourEndPoints;
    short xMin;
    short xMax;
    short yMin;
    short yMax;
    GlyphPoint* points;
};

struct vector2f{
    float x;
    float y;

    bool operator==(vector2f v){
        return x == v.x && y == v.y;
    }

    bool operator!=(vector2f v){
        return x != v.x || y != v.y;
    }
};

struct vecLine{
    vector2f p1;
    vector2f p2;
};

struct LineGroup{
    unsigned int totalLines;
    vecLine* lines;

    LineGroup(){
        totalLines = 0;
        lines = 0;
    }

    void addLine(vecLine l){
        vecLine* newLines = new vecLine[totalLines + 1];
        
        for(int i = 0; i < totalLines; i++){
            newLines[i] = lines[i];
        }
        newLines[totalLines] = l;

        if(lines){
            delete[] lines;
            lines = 0;
        }

        lines = newLines;
        totalLines++;
    }

    void clear(){
        if(lines){
            delete[] lines;
            lines = 0;
        }
        totalLines = 0;
    }
};

struct Glyph{
    unsigned short characterCode;
    unsigned int width;
    unsigned int height;
    unsigned int yOffset;
};

void printIntAsString(unsigned int i){
    printf("%c%c%c%c\n", i >> 24, i >> 16, i >> 8, i);
}

void print8BitBinary(unsigned char c){
    printf("%i\t%i\t%i\t%i\t%i\t%i\t%i\t%i",
    (c & (1 << 7)) >> 7,
    (c & (1 << 6)) >> 6,
    (c & (1 << 5)) >> 5,
    (c & (1 << 4)) >> 4,
    (c & (1 << 3)) >> 3,
    (c & (1 << 2)) >> 2,
    (c & (1 << 1)) >> 1,
    (c & (1 << 0)) >> 0);
}

void print16BitBinary(unsigned short s){
    unsigned char* c = (unsigned char*)&s;
    print8BitBinary(*(c + 1));
    printf("\t");
    print8BitBinary(*c);
}

void printGlyphShapeData(GlyphShape shape){
    printf("numContours: %i\n", shape.numContours);
    printf("totalPoints: %i\n", shape.totalPoints);
    printf("xMin: %i\n", shape.xMin);
    printf("xMax: %i\n", shape.xMax);
    printf("yMin: %i\n", shape.yMin);
    printf("yMax: %i\n", shape.yMax);

    int ctr = 0;
    for(int i = 0; i < shape.totalPoints; i++){
        if(i == 0){
            printf("contour 1:\n");
        }else if(i == shape.contourEndPoints[ctr] + 1){
            printf("contour %i:\n", ++ctr + 1);
        }
        
        printf("x: %i\ty:%i", shape.points[i].x, shape.points[i].y);
        if(shape.points[i].onCurve){
            printf("\t onCurve\n");
        }else{
            printf("\t offCurve\n");
        }
    }
}

unsigned char* getPointerToTableData(unsigned char* fileData, const char* table){
    OffsetSubtable os = *(OffsetSubtable*)fileData;
    os.numTables = SWAP16(os.numTables);
    
    unsigned int val = strToInt(table);
    val = SWAP32(val);

    for(int i = 0; i < os.numTables; i++){
        Table* t = (Table*)&fileData[sizeof(OffsetSubtable) + (i * sizeof(Table))];
        if((strToInt(table)) == (SWAP32(t->tag))){
            unsigned int offset = SWAP32(t->offset);
            return &fileData[offset];
        }
    }
    return 0;
}

unsigned int getGlyphIndex(unsigned char* fileData, unsigned short characterCode){
    CmapIndex* ci = (CmapIndex*)getPointerToTableData(fileData, "cmap");
    unsigned int offset = SWAP32(ci->offset);

    CmapSubtable* cs = (CmapSubtable*)((unsigned char*)ci + offset);
    unsigned short format = SWAP16(cs->format);
    unsigned int glyphIndex = -1;
    if(format == 4){
        unsigned short segCount = SWAP16(cs->segCountX2);
        segCount /= 2;

        unsigned short* endCode = (unsigned short*)((unsigned short*)(ci) + sizeof(CmapSubtable) + 7);
        unsigned short* startCode = (unsigned short*)((unsigned short*)(ci) + sizeof(CmapSubtable) + 7 + segCount + 1);
        unsigned short* idDelta = (unsigned short*)((unsigned short*)(ci) + sizeof(CmapSubtable) + 7 + segCount + 1 + segCount);
        unsigned short* idRangeOffset = (unsigned short*)((unsigned short*)(ci) + sizeof(CmapSubtable) + 7 + segCount + 1 + (segCount * 2));
        unsigned short* glyphIndexArray = (unsigned short*)((unsigned short*)(ci) + sizeof(CmapSubtable) + 7 + segCount + 1 + (segCount * 3));

        for(int i = 0; i < segCount; i++){
            unsigned short ec = *endCode;
            unsigned short sc = *startCode;
            unsigned short id = *idDelta;
            unsigned short ro = *idRangeOffset;

            ec = SWAP16(ec);
            sc = SWAP16(sc);
            id = SWAP16(id);
            ro = SWAP16(ro);

            if(ec >= characterCode){
                if(ro != 0){
                    unsigned short* addr = &idRangeOffset[i];
                    addr += ro;
                    addr += 2 * (characterCode - sc);
                    unsigned short val = SWAP16(*addr);
                    if(val == 0){
                        glyphIndex = 0;
                    }else {
                        glyphIndex = (val + id) % 65536;
                    }
                }else{
                    glyphIndex = (characterCode + id) % 65536;
                }

                break;
            }

            endCode++;
            startCode++;
            idDelta++;
            idRangeOffset++;
        }
    }else{
        //TODO: Handle other formats besides 4
    }

    return glyphIndex;
}

unsigned char* getPointerToGlyphData(unsigned char* fileData, unsigned short characterCode){
    HeadTable* ht = (HeadTable*)getPointerToTableData(fileData, "head");
    unsigned short fontFormat = SWAP16(ht->indexToLocFont);
    unsigned int glyphIndex = getGlyphIndex(fileData, characterCode);
    
    if(fontFormat == 0){
        unsigned short* loca = (unsigned short*)getPointerToTableData(fileData, "loca");
        loca += glyphIndex;
        unsigned short glf = SWAP16(*loca) / 2;
        //TODO: check and finish this
    }else if(fontFormat == 1){
        unsigned int* loca = (unsigned int*)getPointerToTableData(fileData, "loca");
        loca += glyphIndex;
        unsigned int glf = SWAP32(*loca);
        unsigned char* glyf = getPointerToTableData(fileData, "glyf");
        glyf += glf;
        
        return glyf;
    }
    

    return getPointerToTableData(fileData, "glyf");
}

void getGlyphShape(unsigned char* fileData, unsigned short characterCode, GlyphShape* shape){
    unsigned char* glyfData = getPointerToGlyphData(fileData, characterCode);
    FileGlyph *g = (FileGlyph*)glyfData;
    FileGlyph gg;
    gg.numberOfContours = (unsigned short)SWAP16(g->numberOfContours);
    gg.xMin = (unsigned short)SWAP16(g->xMin);
    gg.yMin = (unsigned short)SWAP16(g->yMin);
    gg.xMax = (unsigned short)SWAP16(g->xMax);
    gg.yMax = (unsigned short)SWAP16(g->yMax);

    shape->numContours = gg.numberOfContours;
    shape->xMin = gg.xMin;
    shape->xMax = gg.xMax;
    shape->yMin = gg.yMin;
    shape->yMax = gg.yMax;

    if(gg.numberOfContours < 0){
        //TODO: handle complex glyphs
        return; 
    }

    unsigned short* contourEndPoints = (unsigned short*)(glyfData + sizeof(FileGlyph));
    shape->contourEndPoints = new unsigned short[gg.numberOfContours];
    for(int i = 0; i < gg.numberOfContours; i++){
        unsigned short ep = SWAP16(*contourEndPoints);
        shape->contourEndPoints[i] = ep;
        contourEndPoints++;
    }

    unsigned short instLn = SWAP16(*contourEndPoints);
    contourEndPoints++;
    unsigned char* inst = (unsigned char*)(contourEndPoints) + instLn;

    int totalPoints = shape->contourEndPoints[gg.numberOfContours - 1] + 1;
    unsigned char *flags = new unsigned char[totalPoints];
    int totalFlags = 0;
    while(totalFlags < totalPoints){
        flags[totalFlags] = *inst;
        totalFlags++;
        inst++;
        if(flags[totalFlags - 1] & 0x8){
            unsigned char loc = flags[totalFlags - 1];
            unsigned char repeat = *inst;
            inst++;
            for(unsigned char j = 0; j < repeat; j++){
                flags[totalFlags] = loc;
                totalFlags++;
            }
        }
    }

    short *xPositions = new short[totalPoints];
    for(int i = 0; i < totalPoints; i++){
        unsigned char flag = flags[i];
        short prevX = i == 0 ? 0 : xPositions[i - 1];

        if(flag & 0x2){
            unsigned char c = *inst;
            short s = c;
            inst++;
            if(!(flag & 0x10)){
                s *= -1;
            }  
            xPositions[i] = prevX + s;
        }else{
            if(flag & 0x10){
                xPositions[i] = prevX;
            }else {
                short v = SWAP16(*(unsigned short*)inst);
                xPositions[i] = prevX + v;
                inst += 2;
            }
        }
    }

    short *yPositions = new short[totalPoints];
    for(int i = 0; i < totalPoints; i++){
        unsigned char flag = flags[i];
        short prevY = i == 0 ? 0 : yPositions[i - 1];

        if(flag & 0x4){
            unsigned char c = *inst;
            short s = c;
            inst++;
            if(!(flag & 0x20)){
                s *= -1;
            }  
            yPositions[i] = prevY + s;
        }else{
            if(flag & 0x20){
                yPositions[i] = prevY;
            }else {
                short v = SWAP16(*(unsigned short*)inst);
                yPositions[i] = prevY + v;
                inst += 2;
            }
        }
    }

    GlyphPoint* points = new GlyphPoint[totalPoints];

    for(int i = 0; i < totalPoints; i++){
        unsigned char flag = flags[i];
        if(flag & 0x1){
            points[i].onCurve = true;
        }else{
            points[i].onCurve = false;
        }
        points[i].x = xPositions[i];
        points[i].y = yPositions[i];
    }

    shape->totalPoints = totalPoints;
    shape->points = points;

    delete[] xPositions;
    delete[] yPositions;
}

void getLinesFromCurve(float x1, float y1, float x2, float y2, float ox, float oy, float interval, LineGroup& lg){
    float t = 0;

    float nx = (((1 - t) * (1 - t)) * x1) + ((2 * t) * (1 - t) * ox) + (t * t * x2);
    float ny = (((1 - t) * (1 - t)) * y1) + ((2 * t) * (1 - t) * oy) + (t * t * y2);

    while(t < 1){
        float x = nx;
        float y = ny;
        t += interval;
        nx = (((1 - t) * (1 - t)) * x1) + ((2 * t) * (1 - t) * ox) + (t * t * x2);
        ny = (((1 - t) * (1 - t)) * y1) + ((2 * t) * (1 - t) * oy) + (t * t * y2);
        vecLine l = {x, y, nx, ny};
        lg.addLine(l);
    }
}

void getGlyphLines(GlyphShape g, LineGroup& lg){
    for(int i = 0; i < g.numContours; i++){
        int start = i == 0 ? 0 : g.contourEndPoints[i - 1] + 1;
        int end = g.contourEndPoints[i] + 1;

        GlyphPoint gp = g.points[start];
        for(int j = start; j < end; j++){
            GlyphPoint np = j == end - 1 ? g.points[start] : g.points[j + 1];
            if(np.onCurve){
                vecLine l = {gp.x, gp.y, np.x, np.y};
                lg.addLine(l);
                gp = np;
            }else{
                GlyphPoint p3 = j == end - 2 ? g.points[start] : g.points[j + 2];
                if(p3.onCurve){
                    getLinesFromCurve(gp.x, gp.y, p3.x, p3.y, np.x, np.y, 0.125, lg);
                    gp = p3;
                }else{
                    GlyphPoint bnp;
                    bnp.onCurve = true;
                    bnp.x = np.x + ((p3.x - np.x) / 2);
                    bnp.y = np.y + ((p3.y - np.y) / 2);
                    getLinesFromCurve(gp.x, gp.y, bnp.x, bnp.y, np.x, np.y, 0.125, lg);
                    gp = bnp;
                }
            }
        }
    }
}

unsigned short getGlyphAdvance(unsigned char* fileData, unsigned short characterCode){
    HheaTable* hhea = (HheaTable*)getPointerToTableData(fileData, "hhea");
    unsigned short numOfLongHorMetrics = SWAP16(hhea->numOfLongHorMetrics);
    unsigned short* hmtx = (unsigned short*)getPointerToTableData(fileData, "hmtx");
    MaxpTable* mp = (MaxpTable*)getPointerToTableData(fileData, "maxp");
    unsigned short numGlyphs = SWAP16(mp->numGlyphs);
    unsigned short adv = 0;

    if(numOfLongHorMetrics == numGlyphs){
        unsigned int indx = getGlyphIndex(fileData, characterCode);
        adv = SWAP16(hmtx[indx * 2]); 
    }else{
        //TODO: This may not be correct
        adv = SWAP16(hmtx[0]); 
    }
    return adv;
}

bool isPixelInside(float x, float y, LineGroup lg){
    int windCount = 0;

    for(int i = 0; i < lg.totalLines; i++){
        vecLine l = lg.lines[i];

        if((l.p1.x > x && l.p2.x > x) ||
           (l.p1.y > y && l.p2.y > y) ||
           (l.p1.y < y && l.p2.y < y)) {
               continue;
        }else if(l.p1.x == l.p2.x){
            if(l.p1.y <= y && l.p2.y > y){
                windCount++;
            } else if(l.p2.y <= y && l.p1.y > y){
                windCount--;
            }else{
                continue;
            }
        }else{
            float m = (l.p2.y - l.p1.y) / (l.p2.x - l.p1.x);
            float b = l.p2.y - (m * l.p2.x);
            float xCrs = (y / m) - (b / m);
            if(xCrs <= x){
                if(l.p1.y <= y && l.p2.y > y){
                    windCount++;
                } else if(l.p2.y <= y && l.p1.y > y){
                    windCount--;
                }
            }
        }
    }

    if(windCount != 0){
        return true;
    }
    return false;
}

unsigned char* getBitmapFromCharCode(unsigned char* fileData, unsigned short characterCode, unsigned int* width, unsigned int* height){
    GlyphShape gs;
    getGlyphShape(fileData, characterCode, &gs);
    LineGroup lg;
    getGlyphLines(gs, lg);

    *width = gs.xMax - gs.xMin;
    *height = gs.yMax - gs.yMin;

    unsigned char* bitmap = new unsigned char[*width * *height];
    int ctr = 0;
    for(int i = gs.yMin; i < gs.yMax; i++){
        for(int j = gs.xMin; j < gs.xMax; j++){
            if(isPixelInside(j, i, lg)){
                bitmap[ctr++] = 255;
            }else{
                bitmap[ctr++] = 0;
            }
        }
    }

    return bitmap;
}

unsigned char* getReducedBitmapFromCharCode(unsigned char* fileData, unsigned short characterCode, unsigned int* width, unsigned int* height, float* horzBng, float* vertBng, unsigned int divisions){
    GlyphShape gs;
    getGlyphShape(fileData, characterCode, &gs);
    LineGroup lg;
    getGlyphLines(gs, lg);

    *horzBng = (float)getGlyphAdvance(fileData, characterCode) / (float)divisions;
    *vertBng = (float)gs.yMin / (float)divisions;

    unsigned int gWidth = gs.xMax - gs.xMin;
    unsigned int gHeight = gs.yMax - gs.yMin;
    *width = (gWidth / divisions) + 1;
    *height = (gHeight / divisions) + 1;

    unsigned char* bitmap = new unsigned char[*width * *height];
    unsigned int ctr = 0;
    for(int i = 0; i < *height; i++){
        for(int j = 0; j < *width; j++){
            unsigned int pixTotal = 0;
            float k = (i * divisions * 0.9999) + gs.yMin;
            float kLimit = ((i + 1) * divisions * 0.9999) + gs.yMin;
            float l = (j * divisions * 0.9999) + gs.xMin;
            float lLimit = ((j + 1) * divisions * 0.9999) + gs.xMin;

            while(k < kLimit){
                while(l < lLimit){
                    if(!isPixelInside((int)l, (int)k, lg)){
                        pixTotal += 255;
                    }
                    l++;
                }
                k++;
            }
            if(pixTotal / divisions < 255){
                bitmap[ctr++] = 255;
            }else{
                bitmap[ctr++] = 0;
            } 
        }
    }
    return bitmap;
}

void freeBitmapMemory(unsigned char* mem){
    if(mem){
        delete[] mem;
        mem = 0;
    }
}