#pragma once
#pragma warning (disable : 4996)	
#include "sys/ipc.h"
#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#define lba_num 4096

struct info {
	int operation; //1 : read, 2 : write
	int LBA; //0~2048
	unsigned int data; //any data with 1~32bit
	int readable; //read sign from FTL to Protocol
	int ftl_sign; //ftl work start
	float write_data;
	int written_data;
	int gc_count;
};
typedef struct info info;

extern int shmid;
extern info* shm;

int return_opnum(char*);
unsigned int random_generator(int bit_num);
void read();
void add_write_data(unsigned int data);
void write_data(unsigned int lba, unsigned int data);
void write_file();
void write_immediate();
void make_input_file();