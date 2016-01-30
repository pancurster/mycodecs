#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

char code_table[66] = {
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
    'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
    'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm',
    'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '+', '/',
    ' ', '=' }; // last index 64,65

void encode_base64(char* buff, int size);
void encode_3to4(uint8_t one, uint8_t two, uint8_t three, char out[4]);

int main(int argc, char *argv[])
{
    if (argc < 2) {
        fprintf(stderr, "No file argument passed\n");
        exit(1);
    }

    // read file statistics
    struct stat st;
    const int fname = 1;
    if (stat(argv[fname], &st) == -1) {
        fprintf(stderr, "Error while stat %s\n", argv[fname]);
        exit(1);
    }

    // allocate buffer for file payload
    char* filebuf = NULL;
    if ((filebuf = calloc(sizeof(char), st.st_size + 1)) == NULL) {
        fprintf(stderr, "Error during calloc\n");
    }

    // open and read file content
    int fd = open(argv[fname], O_RDONLY);
    if (fd == -1) {
        fprintf(stderr, "Error opening %s\n", argv[fname]);
        exit(1);
    }
    int num_read = read(fd, filebuf, st.st_size + 1);
    if (num_read != st.st_size) {
        fprintf(stderr, "Error while reading %s\n", argv[fname]);
    }

    // encode to base64 and print to stdout
    encode_base64(filebuf, st.st_size);

    return 0;
}

void encode_base64(char* buff, int size)
{
    int i;
    char out[4];
    for (i = 0; i+3 < size; i+=3) {
        encode_3to4(buff[i], buff[i+1], buff[i+2], out);
        printf("%c%c%c%c", out[0], out[1], out[2], out[3]);
    }
    if (size % 3 == 1) {
        encode_3to4(buff[i], 0, 0, out);
        printf("%c%c%c%c", out[0], out[1], '=', '=');
    }
    if (size % 3 == 2) {
        encode_3to4(buff[i], buff[i+1], 0, out);
        printf("%c%c%c%c", out[0], out[1], out[2], '=');
    }
    printf("\n");
}

void encode_3to4(uint8_t one, uint8_t two, uint8_t three, char out[4])
{
    out[0] = code_table[ ((one & 0xFC) >> 2) ];
    out[1] = code_table[ (((one & 0x03) << 4) | ((two & 0xF0) >> 4)) ];
    out[2] = code_table[ (((two & 0x0F) << 2) | ((three & 0xC0) >> 6)) ];
    out[3] = code_table[ (three & 0x3F) ];
}

