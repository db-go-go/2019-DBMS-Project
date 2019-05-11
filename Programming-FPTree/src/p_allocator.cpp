#include"utility/p_allocator.h"
#include<iostream>
#include<libpmem.h>
using namespace std;

// the file that store the information of allocator
const string P_ALLOCATOR_CATALOG_NAME = "p_allocator_catalog";
// a list storing the free leaves
const string P_ALLOCATOR_FREE_LIST    = "free_list";

PAllocator* PAllocator::pAllocator = new PAllocator();

PAllocator* PAllocator::getAllocator() {
    if (PAllocator::pAllocator == NULL) {
        PAllocator::pAllocator = new PAllocator();
    }
    return PAllocator::pAllocator;
}

/* data storing structure of allocator
   In the catalog file, the data structure is listed below
   | maxFileId(8 bytes) | freeNum = m | treeStartLeaf(the PPointer) |
   In freeList file:
   | freeList{(fId, offset)1,...(fId, offset)m} |
*/
PAllocator::PAllocator() {
//    printf("LEAF_GROUP_AMOUNT: %d\n", LEAF_GROUP_AMOUNT);
    string allocatorCatalogPath = DATA_DIR + P_ALLOCATOR_CATALOG_NAME;
    string freeListPath         = DATA_DIR + P_ALLOCATOR_FREE_LIST;
    ifstream allocatorCatalog(allocatorCatalogPath, ios::in|ios::binary);
    ifstream freeListFile(freeListPath, ios::in|ios::binary);
    // judge if the catalog exists
    if (allocatorCatalog.is_open() && freeListFile.is_open()) {
        // exist
        // TODO
        allocatorCatalog.seekg(ios::beg);
        allocatorCatalog.read((char*)&this->maxFileId, sizeof(uint64_t));
        allocatorCatalog.read((char*)&this->freeNum, sizeof(uint64_t));
        allocatorCatalog.read((char*)&this->startLeaf, sizeof(PPointer));
        freeListFile.seekg(ios::beg);
        for(uint64_t i = 0; i < this->freeNum; i ++) {
            PPointer tmp;
            freeListFile.read((char*)&tmp, sizeof(PPointer));
            this->freeList.push_back(tmp);
        }
        allocatorCatalog.close();
        freeListFile.close();
    } else {
        // not exist, create catalog and free_list file, then open.
        // TODO
        ofstream allocatorCatalogOut(allocatorCatalogPath, ios::out|ios::binary);
        ofstream freeListFileOut(freeListPath, ios::out|ios::binary);
        int maxFileId = 1;//all free
        int freeNum = 0;
        PPointer startLeaf;
        startLeaf.fileId = ILLEGAL_FILE_ID;
        startLeaf.offset = 0;
        PPointer freepoint;
        freepoint.fileId = ILLEGAL_FILE_ID;
        freepoint.offset = 0;
        allocatorCatalogOut.write((char*)&maxFileId,sizeof(uint64_t));
        allocatorCatalogOut.write((char*)&freeNum,sizeof(uint64_t));
        allocatorCatalogOut.write((char*)&startLeaf,sizeof(PPointer));
        freeListFileOut.write((char*)&freepoint,sizeof(PPointer));

        this->maxFileId = 1;
        this->freeNum = 0;
        this->startLeaf = startLeaf;
        allocatorCatalogOut.close();
        freeListFileOut.close();
    }
//    printf("PAllocator() :maxFileId %lu freeNum %lu\n", this->maxFileId, this->freeNum);
    this->initFilePmemAddr();
    //printf("%lx\n", this->freeList.size());
//    printf("PAllocator() :%lx\n", this->freeNum);
}

PAllocator::~PAllocator() {
    // TODO
    string allocatorCatalogPath = DATA_DIR + P_ALLOCATOR_CATALOG_NAME;
    string freeListPath = DATA_DIR + P_ALLOCATOR_FREE_LIST;
    
    ofstream allocatorCatalog(allocatorCatalogPath, ios::out|ios::binary);
    ofstream freeListFile(freeListPath, ios::out|ios::binary);
    
    if(!(allocatorCatalog.is_open() && freeListFile.is_open())) {
        printf("~PAllocator() :file open fail\n");
    }
//    printf("~PAllocator() :file open ok\n");

    allocatorCatalog.seekp(ios::beg);
    allocatorCatalog.write((char*)&this->maxFileId, sizeof(uint64_t));
    allocatorCatalog.write((char*)&this->freeNum, sizeof(uint64_t));
    allocatorCatalog.write((char*)&this->startLeaf, sizeof(PPointer));

    freeListFile.seekp(ios::beg);
    for(uint64_t i = 0; i < this->freeNum; i ++) {
        freeListFile.write((char*)&this->freeList[i], sizeof(PPointer));
//        printf("~PAllocator(): %lu offset %lu\n", i+1, this->freeList[i].offset);
    }
    
    allocatorCatalog.close();
    freeListFile.close();
    this->persistCatalog();
    PAllocator::pAllocator = NULL;
}

