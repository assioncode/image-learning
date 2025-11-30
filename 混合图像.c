#define _CRT_SECURE_NO_WARNINGS 1
#define _CRT_SECURE_NO_WARNINGS 1
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 像素结构定义
typedef struct {
    int r;
    int g;
    int b;
} Pixel;

// PPM图像结构定义
typedef struct {
    int width;
    int height;
    int colorset;  // 最大颜色值
    Pixel* data;
} PPM;

// 文件路径
const char* READ_PATH_1 = "C://code//图像学习//helloworld.ppm";
const char* READ_PATH_2 = "C://code//图像学习//apple.ppm";
const char* WRITE_PATH = "C://code//图像学习//混合像.ppm";

// 错误状态与枚举
int ERR_STATE = 0;
enum {
    ERR_FILE_NOT_FOUND = 1,
    ERR_WRONG_FORMAT_HEADER,
    ERR_ILLEGAL_SIZE,
    ERR_FILE_BROKEN,
    ERR_FAILED_TO_WRITE,
    ERR_MEMORY_ALLOC
};

// 全局图像变量
PPM inPPM_1;
PPM inPPM_2;
PPM outPPM;

// 错误处理函数
int checkError() {
    return ERR_STATE;
}

void throwError(int error) {
    ERR_STATE = error;
}

// 安全的内存分配函数
void* safeMalloc(size_t size) {
    void* ptr = malloc(size);
    if (!ptr) {
        throwError(ERR_MEMORY_ALLOC);
    }
    return ptr;
}

// 读取PPM图像
void readPPM(const char* path, PPM* ppm) {
    FILE* file = fopen(path, "r");
    if (!file) {
        throwError(ERR_FILE_NOT_FOUND);
        return;
    }

    char formatHeader[4];
    if (fscanf(file, "%3s", formatHeader) != 1 || strcmp(formatHeader, "P3") != 0) {
        fclose(file);
        throwError(ERR_WRONG_FORMAT_HEADER);
        return;
    }

    int width, height, colorset;
    if (fscanf(file, "%d %d %d", &width, &height, &colorset) != 3) {
        fclose(file);
        throwError(ERR_FILE_BROKEN);
        return;
    }

    if (width <= 0 || height <= 0) {
        fclose(file);
        throwError(ERR_ILLEGAL_SIZE);
        return;
    }

    ppm->width = width;
    ppm->height = height;
    ppm->colorset = colorset;
    ppm->data = (Pixel*)safeMalloc(sizeof(Pixel) * width * height);
    if (checkError()) {
        fclose(file);
        return;
    }

    int r, g, b;
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            if (fscanf(file, "%d %d %d", &r, &g, &b) != 3) {
                fclose(file);
                free(ppm->data);
                ppm->data = NULL;
                throwError(ERR_FILE_BROKEN);
                return;
            }
            // 确保颜色值在有效范围内
            r = (r < 0) ? 0 : (r > colorset ? colorset : r);
            g = (g < 0) ? 0 : (g > colorset ? colorset : g);
            b = (b < 0) ? 0 : (b > colorset ? colorset : b);

            ppm->data[x + y * width].r = r;
            ppm->data[x + y * width].g = g;
            ppm->data[x + y * width].b = b;
        }
    }

    fclose(file);
}

// 读取两张图像
void read() {
    readPPM(READ_PATH_1, &inPPM_1);
    if (checkError()) return;

    readPPM(READ_PATH_2, &inPPM_2);
}

// 正片叠底混合
Pixel multiplyBlend(Pixel* source_1, Pixel* source_2, int maxVal) {
    Pixel p;
    // 正片叠底公式: (a * b) / maxVal
    p.r = (source_1->r * source_2->r) / maxVal;
    p.g = (source_1->g * source_2->g) / maxVal;
    p.b = (source_1->b * source_2->b) / maxVal;
    return p;
}

