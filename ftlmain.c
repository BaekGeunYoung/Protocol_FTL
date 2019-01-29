#pragma warning (disable : 4996)	
#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include "FTL.h"
#include "sys/ipc.h"

info* shm;
int shmid;
int candidate1[cand1_size];
int candidate2[cand2_size];
int hot[hot_size];

int write_count = 0;
int global_lba = 0;
int host_hot_blk = -1;
int host_cold_blk = -1;
int gc_blk = -1;
int written_data = 0;
blk_info bl[blk_num];
l2p L2P[lba_num];
int P2L[blk_num][page_num];

int main() {
	shmid = shmget((key_t)1, 1000, IPC_CREAT | 0666);
	shm = (info*)shmat(shmid, NULL, 0);

	initialize();

	while (1) {
		if (shm->ftl_sign == 1) {
			if (shm->operation == 1) read();
			if (shm->operation == 2) write();
			shm->ftl_sign = 0;
		}
		if (shm->ftl_sign == 2) {
			map_txt_update();
			erase_txt_update();
			break;
		} //program end, map file update
	}
}