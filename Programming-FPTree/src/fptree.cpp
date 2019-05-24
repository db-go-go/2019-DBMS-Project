#include"fptree/fptree.h"

using namespace std;

// Initial the new InnerNode
InnerNode::InnerNode(const int& d, FPTree* const& t, bool _isRoot) {
    // TODO
    this->tree = t;
    this->degree = d;
    this->isLeaf = false;

    this->isRoot = _isRoot;
    this->nKeys = 0;
    this->nChild = 0;
    this->keys = new Key[2 * d + 1];
    this->childrens = new Node*[2* d + 2];
}

// delete the InnerNode
InnerNode::~InnerNode() {
    // TODO
    delete[] this->keys;
    delete[] this->childrens;
}

// binary search the first key in the innernode larger than input key
int InnerNode::findIndex(const Key& k) {
    // TODO

    //int low = 0, high = nKeys, middle = 0;
    int low = 0, high = nKeys-1, middle = 0;
    //while(low < high) {
    while(low <= high) {
        middle = (low + high)/2;
        if(k == this->keys[middle]) {
            return middle + 1;
        } else if(k < this->keys[middle]) {
            //high = middle;
            high = middle-1;
        } else if(k > this->keys[middle]) {
            low = middle + 1;
        }
    }
//    printf("findIndex(): key %lu finish\n", k);
    return low;
}

// insert the node that is assumed not full
// insert format:
// ======================
// | key | node pointer |
// ======================
// WARNING: can not insert when it has no entry
void InnerNode::insertNonFull(const Key& k, Node* const& node) {
    // TODO
    //if(this->nChild > 0 && this->nKeys <= 2 * this->degree + 1) {
    if(this->nChild > 0 && this->nKeys <= 2 * this->degree) {
        int index = this->findIndex(k);
        // ATTENTION!!
        if(index == 0) {
            Node* n_t = this->getChild(0);
            int minKey = MAX_KEY;
            if(n_t->ifLeaf()) {
                LeafNode* l_t = (LeafNode*)n_t;
                for(int i = 0; i < LEAF_DEGREE*2; i ++) {
                    if(l_t->getBit(i)) {
                        if(l_t->kv[i].k < minKey) minKey = l_t->kv[i].k;
                    }
                }
            }
            else {
                InnerNode* i_t = (InnerNode*)n_t;
                minKey = i_t->keys[0];
            }
            if(minKey > k) {
                this->childrens[0] = node;

                for(int i = this->nKeys-1; i >= index; i --) {
                    this->keys[i+1] = this->keys[i];
                }
                this->keys[index] = minKey;
                this->nKeys ++;
                for(int i = this->nChild-1; i >= index+1; i --) {
                    this->childrens[i+1] = this->childrens[i];
                }
                this->childrens[index+1] = n_t;
                this->nChild ++;

                return ;
            }
        }

        for(int i = this->nKeys-1; i >= index; i --) {
            this->keys[i+1] = this->keys[i];
        }
        this->keys[index] = k;
        this->nKeys ++;
        for(int i = this->nChild-1; i >= index+1; i --) {
            this->childrens[i+1] = this->childrens[i];
        }
        this->childrens[index+1] = node;
        this->nChild ++;
    }
//    printf("insertNonFull) :nKeys %d nChild %d\n", this->nKeys, this->nChild);
}

// insert func
// return value is not NULL if split, returning the new child and a key to insert
KeyNode* InnerNode::insert(const Key& k, const Value& v) {
    KeyNode* newChild = NULL;

    // 1.insertion to the first leaf(only one leaf)
    if (this->isRoot && this->nChild == 0) {
        // TODO
        LeafNode* ln = new LeafNode(this->tree);
        ln->insertNonFull(k, v);
        KeyNode leaf;
        leaf.key = k;
        leaf.node = ln;
        this->insertLeaf(leaf);
        return newChild;
    }

    // 2.recursive insertion
    // TODO
    int index = this->findIndex(k);
    KeyNode* kn = this->childrens[index]->insert(k, v);
    if(kn != NULL) {
        this->insertNonFull(kn->key, kn->node);
        if(this->nKeys > this->degree*2) {
            newChild = this->split();
            if(this->isRoot) {
                this->isRoot = false;
                InnerNode* newRoot = new InnerNode(this->degree, this->tree, true);
                (*this->tree).changeRoot(newRoot);
                newRoot->childrens[0] = this;
                newRoot->childrens[1] = newChild->node;
                newRoot->nChild += 2;
                newRoot->keys[0] = newChild->key;
                newRoot->nKeys ++;
                newChild = NULL;
            }
        }
    }
    return newChild;
}

