#define _CRT_SECURE_NO_WARNINGS 1
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 像素结构体（RGB三通道）
typedef struct {
    int r;
    int g;
    int b;
} Pixel;

// PPM图像结构体
typedef struct {
    int width;    // 图像宽度
    int height;   // 图像高度
    int max_val;  // 最大像素值
    Pixel* data;  // 像素数据数组
} PPM;

// 错误码枚举
typedef enum {
    SUCCESS = 0,
    ERR_FILE_NOT_FOUND,
    ERR_WRONG_FORMAT,
    ERR_ILLEGAL_SIZE,
    ERR_MEMORY_ALLOC,
    ERR_FILE_BROKEN,
    ERR_WRITE_FAILED,
    ERR_CROP_OUT_OF_BOUNDS
} ErrorCode;

// 全局错误信息映射
const char* error_messages[] = {
    "操作成功",
    "错误：文件未找到",
    "错误：不是PPM P3格式",
    "错误：图像尺寸或最大像素值不合法",
    "错误：内存分配失败",
    "错误：文件数据损坏",
    "错误：写入文件失败",
    "错误：裁剪区域超出图像边界"
};

/**
 * 释放PPM图像的动态内存
 */
void freePPM(PPM* ppm) {
    if (ppm != NULL && ppm->data != NULL) {
        free(ppm->data);
        ppm->data = NULL;
    }
}

/**
 * 读取PPM P3格式图像
 * @param filename：输入文件路径
 * @param ppm：输出PPM结构体（需提前声明）
 * @return 错误码（SUCCESS=成功）
 */
ErrorCode readPPM(const char* filename, PPM* ppm) {
    // 初始化PPM结构体
    ppm->width = 0;
    ppm->height = 0;
    ppm->max_val = 0;
    ppm->data = NULL;

    // 打开文件
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        return ERR_FILE_NOT_FOUND;
    }

    // 读取魔数（验证P3格式）
    char format[4];
    if (fscanf(file, "%3s", format) != 1 || strcmp(format, "P3") != 0) {
        fclose(file);
        return ERR_WRONG_FORMAT;
    }

    // 跳过注释行（以#开头的行）
    char ch;
    while ((ch = fgetc(file)) == '#') {
        while (fgetc(file) != '\n');  // 跳过整行注释
    }
    ungetc(ch, file);  // 把非#字符放回输入流

    // 读取宽高和最大像素值
    if (fscanf(file, "%d%d%d", &ppm->width, &ppm->height, &ppm->max_val) != 3) {
        fclose(file);
        return ERR_FILE_BROKEN;
    }

    // 验证尺寸和最大像素值合法性
    if (ppm->width <= 0 || ppm->height <= 0 || ppm->max_val <= 0 || ppm->max_val > 255) {
        fclose(file);
        return ERR_ILLEGAL_SIZE;
    }

    // 分配像素内存
    ppm->data = (Pixel*)malloc(sizeof(Pixel) * ppm->width * ppm->height);
    if (ppm->data == NULL) {
        fclose(file);
        return ERR_MEMORY_ALLOC;
    }

    // 读取所有像素数据
    for (int i = 0; i < ppm->width * ppm->height; i++) {
        if (fscanf(file, "%d%d%d", &ppm->data[i].r, &ppm->data[i].g, &ppm->data[i].b) != 3) {
            freePPM(ppm);
            fclose(file);
            return ERR_FILE_BROKEN;
        }
    }

    fclose(file);
    return SUCCESS;
}

/**
 * 图像裁剪核心函数（安全版，自动校验边界）
 * @param in：输入原图像
 * @param out：输出裁剪后图像
 * @param x0：裁剪区域左上角列索引（0-based）
 * @param y0：裁剪区域左上角行索引（0-based）
 * @param cropW：裁剪后图像宽度（≥1）
 * @param cropH：裁剪后图像高度（≥1）
 * @return 错误码（SUCCESS=成功）
 */
