#include "font_atlas.h"
#include "truetype_parser.h"

struct Bitmap {
    unsigned int width;
    unsigned int height;
    unsigned short charCode;
    unsigned char* bytes;
    float xShift;
    float yShift;

    Bitmap(){}
    Bitmap(unsigned char* bytes, unsigned width, unsigned height): width(width), height(height), bytes(bytes){}
};

struct Rectangle {
    unsigned int left;
    unsigned int right;
    unsigned int top;
    unsigned int bottom;
    unsigned int width;
    unsigned int height;
    unsigned int imageId;
    unsigned long area;
    Bitmap bitmap;
    Rectangle(){}
    Rectangle(unsigned int l, unsigned int r, unsigned int b, unsigned int t){
        left = l;
        right = r;
        top = t;
        bottom = b;
        width = r - l;
        height = t - b; 
        area = width * height;
        imageId = -1;
    }
};

struct RectangleList {
    Rectangle* rects;
    unsigned int totalRects;
    
    RectangleList(){
        totalRects = 0;
        rects = new Rectangle[0];
    }

    void add(Rectangle r){
        Rectangle* tRct = new Rectangle[totalRects + 1];
        tRct[totalRects] = r;

        for(int i = 0; i < totalRects; i++){
            tRct[i] = rects[i];
        }

        delete[] rects;
        rects = tRct;
        totalRects++;
    }

    Rectangle get(unsigned int i){
        return rects[i];
    }

    Rectangle remove(unsigned int v){
        Rectangle r = rects[v];

        Rectangle* tRct = new Rectangle[totalRects - 1];

        for(int i = 0; i < totalRects; i++){
            if(i < v){
                tRct[i] = rects[i];
            }else if(i > v){
                tRct[i - 1] = rects[i];
            }
        }

        delete[] rects;        
        rects = tRct;

        totalRects--;
        return r;
    }

    void clear(){
        if(rects){
            delete[] rects;
            rects = 0;
        }
        totalRects = 0;
    }
};

struct RectNode{
    bool set;
    Rectangle rect;
    RectNode* child1;
    RectNode* child2;

    RectNode(){
        set = false;
        child1 = 0;
        child2 = 0;
    }

    RectNode* add(Bitmap bmp){
        if(child1 && child2){
            RectNode* newNode = child1->add(bmp);
            if(newNode){
                return newNode;
            }else{
                return child2->add(bmp);
            }
        }else{
            if(set){
                return 0;
            }else if(rect.width < bmp.width || rect.height < bmp.height){
                return 0;
            }else if(rect.width == bmp.width && rect.height == bmp.height){
                rect.bitmap = bmp;
                set = true;
                return this;
            }else{
                child1 = new RectNode;
                child2 = new RectNode;
                int dw = rect.width - bmp.width;
                int dh = rect.height - bmp.height;

                if(dw > dh){
                    child1->rect = Rectangle(rect.left, rect.left + bmp.width, rect.bottom, rect.top);
                    child2->rect = Rectangle(rect.left + bmp.width, rect.right, rect.bottom, rect.top);
                }else{
                    child1->rect = Rectangle(rect.left, rect.right, rect.bottom, rect.bottom + bmp.height);
                    child2->rect = Rectangle(rect.left, rect.right, rect.bottom + bmp.height, rect.top);
                }

                return child1->add(bmp);
            }
        }
    }
};

static void sortBitmapsByDescendingArea(Bitmap* bitmaps, unsigned int totalBitmaps){
    for(int i = 0; i < totalBitmaps - 1; i++){
        for(int j = i + 1; j < totalBitmaps; j++){
            unsigned int area1 = bitmaps[i].width * bitmaps[i].height;
            unsigned int area2 = bitmaps[j].width * bitmaps[j].height;
            if(area1 < area2){
                Bitmap tempBitmap = bitmaps[i];
                bitmaps[i] = bitmaps[j];
                bitmaps[j] = tempBitmap;
            }
        }
    }
}

static void flattenNodeTree(RectNode* node, RectangleList* list){
    if(node->child1){
        flattenNodeTree(node->child1, list);
    }
    if(node->child2){
        flattenNodeTree(node->child2, list);
    }
    if(node->set){
        list->add(node->rect);
    }
}

