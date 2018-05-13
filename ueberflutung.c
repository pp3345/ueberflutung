#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stddef.h>

#define HOST "127.0.0.1"
#define PORT 1337

typedef struct tagBITMAPFILEHEADER {
 short  bfType;
  int bfSize;
  short  bfReserved1;
  short  bfReserved2;
  int bfOffBits;
} __attribute__((packed)) BITMAPFILEHEADER;

typedef struct tagBITMAPINFOHEADER {
  int biSize;
  int  biWidth;
  int  biHeight;
  short  biPlanes;
  short  biBitCount;
  int biCompression;
  int biSizeImage;
  int  biXPelsPerMeter;
  int  biYPelsPerMeter;
  int biClrUsed;
  int biClrImportant;
} __attribute__((packed)) BITMAPINFOHEADER;

typedef struct {
 BITMAPFILEHEADER fileh;
 BITMAPINFOHEADER infoh;
 unsigned char content[1048576];
} __attribute__((packed)) bitmap;

void main(int argc, char **argv) {
	bitmap image;
	unsigned char*content;

	if(argc < 2) {
		printf("Missing filename\n");
		exit(1);
	}

	FILE *bmp = fopen(argv[1], "r");
	fread(&image, 1048576, 1, bmp);
	fclose(bmp);

	content = ((char*) &image) + image.fileh.bfOffBits;

	printf("h: %i, w: %i\n", image.infoh.biHeight, image.infoh.biWidth);

	struct sockaddr_in addr = {
		.sin_family = AF_INET,
		.sin_port = htons(PORT)
	};

	inet_pton(AF_INET, HOST, &addr.sin_addr);

	int sock = socket(AF_INET, SOCK_STREAM, SOL_TCP);
	connect(sock, (struct sockaddr*) &addr, sizeof(struct sockaddr_in));

	char* hexified = malloc(image.infoh.biHeight * image.infoh.biWidth * 6);
	for(int y = 0; y < image.infoh.biHeight; y++) {
		for(int x = 0; x < image.infoh.biWidth; x++) {
			char r[3], g[3], b[3];
			sprintf(r, "%02X", (unsigned int) content[(y * image.infoh.biWidth + x) * 3 + 2]);
			sprintf(g, "%02X", (unsigned int) content[(y * image.infoh.biWidth + x) * 3 + 1]);
			sprintf(b, "%02X", (unsigned int) content[(y * image.infoh.biWidth + x) * 3]); // bitmaps are BGR, not RGB
			/*write(1, r, 2);
			write(1, g, 2);
			write(1, b, 2);
			write(1, "\n", 1);*/
			hexified[(y * image.infoh.biWidth + x) * 6] = r[0];
			hexified[(y * image.infoh.biWidth + x) * 6 + 1] = r[1];
			hexified[(y * image.infoh.biWidth + x) * 6 + 2] = g[0];
			hexified[(y * image.infoh.biWidth + x) * 6 + 3] = g[1];
			hexified[(y * image.infoh.biWidth + x) * 6 + 4] = b[0];
			hexified[(y * image.infoh.biWidth + x) * 6 + 5] = b[1];
		}
	}

	while(1) {
		for(int y = 0; y < image.infoh.biHeight; y++) {
			for(int x = 0; x < image.infoh.biWidth; x++) {
				char cmd[18] = "PX ";
				char *ptr = cmd + 3;
				ptr += sprintf(ptr, "%d", x + 1);
				*ptr = ' ';
				ptr++;
				ptr += sprintf(ptr, "%d", y + 1);
				*ptr = ' ';
				ptr++;
				//memcpy(ptr, "000000", 6);
				memcpy(ptr, hexified + ((image.infoh.biHeight - y - 1) * image.infoh.biWidth + x) * 6, 6);
				ptr += 6;
				*ptr = '\n';
				//write(1, cmd, ptr - cmd + 1);
				write(sock, cmd, ptr - cmd + 1);
			}
		}
	}
}
