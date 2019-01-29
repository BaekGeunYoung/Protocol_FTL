#pragma warning (disable : 4996)	
#ifndef FTL_H
#define FTL_H

#define blk_num 36
#define page_num 128
#define lba_num 4096
#define cand1_size 410
#define cand2_size 820
#define hot_size 1229

#define GC_interval1 500
#define GC_interval2 300
#define GC_interval3 100

#define condition1 ((free_blk_cnt >= 20) && (write_count >= GC_interval1))
#define condition2 ((free_blk_cnt >= 10 && free_blk_cnt < 20) && (write_count >= GC_interval2))
#define condition3 ((free_blk_cnt < 10) && (write_count >= GC_interval3))
#define Extreme_Condition (free_blk_cnt <= 3) && (written_page_cnt >= page_num -2)

#define GC_condition (Extreme_Condition || condition1 || condition2 || condition3)

struct info {
	int operation; //1 : read, 2 : write
	int LBA;
	unsigned int data; //any data with 4byte
	int readable; //read sign from FTL to Protocol
	int ftl_sign; //ftl work start
	float write_data;
	int written_data;
	int gc_count;
}; typedef struct info info;

struct blk_info {
	int erase; // the number erased
	int free; // 0 : not free, 1 : free, 2 : full
	int vpc; // valid page count
	int entry; // the number of entry
	int gc_position; // 0, 1, 2, 3
}; typedef struct blk_info blk_info;

struct l2p {
	int blk;
	int page;
}; typedef struct l2p l2p;

extern info* shm;
extern int shmid;
extern int candidate1[cand1_size];
extern int candidate2[cand2_size];
extern int hot[hot_size];
extern l2p L2P[lba_num];
extern int P2L[blk_num][page_num];

extern int write_count;
extern int global_lba;
extern int host_hot_blk;
extern int host_cold_blk;
extern int gc_blk;
extern int written_data;
extern blk_info bl[blk_num];

void write();
void read();
void update_vpc();
void partial_gc();
void complete_gc();
void blk_erase(int num);
void initialize();
void gc_L2P_update(int gc_blk, int update);
void gc_P2L_update(int gc_blk, int update);
int examine_GCfault(int min_num);
int victim_selection();
int dest_selection(int victim);
void valid_data_copy_partial(int min_num);
void valid_data_copy_complete(int min_num);
int isvalid(int blk, int page);
char* make_filename(int num);
int return_blk(char* str);
int return_page(char* str);
void write_data(int hotness);
void write_map_update(int hotness);
void host_selection(int hotness);
int hotness_LRU(int lba); //1 : hot, 0: cold
void write_LRU(int lba);
int count_written_page(int hotness);
void static_wear_leveling();
void move_data(int src, int dest);
void map_txt_update();
void erase_txt_update();
#endif