static void clearNodeTree(RectNode* node){
    if(node->child1){
        clearNodeTree(node->child1);
    }
    if(node->child2){
        clearNodeTree(node->child2);
    }
    if(node){
        delete node;
    }
}

void clearFontAtlas(FontAtlas* fa){
    fa->id = -1;
    fa->totalCharacters = 0;
    if(fa->bitmap) delete[] fa->bitmap;
    if(fa->characterCodes) delete[] fa->characterCodes;
    if(fa->xOffsets) delete[] fa->xOffsets;
    if(fa->yOffsets) delete[] fa->yOffsets;
    if(fa->widths) delete[] fa->widths;
    if(fa->heights) delete[] fa->heights;
    if(fa->xShifts) delete[] fa->xShifts;
    if(fa->yShifts) delete[] fa->yShifts;
}

void buildFontAtlas(FontAtlas* fa, unsigned char* fontFileData, unsigned int totalCharacters, unsigned short* charCodes){
    unsigned int totalAcceptedChars = 0;
    Bitmap* bitmaps = new Bitmap[totalCharacters];
    for(int i = 0; i < totalCharacters; i++){
        unsigned int w, h;
        float ho, v;
        unsigned char* dats = getReducedBitmapFromCharCode(fontFileData, charCodes[i], &w, &h, &ho, &v, 32);
        if(dats){
            bitmaps[totalAcceptedChars].bytes = dats;
            bitmaps[totalAcceptedChars].width = w;
            bitmaps[totalAcceptedChars].height = h;
            bitmaps[totalAcceptedChars].xShift = ho;
            bitmaps[totalAcceptedChars].yShift = v;
            bitmaps[totalAcceptedChars].charCode = charCodes[i];
            totalAcceptedChars++;
        }
    }

    static const unsigned int MAX_SIZE = 20000;
    sortBitmapsByDescendingArea(bitmaps, totalAcceptedChars);
    RectNode* node = new RectNode;
    node->rect = Rectangle(0, MAX_SIZE, 0, MAX_SIZE);
    for(int i = 0; i < totalAcceptedChars; i++){
        node->add(bitmaps[i]);
    }
    RectangleList rects;
    flattenNodeTree(node, &rects);
    clearNodeTree(node);

    fa->widths = new unsigned int[totalAcceptedChars];
    fa->heights = new unsigned int[totalAcceptedChars];
    fa->xOffsets = new unsigned int[totalAcceptedChars];
    fa->yOffsets = new unsigned int[totalAcceptedChars];
    fa->xShifts = new float[totalAcceptedChars];
    fa->yShifts = new float[totalAcceptedChars];
    fa->characterCodes = new unsigned short[totalAcceptedChars];  

    unsigned int totalWidth = 0;
    unsigned int totalHeight = 0;
    for(int i = 0; i < rects.totalRects; i++){
        fa->widths[i] = rects.get(i).width;
        fa->heights[i] = rects.get(i).height;
        fa->xOffsets[i] = rects.get(i).left;
        fa->yOffsets[i] = rects.get(i).bottom;
        fa->characterCodes[i] = rects.get(i).bitmap.charCode;
        fa->xShifts[i] = rects.get(i).bitmap.xShift;
        fa->yShifts[i] = rects.get(i).bitmap.yShift;

        if(rects.get(i).right > totalWidth){
            totalWidth = rects.get(i).right;
        }
        if(rects.get(i).top > totalHeight){
            totalHeight = rects.get(i).top;
        }
    }

    unsigned char* bitmapData = new unsigned char[totalWidth * totalHeight];
    for(int i = 0; i < rects.totalRects; i++){
        Rectangle r = rects.get(i);
        Bitmap b = rects.get(i).bitmap;
        for(int j = r.bottom; j < r.top; j++){
            for(int k = r.left; k < r.right; k++){
                bitmapData[(j * totalWidth) + k] = b.bytes[((j - r.bottom) * b.width) + (k - r.left)];
            }
        }
    }

    fa->bitmap = bitmapData;
    fa->totalBitmapWidth = totalWidth;
    fa->totalBitmapHeight = totalHeight;
    fa->totalCharacters = totalAcceptedChars;
}