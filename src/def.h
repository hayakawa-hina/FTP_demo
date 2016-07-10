#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>
#include <sys/stat.h>

#define BUF_LEN 256
#define DATASIZE 1024
#define NO_CHANGE -1

#define TYPE_QUIT 0x01
#define TYPE_PWD 0x02
#define TYPE_CWD 0x03
#define TYPE_LIST 0x04
#define TYPE_RETR 0x05
#define TYPE_STOR 0x06
#define TYPE_OK 0x10
#define TYPE_CMD_ERR 0x11
#define TYPE_FILE_ERR 0x12
#define TYPE_UKWN_ERR 0x13
#define TYPE_DATA 0x20

#define CODE_0x00 0x00
#define CODE_0x01 0x01
#define CODE_0x02 0x02
#define CODE_0x03 0x03
#define CODE_0x05 0x05
#define CODE_0x06 0x06//指定したファイル/ディレクトリの区分が違う（ファイルを指定するはずなのにディレクトリ）

struct myftph {
	uint8_t type;
	uint8_t code;
	uint16_t data_length;
};

struct myftph_data {
	uint8_t type;
	uint8_t code;
	uint16_t data_length;
	char data[DATASIZE];
};