// ensure that the leaves inserted are ordered
// used by the bulkLoading func
// inserted data: | minKey of leaf | LeafNode* |
KeyNode* InnerNode::insertLeaf(const KeyNode& leaf) {
    KeyNode* newChild = NULL;
    // first and second leaf insertion into the tree
    if (this->isRoot && this->nKeys == 0) {
        // TODO
        this->childrens[this->nChild] = leaf.node;
        this->nChild ++;
        if(this->nChild == 2) {
            Key minKey = MAX_KEY;
            for(int i = 0; i < this->degree*2; i ++) {
                if((*(LeafNode*)this->childrens[0]).getBit(i)) {
                    if((*(LeafNode*)this->childrens[0]).kv[i].k < minKey) 
                        minKey = (*(LeafNode*)this->childrens[0]).kv[i].k;
                }
            }
            this->nKeys ++;
            if(minKey < leaf.key)
                this->keys[0] = leaf.key;
            else {
                this->childrens[1] = this->childrens[0];
                this->childrens[0] = leaf.node;
                this->keys[0] = minKey;
            }
        }
        return newChild;
    }
    
    // recursive insert
    // Tip: please judge whether this InnerNode is full
    // next level is not leaf, just insertLeaf
    // TODO
    KeyNode* kn = new KeyNode();
    if(!(*this->childrens[0]).ifLeaf()) {
        int index = this->findIndex(leaf.key);
        kn = (*(InnerNode*)this->childrens[index]).insertLeaf(leaf);
    }
    // next level is leaf, insert to childrens array
    // TODO
    else {
        *kn = leaf;
    }
    if(kn != NULL) {

        this->insertNonFull(kn->key, kn->node);

        if(this->nKeys > 2*this->degree) {
            newChild = this->split();
            if(this->isRoot) {
                this->isRoot = false;
                InnerNode* newRoot = new InnerNode(this->degree, this->tree, true);
                (*this->tree).changeRoot(newRoot);
                newRoot->childrens[0] = this;
                newRoot->childrens[1] = newChild->node;
                newRoot->nChild += 2;
                newRoot->keys[0] = newChild->key;
                newRoot->nKeys ++;
                newChild = NULL;
            }
        }
    }
 
    return newChild;
}

KeyNode* InnerNode::split() {

    KeyNode* newChild = new KeyNode();
    // right half entries of old node to the new node, others to the old node. 
    // TODO
    newChild->key = this->keys[this->degree];
    InnerNode* newIn = new InnerNode(this->degree, this->tree, false);
    newIn->nKeys = this->degree;
    newIn->nChild = this->degree+1;
    newIn->childrens[0] = this->childrens[this->degree+1];
    for(uint64_t i = 0; i < this->degree; i ++) {
        newIn->keys[i] = this->keys[this->degree+1+i];
        newIn->childrens[i+1] = this->childrens[this->degree+2+i];
    }
    newChild->node = newIn;
    this->nKeys = this->degree;
    this->nChild = this->degree+1;

    return newChild;
}

