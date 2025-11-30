#define _CRT_SECURE_NO_WARNINGS 1
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

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
    ERR_WRITE_FAILED
} ErrorCode;

// 全局错误信息映射
const char* error_messages[] = {
    "操作成功",
    "错误：文件未找到",
    "错误：不是PPM P3格式",
    "错误：图像尺寸或最大像素值不合法",
    "错误：内存分配失败",
    "错误：文件数据损坏",
    "错误：写入文件失败"
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
 * RGB转灰度图（生成临时灰度数组）
 * @param in：输入彩色PPM图像
 * @param gray：输出灰度数组（需提前分配内存）
 */
void rgbToGray(const PPM* in, unsigned char* gray) {
    if (in == NULL || gray == NULL || in->data == NULL) {
        return;
    }

    for (int i = 0; i < in->width * in->height; i++) {
        // 加权灰度公式（符合人眼亮度感知）
        double r = in->data[i].r;
        double g = in->data[i].g;
        double b = in->data[i].b;
        gray[i] = (unsigned char)(0.299 * r + 0.587 * g + 0.114 * b);
    }
}

/**
 * Sobel边缘检测核心函数
 * @param in：输入彩色PPM图像
 * @param out：输出边缘图（PPM格式，灰度边缘）
 * @param threshold：边缘阈值（0~255）
 * @return 错误码（SUCCESS=成功）
 */
ErrorCode sobelEdgeDetect(const PPM* in, PPM* out, unsigned char threshold) {
    // 基础校验
    if (in == NULL || out == NULL || in->data == NULL) {
        return ERR_FILE_BROKEN;
    }
    if (in->width < 3 || in->height < 3) {
        return ERR_ILLEGAL_SIZE;  // 至少3×3图像才能进行3×3卷积
    }

    // 初始化输出图像结构体
    out->width = in->width;
    out->height = in->height;
    out->max_val = 255;  // 边缘图用255（白）表示边缘，0（黑）表示背景
    out->data = (Pixel*)malloc(sizeof(Pixel) * out->width * out->height);
    if (out->data == NULL) {
        return ERR_MEMORY_ALLOC;
    }

    // 分配灰度数组内存
    unsigned char* gray = (unsigned char*)malloc(sizeof(unsigned char) * in->width * in->height);
    if (gray == NULL) {
        freePPM(out);
        return ERR_MEMORY_ALLOC;
    }

    // RGB转灰度
    rgbToGray(in, gray);

    // Sobel卷积核（Gx：水平边缘，Gy：垂直边缘）
    int Gx[3][3] = { {-1, 0, 1}, {-2, 0, 2}, {-1, 0, 1} };
    int Gy[3][3] = { {-1, -2, -1}, {0, 0, 0}, {1, 2, 1} };

    // 核心卷积计算（跳过边界像素）
    for (int y = 1; y < in->height - 1; y++) {  // 行索引（跳过第0行和最后一行）
        for (int x = 1; x < in->width - 1; x++) {  // 列索引（跳过第0列和最后一列）
            int gx = 0, gy = 0;

            // 3×3邻域卷积
            for (int ky = -1; ky <= 1; ky++) {  // 卷积核行偏移
                for (int kx = -1; kx <= 1; kx++) {  // 卷积核列偏移
                    // 邻域像素在灰度数组中的索引
                    int gray_idx = (x + kx) + (y + ky) * in->width;
                    // 累加卷积结果
                    gx += gray[gray_idx] * Gx[ky + 1][kx + 1];  // ky+1：将-1→0，适配卷积核索引
                    gy += gray[gray_idx] * Gy[ky + 1][kx + 1];
                }
            }

            // 计算梯度幅值（精确版：sqrt(Gx? + Gy?)）
            double magnitude = sqrt((double)(gx * gx + gy * gy));
            // 简化版（效率更高，可替换上面一行）：int magnitude = abs(gx) + abs(gy);

            // 阈值二值化：超过阈值为边缘（白色），否则为背景（黑色）
            unsigned char edge = (magnitude >= threshold) ? 255 : 0;

            // 边缘图存入PPM（RGB三通道相同，表现为灰度图）
            int out_idx = x + y * out->width;
            out->data[out_idx].r = edge;
            out->data[out_idx].g = edge;
            out->data[out_idx].b = edge;
        }
    }

    // 边界像素设为黑色（无卷积结果）
    for (int y = 0; y < out->height; y++) {
        for (int x = 0; x < out->width; x++) {
            if (x == 0 || x == out->width - 1 || y == 0 || y == out->height - 1) {
                int out_idx = x + y * out->width;
                out->data[out_idx].r = 0;
                out->data[out_idx].g = 0;
                out->data[out_idx].b = 0;
            }
        }
    }

    // 释放临时灰度数组
    free(gray);
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

    // 写入文件头
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
        if ((i + 1) % 3 == 0) {  // 每3个像素换行
            fprintf(file, "\n");
        }
    }

    fclose(file);
    return SUCCESS;
}

/**
 * 主函数：完整流程控制
 */
int main() {
    // 1. 配置参数
    const char* input_path = "C:\\code\\001 图像学习\\man.ppm";    // 输入彩色PPM路径
    const char* output_path = "C:\\code\\001 图像学习\\(边缘查找)man.ppm";  // 输出边缘图路径
    unsigned char sobel_threshold = 50;      // 边缘阈值（0~255，可调整）

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
    printf("读取成功：%dx%d 像素，最大像素值：%d\n", in_ppm.width, in_ppm.height, in_ppm.max_val);

    // 4. Sobel边缘检测
    printf("正在进行Sobel边缘检测（阈值：%d）...\n", sobel_threshold);
    ErrorCode sobel_ret = sobelEdgeDetect(&in_ppm, &out_ppm, sobel_threshold);
    if (sobel_ret != SUCCESS) {
        printf("%s\n", error_messages[sobel_ret]);
        freePPM(&in_ppm);
        freePPM(&out_ppm);
        return sobel_ret;
    }
    printf("边缘检测完成\n");

    // 5. 保存结果
    printf("正在保存边缘图：%s...\n", output_path);
    ErrorCode write_ret = writePPM(output_path, &out_ppm);
    if (write_ret != SUCCESS) {
        printf("%s\n", error_messages[write_ret]);
        freePPM(&in_ppm);
        freePPM(&out_ppm);
        return write_ret;
    }
    printf("保存成功\n");

    // 6. 释放内存
    freePPM(&in_ppm);
    freePPM(&out_ppm);

    return SUCCESS;
}