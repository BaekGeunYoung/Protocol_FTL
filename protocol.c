#pragma warning (disable : 4996)	
#include "sys/ipc.h"
#include "protocol.h"
#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

int return_opnum(char* str) {
	int len = strlen(str);
	int count = 0;
	int i;
	int dec = 1;
	int res = 0;

	for (i = len - 4; i >= 0; i--) {
		if (str[i] != '_') count++;
		else break;
	}
	char new_str[50];
	memcpy(new_str, &str[++i], --count);
	new_str[count] = '\0';

	for (i = count - 1; i >= 0; i--) {
		res += ((int)new_str[i] - 48) * dec;
		dec *= 10;
	}
	return res;
}
unsigned int random_generator(int bit_num) {
	unsigned int res = 1;
	for (int i = 0; i < bit_num-1; i++) {
		res *= 2;
	}
	return res;
}
void read() {
	unsigned int lba;
	int read_data;
	printf("enter LBA : ");
	scanf("%d", &lba);

	shm->LBA = lba;
	shm->operation = 1;
	shm->ftl_sign = 1;

	while (1) {
		if (shm->readable == 1) {
			read_data = shm->data;
			printf("Data at %d : %d\n", lba, read_data);
			break;
		}
		if (shm->readable == 2) {
			printf("There is no data in this LBA.\n");
			break;
		}
	}
	shm->readable = 0;
}
void add_write_data(unsigned int data) {
	unsigned int exp = 1;
	for (int i = 0; i < 32; i++) {
		if (exp <= shm->data && shm->data < exp * 2) shm->write_data += (float)(i + 1) / (float)8;
		exp *= 2;
	}
}
void write_data(unsigned int lba, unsigned int data) {
	shm->LBA = lba;
	shm->data = data;
	shm->operation = 2;
	add_write_data(data);
	shm->readable = 0;
	shm->ftl_sign = 1;
}
void write_file() {
	char file_name[32];
	FILE* fp;
	unsigned int lba;
	unsigned int data;
	clock_t start, end;
	printf("insert file name : ");
	scanf("%s", file_name);
	fp = fopen(file_name, "r");
	if (fp == NULL) printf("there is no such file.\n");
	else {
		start = clock();
		int iter = 0;
		while (iter < return_opnum(file_name)) {
			if (shm->ftl_sign == 0) {
				fscanf(fp, "%u", &lba);
				fscanf(fp, "%u", &data);
				write_data(lba, data);
				iter++;
			}
		}
		fclose(fp);
		end = clock();
		printf("execution time : %f\n", (double)(end - start) / (double)(CLOCKS_PER_SEC));
	}
}
void write_immediate() {
	unsigned int exp = 1;
	srand((unsigned)time(NULL));
	int key2 = 0;
	int op_num = 0;
	int iter = 0;
	printf("select option:\n 1. sequential LBA\n 2. random LBA\n 3. skew\n");
	scanf("%d", &key2);
	printf("enter the number of operations : ");
	scanf("%d", &op_num);
	if (key2 == 1) {
		while (iter < op_num) {
			if (shm->ftl_sign == 0) {
				write_data(iter, random_generator(rand() % 32 + 1));
				iter++;
			}
		}
	}
	if (key2 == 2) {
		while (iter < op_num) {
			if (shm->ftl_sign == 0) {
				write_data(rand() % lba_num, random_generator(rand() % 32 + 1));
				iter++;
			}
		}
	}
	if (key2 == 3) {
		int ran = 0;
		int skewness = 0;
		int hot_num = 0;
		int cold_num = 0;
		unsigned int lba = 0;

		printf("enter the skewness(0~100) : ");
		scanf("%d", &skewness);
		hot_num = (lba_num * (100 - skewness)) / 100;
		cold_num = lba_num - hot_num;
		while (iter < op_num) {
			if (shm->ftl_sign == 0) {
				ran = rand() % 100;
				lba = (ran < skewness) ? (rand() % hot_num) : (rand() % cold_num + hot_num);
				write_data(lba, random_generator(rand() % 32 + 1));
				iter++;
			}
		}
	}
}
void make_input_file() {
	srand((unsigned)time(NULL));
	FILE* fp;
	char file_name[32];
	int key2 = 0;
	int op_num = 0;
	int ran_data = 0;

	printf("selcect option :\n 1. sequential LBA\n 2. random LBA\n 3. skew\n");
	scanf("%d", &key2);
	printf("enter the number of operations : ");
	scanf("%d", &op_num);
	if (key2 == 1) {
		sprintf(file_name, "sequential_%d.txt", op_num);
		fp = fopen(file_name, "w");
		for (int i = 0; i < op_num; i++) {
			fprintf(fp, "%d\n%d\n", i, rand()*rand());
		}
		fclose(fp);
	}
	if (key2 == 2) {
		sprintf(file_name, "random_%d.txt", op_num);
		fp = fopen(file_name, "w");
		for (int i = 0; i < op_num; i++) {
			fprintf(fp, "%d\n%d\n", rand() % 1024, rand()*rand());
		}
		fclose(fp);
	}
	if (key2 == 3) {
		int skewness = 0;
		printf("enter the skewness(0~100) : ");
		scanf("%d", &skewness);
		sprintf(file_name, "skew%d_%d.txt", skewness, op_num);
		int hot_num = (lba_num * (100 - skewness)) / 100;
		int cold_num = lba_num - hot_num;
		int lba = 0;
		fp = fopen(file_name, "w");
		for (int i = 0; i < op_num; i++) {
			lba = ((rand() % 100) < skewness) ? (rand() % hot_num) : (rand() % cold_num + hot_num);
			fprintf(fp, "%d\n%u\n", lba, random_generator(rand() % 32 + 1));
		}
		fclose(fp);
	}
}