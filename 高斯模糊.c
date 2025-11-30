#define _CRT_SECURE_NO_WARNINGS 1
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

//TYPE BEGIN
typedef struct {
	int r;
	int g;
	int b;
} Pixel;

typedef struct {
	int width;
	int height;
	int colorset;
	Pixel* data;
} PPM;
//TYPE END

//VAR BEGIN
const char* READ_PATH = "C:\\code\\001 Í¼ÏñÑ§Ï°\\man.ppm";
const char* WRITE_PATH = "C:\\code\\001 Í¼ÏñÑ§Ï°\\man-blur-eye.ppm";
int ERR_STATE = 0;

enum {
	ERR_FILE_NOT_FOUND = 1,
	ERR_WRONG_FORMAT_HEADER,
	ERR_ILLEGAL_SIZE,
	ERR_FILE_BROKEN,
	ERR_FAILED_TO_WRITE
};

PPM inPPM;
PPM outPPM;
//VAR END

//FCUNTION BEGIN
int checkError() {
	return ERR_STATE;
}

void throwError(int error) {
	ERR_STATE = error;
}

void read() {
	if (freopen(READ_PATH, "r", stdin) == NULL) {
		throwError(ERR_FILE_NOT_FOUND);
		return;
	}
	char* FORMAT_HEADER = malloc(sizeof(char*));
	scanf("%s", FORMAT_HEADER);
	if (strcmp(FORMAT_HEADER, "P3") != 0) {
		throwError(ERR_WRONG_FORMAT_HEADER);
		return;
	}
	int width, height, colorset;
	scanf("%d%d%d", &width, &height, &colorset);
	if (width <= 0 || height <= 0) {
		throwError(ERR_ILLEGAL_SIZE);
		return;
	}
	inPPM.width = width;
	inPPM.height = height;
	inPPM.colorset = colorset;
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
}

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
	/*
	After this function is called once, the program still has other tasks to complete and will not terminate,
	so remember to free the memory
	(it's just a good habit??this small memory leak won't matter much on modern PCs).
	*/
	free(outPPM.data);
}


Pixel BLACK = { 0, 0, 0 };

Pixel* getPixel(PPM* source, int x, int y) {
	if (x < 0 || y < 0 || x >= source->width || y >= source->height) {
		return &BLACK;
	}
	return source->data + x + y * source->width;
}
double weight(double a, int x, int y, double* sum_weight) {
	double pi = 3.14;
	*sum_weight += 1 / (2 * pi * a * a) * exp(-(x * x + y * y) / (2 * a * a));
	return  1 / (2 * pi * a * a) * exp(-(x * x + y * y) / (2 * a * a));
}

Pixel blur(PPM* source, int x, int y, int radius) {
	Pixel p;
	p.r = 0;
	p.g = 0;
	p.b = 0;
	Pixel* temp;
	double sum_weight = 0.0;
	for (int i = x - radius; i <= x + radius; i++) {
		for (int j = y - radius; j <= y + radius; j++) {
			double WEIGHT = weight(5.0, i - x, j - y, &sum_weight);
			//printf("%lf\n ", WEIGHT);
			temp = getPixel(source, i, j);
			p.r += WEIGHT*temp->r;
			p.g += WEIGHT*temp->g;
			p.b += WEIGHT*temp->b;
		}
	}
	p.r /= sum_weight;
	p.g /= sum_weight;
	p.b /= sum_weight;
	return p;
}

void handle(int x1, int y1, int x2, int y2) {
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
			if (x1 <= x && x <= x2 && y1 <= y && y <= y2) {
				outPPM.data[x + y * width] = blur(&inPPM, x, y, 3);
			}
			else {
				outPPM.data[x + y * width] = inPPM.data[x + y * width];
			}
		}
	}
}
//FUNCTION END

int main() {
	read();
	handle(214, 339, 690, 417);
	write();
	return ERR_STATE;
}