// remove the target entry
// return TRUE if the children node is deleted after removement.
// the InnerNode need to be redistributed or merged after deleting one of its children node.
bool InnerNode::remove(const Key& k, const int& index, InnerNode* const& parent, bool &ifDelete) {
    bool ifRemove = false;
    // only have one leaf
    // TODO
    this->printNode();
    cout << "###MYTEST InnerNode::remove() to remove key=" <<k<<"\n";
    if(this->getChildNum() == 1 && (*this->getChild(0)).ifLeaf()) {
        ifRemove = (*this->getChild(0)).remove(k, 0, this, ifDelete);
        //cout << "###MYTEST InnerNode::remove() childNum==1 case on\n";
        //cout << "###MYTEST InnerNode::remove() ifRemove=" <<ifRemove<<"\n";
        if(ifRemove) 
            delete this->getChild(0);
    }
    // recursive remove
    // TODO
    else {
        int idx = this->findIndex(k);
        //cout << "###MYTEST InnerNode::remove() idx="<<idx<<"\n";
        ((InnerNode*)this->getChild(idx))->printNode();
        ifRemove = (*this->getChild(idx)).remove(k, idx, this, ifDelete);
        //cout << "###MYTEST InnerNode::remove() ifRemove="<<ifRemove<<"\n";
        if(ifRemove) {
            delete this->getChild(idx);
            this->removeChild(idx-1, idx);
            if(!this->isRoot && this->nKeys < this->degree) {
                InnerNode* leftBro, *rightBro;
                this->getBrother(index, parent, leftBro, rightBro);

                bool managed = false;
                if(rightBro != NULL) {
                    cout <<  "###MYTEST InnerNode::reomve() rightBro->nKeys="<<rightBro->nKeys<<"\n";
                    if(rightBro->nKeys-1 >= this->degree) {
                        cout <<"###MYTEST InnerNode::remove() index passed to redistributeRight()="<<index<<"\n";
                        this->redistributeRight(index, rightBro, parent);
                        ifDelete = false;
                        managed = true;
                        cout << "###MYTEST  redistributeRight\n";
                    }
                    else {
                        cout << "###MYTEST mergeParentRight\n";
                        if(leftBro == NULL || leftBro->nKeys-1 < this->degree) {
                            if(parent->isRoot && parent->getChildNum() == 2) {
                                cout << "###MYTEST mergeParentRight\n";
                                this->mergeParentRight(parent, rightBro);
                                ifDelete = false;
                            }
                            else {
                                cout << "###MYTEST mergeRight\n";
                                this->mergeRight(rightBro, parent->getKey(index));
                                ifDelete = true;
                                managed = true;
                            }
                        }
                    }
                }
                if(!managed) {
                    if(leftBro->nKeys-1 >= this->degree) {
                        this->redistributeLeft(index, leftBro, parent);
                        ifDelete = false;
                    }
                    else {
                        if(parent->isRoot && parent->getChildNum() == 2) {
                            this->mergeParentLeft(parent, leftBro);
                            ifDelete = false;
                        }
                        else {
                            this->mergeLeft(leftBro, parent->getKey(index-1));
                            ifDelete = true;
                        }
                    }
                }
            }
        }
    }
    //return ifRemove;
    return ifDelete;
}

// If the leftBro and rightBro exist, the rightBro is prior to be used
void InnerNode::getBrother(const int& index, InnerNode* const& parent, InnerNode* &leftBro, InnerNode* &rightBro) {
    // TODO
    if(index == 0) {
        leftBro = NULL;
    }
    else {
        leftBro = (InnerNode*)parent->getChild(index-1);
    }
    if(index == parent->nChild-1) {
        rightBro = NULL;
    }
    else {
        rightBro = (InnerNode*)parent->getChild(index+1);
    }
}

// merge this node, its parent and left brother(parent is root)
void InnerNode::mergeParentLeft(InnerNode* const& parent, InnerNode* const& leftBro) {
    // TODO
    Key k = parent->keys[0];
    // ATTENTION!
    this->mergeLeft(leftBro, k);
    memcpy(parent, this, sizeof(InnerNode));
    parent->isRoot = true;
}

// merge this node, its parent and right brother(parent is root)
void InnerNode::mergeParentRight(InnerNode* const& parent, InnerNode* const& rightBro) {
    // TODO
    Key k = parent->keys[0];
    // ATTENTION!
    this->mergeRight(rightBro, k);
    memcpy(parent, this, sizeof(InnerNode));
    parent->isRoot = true;
}

