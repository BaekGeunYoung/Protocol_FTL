#pragma warning (disable : 4996)
#include "sys/ipc.h"
#include "protocol.h"
#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

int shmid;
info* shm;

int main() {
	shmid = shmget((key_t)1, 1000, IPC_CREAT | 0666);
	shm = (info*)shmat(shmid, NULL, 0);
	shm->ftl_sign = 0;
	int key;
	shm->write_data = 0;
	shm->written_data = 0;
	shm->gc_count = 0;
	while (1) {
		printf("select option :\n 1. read\n 2. write(using file)\n 3. write(immediate input)\n 4. make input file\n 5. end\n");
		scanf("%d", &key);
		if (key == 1) read();
		if (key == 2) write_file();
		if (key == 3) write_immediate();
		if (key == 4) make_input_file();
		if (key == 5) {
			printf("WAI : %f\n", (float)(shm->written_data) / (float)shm->write_data);
			printf("GC count : %d\n", shm->gc_count);
			printf("program ends.\n");
			shm->ftl_sign = 2;
			break;
		}
	}
}