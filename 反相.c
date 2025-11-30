#define _CRT_SECURE_NO_WARNINGS 1
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
const char* READ_PATH = "C:\\code\\helloworld.ppm";
const char* WRITE_PATH = "C:\\code\\(∑¥œ‡)helloworld.ppm";

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
}

Pixel invert(Pixel* source, int colorset) {
	Pixel p;
	p.r = colorset - source->r;
	p.g = colorset - source->g;
	p.b = colorset - source->b;
	return p;
}

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
//FUNCTION END

int main() {
	read();
	handle();
	write();
	return ERR_STATE;
}