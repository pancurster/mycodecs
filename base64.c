#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

#define CODE_TABLE_SIZE 66
#define DECODE_TABLE_SIZE ('z' + 1)

char code_table[CODE_TABLE_SIZE] = {
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
    'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
    'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm',
    'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '+', '/',
    ' ', '=' }; // last index 64,65

char decode_table[DECODE_TABLE_SIZE] = {0xFF};

char* read_file(const char* fname, size_t* filebuf_size);
void use(const char* progname);
void encode_base64(char* buff, int size);
void encode_3to4(uint8_t one, uint8_t two, uint8_t three, char out[4]);
void decode_base64(char* buff, int size);
void decode_4to3(uint8_t four[4], uint8_t out[3]);

int main(int argc, char *argv[])
{
    if (argc != 3) {
        fprintf(stderr, "Bad arguments passed\n");
        use(argv[0]);
        exit(1);
    }

    const int fname = 2;
    size_t filebuf_size = 0;
    char* filebuf = read_file(argv[fname], &filebuf_size);

    char option = argv[1][1];
    switch (option) {
        case 'e':
            encode_base64(filebuf, filebuf_size);
            break;
        case 'd':
            decode_base64(filebuf, filebuf_size);
            break;
        default:
            use(argv[0]);
            exit(1);
    };
    return 0;
}

char* read_file(const char* fname, size_t* filebuf_size)
{
    // read file statistics
    struct stat st;
    if (stat(fname, &st) == -1) {
        fprintf(stderr, "Error while stat %s\n", fname);
        exit(1);
    }

    // allocate buffer for file payload
    char* filebuf = NULL;
    if ((filebuf = calloc(sizeof(char), st.st_size + 1)) == NULL) {
        fprintf(stderr, "Error during calloc\n");
    }

    // open and read file content
    int fd = open(fname, O_RDONLY);
    if (fd == -1) {
        fprintf(stderr, "Error opening %s\n", fname);
        exit(1);
    }
    int num_read = read(fd, filebuf, st.st_size + 1);
    if (num_read != st.st_size) {
        fprintf(stderr, "Error while reading %s\n", fname);
    }

    *filebuf_size = st.st_size;
    return filebuf;
}

void use(const char* progname)
{
    printf("%s -[e] -[d] file\n", progname);
    printf("Options:\n");
    printf("  -e    encode file to base64 format\n");
    printf("  -d    decode base64 coded file to binary\n");
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

void generate_decode_table(char decode_table[DECODE_TABLE_SIZE])
{
    char a;
    int i = 0;
    for (a='A'; a <= 'Z'; a++, i++) {
        decode_table[a] = i;
    }
    for (a='a'; a <= 'z'; a++, i++) {
        decode_table[a] = i;
    }
    for (a='0'; a <= '9'; a++, i++) {
        decode_table[a] = i;
    }
    decode_table['+'] = i++;
    decode_table['/'] = i; // 63/0x3F
}

void decode_base64(char* buff, int size)
{
    generate_decode_table(decode_table);

    const char* fname = "output.bin";
    int fd = open(fname, O_WRONLY | O_CREAT, 00644);
    if (fd == -1) {
        perror("Error opening file");
        exit(1);
    }

    int i;
    int bytes_writen = 0;
    char four[4];
    char out[3];
    for (i=0; i+4 < size; i+=4) {
        four[0] = buff[i];
        four[1] = buff[i+1];
        four[2] = buff[i+2];
        four[3] = buff[i+3];
        decode_4to3(four, out);
        if (write(fd, out, 3) <= 0) {
            perror("Error");
            exit(1);
        }
        bytes_writen += 3;
    }
    if (buff[i-2] == '=') {
        ftruncate(fd, bytes_writen-2);
    } else if (buff[i-1] == '=') {
        ftruncate(fd, bytes_writen-1);
    }
    close(fd);
}

void decode_4to3(uint8_t four[4], uint8_t out[3])
{
    out[0] = decode_table[ four[0] ] << 2;
    out[0] = out[0] | ((decode_table[ four[1] ] & 0x30) >> 4);
    out[1] = (decode_table[ four[1] ] & 0x0F) << 4;
    out[1] = out[1] | ((decode_table[ four[2] ] & 0x3C) >> 2);
    out[2] = (decode_table[ four[2] ] & 0x03) << 6;
    out[2] = out[2] | (decode_table[ four[3] ] & 0x03F);
}
