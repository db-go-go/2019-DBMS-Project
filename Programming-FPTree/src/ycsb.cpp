#include "fptree/fptree.h"
#include <leveldb/db.h>
#include "leveldb/write_batch.h"
#include <string>

#define KEY_LEN 8
#define VALUE_LEN 8
using namespace std;

const string workload = "../../workloads/"; // TODO: the workload folder filepath

const string load = workload + "1w-rw-50-50-load.txt"; // TODO: the workload_load filename
const string run  = workload + "1w-rw-50-50-run.txt"; // TODO: the workload_run filename

const int READ_WRITE_NUM = 10000; // TODO: amount of operations

const string catalogPath = DATA_DIR + "p_allocator_catalog";
const string freePath = DATA_DIR + "free_list";
const string file1 = DATA_DIR + "1";
const string file2 = DATA_DIR + "2";
const string file3 = DATA_DIR + "3";
const string file4 = DATA_DIR + "4";

void removeFile() {
    PAllocator::getAllocator()->~PAllocator();
    remove(catalogPath.c_str());
    remove(file1.c_str());
    remove(freePath.c_str());
}

int main()
{        
    FPTree fptree(1028);
    uint64_t inserted = 0, queried = 0, t = 0;
    uint64_t* key = new uint64_t[2200000];
    bool* ifInsert = new bool[2200000];
	FILE *ycsb, *ycsb_read;
	char *buf = NULL;
	size_t len = 0;
    struct timespec start, finish;
    double single_time;

    printf("===================FPtreeDB===================\n");
    printf("Load phase begins \n");

    // TODO read the ycsb_load
	ycsb_read = fopen(const_cast<char*>(load.c_str()), "r");
    if(ycsb_read == NULL)   {
        cout << "Open file load failed\n";
        return 1;
    }
    char ope[20] ={0};
    for (int i = 0; i < READ_WRITE_NUM; i ++) {
        fscanf(ycsb_read, "%s%ld", ope, &key[i]);
        memcpy(&key[i], &key[i], KEY_LEN);
        if (strcmp(ope, "INSERT") == 0) {
            ifInsert[i] = true;
            inserted ++;
        }
        else {
            ifInsert[i] = false;
        }
    }
    // cout << "load successfully\n";
    fclose(ycsb_read);
    // cout << "close file successfully\n";

    clock_gettime(CLOCK_MONOTONIC, &start);
    // TODO load the workload in the fptree
    // cout << "begin to load\n";
    for (int i = 0; i < READ_WRITE_NUM; i ++) {
        if (ifInsert(i))
            fptree.insert(key[i], key[i]);
    }
    // cout << "fptree load successfully\n";
    clock_gettime(CLOCK_MONOTONIC, &finish);

    // fptree.printTree();
	single_time = (finish.tv_sec - start.tv_sec) * 1000000000.0 + (finish.tv_nsec - start.tv_nsec);
    printf("Load phase finishes: %d items are inserted \n", inserted);
    printf("Load phase used time: %fs\n", single_time / 1000000000.0);
    if (inserted != 0) printf("Load phase single insert time: %fns\n", single_time / inserted);

	printf("Run phase begins\n");

	int operation_num = 0;
    inserted = 0;
    // TODO read the ycsb_run
    ycsb = fopen(const_cast<char *>(run.c_str()), "r");
    if (ycsb == NULL) {
        cout << "open file run failed\n";
        return 1;
    }
    memset(key, 0, 2200000);
    memset(ifInsert, 0, 2200000);
    for (int i = 0; i < READ_WRITE_NUM; i ++) {
        fscanf(ycsb_read, "%s%ld", ope, &key[i]);
        memcpy(&key[i], &key[i], KEY_LEN);
        if (strcmp(ope, "UPDATE") == 0) {
            ifInsert[i] = true;
            inserted ++;
        }
        else if (strcmp(ope, "READ") == 0) {
            ifInsert[i] = false;
            //queried ++;
        }
        operation_num ++;
    }
    fclose(ycsb);

    clock_gettime(CLOCK_MONOTONIC, &start);
    // TODO operate the fptree
    for (int i = 0; i < READ_WRITE_NUM; i ++) {
        if (ifInsert[i]) {
            fptree.update(key[i], key[i]);
        }
        else {
            fptree.find(key[i]);
            //queried ++;
        }
    }
	clock_gettime(CLOCK_MONOTONIC, &finish);

	single_time = (finish.tv_sec - start.tv_sec) + (finish.tv_nsec - start.tv_nsec) / 1000000000.0;
    printf("Run phase finishes: %d/%d items are inserted/searched\n", inserted, operation_num - inserted);
    if (single_time != 0) printf("Run phase throughput: %f operations per second \n", READ_WRITE_NUM/single_time);	
    
    // LevelDB
    printf("===================LevelDB====================\n");
    const string filePath = "/mnt/pmemdir/"; // data storing folder(NVM)

    memset(key, 0, 2200000);
    memset(ifInsert, 0, 2200000);
    uint64_t number = 0;

    leveldb::DB* db;
    leveldb::Options options;
    leveldb::WriteOptions write_options;

    // TODO initial the levelDB
    options.create_if_missing = true;
    leveldb::Status status = leveldb::DB::Open(options, "leveldb", &db);

    inserted = 0;
    printf("Load phase begins \n");

    // TODO read the ycsb_read
    ycsb_read = fopen(const_cast<char*>(load.c_str()), "r");
    if(ycsb_read == NULL)   {
        cout << "Open file load failed\n";
        return 1;
    }
    for (int i = 0; i < READ_WRITE_NUM; i ++) {
        fscanf(ycsb_read, "%s%ld", ope, &key[i]);
        memcpy(&key[i], &key[i], KEY_LEN);
        inserted ++;
    }
    fclose(ycsb_read);

    clock_gettime(CLOCK_MONOTONIC, &start);
    // TODO load the levelDB = Write data to levelDB
    for (int i = 0; i < READ_WRITE_NUM; i ++) {
        sprintf(buf, "%ld", key[i]);
        status = db->Put(leveldb::WriteOptions(), buf, buf);
    }
    clock_gettime(CLOCK_MONOTONIC, &finish);

	single_time = (finish.tv_sec - start.tv_sec) * 1000000000.0 + (finish.tv_nsec - start.tv_nsec);
    printf("Load phase finishes: %d items are inserted \n", inserted);
    printf("Load phase used time: %fs\n", single_time / 1000000000.0);
    if (inserted != 0) printf("Load phase single insert time: %fns\n", single_time / inserted);

	printf("Run phase begin\n");
	operation_num = 0;
    inserted = 0;		

    // TODO read the ycsb_run
    memset(key, 0, 2200000);
    memset(ifInsert, 0, 2200000);
    ycsb = fopen(const_cast<char *>(run.c_str()), "r");
    if (ycsb == NULL) {
        cout << "open file run failed\n";
        return 1;
    }
    for (int i = 0; i < READ_WRITE_NUM; i ++) {
        fscanf(ycsb, "%s%ld", ope, &key[i]);
        memcpy(&key[i], &key[i], KEY_LEN);
        if (strcmp(ope, "UPDATE") == 0) {
            ifInsert[i] = true;
            inserted ++;
        }
        operation_num ++;
    }
    fclose(ycsb);

    clock_gettime(CLOCK_MONOTONIC, &start);
    // TODO run the workload_run in levelDB
    string t_value = "";
    for (int i = 0; i < READ_WRITE_NUM; i ++) {
        sprintf(buf, "%ld", key[i]);
        if (ifInsert[i]) {
            status = db->Get(leveldb::ReadOptions(), buf, &t_value);
            if (status.ok()) {
                leveldb::WriteBatch batch;
                batch.Delete(buf);
                batch.Put(buf, buf);
                status = db->Write(leveldb::WriteOptions(), &batch);
            }
        }
        else {
            status = db->Get(leveldb::ReadOptions(), buf, &t_value);
            //queried ++;
        }
        operation_num ++;
    }
	clock_gettime(CLOCK_MONOTONIC, &finish);

	single_time = (finish.tv_sec - start.tv_sec) + (finish.tv_nsec - start.tv_nsec) / 1000000000.0;
    printf("Run phase finishes: %d/%d items are inserted/searched\n", inserted, operation_num - inserted);
    if (single_time != 0) printf("Run phase throughput: %f operations per second \n", READ_WRITE_NUM/single_time);	
    
    delete [] key;
    delete [] ifInsert;
    removeFile();
    return 0;
}
