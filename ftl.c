#pragma warning (disable : 4996)	
#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include "FTL.h"
#include "sys/ipc.h"

char* make_filename(int num) {
	char st[12];
	char name[32] = "block";
	sprintf(st, "%d", num);
	strcat(name, st);
	strcat(name, ".txt");
	return name;
}

int return_page(char* str) {
	int len = strlen(str);
	int count = 0;
	int i;
	int dec = 1;
	int res = 0;

	for (i = len - 1; i >= 0; i--) {
		if (str[i] != ' ') count++;
		else break;
	}
	char new_str[50];
	memcpy(new_str, &str[++i], --count);
	new_str[count] = '\0';

	for (i = count-1; i >= 0; i--) {
		if (new_str[i] == '-') res = -res;
		else {
			res += ((int)new_str[i] - 48) * dec;
			dec *= 10;
		}
	}
	return res;
}
int return_blk(char* str) {
	int space = 0;
	int len = strlen(str);
	int count = 0;
	int i;
	int res = 0;
	int dec = 1;

	for (i = len - 1; i >= 0; i--) {
		if (space == 2) count++;
		if (str[i] == ' ') space++;
		if (space == 3) break;
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

void initialize() {
	char file_name[32], str[50];
	char chr;
	int enter = 0, pnum = 0, bnum = 0, lba = 0;
	FILE* map;
	FILE* fp;
	map = fopen("L2Pmap.txt", "r");
	if (map == NULL) {
		map = fopen("L2Pmap.txt", "w");
		for (int i = 0; i < lba_num; i++) fprintf(map, "%d : -1\n", i);
	}
	fclose(map); //initialize L2Pmap

	map = fopen("P2Lmap.txt", "r");
	if (map == NULL) {
		map = fopen("P2Lmap.txt", "w");
		for (int i = 0; i < blk_num; i++) for (int j = 0; j < page_num; j++)fprintf(map, "-1 : %d - %d\n", i, j);
	}
	fclose(map); //initialize P2Lmap

	for (int i = 0; i < blk_num; i++) {
		strcpy(file_name, make_filename(i));
		fp = fopen(file_name, "r");
		if (fp == NULL) {
			fp = fopen(file_name, "w");
		}
		fclose(fp);
	} //initialize block

	fp = fopen("erase.txt", "r");
	if (fp == NULL) {
		fp = fopen("erase.txt", "w");
		for (int i = 0; i < blk_num; i++) fprintf(fp, "0\n");
	}
	fclose(fp); //initialize erase

	map = fopen("erase.txt", "r");
	for (int i = 0; i < blk_num; i++) {
		bl[i].gc_position = 0;
		enter = 0;
		strcpy(file_name, make_filename(i));
		fp = fopen(file_name, "r");
		if ((chr = fgetc(fp)) == EOF) {
			bl[i].entry = 0;
			bl[i].free = 1;
		}
		else {
			while ((chr = fgetc(fp)) != EOF) if (chr == '\n') enter++;
			bl[i].entry = enter;
			if (enter == page_num) bl[i].free = 2;
			else bl[i].free = 0;
		}
		fclose(fp);
		fscanf(map, "%d", &bl[i].erase);
	}
	fclose(map);//initializing block information

	for (int i = 0; i < hot_size; i++) hot[i] = -1;
	for (int i = 0; i < cand1_size; i++) candidate1[i] = -1;
	for (int i = 0; i < cand2_size; i++) candidate2[i] = -1; // initializing hot and candidate list

	map = fopen("L2Pmap.txt", "r");
	for (int i = 0; i < lba_num; i++) {
		fgets(str, 50, map);
		pnum = return_page(str);
		if (pnum == -1) {
			L2P[i].blk = -1;
			L2P[i].page = -1;
		}
		else {
			bnum = return_blk(str);
			L2P[i].blk = bnum;
			L2P[i].blk = pnum;
		}
	}
	fclose(map); // initializing L2P array

	map = fopen("P2Lmap.txt", "r");
	for (int i = 0; i < blk_num; i++) {
		for (int j = 0; j < page_num; j++) {
			fscanf(map, "%d", &lba);
			fgets(str, 50, map);
			P2L[i][j] = lba;
		}
	}
	fclose(map); // initializing P2L array
}

void read() {
// read operation
	int blk = 0, page = 0;
	char map_content[50];
	unsigned int mem_content = 0;
	char file_name[32];
	FILE* fp;

	blk = L2P[shm->LBA].blk;
	page = L2P[shm->LBA].page;
	
	//update shared memory data and readable sign
	if (return_page(map_content) == -1) shm->readable = 2;
	else {
		blk = return_blk(map_content);
		page = return_page(map_content);
		strcpy(file_name, make_filename(blk));
		fp = fopen(file_name, "r");
		for (int i = 0; i <= page; i++) fscanf(fp, "%d", &mem_content);
		shm->data = mem_content;
		shm->readable = 1;
	}
}
void write() {
	write_count++;
	shm->written_data += 4;
	int free_blk_cnt = 0;
	int written_page_cnt = 0;
	int hotness = 0;
	
	hotness = hotness_LRU(shm->LBA);
	write_LRU(shm->LBA);

	static_wear_leveling();
	host_selection(hotness);
	write_data(hotness);
	for (int i = 0; i < blk_num; i++) if (bl[i].free == 1) free_blk_cnt++;
	written_page_cnt = count_written_page(hotness);
	write_map_update(hotness);
	if (Extreme_Condition) complete_gc();
	else if (condition1 || condition2 || condition3) {
		write_count = 0;
		partial_gc();
	}
}

void partial_gc() {
	shm->gc_count++;
	int min_num = 0; //victim block
	printf("garbage collection start.\n");
	update_vpc();
	min_num = victim_selection();
	if (min_num < blk_num) {
		gc_blk = dest_selection(min_num);
		if (examine_GCfault(min_num) == 0) {
			printf("there is no available block. GC FAULT.\nWAF : %f\n", (float)(shm->written_data) / (float)(shm->write_data));
			getchar();
		}
		valid_data_copy_partial(min_num);
		if (bl[min_num].gc_position == 0) {
			printf("block %d is erased.\n", min_num);
			blk_erase(min_num);
		}
	}
	printf("garbage collection finished.\n\n");
}
void complete_gc() {
	shm->gc_count++;
	int min_num = 0; //victim block
	printf("garbage collection start.\n");
	update_vpc();
	min_num = victim_selection();
	if (min_num < blk_num) {
		gc_blk = dest_selection(min_num);
		if (examine_GCfault(min_num) == 0) {
			printf("there is no available block. GC FAULT.\nWAF : %f\n", (float)(shm->written_data) / (float)(shm->write_data));
			getchar();
		}
		valid_data_copy_complete(min_num);
		printf("block %d is erased.\n", min_num);
		blk_erase(min_num);
	}
	printf("garbage collection finished.\n\n");
}

void valid_data_copy_partial(int min_num) {
	char file_name[32], data[50];
	int valid = 0;

	strcpy(file_name, make_filename(min_num));
	FILE* fp1 = fopen(file_name, "r");
	strcpy(file_name, make_filename(gc_blk));
	FILE* fp2 = fopen(file_name, "a+");

	for (int i = 0; i < bl[min_num].gc_position * 32; i++) fgets(data, 50, fp1);
	for (int i = bl[min_num].gc_position * 32; i < (bl[min_num].gc_position + 1) * 32; i++) {
		fgets(data, 50, fp1);
		if (isvalid(min_num, i)) {
			if (bl[gc_blk].entry == page_num) {
				printf("block %d is full. new ", gc_blk);
				bl[gc_blk].free = 2;
				gc_blk = dest_selection(min_num);
				fclose(fp2);
				strcpy(file_name, make_filename(gc_blk));
				fp2 = fopen(file_name, "w");
				bl[gc_blk].entry = 0;
			}
			else bl[gc_blk].free = 0;

			fputs(data, fp2);
			gc_L2P_update(gc_blk, bl[gc_blk].entry);
			gc_P2L_update(gc_blk, bl[gc_blk].entry);
			bl[gc_blk].entry++;
			valid++;
		}
	}
	int tmp = (bl[min_num].gc_position == 3) ? 0 : bl[min_num].gc_position + 1;
	bl[min_num].gc_position = tmp;

	printf("gained %d byte (%d page) memory space.\n", (32 - valid) * 4, 32 - valid);
	shm->written_data += 4 * valid;
	fclose(fp1);
	fclose(fp2);
}
void valid_data_copy_complete(int min_num) {
	char file_name[32], data[50];
	int valid = 0;

	strcpy(file_name, make_filename(min_num));
	FILE* fp1 = fopen(file_name, "r");
	strcpy(file_name, make_filename(gc_blk));
	FILE* fp2 = fopen(file_name, "a+");

	for (int i = 0; i < bl[min_num].gc_position * 32; i++) fgets(data, 50, fp1);
	for (int i = bl[min_num].gc_position * 32; i < page_num; i++) {
		fgets(data, 50, fp1);
		if (isvalid(min_num, i)) {
			if (bl[gc_blk].entry == page_num) {
				printf("block %d is full. new ", gc_blk);
				bl[gc_blk].free = 2;
				gc_blk = dest_selection(min_num);
				fclose(fp2);
				strcpy(file_name, make_filename(gc_blk));
				fp2 = fopen(file_name, "w");
				bl[gc_blk].entry = 0;
			}
			else bl[gc_blk].free = 0;

			fputs(data, fp2);
			gc_L2P_update(gc_blk, bl[gc_blk].entry);
			gc_P2L_update(gc_blk, bl[gc_blk].entry);
			bl[gc_blk].entry++;
			valid++;
		}
	}
	bl[min_num].gc_position = 0;
	printf("gained %d byte (%d page) memory space.\n", (page_num - valid) * 4, page_num - valid);
	shm->written_data += 4 * valid;
	fclose(fp1);
	fclose(fp2);
}

int victim_selection() {
	int min_num = 0;
	for (; min_num < blk_num; min_num++) {
		if ((bl[min_num].vpc != 0) && (min_num != host_cold_blk) && (min_num != host_hot_blk) && (min_num != gc_blk)) break;
	}
	for (int i = 0; i < blk_num; i++) {
		if ((bl[i].free != 1) && (i != host_cold_blk) && (i != host_hot_blk) && (i != gc_blk)) {
			if (bl[i].vpc == 0) {
				printf("all data in block %d is invalid. block %d is erased.\ngained 512 byte (128 page) memory space.\n", i, i);
				blk_erase(i);
			}
			else {
				if (bl[min_num].vpc > bl[i].vpc) min_num = i;
			}
		}
	}
	if(min_num < blk_num)	 printf("victim block selection : block %d \n", min_num);
	return min_num;
}
int dest_selection(int victim) {
	if ((gc_blk == -1) || bl[gc_blk].free == 2) {
		int i = 0;
		for (i = 0; i < blk_num; i++) {
			if ((i != host_hot_blk) && (i != host_cold_blk) && (bl[i].free == 1) && (i != victim)) {
				gc_blk = i;
				break;
			}
		}
		if (i == blk_num) return -2;

		for (i = 0; i < blk_num; i++) {
			if ((i != host_hot_blk) && (i != host_cold_blk) && (bl[i].free == 1) && (i != victim)) {
				if (bl[gc_blk].erase > bl[i].erase) gc_blk = i;
			}
		}
	}
	printf("destination block selection : block %d \n", gc_blk);
	return gc_blk;
}

void update_vpc() {
	int vpc = 0;

	for (int i = 0; i < blk_num; i++) {
		vpc = 0;
		for (int j = 0; j < page_num; j++) {
			if (isvalid(i, j)) vpc++;
		}
		bl[i].vpc = vpc;
	}
	//for(int i = 0 ; i < blk_num ; i++) printf("%d, ", bl[i].vpc);
	//printf("\n");
}
int isvalid(int blk, int page) {
	global_lba = P2L[blk][page];
	if (global_lba == -1) return 0;
	else if ((L2P[global_lba].blk == blk) && (L2P[global_lba].page == page)) return 1;
	else return 0;
}

void blk_erase(int num) {
	char file_name[32];
	strcpy(file_name, make_filename(num));
	FILE *fp = fopen(file_name, "w");
	fclose(fp);
	bl[num].erase++;
	bl[num].free = 1;
	bl[num].entry = 0;
	for (int i = 0; i < page_num; i++) P2L[num][i] = -1;
}

void gc_L2P_update(int gc_blk, int update) {
	L2P[global_lba].blk = gc_blk;
	L2P[global_lba].page = update;
}
void gc_P2L_update(int gc_blk, int update) {
	P2L[gc_blk][update] = global_lba;
}

int examine_GCfault(int min_num) {
	if (gc_blk == -2) return 0;
	int r = 0;
	FILE* fp;
	char file_name[32], chr;
	int enter = 0;
	for (int i = 0; i < blk_num; i++) 
		if (bl[i].free == 1) {
			r = 1;
			break;
		}
	if (r == 0) {
		strcpy(file_name, make_filename(gc_blk));
		fp = fopen(file_name, "r");
		while ((chr = fgetc(fp)) != EOF) if (chr == '\n') enter++;
		fclose(fp);
		if (page_num - enter < bl[min_num].vpc) return 0;
		else return 1;
	}
	else return 1;
}

void write_data(int hotness) {
	char file_name[32];
	FILE* fp;

	int host_blk = (hotness == 1) ? host_hot_blk : host_cold_blk;
	bl[host_blk].entry++;
	if (bl[host_blk].entry >= page_num) bl[host_blk].free = 2;
	if (bl[host_blk].free == 1) bl[host_blk].free = 0;

	strcpy(file_name, make_filename(host_blk));
	fp = fopen(file_name, "a");
	fprintf(fp, "%u\n", shm->data);
	fclose(fp);
}
void host_selection(int hotness) {
	int* host_blk = (hotness == 1) ? &host_hot_blk : &host_cold_blk;
	int other_blk = (hotness == 1) ? host_cold_blk : host_hot_blk;

	if ((*host_blk == -1) || (bl[*host_blk].free == 2)) {
		for (*host_blk = 0; *host_blk < blk_num; (*host_blk)++) {
			if ((bl[*host_blk].free != 2) && (*host_blk != gc_blk) && (*host_blk != other_blk)) break;
		}
		for (int i = *host_blk; i < blk_num; i++) {
			if ((bl[i].free != 2) && (i != gc_blk) && (i != other_blk)) {
				if (bl[i].erase < bl[*host_blk].erase) *host_blk = i;
			}
		}
	}

	if (*host_blk >= blk_num) {
		printf("There is no available block. WRITE FAULT.");
		getchar();
	}
}
void write_map_update(int hotness) {
	int host_blk = (hotness == 1) ? host_hot_blk : host_cold_blk;
	
	L2P[shm->LBA].blk = host_blk;
	L2P[shm->LBA].page = bl[host_blk].entry - 1;

	P2L[host_blk][bl[host_blk].entry-1] = shm->LBA;
}

int hotness_LRU(int lba) {
	for (int i = 0; i < hot_size; i++) {
		if (hot[i] == lba) return 1;
	}
	return 0;
}
void write_LRU(int lba) {
	int index1 = -1;
	int index2 = -1;
	int index3 = -1;

	for (int i = 0; i < hot_size; i++) {
		if (hot[i] == lba) index1 = i;
	}
	if (index1 != -1) {
		for (int i = index1 -1; i >= 0; i--) hot[i+1] = hot[i];
		hot[0] = lba;
	} // lba is in hot list
	else {
		for (int i = 0; i < cand1_size; i++) {
			if (candidate1[i] == lba) index2 = i;
		}

		if (index2 != -1) {
			int arr_len = 0;
			for (int i = 0; i < hot_size; i++) {
				if (hot[i] != -1) arr_len++;
				else break;
			}
			if (arr_len == hot_size) {
				for (int i = index2 - 1; i >= 0; i--) candidate1[i + 1] = candidate1[i];
				candidate1[0] = hot[hot_size - 1];
				for (int i = hot_size - 2; i >= 0; i--) hot[i + 1] = hot[i];
				hot[0] = lba;
			}
			else {
				for (int i = arr_len - 1; i >= 0; i--) hot[i + 1] = hot[i];
				hot[0] = lba;
				for (int i = index2; i < cand1_size-1; i++) candidate1[i] = candidate1[i + 1];
				candidate1[cand1_size - 1] = -1;
			} 
		} // lba is in candidate1 list
		else {
			for (int i = 0; i < cand2_size; i++) if (candidate2[i] == lba) index3 = i;
			if (index3 != -1) {
				int arr_len = 0;
				for (int i = 0; i < cand1_size; i++) {
					if (candidate1[i] != -1) arr_len++;
					else break;
				}
				if (arr_len == cand1_size) {
					for (int i = index3 - 1; i >= 0; i--) candidate2[i + 1] = candidate2[i];
					candidate2[0] = candidate1[cand1_size - 1];
					for (int i = cand1_size - 2; i >= 0; i--) candidate1[i + 1] = candidate1[i];
					candidate1[0] = lba;
				}
				else {
					for (int i = arr_len - 1; i >= 0; i--) candidate1[i + 1] = candidate1[i];
					candidate1[0] = lba;
					for (int i = index3; i < cand2_size - 1; i++) candidate2[i] = candidate2[i + 1];
					candidate2[cand2_size - 1] = -1;
				}
			} // lba is in candidate2 list
			else {
				for (int i = cand2_size - 2; i >= 0; i--) candidate2[i + 1] = candidate2[i];
				candidate2[0] = lba;
			} // lba is not in any list
		}
	}
}

int count_written_page(int hotness) {
	int host_blk = (hotness == 1) ? host_hot_blk : host_cold_blk;
	return bl[host_blk].entry;
}

void static_wear_leveling() {
	int min_erase_cnt = 0;
	int max_erase_cnt = 0;
	int i;
	for (i = 0; i < blk_num; i++) {
		if (bl[i].free == 2) {
			min_erase_cnt = i;
			break;
		}
	}
	for (i = min_erase_cnt; i < blk_num; i++) {
		if (bl[i].free == 2)
			if (bl[i].erase < bl[min_erase_cnt].erase) min_erase_cnt = i;
	}
	for (i = 0; i < blk_num; i++) {
		if (bl[i].free == 1) {
			max_erase_cnt = i;
			break;
		}
	}
	for (i = max_erase_cnt; i < blk_num; i++) {
		if (bl[i].free == 1)
			if (bl[i].erase > bl[max_erase_cnt].erase) max_erase_cnt = i;
	}
	if ((bl[min_erase_cnt].erase+1) * 5 < bl[max_erase_cnt].erase) {
		move_data(min_erase_cnt, max_erase_cnt);
		printf("static wear leveling : the data in block %d moved to block %d\n\n", min_erase_cnt, max_erase_cnt);
	}
}
void move_data(int src, int dest) {
	int tmp = gc_blk;
	gc_blk = dest;
	update_vpc();
	valid_data_copy_complete(src);
	blk_erase(src);
	gc_blk = tmp;
}

void map_txt_update() {
	FILE* fp = fopen("L2Pmap.txt","w");
	for (int i = 0; i < lba_num; i++) {
		if (L2P[i].page == -1) fprintf(fp, "%d : -1\n", i);
		else fprintf(fp, "%d : %d - %d\n", i, L2P[i].blk, L2P[i].page);
	}
	fclose(fp);

	fopen("P2Lmap.txt", "w");
	for (int i = 0; i < blk_num; i++) {
		for (int j = 0; j < page_num; j++) {
			fprintf(fp, "%d : %d - %d\n", P2L[i][j], i, j);
		}
	}
	fclose(fp);
}
void erase_txt_update() {
	FILE* fp = fopen("erase.txt", "w");
	for (int i = 0; i < blk_num; i++) fprintf(fp, "%d\n", bl[i].erase);
	fclose(fp);
}