// this node and its left brother redistribute
// the left has more entries
void InnerNode::redistributeLeft(const int& index, InnerNode* const& leftBro, InnerNode* const& parent) {
    // TODO
    this->insertNonFull(leftBro->getKey(leftBro->getKeyNum()-1), leftBro->getChild(leftBro->getChildNum()-1));
    leftBro->nKeys --;
    leftBro->nChild --;
    parent->keys[index-1] = leftBro->keys[leftBro->getKeyNum()];
    leftBro->printNode();
    parent->printNode();
}

// this node and its right brother redistribute
// the right has more entries
void InnerNode::redistributeRight(const int& index, InnerNode* const& rightBro, InnerNode* const& parent) {
    // TODO
    this->insertNonFull(parent->getKey(index), rightBro->getChild(0));
    rightBro->nKeys --;
    rightBro->nChild --;
    parent->keys[index] = rightBro->keys[0];
    rightBro->removeChild(0, 0);
    rightBro->printNode();
    parent->printNode();
}

// merge all entries to its left bro, delete this node after merging.
void InnerNode::mergeLeft(InnerNode* const& leftBro, const Key& k) {
    // TODO
    int nKeys_t = leftBro->nKeys;
    leftBro->keys[nKeys_t] = k;
    
    for(int i = 0; i < this->nKeys; i ++) {
        leftBro->keys[nKeys_t+1+i] = this->keys[i];
        leftBro->childrens[nKeys_t+1+i] = this->childrens[i];
    }
    leftBro->nKeys += (1+this->nKeys);
    leftBro->nChild += this->nChild;
    leftBro->childrens[leftBro->nChild-1] = this->childrens[this->nChild-1];

    delete this;
}

// merge all entries to its right bro, delete this node after merging.
void InnerNode::mergeRight(InnerNode* const& rightBro, const Key& k) {
    // TODO
    
    for(int i = 0; i < rightBro->nKeys; i ++) {
        rightBro->keys[rightBro->nKeys-i+this->nKeys] = rightBro->keys[rightBro->nKeys-1-i];
    }
    memcpy(rightBro->keys, this->keys, this->nKeys*sizeof(Key));
    rightBro->keys[rightBro->nKeys] = k;
    for(int i = 0; i < rightBro->nChild; i ++) {
        rightBro->childrens[rightBro->nChild-i+this->nChild-1] = rightBro->childrens[rightBro->nChild-1-i];
    }
    memcpy(rightBro->childrens, this->childrens, this->nChild*sizeof(InnerNode*));
    
    rightBro->nKeys += (this->nKeys+1);
    rightBro->nChild += this->nChild;

    delete this;
}

// remove a children from the current node, used by remove func
void InnerNode::removeChild(const int& keyIdx, const int& childIdx) {
    // TODO
    this->nKeys --;
    this->nChild --;
    for(int i = keyIdx; i < this->nKeys; i ++) {
        this->keys[i] = this->keys[i+1];
    }
    for(int i = childIdx; i < this->nChild; i ++) {
        this->childrens[i] = this->childrens[i+1];
    }
}

// update the target entry, return true if the update succeed.
bool InnerNode::update(const Key& k, const Value& v) {
    // TODO
    int index = this->findIndex(k);
    return (*this->getChild(index)).update(k, v);
}

// find the target value with the search key, return MAX_VALUE if it fails.
Value InnerNode::find(const Key& k) {
    // TODO
 //   printNode();
    int index = this->findIndex(k);
    Value v;
    v = (*this->getChild(index)).find(k);
    return v;
}

// get the children node of this InnerNode
Node* InnerNode::getChild(const int& idx) {
    // TODO
    return this->childrens[idx];
}

// get the key of this InnerNode
Key InnerNode::getKey(const int& idx) {
    if (idx < this->nKeys) {
        return this->keys[idx];
    } else {
        return MAX_KEY;
    }
}