ErrorCode cropPPM(const PPM* in, PPM* out, int x0, int y0, int cropW, int cropH) {
    // 基础校验
    if (in == NULL || out == NULL || in->data == NULL) {
        return ERR_FILE_BROKEN;
    }

    // 校验裁剪区域合法性
    if (x0 < 0 || y0 < 0 || cropW <= 0 || cropH <= 0) {
        return ERR_CROP_OUT_OF_BOUNDS;
    }
    if (x0 + cropW > in->width || y0 + cropH > in->height) {
        return ERR_CROP_OUT_OF_BOUNDS;
    }

    // 初始化裁剪后图像信息
    out->width = cropW;
    out->height = cropH;
    out->max_val = in->max_val;

    // 分配裁剪后像素内存
    out->data = (Pixel*)malloc(sizeof(Pixel) * cropW * cropH);
    if (out->data == NULL) {
        return ERR_MEMORY_ALLOC;
    }

    // 核心裁剪逻辑：逐像素复制裁剪区域
    for (int y = 0; y < cropH; y++) {          // 遍历裁剪后图像的行
        for (int x = 0; x < cropW; x++) {      // 遍历裁剪后图像的列
            // 原图像中对应像素的索引：(x0 + x) + (y0 + y) * 原宽
            int inIndex = (x0 + x) + (y0 + y) * in->width;
            // 裁剪后图像的像素索引：x + y * 裁剪宽
            int outIndex = x + y * cropW;
            // 复制像素数据
            out->data[outIndex] = in->data[inIndex];
        }
    }

    return SUCCESS;
}

/**
 * 保存PPM P3格式图像
 * @param filename：输出文件路径
 * @param ppm：输入PPM图像
 * @return 错误码（SUCCESS=成功）
 */
ErrorCode writePPM(const char* filename, const PPM* ppm) {
    if (filename == NULL || ppm == NULL || ppm->data == NULL) {
        return ERR_WRITE_FAILED;
    }

    // 打开文件（文本模式写入）
    FILE* file = fopen(filename, "w");
    if (file == NULL) {
        return ERR_WRITE_FAILED;
    }

    // 写入文件头（魔数、宽高、最大像素值）
    if (fprintf(file, "P3\n") < 0 ||
        fprintf(file, "%d %d\n", ppm->width, ppm->height) < 0 ||
        fprintf(file, "%d\n", ppm->max_val) < 0) {
        fclose(file);
        return ERR_WRITE_FAILED;
    }

    // 写入像素数据（每行3个像素，格式紧凑）
    for (int i = 0; i < ppm->width * ppm->height; i++) {
        if (fprintf(file, "%d %d %d ", ppm->data[i].r, ppm->data[i].g, ppm->data[i].b) < 0) {
            fclose(file);
            return ERR_WRITE_FAILED;
        }
        if ((i + 1) % 3 == 0) {  // 每3个像素换行，提升可读性
            fprintf(file, "\n");
        }
    }

    fclose(file);
    return SUCCESS;
}

/**
 * 主函数：完整裁剪流程控制
 */
int main() {
    // 1. 配置参数（可根据需求修改）
    const char* input_path = "C:\\code\\001 图像学习\\man.ppm";       // 输入PPM图像路径
    const char* output_path = "C:\\code\\001 图像学习\\(裁剪)man.ppm";  // 输出裁剪图像路径
    int crop_x0 = 50;    // 裁剪区域左上角列索引（0-based）
    int crop_y0 = 50;    // 裁剪区域左上角行索引（0-based）
    int crop_width = 500; // 裁剪后图像宽度
    int crop_height = 750; // 裁剪后图像高度

    // 2. 声明PPM结构体
    PPM in_ppm, out_ppm;
    memset(&in_ppm, 0, sizeof(PPM));
    memset(&out_ppm, 0, sizeof(PPM));

    // 3. 读取输入图像
    printf("正在读取图像：%s...\n", input_path);
    ErrorCode read_ret = readPPM(input_path, &in_ppm);
    if (read_ret != SUCCESS) {
        printf("%s\n", error_messages[read_ret]);
        return read_ret;
    }
    printf("读取成功：原图像尺寸 %dx%d 像素，最大像素值：%d\n",
        in_ppm.width, in_ppm.height, in_ppm.max_val);

    // 4. 执行图像裁剪
    printf("正在裁剪图像：起始坐标(%d,%d)，裁剪尺寸 %dx%d...\n",
        crop_x0, crop_y0, crop_width, crop_height);
    ErrorCode crop_ret = cropPPM(&in_ppm, &out_ppm, crop_x0, crop_y0, crop_width, crop_height);
    if (crop_ret != SUCCESS) {
        printf("%s\n", error_messages[crop_ret]);
        freePPM(&in_ppm);
        freePPM(&out_ppm);
        return crop_ret;
    }
    printf("裁剪成功：新图像尺寸 %dx%d 像素\n", out_ppm.width, out_ppm.height);

    // 5. 保存裁剪结果
    printf("正在保存裁剪图像：%s...\n", output_path);
    ErrorCode write_ret = writePPM(output_path, &out_ppm);
    if (write_ret != SUCCESS) {
        printf("%s\n", error_messages[write_ret]);
        freePPM(&in_ppm);
        freePPM(&out_ppm);
        return write_ret;
    }
    printf("保存成功！\n");

    // 6. 释放动态内存
    freePPM(&in_ppm);
    freePPM(&out_ppm);

    return SUCCESS;
}