#include <leveldb/db.h>
#include <string>
#include <ctype.h>
#include <stdio.h>
#include <vector>
#include <iostream>
#define KEY_LEN 8
#define VALUE_LEN 8
using namespace std;

const string workload = "../../workloads/";

const string load = workload + "1w-rw-50-50-load.txt"; // TODO: the workload_load filename
const string run = workload + "1w-rw-50-50-run.txt";   // TODO: the workload_run filename

const string filePath = "";

const int READ_WRITE_NUM = 999; // TODO: how many operations

int main()
{
	leveldb::DB *db;
	leveldb::Options options;
	leveldb::WriteOptions write_options;
	// TODO: open and initial the levelDB
	// cout<<"PASS open and initial the levelDB"<<endl;
	leveldb::DB **dbptr = nullptr;
	options.create_if_missing = true;
	leveldb::Status status = leveldb::DB::Open(options, "db1", &db);

	uint64_t inserted = 0, queried = 0, t = 0;
	uint64_t *key = new uint64_t[2200000]; // the key and value are same
	bool *ifInsert = new bool[2200000];    // the operation is insertion or not
	FILE *ycsb_load, *ycsb_run;	    // the files that store the ycsb operations
	char *buf = NULL;
	size_t len = 0;
	struct timespec start, finish; // use to caculate the time
	double single_time;	    // single operation time

	printf("Load phase begins \n");
	// TODO: read the ycsb_load and store

	// cout<<"PASS"<<endl;
	FILE *FLOAD;
	FLOAD = fopen(const_cast<char*>(load.c_str()), "r");
	FILE *FRUN;
	FRUN = fopen(const_cast<char*>(run.c_str()), "r");
    if(FLOAD == NULL || FRUN == NULL)   {
        cout << "Open file failed\n";
        return 1;
    }
	// cout<<"PASS open file successfully"<<endl;

	clock_gettime(CLOCK_MONOTONIC, &start);

	// TODO: load the workload in LevelDB
	// cout << "PASS" << endl;
	string ss;
	char temps[100];
	string temp;
	char sss[100];
	int i = 0;
	// cout << "PASS" << endl;
	// fscanf(FLOAD, "%s%s", sss, temps);
	while (i < 998)
	{
		fscanf(FLOAD, "%s%s", sss, temps);
		i++;
		// cout << i << endl;
		// cout << sss << temps;
		ss = sss;
		temp = temps;
		string tt = temp;
		leveldb::Status status = db->Put(leveldb::WriteOptions(), tt.substr(0, 8), temp.substr(0, 8));
		if (status.ok())
			inserted++;
	}
	// cout << "PASS" << endl;
	clock_gettime(CLOCK_MONOTONIC, &finish);
	single_time = (finish.tv_sec - start.tv_sec) * 1000000000.0 + (finish.tv_nsec - start.tv_nsec);

	printf("Load phase finishes: %lu items are inserted \n", inserted);
	printf("Load phase used time: %fs\n", single_time / 1000000000.0);
	printf("Load phase single insert time: %fns\n", single_time / inserted);

	int operation_num = 0;
	inserted = 0;

	// TODO:read the ycsb_run and store

	vector<string> oper;
	vector<string> keying;
	int las = -1;
	// cout << "PASS" << endl;
	for (int i = 0; i < READ_WRITE_NUM - 1; ++i)
	{
		fscanf(FRUN, "%s%s", sss, temps);
		ss = sss;
		temp = temps;
		// cout<<ss<<' '<<temp<<endl;
		oper.push_back(ss);
		keying.push_back(temp);
		if (ss == "UPDATE")
			las = i;
	}

	string val;
	clock_gettime(CLOCK_MONOTONIC, &start);
	operation_num = inserted = 0;
	// TODO: operate the levelDB

	for (int i = 0; i < READ_WRITE_NUM; ++i)
	{

		if (las != -1 && las == i)
			write_options.sync = true;
		// cout << oper[i] << ' ' << keying[i] << endl;
		if (oper[i] == "READ")
		{
			leveldb::Status status = db->Get(leveldb::ReadOptions(), keying[i].substr(0, 8), &val);
			if (status.ok())
				operation_num++;
		}
		if (oper[i] == "UPDATE")
		{
			leveldb::Status status = db->Put(write_options, keying[i].substr(0, 8), keying[i].substr(0, 8));
			if (status.ok())
			{
				inserted++;
				operation_num++;
			}
		}
	}

	clock_gettime(CLOCK_MONOTONIC, &finish);
	single_time = (finish.tv_sec - start.tv_sec) + (finish.tv_nsec - start.tv_nsec) / 1000000000.0;
	printf("Run phase finishes: %lu/%lu items are inserted/searched\n", operation_num - inserted, inserted);
	printf("Run phase throughput: %f operations per second \n", READ_WRITE_NUM / single_time);
	return 0;
}
