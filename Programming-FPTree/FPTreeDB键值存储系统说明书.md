# FPTreeDB键值存储系统说明书

## 1. 简介
本系统为基于针对NVM优化的数据结构FPTree的一个简单的键值存储引擎FPTreeDB。我们通过将其包装成一个调用库，供用户程序使用并管理其数据存储。

## 2. 系统架构
![系统架构](https://github.com/ZhangJiaQiao/2019-DBMS-Project/raw/master/asset/FPTreeDB.png "系统架构")

## 3. 数据结构
### 3.1 FPTree
#### 介绍
这是整个键值存储系统的接口类，通过其调用InnerNode进而调用LeafNode进行键值对操作。  一个FPTree就是一个键值对数据库，对应一个文件夹。  其数据文件与PAllocator的管理文件存在同一个文件夹下。
#### 操作
##### 增删改查
这些只是操作的接口函数，将会调用节点的函数
##### BulkLoading
###### 函数定义
bool FPTree::bulkLoading()
###### 函数说明
这是树重建的主要函数。在新建一棵树的时候，先检查目标文件夹内有没有数据，遍历文件进行BulkLoading。没有数据文件则进行新树的升恒。
### 3.2 Node（节点）
#### 介绍
这是InnerNode和LeafNode的父类。它有一些InnerNode和LeafNode共有的属性。
#### 操作
大多是纯虚函数，由子类实现。
### 3.3 KeyNode
#### 介绍
由一个代表键值和节点的索引组成。  用于节点分裂时，将新生成的节点索引返回给上层节点插入记录。  在插入操作和分裂操作中，将会使用到这一数据结构。
### 3.4 InnerNode（内部节点）（中间节点）
#### 介绍
这是FPTree的中间索引节点，其不存放键值对数据信息。其结构如图所示：
![InnerNode结构](https://github.com/ZhangJiaQiao/2019-DBMS-Project/raw/master/asset/InnerNode.png "InnerNode结构")
  节点元素是有序的。
  每个节点的元素个数限制在以下范围内：
  *key个数：d <= m <= 2d*  
  *node指针个数：d+1 <= m <= 2d+1*
#### 操作
##### Remove（删除键值对）
###### 函数定义
bool InnerNode::remove(Key k, int index, InnerNode* parent, bool & ifDelete)
###### 函数参数说明
k：目标键值对的键  
index：当前节点属于其父亲节点的第几个元素  
parent：当前节点的父亲节点  
ifDelete：当前节点为空则需要删除，返回给父亲节点是否删除的信息
###### 返回值
删除成功，返回true；否则，返回false
###### 函数说明
这是InnerNode删除键值对元素的函数，其不进行实际的键值对删除，通过递归调用其子节点删除，一直到叶子节点进行实际的删除。  
它会对子节点返回的信息进行处理，识别其下层调用的节点是否要被删除。  
删除子节点的时候，它会识别当前节点元素是否符合限制，不符合则进行重分布，重分布不行则再进行合并。
##### Insert（插入键值对）
###### 函数定义
keyNode* InnerNode::insert(Key k, Value v)
###### 函数参数说明
k：插入键值对的键  
v：插入键值对的值
###### 返回值
返回值类型为KeyNode，其内容为下层子节点分裂后生成新节点的节点索引以及代表键值，用以给上层节点插入
###### 函数说明
这是InnerNode插入键值对的函数，不进行实际的键值对插入，通过递归调用其子节点插入，直至叶子结点进行实际的插入。对子节点分裂后返回的KeyNode值进行插入。
##### Find（键值对查询）
###### 函数定义
bool InnerNode::find(Key k)
###### 函数参数说明
k：需要查询的键值对的键
###### 返回值
查询成功，则返回查询得到的值；查询失败，则返回MAX_VALUE
###### 函数说明
这是InnerNode的查询函数，不进行实际的查询。二分查找目标子节点，递归调用其对应子节点的查询函数，直至叶子节点进行实际的查询，返回查询得到的值。
##### Update（修改键值对）
###### 函数定义
bool InnerNode:update(Key k, Value, v)
###### 函数参数说明
k：被修改键值对的键
v：被修改键值对的值
###### 返回值
修改键值对成功，返回true；否则，返回false
###### 函数说明
这是InnerNode更新键值对的函数，递归调用至叶子节点进行实际的更新
### 3.5 LeafNode（叶子节点）
#### 介绍
这是整个FPTree存储数据的直接对象，所有键值对数据之存放于叶子节点中。  
叶子节点也因此是与NVM交互的对象，只要是操作PAllocator映射NVM文件后的虚拟地址，通过文件映射的方式操作相应数据文件。  
因为节点的操作对象是内存映射文件数据后的虚拟地址，所以关键是设置好NVM数据的指针。  
其结构如下：
![LeafNode结构](https://github.com/ZhangJiaQiao/2019-DBMS-Project/raw/master/asset/LeafNode.png "LeafNode结构")  
叶子节点中的键值对个数要控制在一定范围内：*0 < m < 2d*
#### 操作
##### Remove（删除键值对）
###### 函数定义
bool LeafNode:remove(Key k, int index, InnerNode* parent, bool & ifDelete)
###### 函数参数说明
k：目标键值对的键  
index：当前节点属于其父亲节点的第几个元素  
parent：当前节点的父亲节点  
ifDelete：当前节点为空则需要删除，返回给父亲节点是否删除的信息
###### 返回值
删除成功，返回true；否则，返回false
###### 函数说明
这个函数时删除键值对操作流程的最后一个调用的函数，执行对存放在NVM的叶子进行键值对的删除，将位图置0
##### Insert（插入键值对）
###### 函数定义
KeyNode* LeafNode::insert(Key k, Value v)
###### 函数参数说明
k：插入键值对的键  
v：插入键值对的值
###### 返回值
返回值类型为KeyNode，其内容为叶子节点分裂后生成新节点的节点索引以及代表键值，用以给上层节点插入
###### 函数说明
这个是键值对插入流程的最后调用，其执行数据的真正插入。主要操作是对文件的插入，这个通过操作映射后的虚拟地址就可以像操作内存变量一样写入，然后调用持久化命令即可。
##### Find（键值对查询）
###### 函数定义
Value LeafNode::find(Key k)
###### 函数参数说明
k：需要查询的键值对的键
###### 返回值
查询成功，则返回查询得到的值；查询失败，则返回MAX_VALUE
###### 函数说明
这是叶子节点进行真正的数据查找函数。其首先遍历位图，找寻有数据的槽。然后先对比键值的键值是否一样，进行过滤。最后再对比键值是否一样。
##### Update（修改键值对）
###### 函数定义
bool update(Key k, Value v)
###### 函数参数说明
k：被修改键值对的键
v：被修改键值对的值
###### 返回值
修改键值对成功，返回true；否则，返回false
###### 函数说明
直接对目标键值对修改更新
### 3.6 PAllocator[实现时间：2019/5/4]
#### 介绍
这是NVM文件管理的主要对象，其负责分配LeafNode在NVM中的空间，映射数据文件并返回虚拟地址给LeafNode使用。其管理的叶子文件的粒度是一个LeafGroup，一个LeafGroup由多个叶子以及一个数据头组成，数据头由一个8字节的当前LeafGroup使用叶子数和叶子槽的bitmap，bitmap为了简单使用1个byte指明一个槽位。  
其主要数据文件如下：  
> LeafGroup结构：| usedNum(8 bytes) | bitmap(n bytes) | Leaf1 | ... | leafN |
> catelog：| maxFileId(8 bytes) | freeNum(8 bytes) | treeStartLeaf(PPointer) |
> freeList：| (fId, offset)1, ..., (fId)N |
其中，LeafGroup是数据文件，其文件名用整数表示，从1递增分配即可，规定0为非法标号。PAllocator需要记录分配文件的最大标号，即catalog文件的maxFileId。catalog文件中freeNum为当前可用叶子数，treeStartLeaf为第一个叶子的持久化指针，用于重载树时从其开始，通过链表形式重载。freeList文件每个条目为空叶子的持久化指针，用于启动即可知道可用叶子。数据文件中所有的数据均是定长的，没有变长，所以每个数据变量头尾相接存放即可。
#### 操作
##### PAllocator()构造函数
###### 函数定义
PAllocator::PAllocator()
###### 函数说明
首先判断catalog是否存在，存在则直接将文件中的相应数据读到PAllocator的数据域中；如果不存在，则需要创建catalog和free_list文件，并对它们进行修改，同时将修改PAllocator的数据域。
##### ~PAllocator()解构函数
###### 函数定义
PAllocator::~PAllocator()
###### 函数说明
直接释放pAllocator指针即可。
##### initFilePmemAddr()
###### 函数定义
void PAllocator::initFilePmemAddr()
###### 函数说明
映射数据文件，得到它们的虚拟地址, 并存到fId2PmAddr中
##### getLeafPmemAddr()
###### 函数定义
char* PAllocator::getLeafPmemAddr(PPointer p)
###### 函数说明
从fid2pmadr中获取目标持久化指针的虚拟地址
##### getLeaf()
###### 函数定义
bool PAllocator::getLeaf(PPointer &p, char* &pmem_addr)
###### 函数说明
取一个空闲叶
##### ifLeafUsed()
###### 函数定义
bool PAllocator::ifLeafUsed(PPointer p)
###### 函数说明
判断持久化指针p指向的一个叶子是否已经被使用
##### ifLeafFree()
###### 函数定义
bool PAllocator::ifLeafFree(PPointer p)
###### 函数说明
判断持久化指针p指向的一个叶子是否为空闲状态
##### ifLeafExist()
###### 函数定义
bool PAllocator::ifLeafExist(PPointer p)
###### 函数说明
判断持久化指针p指向的一个叶子是否存在
##### freeLeaf()
###### 函数定义
bool PAllocator::freeLeaf(PPointer p)
###### 函数说明
将持久化指针p指向的叶子置为空闲态，需要相应的修改catalog和Leafgroup中的内容
##### persistCatalog()
###### 函数定义
bool PAllocator::persistCatalog()
###### 函数说明
将catelog持久化
##### newLeafGroup()
###### 函数定义
bool PAllocator::newLeafGroup()
###### 函数说明
创建leafgroup，每个leafgroup一个文件
## 4. Recovery系统恢复
采取BulkLoading方式。
