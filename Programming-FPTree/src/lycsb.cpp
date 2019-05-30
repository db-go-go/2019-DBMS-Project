#include <leveldb/db.h>
#include <leveldb/write_batch.h>
#include <ctype.h>
#include <stdio.h>
#include <string>
#include <iostream>
#define KEY_LEN 8
#define VALUE_LEN 8
using namespace std;

const string workload = "../../workloads/";

const string load = workload + "1w-rw-50-50-load.txt"; // TODO: the workload_load filename
const string run = workload + "1w-rw-50-50-run.txt";   // TODO: the workload_run filename

const string filePath = "/mnt/pmemdir/";

const int READ_WRITE_NUM = 100; // TODO: how many operations

int main()
{
	leveldb::DB *db;
	leveldb::Options options;
	leveldb::WriteOptions write_options;
	// TODO open and initial the levelDB
	leveldb::DB **dbptr = nullptr;
	options.create_if_missing = true;
	leveldb::Status status = leveldb::DB::Open(options, filePath, &db);

	uint64_t inserted = 0, queried = 0, t = 0;
	uint64_t *key = new uint64_t[2200000]; // the key and value are same
	bool *ifInsert = new bool[2200000];    // the operation is insertion or not
	FILE *ycsb_load, *ycsb_run;	    // the files that store the ycsb operations
	char *buf = NULL;
	size_t len = 0;
	struct timespec start, finish; // use to caculate the time
	double single_time;	    // single operation time

	printf("Load phase begins \n");
	// TODO read the ycsb_load and store
	ycsb_load = fopen(const_cast<char*>(load.c_str()), "r");
	if(ycsb_load == NULL)   {
        cout << "Open file load failed\n";
        return 1;
    }
	char ope[20] ={0};
    for (int i = 0; i < READ_WRITE_NUM; i ++) {
        fscanf(ycsb_load, "%s%ld", ope, &key[i]);
        memcpy(&key[i], &key[i], KEY_LEN);
        inserted ++;
    }
    fclose(ycsb_load);

	clock_gettime(CLOCK_MONOTONIC, &start);
	// TODO load the workload in LevelDB
	buf = new char[100];
	for (int i = 0; i < READ_WRITE_NUM; i ++) {
        sprintf(buf, "%ld", key[i]);
        status = db->Put(leveldb::WriteOptions(), buf, buf);
    }
	clock_gettime(CLOCK_MONOTONIC, &finish);

	single_time = (finish.tv_sec - start.tv_sec) * 1000000000.0 + (finish.tv_nsec - start.tv_nsec);
	printf("Load phase finishes: %lu items are inserted \n", inserted);
	printf("Load phase used time: %fs\n", single_time / 1000000000.0);
	if (inserted != 0) printf("Load phase single insert time: %fns\n", single_time / inserted);

	int operation_num = 0;
	inserted = 0;

	// TODO read the ycsb_run and store
	memset(key, 0, 2200000);
    memset(ifInsert, 0, 2200000);
	ycsb_run = fopen(const_cast<char*>(run.c_str()), "r");
    if(ycsb_run == NULL)   {
        cout << "Open file failed\n";
        return 1;
    }
	for (int i = 0; i < READ_WRITE_NUM; i ++) {
        fscanf(ycsb_run, "%s%ld", ope, &key[i]);
        memcpy(&key[i], &key[i], KEY_LEN);
        if (strcmp(ope, "UPDATE") == 0) {
            ifInsert[i] = true;
            inserted ++;
        }
        operation_num ++;
    }
    fclose(ycsb_run);

	clock_gettime(CLOCK_MONOTONIC, &start);
	operation_num = inserted = 0;
	// TODO operate the levelDB
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
	fclose(ycsb_load);
	fclose(ycsb_run);
	printf("Run phase finishes: %lu/%lu items are inserted/searched\n", operation_num - inserted, inserted);
	if (single_time != 0) printf("Run phase throughput: %f operations per second \n", READ_WRITE_NUM / single_time);
	
	delete [] key;
	delete [] ifInsert;
	return 0;
}
