#pragma once

struct Bitmap {
    unsigned int width;
    unsigned int height;
    unsigned char* bytes;
};

struct BitmapAtlas {
    unsigned int totalBitmaps;
    unsigned int totalWidth;
    unsigned int totalHeight;
    unsigned char* bitmapData;
    unsigned int* widths;
    unsigned int* heights;
    unsigned int* xOffsets;
    unsigned int* yOffsets;
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
        }
        totalRects = 0;
    }
};

Bitmap genBitmap(unsigned char* bytes, unsigned width, unsigned height){
    Bitmap b;
    b.bytes = bytes;
    b.width = width;
    b.height = height;
    return b;
}

void sortBitmapsByDescendingArea(Bitmap* bitmaps, unsigned int totalBitmaps){
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

BitmapAtlas createBitmapAtlas(Bitmap* bitmaps, unsigned int totalBitmaps){
    BitmapAtlas ba;
    sortBitmapsByDescendingArea(bitmaps, totalBitmaps);
    static const unsigned int MAX_SIZE = 16384;

    RectangleList rectList;
    RectangleList completed;
    Rectangle root(0, MAX_SIZE, 0, MAX_SIZE);
    rectList.add(root);

    unsigned int totalWidth = 0;
    unsigned int totalHeight = 0;

    for(int i = 0; i < totalBitmaps; i++){
        for(int j = 0; j < rectList.totalRects; j++){
            Rectangle r = rectList.get(j);
            if(r.width > bitmaps[i].width && r.height > bitmaps[i].height){
                rectList.rects[j] = Rectangle(r.left, r.left + bitmaps[i].width, r.bottom, r.bottom + bitmaps[i].height); 
                if(rectList.rects[j].right > totalWidth){
                    totalWidth = rectList.rects[j].right;
                }
                if(rectList.rects[j].top > totalHeight){
                    totalHeight = rectList.rects[j].top;
                }
                rectList.rects[j].imageId = i;
                completed.add(rectList.remove(j));
                unsigned int dw = r.width - bitmaps[i].width;
                unsigned int dh = r.height - bitmaps[i].height;
                if(dw > dh){
                    rectList.add(Rectangle(r.left, r.left + bitmaps[i].width, r.bottom + bitmaps[i].height, r.top));
                    rectList.add(Rectangle(r.left + bitmaps[i].width, r.right, r.bottom, r.top));
                }else{
                    rectList.add(Rectangle(r.left + bitmaps[i].width, r.right, r.bottom, r.bottom + bitmaps[i].height));
                    rectList.add(Rectangle(r.left, r.right, r.bottom + bitmaps[i].height, r.top));
                }
                break;
            }
        }
    }
    
    rectList.clear();

    unsigned char* bitmapData = new unsigned char[totalWidth * totalHeight];

    for(int i = 0; i < completed.totalRects; i++){
        Rectangle r = completed.get(i);
        Bitmap b = bitmaps[r.imageId];
        for(int j = r.bottom; j < r.top; j++){
            for(int k = r.left; k < r.right; k++){
                bitmapData[(j * totalWidth) + k] = b.bytes[((j - r.bottom) * b.width) + (k - r.left)];
            }
        }
    }
    ba.bitmapData = bitmapData;
    ba.totalWidth = totalWidth;
    ba.totalHeight = totalHeight;
    return ba;
}

Bitmap combine2Bitmaps(Bitmap b1, Bitmap b2){
    Bitmap result;
    result.width = b1.width + b2.width;
    if(b1.height >= b2.height){
        result.height = b1.height;
    }else{
        result.height = b2.height;
    }

    result.bytes = new unsigned char[result.width * result.height];

    for(int i = 0; i < result.height; i++){
        for(int j = 0; j < result.width; j++){
            if(j < b1.width){
                if(i < b1.height){
                    result.bytes[(i * result.width) + j] = b1.bytes[(i * b1.width) + j];
                }else{
                    result.bytes[(i * result.width) + j] = 0;
                }
            }else{
                if(i < b2.height){
                    result.bytes[(i * result.width) + j] = b2.bytes[(i * b2.width) + (j - b1.width)];
                }else{
                    result.bytes[(i * result.width) + j] = 0;
                }
            }
        }
    }
    
    return result;
}