// 处理不同尺寸图像的混合
void handle() {
    if (checkError()) return;

    // 确定输出图像的尺寸为两张图像中的较大尺寸
    int outWidth = (inPPM_1.width > inPPM_2.width) ? inPPM_1.width : inPPM_2.width;
    int outHeight = (inPPM_1.height > inPPM_2.height) ? inPPM_1.height : inPPM_2.height;
    int maxColorset = (inPPM_1.colorset > inPPM_2.colorset) ? inPPM_1.colorset : inPPM_2.colorset;

    // 分配输出图像内存
    outPPM.width = outWidth;
    outPPM.height = outHeight;
    outPPM.colorset = maxColorset;
    outPPM.data = (Pixel*)safeMalloc(sizeof(Pixel) * outWidth * outHeight);
    if (checkError()) return;

    // 逐像素处理
    for (int y = 0; y < outHeight; y++) {
        for (int x = 0; x < outWidth; x++) {
            // 检查当前坐标是否在两张图像的范围内
            int inImg1 = (x < inPPM_1.width && y < inPPM_1.height);
            int inImg2 = (x < inPPM_2.width && y < inPPM_2.height);

            if (inImg1 && inImg2) {
                // 两个图像都有此像素，进行混合
                Pixel p1 = inPPM_1.data[x + y * inPPM_1.width];
                Pixel p2 = inPPM_2.data[x + y * inPPM_2.width];
                outPPM.data[x + y * outWidth] = multiplyBlend(&p1, &p2, maxColorset);
            }
            else if (inImg1) {
                // 只有第一张图像有此像素，直接使用
                outPPM.data[x + y * outWidth] = inPPM_1.data[x + y * inPPM_1.width];
            }
            else if (inImg2) {
                // 只有第二张图像有此像素，直接使用
                outPPM.data[x + y * outWidth] = inPPM_2.data[x + y * inPPM_2.width];
            }
            // 理论上不会走到这里，因为输出尺寸是较大的那个
        }
    }
}

// 写入混合后的图像
void write() {
    if (checkError()) return;

    FILE* file = fopen(WRITE_PATH, "w");
    if (!file) {
        throwError(ERR_FAILED_TO_WRITE);
        return;
    }

    int flag = 0;
    // 写入PPM文件头
    flag |= fprintf(file, "P3\n") < 0;
    flag |= fprintf(file, "%d %d\n", outPPM.width, outPPM.height) < 0;
    flag |= fprintf(file, "%d\n", outPPM.colorset) < 0;

    // 写入像素数据
    for (int y = 0; y < outPPM.height; y++) {
        for (int x = 0; x < outPPM.width; x++) {
            Pixel* p = &outPPM.data[x + y * outPPM.width];
            flag |= fprintf(file, "%d %d %d ", p->r, p->g, p->b) < 0;
        }
        flag |= fprintf(file, "\n") < 0;  // 每行结束添加换行
    }

    if (flag) {
        throwError(ERR_FAILED_TO_WRITE);
    }
    fclose(file);
}

// 释放图像内存
void freePPM(PPM* ppm) {
    if (ppm->data) {
        free(ppm->data);
        ppm->data = NULL;
    }
}

// 错误信息提示
const char* getErrorMsg() {
    switch (ERR_STATE) {
    case ERR_FILE_NOT_FOUND: return "文件未找到";
    case ERR_WRONG_FORMAT_HEADER: return "文件格式错误（不是P3格式）";
    case ERR_ILLEGAL_SIZE: return "图像尺寸无效";
    case ERR_FILE_BROKEN: return "文件损坏或格式错误";
    case ERR_FAILED_TO_WRITE: return "写入文件失败";
    case ERR_MEMORY_ALLOC: return "内存分配失败";
    default: return "未知错误";
    }
}

int main() {
    read();
    if (checkError()) {
        printf("读取图像失败: %s\n", getErrorMsg());
        freePPM(&inPPM_1);
        freePPM(&inPPM_2);
        return ERR_STATE;
    }

    handle();
    if (checkError()) {
        printf("处理图像失败: %s\n", getErrorMsg());
        freePPM(&inPPM_1);
        freePPM(&inPPM_2);
        freePPM(&outPPM);
        return ERR_STATE;
    }

    write();
    if (checkError()) {
        printf("写入图像失败: %s\n", getErrorMsg());
        freePPM(&inPPM_1);
        freePPM(&inPPM_2);
        freePPM(&outPPM);
        return ERR_STATE;
    }

    printf("图像混合成功，已保存至: %s\n", WRITE_PATH);

    // 释放内存
    freePPM(&inPPM_1);
    freePPM(&inPPM_2);
    freePPM(&outPPM);

    return 0;
}