// print the InnerNode
void InnerNode::printNode() {
    cout << "||#|";
    for (int i = 0; i < this->nKeys; i++) {
        cout << " " << this->keys[i] << " |#|";
    }
    cout << "|" << "    ";
}




// print the LeafNode
void LeafNode::printNode() {
    cout << "||";
    for (int i = 0; i < 2 * this->degree; i++) {
        if (this->getBit(i)) {
            cout << " " << this->kv[i].k << " : " << this->kv[i].v << " |";
        }
    }
    cout << "|" << " ====>> ";
}

// new a empty leaf and set the valuable of the LeafNode
LeafNode::LeafNode(FPTree* t) {
    // TODO
    this->tree = t;
    this->degree = LEAF_DEGREE;
    this->isLeaf = true;
    PAllocator::getAllocator()->getLeaf(this->pPointer, this->pmem_addr);
    this->bitmapSize = (this->degree* 2) / (8*sizeof(Byte));
    this->bitmap = new Byte[this->bitmapSize];
    memset(this->bitmap,0,this->bitmapSize); 
    this->fingerprints = new Byte[2*this->degree];
    this->kv = new KeyValue[2*this->degree];
    n = 0;
    this->filePath = DATA_DIR + to_string(this->pPointer.fileId);

}

// reload the leaf with the specific Persistent Pointer
// need to call the PAllocator
LeafNode::LeafNode(PPointer p, FPTree* t) {
    // TODO
    this->tree = t;
    this->degree = LEAF_DEGREE;
    this->isLeaf = true;
    this->pPointer = p;
    this->pmem_addr = PAllocator::getAllocator()-> getLeafPmemAddr(p);
    this->bitmapSize = (this->degree* 2) / (8*sizeof(Byte));
    this->bitmap = new Byte[this->bitmapSize];
    uint64_t offset = 0;
    for(int i = 0; i < this->bitmapSize; i ++) {
        memcpy(&this->bitmap[i], &pmem_addr[offset], sizeof(Byte));
        offset += sizeof(Byte);
    }

    this->fingerprints = new Byte[2*this->degree];
    offset += sizeof(PPointer);
    for(int i = 0; i < LEAF_DEGREE*2; i ++) {
        memcpy(&fingerprints[i], &pmem_addr[offset], sizeof(Byte));
        offset += sizeof(Byte);
    }
    this->kv = new KeyValue[2*this->degree];
    n = 0;
    for(int i = 0; i < LEAF_DEGREE*2; i ++)  {
        if(getBit(i) == 1) {
            memcpy(&kv[n].k, &pmem_addr[offset], sizeof(uint64_t));
            memcpy(&kv[n].v, &pmem_addr[offset+sizeof(uint64_t)], sizeof(uint64_t));
            n ++;
            offset += sizeof(KeyValue);
        }
    }
    this->filePath = DATA_DIR + to_string(p.fileId);
//    printf("LeafNode(2) :finish\n");
}

LeafNode::~LeafNode() {
    // TODO
    delete [] this->bitmap;
    delete [] this->fingerprints;
    delete [] this->kv;

    // TODO
}

// insert an entry into the leaf, need to split it if it is full
KeyNode *LeafNode::insert(const Key &k, const Value &v)
{

    KeyNode *newChild = NULL;
    // TODO
    this->insertNonFull(k, v);
    if (this->n >= 2 * this->degree)
    { //full
       //cout << "###MYTEST LeafNode::insert() a leaf is full\n";
        newChild = split();
    }

    return newChild;
}