const uint64_t LEAF_GROUP_SIZE = LEAF_GROUP_HEAD + LEAF_GROUP_AMOUNT*calLeafSize();

// memory map all leaves to pmem address, storing them in the fId2PmAddr
void PAllocator::initFilePmemAddr() {
    // TODO
    for(uint64_t i = 1; i < this->maxFileId; i ++) {
        size_t mapped_lenp;
        int is_pmemp;
        string leafPath = DATA_DIR + to_string(i);
        const char * c_leafPath = leafPath.c_str();
        void* pmem_addr = pmem_map_file(c_leafPath, LEAF_GROUP_SIZE, PMEM_FILE_CREATE,
    0666, &mapped_lenp, &is_pmemp);
        if(pmem_addr == NULL) printf("initFilePmemAddr() :fail\n");
        else {
            char* c_pmem_addr = (char*)pmem_addr;
//            printf("initFilePmemAddr() :%p %s %p\n", pmem_addr, c_leafPath, c_pmem_addr);
            this->fId2PmAddr.insert(pair<uint64_t, char*>(i, c_pmem_addr));
        }
    }
}

// get the pmem address of the target PPointer from the map fId2PmAddr
char* PAllocator::getLeafPmemAddr(PPointer p) {
    // TODO
    map<uint64_t, char*>::iterator it = this->fId2PmAddr.find(p.fileId);
    if(it != this->fId2PmAddr.end()) return it->second+p.offset;
    return NULL;
}

// get and use a leaf for the fptree leaf allocation
// return 
bool PAllocator::getLeaf(PPointer &p, char* &pmem_addr) {
    // TODO
    if(this->freeNum == 0) {
        if(!this->newLeafGroup()) return false;
    }
//    printf("getLeaf() :freeNum %lu\n", this->freeNum);

    p = this->freeList[this->freeList.size()-1];
    this->freeList.pop_back();
    this->freeNum --;
//   printf("getLeaf() :p %lu %lu\n", p.fileId, p.offset);

    //printf("getLeaf() :pmem_addr %s\n", pmem_addr);
    pmem_addr = this->getLeafPmemAddr(p);

    //printf("getLeaf() :pmem_addr %p\n", pmem_addr);
    
/*    string allocatorCatalogPath = DATA_DIR + P_ALLOCATOR_CATALOG_NAME;
    string freeListPath = DATA_DIR + P_ALLOCATOR_FREE_LIST;
    
    ofstream allocatorCatalog(allocatorCatalogPath, ios::app|ios::binary);
    ofstream freeListFile(freeListPath, ios::out|ios::binary);
    
    if(!(allocatorCatalog.is_open() && freeListFile.is_open() && leafgroupFileRead.is_open())) {
        printf("getLeaf() :file open fail\n");
        return false;
    }
    printf("getLeaf() :file open ok\n");

    allocatorCatalog.seekp(sizeof(uint64_t) , ios::beg);
    allocatorCatalog.write((char*)&this->freeNum, sizeof(uint64_t));
    printf("getLeaf() :allocatorCatalog write ok\n");
    freeListFile.write((char*)&this->freeList, sizeof(PPointer)*this->freeNum);
    printf("getLeaf() :freeListFile write ok\n");*/
    
    string leafgroupPath = DATA_DIR + to_string(p.fileId);
    ifstream leafgroupFileRead(leafgroupPath, ios::in|ios::binary);

    uint64_t usedNum;
    leafgroupFileRead.seekg(ios::beg);
    leafgroupFileRead.read((char*)&usedNum, sizeof(uint64_t));
    leafgroupFileRead.close();
    usedNum ++;
    
    ofstream leafgroupFileWrite(leafgroupPath, ios::ate|ios::in|ios::binary);
    leafgroupFileWrite.seekp(ios::beg);
    leafgroupFileWrite.write((char*)&usedNum, sizeof(uint64_t));
    Byte bit = 1;
    uint64_t leaf_index = (p.offset - LEAF_GROUP_HEAD)/calLeafSize();
//    printf("%lu\n", sizeof(uint64_t)+leaf_index);
    leafgroupFileWrite.seekp(sizeof(uint64_t)+leaf_index, ios::beg);
    leafgroupFileWrite.write((char*)&bit, sizeof(Byte));
//    printf("getLeaf() :leafgroupFile write ok\n");

/*    allocatorCatalog.close();
    freeListFile.close();*/
    leafgroupFileWrite.close();
    return true;
}

bool PAllocator::ifLeafUsed(PPointer p) {
    // TODO
    if(!this->ifLeafExist(p)) return false;
    if(!ifLeafFree(p)) return true;
    return false;
}

bool PAllocator::ifLeafFree(PPointer p) {
    // TODO
    if(!this->ifLeafExist(p)) return false;
    for(vector<PPointer>::iterator it = this->freeList.begin(); it != this->freeList.end(); it ++) {
        if(p == *it) return true;
    }
    return false;
}

// judge whether the leaf with specific PPointer exists. 
bool PAllocator::ifLeafExist(PPointer p) {
    // TODO
    PPointer it = this->startLeaf;
    while(it.fileId != ILLEGAL_FILE_ID) {
        if(it == p) return true;
        it = getPNext(it);
    }
    return false;
}

