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
    string allocatorCatalogPath = DATA_DIR + P_ALLOCATOR_CATALOG_NAME;
    string freeListPath         = DATA_DIR + P_ALLOCATOR_FREE_LIST;
    ifstream allocatorCatalog(allocatorCatalogPath, ios::in|ios::binary);
    ifstream freeListFile(freeListPath, ios::in|ios::binary);
    // judge if the catalog exists
    if (allocatorCatalog.is_open() && freeListFile.is_open()) {
        // exist
        // TODO
        allocatorCatalog.read((char*)&this->maxFileId, sizeof(uint64_t));
        allocatorCatalog.read((char*)&this->freeNum, sizeof(uint64_t));
        allocatorCatalog.read((char*)&this->startLeaf, sizeof(PPointer));
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
    this->initFilePmemAddr();
}

PAllocator::~PAllocator() {
    // TODO
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
        this->fId2PmAddr.insert(pair<uint64_t, char*>(i, (char*)pmem_addr));
    }
}

// get the pmem address of the target PPointer from the map fId2PmAddr
char* PAllocator::getLeafPmemAddr(PPointer p) {
    // TODO
    map<uint64_t, char*>::iterator it = this->fId2PmAddr.find(p.fileId);
    if(it != this->fId2PmAddr.end()) return it->second;
    return NULL;
}

// get and use a leaf for the fptree leaf allocation
// return 
bool PAllocator::getLeaf(PPointer &p, char* &pmem_addr) {
    // TODO
    if(this->freeNum != 0) {
        p = this->freeList[this->freeNum-1];
        this->freeList.pop_back();
        this->freeNum --;
        pmem_addr = this->fId2PmAddr.find(p.fileId)->second;
        
        string allocatorCatalogPath = DATA_DIR + P_ALLOCATOR_CATALOG_NAME;
        string freeListPath = DATA_DIR + P_ALLOCATOR_FREE_LIST;
        string leafgroupPath = DATA_DIR + to_string(p.fileId);
        ofstream allocatorCatalog(allocatorCatalogPath, ios::app|ios::binary);
        ofstream freeListFile(freeListPath, ios::out|ios::binary);
        fstream leafgroupFile(leafgroupPath, ios::in|ios::app|ios::binary);
        
        allocatorCatalog.seekp(8, ios::beg);
        allocatorCatalog.write((char*)&this->freeNum, sizeof(Byte)*8);
        
        freeListFile.write((char*)&this->freeList, sizeof(PPointer)*this->freeNum);
        
        int leaf_index = (p.offset-LEAF_GROUP_HEAD)/calLeafSize();
        uint64_t usedNum;
        string bitmap;
        leafgroupFile.seekg(ios::beg);
        leafgroupFile.read((char*)&usedNum, sizeof(uint64_t));
        leafgroupFile.read((char*)&bitmap, sizeof(Byte)*LEAF_GROUP_AMOUNT);
        usedNum ++;
        bitmap[leaf_index] = '1';
        leafgroupFile.seekg(ios::beg);
        leafgroupFile.write((char*)&usedNum, sizeof(uint64_t));
        leafgroupFile.write((char*)&bitmap, sizeof(Byte)*LEAF_GROUP_AMOUNT);
        
        allocatorCatalog.close();
        freeListFile.close();
        leafgroupFile.close();
        return true;
    }
    return false;
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
        
        string allocatorCatalogPath = DATA_DIR + P_ALLOCATOR_CATALOG_NAME;
        string freeListPath = DATA_DIR + P_ALLOCATOR_FREE_LIST;
        string leafgroupPath = DATA_DIR + to_string(p.fileId);

        uint64_t usedNum;
        string bitmap;
        fstream leafgroupFile(leafgroupPath, ios::in|ios::app|ios::binary);
        leafgroupFile.seekp(ios::beg);
        leafgroupFile.read((char*)&usedNum, sizeof(uint64_t));
        leafgroupFile.read((char*)&bitmap, sizeof(Byte)*LEAF_GROUP_AMOUNT);
        int leaf_index = (p.offset-LEAF_GROUP_HEAD)/calLeafSize();
        usedNum --;
        bitmap[leaf_index] = '0';
        leafgroupFile.seekp(ios::beg);
        leafgroupFile.write((char*)&usedNum, sizeof(uint64_t));
        leafgroupFile.write((char*)&bitmap, sizeof(Byte)*LEAF_GROUP_AMOUNT);
        
        ofstream allocatorCatalog(allocatorCatalogPath, ios::app|ios::binary);
        allocatorCatalog.seekp(8, ios::beg);
        allocatorCatalog.write((char*)&this->freeNum, sizeof(uint64_t));

        ofstream freeListFile(freeListPath, ios::out|ios::binary);
        freeListFile.write((char*)&this->freeList, sizeof(PPointer)*this->freeNum);

        allocatorCatalog.close();
        freeListFile.close();
        leafgroupFile.close();
        return true;
    }
    return false;
}

bool PAllocator::persistCatalog() {
    // TODO
    string allocatorCatalogPath = DATA_DIR + P_ALLOCATOR_CATALOG_NAME;
    ifstream allocatorCatalog(allocatorCatalogPath, ios::in);
    if(allocatorCatalog.is_open()) {
        allocatorCatalog.close();
        int catalog_size = 8*2*sizeof(Byte)+sizeof(PPointer);
        pmem_persist((void*)&allocatorCatalogPath, catalog_size);
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
        int zero = 0;
        leafgroupFile.write((char*)&zero, sizeof(Byte)*(8+LEAF_GROUP_AMOUNT));
        /*int len = (LEAF_DEGREE * 2 + 7) / 8;
        file.seekg(len, ios::beg);*/
        string allocatorCatalogPath = DATA_DIR + P_ALLOCATOR_CATALOG_NAME;
        string freeListPath = DATA_DIR + P_ALLOCATOR_FREE_LIST;
        ofstream allocatorCatalog(allocatorCatalogPath, ios::app|ios::binary);
        ofstream freeListFile(freeListPath, ios::out|ios::binary);
        if(!(allocatorCatalog.is_open() && freeListFile.is_open())) {
            leafgroupFile.close();
            return false;
        }
        this->maxFileId ++;
        this->freeNum += LEAF_GROUP_AMOUNT;
        allocatorCatalog.seekp(ios::beg);
        allocatorCatalog.write((char*)&this->maxFileId, sizeof(uint64_t));
        allocatorCatalog.write((char*)&this->freeNum, sizeof(uint64_t));
        uint64_t offset = LEAF_GROUP_HEAD;
        uint64_t leaf_size = calLeafSize();
        PPointer p = this->startLeaf;
        while(getPNext(p).fileId!=ILLEGAL_FILE_ID) p = getPNext(p);
        for(uint64_t i = 0; i < LEAF_GROUP_SIZE; i ++) {
            PPointer t_p;
            t_p.fileId = this->maxFileId-1;
            t_p.offset = offset;
            string leafPath = DATA_DIR + to_string(p.fileId);
            ofstream leaf(leafPath, ios::app|ios::binary);
            int len = (LEAF_DEGREE * 2 + 7) / 8 + p.offset;
            leaf.seekp(len, ios::beg);
            leaf.write((char*)&(t_p), sizeof(PPointer));
            leaf.close();
            this->freeList.push_back(t_p);
            offset += leaf_size;
            p = t_p;
        }
        freeListFile.write((char*)&this->freeList, sizeof(PPointer)*this->freeNum);
        allocatorCatalog.close();
        freeListFile.close();
        leafgroupFile.close();
    }
    return false;
}