// insert into the leaf node that is assumed not full
void LeafNode::insertNonFull(const Key &k, const Value &v)
{
    // TODO

    int slot = LeafNode::findFirstZero();
    this->kv[slot].k = k;
    this->kv[slot].v = v;
    this->fingerprints[slot] = keyHash(k);
    this->bitmap[(slot / 8)] |= (1 << (7 - slot % 8));

    this->n ++;

    uint64_t offset = slot / 8;
    memcpy(&pmem_addr[offset], &this->bitmap[slot/8], sizeof(Byte));
   
    offset = this->bitmapSize*sizeof(Byte) + sizeof(PPointer) + slot*sizeof(Byte);
    memcpy(&pmem_addr[offset], &fingerprints[slot], sizeof(Byte));
    
    offset = this->bitmapSize*sizeof(Byte) + sizeof(PPointer) + this->degree*2*sizeof(Byte) + slot*sizeof(KeyValue);
    memcpy(&pmem_addr[offset], &kv[slot].k,sizeof(uint64_t));
    memcpy(&pmem_addr[offset+sizeof(uint64_t)], &kv[slot].v,sizeof(uint64_t));
    
    persist();
}


// split the leaf node
KeyNode* LeafNode::split() {
    
    
    KeyNode* newChild = new KeyNode();
    // TODO
    LeafNode * newLeafNode = new LeafNode(this->tree);
    Key SplitKey = findSplitKey();
    for (int i = 0; i < bitmapSize/2; ++i)
    {
        newLeafNode->bitmap[i] = 0;
        newLeafNode->bitmap[bitmapSize/2+i] = this->bitmap[0];
        this->bitmap[bitmapSize/2+i] = 0;
    }

    for(int i = 0; i < this->degree*2; i ++) {
        memcpy(&newLeafNode->fingerprints[i], &this->fingerprints[i], sizeof(Byte));
        memcpy(&newLeafNode->kv[i].k, &this->kv[i].k, sizeof(uint64_t));
        memcpy(&newLeafNode->kv[i].v, &this->kv[i].v, sizeof(uint64_t));
    }

    newLeafNode->prev = this;
    this->next = newLeafNode;
    newLeafNode->n = this->degree;
    this->n = this->degree;

    newChild->key = SplitKey;
    newChild->node = newLeafNode;

    persist();

    return newChild;
}

int partition(KeyValue vi[], Byte finger[], int low, int up)
{
    Key pivot = vi[up].k;
    int i = low-1;
    for (int j = low; j < up; j++)
    {
        if(vi[j].k <= pivot)
        {
            i++;
            KeyValue kv_tmp = vi[i];
            vi[i] = vi[j];
            vi[j] = kv_tmp;
            Byte fp_tmp = finger[i];
            finger[i] = finger[j];
            finger[j] = fp_tmp;
        }
    }
    KeyValue kv_tmp = vi[i+1];
    vi[i+1] = vi[up];
    vi[up] = kv_tmp;
    Byte fp_tmp = finger[i+1];
    finger[i+1] = finger[up];
    finger[up] = fp_tmp;

    return i+1;
}

void quicksort(KeyValue a[], Byte finger[], Key l, Key r) {
    if(l < r)
    {
        int mid = partition(a, finger, l, r);
        //Watch out! The mid position is on the place, so we don't need to consider it again.
        //That's why below is mid-1, not mid! Otherwise it will occur overflow error!!!
        quicksort(a, finger, l, mid-1);
        quicksort(a, finger, mid+1, r);
    }
}


// use to find a mediant key and delete entries less then middle
// called by the split func to generate new leaf-node
// qsort first then find
Key LeafNode::findSplitKey() {
    Key midKey = 0;
    // TODO
    quicksort(this->kv, this->fingerprints, 0, 2 * LEAF_DEGREE - 1);
    midKey = this->kv[LEAF_DEGREE].k;
    return midKey;
}

// get the targte bit in bitmap
// TIPS: bit operation
int LeafNode::getBit(const int& idx) {
    // TODO
    return this->bitmap[(idx / 8)] & (1 << (7 - idx % 8)) ? 1 : 0;
}

Key LeafNode::getKey(const int& idx) {
    return this->kv[idx].k;
}

Value LeafNode::getValue(const int& idx) {
    return this->kv[idx].v;
}

PPointer LeafNode::getPPointer() {
    return this->pPointer;
}

