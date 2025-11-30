# image-learning
## PPM P3程序
### 一、程序核心目标
- 读取 PPM P3 格式图像
- 对像素进行反相处理
- 输出反相后的 PPM P3 文件
- 全程错误捕获与内存安全管理
### 二、数据结构定义
- Pixel 结构体：存储单个像素 RGB 通道（r/g/b 整型变量）
> typedef struct {
	int r;
	int g;
	int b;
} Pixel; 
- PPM 结构体：封装图像信息（width/height/max_val 整型 + Pixel* 像素数组指针）
>typedef struct {
	int width;
	int height;
	int colorset;
	Pixel *data;
} PPM;
### 三、全局变量与枚举
- 路径常量：READ_PATH（输入文件路径）、WRITE_PATH（输出文件路径）
>const char* READ_PATH = "C:\\code\\helloworld.ppm";
const char* WRITE_PATH = "C:\\code\\(反相)helloworld.ppm";

- 错误状态：ERR_STATE（0=无错误，非0=对应错误码）
- 错误枚举：6类错误（文件未找到/格式错误/非法尺寸/文件损坏/写入失败/内存分配失败）
>int ERR_STATE = 0;
enum {
	ERR_FILE_NOT_FOUND = 1,
	ERR_WRONG_FORMAT_HEADER,
	ERR_ILLEGAL_SIZE,
	ERR_FILE_BROKEN,
	ERR_FAILED_TO_WRITE
};
- 全局实例：inPPM（输入图像）、outPPM（输出图像）
>PPM inPPM;
PPM outPPM;
### 四、核心功能函数
#### 1. 错误处理函数
- checkError()：返回当前错误状态码
- throwError(int)：设置全局错误状态码
>int checkError() {
	return ERR_STATE;
}
void throwError(int error) {
	ERR_STATE = error;
}
#### 2. 读取函数（read()）
- 重定向 stdin 到输入文件，检查文件存在性  
    ``` c printf
    if (freopen(READ_PATH, "r", stdin) == NULL) {
        throwError(ERR_FILE_NOT_FOUND);
            return;
    }     
- 读取并验证魔数「P3」
    ``` c printf 
    char* FORMAT_HEADER = malloc(sizeof(char*));
    scanf("%s", FORMAT_HEADER);
    if (strcmp(FORMAT_HEADER, "P3") != 0) {
        throwError(ERR_WRONG_FORMAT_HEADER);
        return;
    }
- 解析宽高、max_val 并验证合法性
    ``` c printf
    int width, height, colorset;
    scanf("%d%d%d", &width, &height, &colorset);
    if (width <= 0 || height <= 0) {
        throwError(ERR_ILLEGAL_SIZE);
        return;
    }
    inPPM.width = width;
    inPPM.height = height;
    inPPM.colorset = colorset;
- 动态分配像素数组，读取所有 RGB 数据存入 inPPM
- 读取失败时释放内存并抛错
    ``` c printf
    inPPM.data = malloc(sizeof(Pixel) * width * height);
	int r, g, b;
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			if (scanf("%d%d%d", &r, &g, &b)) {
				inPPM.data[x + y * width].r = r;
				inPPM.data[x + y * width].g = g;
				inPPM.data[x + y * width].b = b;
			}
			else {
				throwError(ERR_FILE_BROKEN);
				return;
			}
		}
	}
	fclose(stdin);
#### 3. 反相函数（invert()）（可以随需求改变）
- 输入：原像素指针、max_val
- 逻辑：各通道值 = max_val - 原通道值
- 输出：反相后的 Pixel 实例
    ``` c printf
    Pixel invert(Pixel* source, int colorset) {
        Pixel p;
        p.r = colorset - source->r;
        p.g = colorset - source->g;
        p.b = colorset - source->b;
        return p;
    }
#### 4. 处理函数（handle()）(处理函数可以随需求改变)
- 校验无错误后，复制 inPPM 宽高/max_val 到 outPPM
- 为 outPPM 分配像素内存（检查分配结果）
- 逐像素调用 invert()，结果存入 outPPM 像素数组
    ``` c printf
    void handle() {
        if (checkError()) {
            return;
        }
        int width = inPPM.width;
        int height = inPPM.height;
        outPPM.width = width;
        outPPM.height = height;
        outPPM.colorset = inPPM.colorset;
        outPPM.data = malloc(sizeof(Pixel) * width * height);
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                outPPM.data[x + y * width] = invert(inPPM.data + x + y * width, inPPM.colorset);
            }
        }
    }
#### 5. 写入函数（write()）
- 校验无错误后，以文本模式打开输出文件
- 写入文件头（P3 + 宽高 + max_val）
- 按「3像素/行」紧凑格式写入反相像素数据
- 检查写入结果，失败则抛错并关闭文件
    ``` c printf
    void write() {
        if (checkError()) {
            return;
        }
        FILE* file = fopen(WRITE_PATH, "wb");
        if (file == NULL) {
            throwError(ERR_FAILED_TO_WRITE);
            return;
        }
        int flag = 0;
        flag |= 0 >= fprintf(file, "P3\n");
        flag |= 0 >= fprintf(file, "%d %d\n", outPPM.width, outPPM.height);
        flag |= 0 >= fprintf(file, "%d\n", outPPM.colorset);
        int N = outPPM.width * outPPM.height;
        for (int i = 0; i < N; i++) {
            flag |= 0 >= fprintf(file, "%d\n", outPPM.data[i].r);
            flag |= 0 >= fprintf(file, "%d\n", outPPM.data[i].g);
            flag |= 0 >= fprintf(file, "%d\n", outPPM.data[i].b);
        }
        if (flag) {
            throwError(ERR_FAILED_TO_WRITE);
            return;
        }
        fflush(file);
        fclose(file);
    }

#### 6. 内存释放函数（freePPM()）
    void freePPM(PPM *ppm) {
        if (ppm->data != NULL) {  // 避免重复释放（NULL指针free会崩溃）
            free(ppm->data);
            ppm->data = NULL;  // 释放后置为NULL，防止野指针
        }
    }
### 五、主函数（main()）
- 初始化：inPPM/outPPM 像素指针置 NULL
- 执行流程：read() → handle() → write()
- 资源释放：调用 freePPM() 释放 inPPM/outPPM 内存
- 返回值：ERR_STATE（0=成功，非0=错误）
    ``` c printf
    int main() {
        // 1. 初始化：将像素数组指针置为NULL（避免野指针）
        inPPM.data = NULL;
        outPPM.data = NULL;

        // 2. 核心流程：读→处理→写
        read();    // 读输入文件到inPPM
        handle();  // 反相处理，结果存入outPPM
        write();   // 将outPPM写入输出文件

        // 3. 释放内存（必须在所有操作后执行）
        freePPM(&inPPM);
        freePPM(&outPPM);

        return ERR_STATE;  // 程序退出码：0=成功，非0=错误
    }
### 六、关键注意事项
- 格式要求：输入必须是合法 PPM P3 文本格式
- 路径规范：Windows 系统路径用双反斜杠（\\）或单斜杠（/）
- 内存安全：malloc 后必检查 NULL，使用后必 free
- 错误捕获：全流程校验，覆盖文件/格式/内存/写入等场景