// free and reuse a leaf
bool PAllocator::freeLeaf(PPointer p) {
    // TODO
    if(this->ifLeafUsed(p)) {
        this->freeList.push_back(p);
        this->freeNum ++;
        
        string leafgroupPath = DATA_DIR + to_string(p.fileId);

        uint64_t usedNum;
        ifstream leafgroupFileRead(leafgroupPath, ios::in|ios::binary);
        leafgroupFileRead.seekg(ios::beg);
        leafgroupFileRead.read((char*)&usedNum, sizeof(uint64_t));
        leafgroupFileRead.close();

        ofstream leafgroupFileWrite(leafgroupPath, ios::ate|ios::in|ios::binary);
        usedNum --;
        leafgroupFileWrite.seekp(ios::beg);
        leafgroupFileWrite.write((char*)&usedNum, sizeof(uint64_t));
        uint64_t leaf_index = (p.offset-LEAF_GROUP_HEAD)/calLeafSize();
        leafgroupFileWrite.seekp(sizeof(uint64_t)+leaf_index, ios::beg);
        Byte bit = 0;
        leafgroupFileWrite.write((char*)&bit, sizeof(Byte));
        
        leafgroupFileWrite.close();
        return true;
    }
    return false;
}

bool PAllocator::persistCatalog() {
    // TODO
    string allocatorCatalogPath = DATA_DIR + P_ALLOCATOR_CATALOG_NAME;
    string freeListPath = DATA_DIR + P_ALLOCATOR_FREE_LIST;
    ifstream allocatorCatalog(allocatorCatalogPath, ios::in|ios::binary);
    ifstream freeListFile(freeListPath, ios::in|ios::binary);
    if(allocatorCatalog.is_open() && freeListFile.is_open()) {
        allocatorCatalog.close();
        freeListFile.close();
        int catalog_size = 2*sizeof(uint64_t) +sizeof(PPointer);
        uint64_t freelist_size = this->freeNum*sizeof(PPointer);
        pmem_persist((void*)&allocatorCatalogPath, catalog_size);
//        pmem_persist((void*)&freeListPath, freelist_size);
        return true;
    }
    return false;
}

/*
  Leaf group structure: (uncompressed)
  | usedNum(8b) | bitmap(n * byte) | leaf1 |...| leafn |
*/
// create a new leafgroup, one file per leafgroup
bool PAllocator::newLeafGroup() {
    // TODO
   string leafgroupPath = DATA_DIR + to_string(this->maxFileId);
    ofstream leafgroupFile(leafgroupPath, ios::out|ios::binary);
    if(leafgroupFile.is_open()) {
        // leafgroup file initialize
        char zero[LEAF_GROUP_SIZE];
        for(uint64_t i = 0; i < LEAF_GROUP_SIZE; i ++) {
            zero[i] = 0;
        }

        leafgroupFile.write((char*)zero, LEAF_GROUP_SIZE);
 
        this->maxFileId ++;
        this->freeNum += LEAF_GROUP_AMOUNT;

        uint64_t offset = LEAF_GROUP_HEAD;
        if(this->maxFileId-1 == 1) {
            this->startLeaf.fileId = 1;
            this->startLeaf.offset = offset;
        }
        // pNext
        PPointer p;
        p.fileId = this->maxFileId-1;
        p.offset = offset;

        PPointer start = p;

        for(uint64_t i = 1; i < LEAF_GROUP_AMOUNT; i ++) {
            offset = LEAF_GROUP_HEAD + i * calLeafSize();
            PPointer t_p;
            t_p.fileId = this->maxFileId-1;
            t_p.offset = offset;
            int len = (LEAF_DEGREE * 2 + 7) / 8 + p.offset;
            leafgroupFile.seekp(len, ios::beg);
            leafgroupFile.write((char*)&(t_p), sizeof(PPointer));
            this->freeList.push_back(p);
            //printf("newLeafGroup() : %lu offset %lu\n", i, p.offset);
            p = t_p;
        }
        this->freeList.push_back(p);
        //printf("newLeafGroup() : %lu offset %lu\n", LEAF_GROUP_AMOUNT, p.offset);

        // PPointer last_leaf
        // last_leaf.fileId = ILLEGAL_FILE_ID; last_leaf.offset=0;

        // if not 1st leafgroup
        if(this->maxFileId-1 > 1) {
            string lastLeafgroupPath = DATA_DIR + to_string(start.fileId-1);
            ofstream lastLeafgroupFile(lastLeafgroupPath, ios::ate|ios::in|ios::binary);
            int len = (LEAF_DEGREE * 2 + 7) / 8 + (LEAF_GROUP_AMOUNT-1)*calLeafSize();
            lastLeafgroupFile.seekp(len, ios::beg);
            lastLeafgroupFile.write((char*)&(start), sizeof(PPointer));
        }

        leafgroupFile.close();
        this->initFilePmemAddr();
        //printf("newLeafGroup() :%lx\n", this->freeNum);
        return true;
    }
    return false;
}