// remove an entry from the leaf
// if it has no entry after removement return TRUE to indicate outer func to delete this leaf.
// need to call PAllocator to set this leaf free and reuse it
bool LeafNode::remove(const Key& k, const int& index, InnerNode* const& parent, bool &ifDelete) {
    bool ifRemove = true;
    //ifDelete = false;
    ifDelete = true;
    // TODO
    for (uint64_t i = 0; i < 2*LEAF_DEGREE; i ++) {
        if (getBit(i)) {//有数据的槽
            if (this->kv[i].k == k) {
                Byte tmp = 0;
                for(int j = 0; j < 8; j ++) {
                    if(j != i%8) tmp += 1;
                    if(j!=7)  tmp <<= 1;
                }
                this->bitmap[i/8] &= tmp;
                //cout << "###MYTEST LeafNode::remove() getBit(" <<i<<")="<<getBit(i)<<"\n";
                uint64_t offset = (i/8)*sizeof(Byte);
                //cout << "###MYTEST LeafNode::remove() offset="<<offset<<"\n";
                memcpy(&this->pmem_addr[offset], &(this->bitmap[i/8]), sizeof(Byte));
                this->persist();
              //  ifRemove = true;
               // ifDelete = true;
                for(int j = 0; j < this->bitmapSize; j ++) {
                    if(this->bitmap[j] != 0) {
                        ifDelete = false;
                        ifRemove = false;
                        break;
                    }
                }
                break;
            }
        }
    }
    //cout << "###MYTEST LeafNode::remove() ifRemove="<<ifRemove<<"\n";
    //cout << "###MYTEST LeafNode::remove() ifDelete="<<ifDelete<<"\n";
    if(ifDelete) PAllocator::getAllocator()->freeLeaf(this->pPointer);
    return ifRemove;
}

// update the target entry
// return TRUE if the update succeed
bool LeafNode::update(const Key& k, const Value& v) {
 //   this->printNode();
    bool ifUpdate = false;
    // TODO
    for (uint64_t i = 0; i < 2*LEAF_DEGREE; i ++) {
        if (getBit(i)) {//有数据的槽
            if (this->kv[i].k == k) {
                this->kv[i].v = v;
                uint64_t offset = this->bitmapSize*sizeof(Byte)+sizeof(PPointer)+this->degree*2*sizeof(Byte)+i*sizeof(KeyValue)+sizeof(Key);
                memcpy(&this->pmem_addr[offset], &this->kv[i].v, sizeof(Value));
                this->persist();
                ifUpdate = true;
 //               this->printNode();
                return ifUpdate;
            }

        }
    }
    return ifUpdate;
}

// if the entry can not be found, return the max Value
Value LeafNode::find(const Key& k) {
    //cout << "###MYTEST LeafNode::find() printNode\n";
    //this->printNode();
    // TODO
//    printf("LeafNode::find() %lu\n", k);
    for (uint64_t i = 0; i < 2*LEAF_DEGREE; i ++) {
        if (getBit(i)) {//有数据的槽
            //cout << "###MYTEST LeafNode::find() getBit("<<i<<")="<<getBit(i)<<"\n";
            if (this->kv[i].k == k)
                return this->kv[i].v;
        }
    }
    return MAX_VALUE;
}

// find the first empty slot
int LeafNode::findFirstZero() {
    // TODO
    uint64_t idx = 0;
    for (uint64_t i = 0; i < 2*this->degree; i ++) {
    //for (uint64_t i = 0; i < 2*this->degree-1; i ++) {
        if (!getBit(i)) {
            idx = i;
            break;
        }
    }
    return idx;
}

// persist the entire leaf
// use PMDK
void LeafNode::persist() {
    // TODO
    ifstream leafgroupFile(this->filePath, ios::in|ios::binary);
    if(leafgroupFile.is_open()) {
        leafgroupFile.close();
        uint64_t leafgroup_size = LEAF_GROUP_HEAD + LEAF_GROUP_AMOUNT*calLeafSize();
        pmem_persist((void*)&this->filePath, leafgroup_size);
    }
    //uint64_t offset = this->bitmapSize*sizeof(Byte) + sizeof(PPointer) + this->degree*sizeof(Byte);
    //printf("persist() :key[%d] value[%d]\n", pmem_addr[offset], pmem_addr[offset+sizeof(uint64_t)]);
}

// call by the ~FPTree(), delete the whole tree
void FPTree::recursiveDelete(Node* n) {
    if (n->isLeaf) {
        //cout << "###MYTEST FPTree::recursiveDelte() 1 segmentation not fault yet\n";
        delete n;
    } else {
        for (int i = 0; i < ((InnerNode*)n)->nChild; i++) {
            recursiveDelete(((InnerNode*)n)->childrens[i]);
            //cout << "i = " << i << "\n";
            //cout << "###MYTEST FPTree::recursiveDelte() 2 segmentation not fault yet\n";
        }
        //delete n;
    }
}

FPTree::FPTree(uint64_t t_degree) {
    FPTree* temp = this;
    this->root = new InnerNode(t_degree, temp, true);
    this->degree = t_degree;
    bulkLoading();
}

FPTree::~FPTree() {
    recursiveDelete(this->root);
}

// get the root node of the tree
InnerNode* FPTree::getRoot() {
    return this->root;
}

// change the root of the tree
void FPTree::changeRoot(InnerNode* newRoot) {
    this->root = newRoot;
}

void FPTree::insert(Key k, Value v) {
    if (root != NULL) {
        root->insert(k, v);
    }
}

bool FPTree::remove(Key k) {
    //cout << "###MYTEST FPTree::remove() this->degree="<<this->degree<<"\n";
    if (root != NULL) {
        bool ifDelete = false;
        InnerNode* temp = NULL;
        return root->remove(k, -1, temp, ifDelete);
    }
    return false;
}

bool FPTree::update(Key k, Value v) {
    if (root != NULL) {
        return root->update(k, v);
    }
    return false;
}

Value FPTree::find(Key k) {
    if (root != NULL) {
        return root->find(k);
    }
}

// call the InnerNode and LeafNode print func to print the whole tree
// TIPS: use Queue
void FPTree::printTree() {
    // TODO
    queue<InnerNode*> printInnerNode;
    queue<LeafNode*> printLeafNode;
    printInnerNode.push(this->root);
    while (!printInnerNode.empty()) {
        for (int i = 0; i < printInnerNode.front()->nChild; i ++) {
            if (printInnerNode.front()->childrens[i]->isLeaf)
                printLeafNode.push((LeafNode*)printInnerNode.front()->childrens[i]);
            else
                printInnerNode.push((InnerNode*)printInnerNode.front()->childrens[i]);
        }
        printInnerNode.front()->printNode();
        printInnerNode.pop();
    }
    while(!printLeafNode.empty()) {
        printLeafNode.front()->printNode();
        printLeafNode.pop();
    }
}

// bulkLoading the leaf files and reload the tree
// need to traverse leaves chain
// if no tree is reloaded, return FALSE
// need to call the PALlocator
bool FPTree::bulkLoading() {
    // TODO
    PPointer p = PAllocator::getAllocator()->getStartPointer(); // get first leaf's  PPointer of fptree
    if(p.fileId == ILLEGAL_FILE_ID) return false;
    while(p.fileId != ILLEGAL_FILE_ID) {
        
        if(PAllocator::getAllocator()->ifLeafUsed(p)) {
            LeafNode* leaf = new LeafNode(p, this);
            Key minKey = MAX_KEY;
            for(int i = 0; i < this->degree*2; i ++) {
                if(leaf->getBit(i)) {
                    if(leaf->kv[i].k < minKey) minKey = leaf->kv[i].k;
                }
            }
            KeyNode kn_leaf;
            kn_leaf.key = minKey;
            kn_leaf.node = leaf;
            (*this->root).insertLeaf(kn_leaf);
        }
        p = getPNext(p);
    }
    //this->printTree();
    printf("bulkLoading() :finish\n");
    return true;
}
