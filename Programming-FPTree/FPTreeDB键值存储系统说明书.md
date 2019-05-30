# FPTreeDB键值存储系统说明书

## 1. 简介
本系统为基于针对NVM优化的数据结构FPTree的一个简单的键值存储引擎FPTreeDB。我们通过将其包装成一个调用库，供用户程序使用并管理其数据存储，与LevelDB的使用方式类似。其对外可用的对数据的基本操作即为增删改查；至于系统恢复，我们采取的是批量插入新叶的方式。

## 2. 系统架构
![系统架构](https://github.com/ZhangJiaQiao/2019-DBMS-Project/raw/master/asset/FPTreeDB.png "系统架构")

## 3. 使用说明
### 3.1 项目目录
```
|__gtest: 为Google Test项目目录，不用管  
|__include: 里包含所有用到的头文件  
   |__fptree: fptree的头文件所在文件夹  
      |__fptree.h: fptree地头文件  
   |__utility: fptree所用工具的头文件所在文件夹  
      |__utility.h: 指纹计算等工具函数所在头文件  
      |__clhash.h: 指纹计算所用哈希函数头文件  
      |__p_allocator.h: NVM内存分配器头文件  
|__src: 为项目源码所在地，完成里面所有的实现  
   |__bin: 可执行文件所在文件夹
      |__main: main.cpp的可执行文件
      |__lycsb: lycsb.cpp的可执行文件
      |__ycsb: ycsb.cpp的可执行文件
   |__fptree.cpp: fptree的源文件，项目核心文件(TODO)  
   |__clhash.c: 指纹计算的哈希函数源文件  
   |__p_allocator.cpp: NVM内存分配器源文件(TODO)  
   |__lycsb.cpp: LevelDB的YCSB测试代码(TODO)  
   |__ycsb.cpp: FPTreeDB和LevelDB的YCSB对比测试代码(TODO)  
   |__makefile: src下项目的编译文件  
|__workloads: 为YCSB测试负载文件，用于YCSB Benchmark测试  
   |__数据量-rw-读比例-写比例-load.txt: YCSB测试数据库装载文件  
   |__数据量-rw-读比例-写比例-run.txt: YCSB测试运行文件  
|__test: 为Google Test用户测试代码所在，请完成编译并通过所有测试  
   |__bin: 单元测试可执行文件所在文件夹
      |__fptree_test: fptree_test.cpp的可执行文件
      |__utility_test: utility_test.cpp的可执行文件
   |__fptree_test.cpp: fptree相关测试  
   |__utility_test.cpp: PAllocator等相关测试  
   |__makefile: gtest单元测试的编译文件 
```
### 3.2 非易失性内存NVM挂载
可以参考Intel的NVM模拟教程模拟NVM环境，在Ubuntu的配置下，可以跳过内核配置、编译和安装步骤，直接进行以下操作。  
（1）通过```dmesg | grep BIOS-e820```命令可以查看测试机的内存情况；  
（2）```sudo su```进入根模式后,通过命令```gedit /etc/default/grub```打开内存配置文件，修改内存配置，添加语句```memmap=XX!YY```表示从内存YY位置开始，划分XX大小的内存空间作为NVM；当然也可以添加多条该语句以划分多块连续的内存空间作为NVM，对此Intel的教程中已有详细实例，此处不再赘述；  
（3）执行命令```update-grub```,更新成功后，重启测试机；  
（4）重启后，可以通过命令```dmesg | grep user```查看分配后的内存情况；  
（5）若配置成功，在```/dev```目录下会生成```pmem0```设备，可以通过命令```sudo fdisk -l /dev/pmem0```确认系统能够识别设备并查看设备详情；  
（6）通过执行以下命令
> mkdir /mnt/pmemdir  
  mkfs.ext4 /dev/pmem0  
  mount -o dax /dev/pmem0 /mnt/pmemdir  

便将前面分配作为NVM的内存块挂载到了目录mnt/pmemdir下；  
（7）在```/dev```目录下可以通过命令```lsblk```查看挂载情况。
### 3.3 编译安装PMDK库
我们这里推荐[在Linux下用源码编译安装PMDK库](https://docs.pmem.io/getting-started-guide/installing-pmdk/compiling-pmdk-from-source#compile)，详细教程官方已给出，此处不再赘述。  
本项目中使用到的是PMDK中的libpmem库，项目代码中使用到的该库中的函数有：  
```
1. pmem_map_file：打开并映射文件
2. pmem_persist：持久化对NVM内容的修改
```
### 3.4 YCSB测试
这是一个键值数据库性能测试的benchmark，细节请看其[github仓库](https://github.com/brianfrankcooper/YCSB)。  
YCSB大体上分两个步，第一步是读取load文件，插入一定量的数据对数据库进行初始化。第二步是读取run文件，进行数据库相关操作。load和run文件的一条操作语句如下:
> INSERT 6284781860667377211

上面INSERT表示插入操作，后面是键值。因为FPTreeDB键值对为8bytes-8bytes，所以只取这个值的前8字节。  
#### 3.4.1 编译安装LevelDB并生成动态链接库
下载并编译LevelDB库
> git clone https://github.com/google/leveldb.git  
  cmake .  
  make

编译成功后，在当前目录下生成了一个静态库```libleveldb.a```, 我们把这个静态库复制到```/usr/local/lib/```, 并把leveldb相关的头文件复制到```/usr/local/include/```
> sudo cp libleveldb.a /usr/local/lib/  
  sudo cp -r include/leveldb/ /usr/local/include/

#### 3.4.2 运行LevelDB的YCSB测试
在```/src```目录下，先通过命令```mkdir bin```创建一个可执行文件夹后，再执行```make all```命令  
编译成功后，
> cd bin
  ./lycsb

注意：由于lycsb.cpp中采用了相对路径对测试文件进行寻址，所以必须在bin目录下执行命令```./lycsb```才能正确打开测试文件。另外此测试并没有使用NVM。  
测试结果：  
1）10w-rw-0-100测试结果
![10w-rw-0-100测试结果图](https://github.com/Bedmote/DB-Go-Go-Go/raw/master/10w-rw-0-100.png "10w-rw-0-100测试结果图")  
2）10w-rw-100-0测试结果
![10w-rw-100-0测试结果](https://github.com/Bedmote/DB-Go-Go-Go/raw/master/10w-rw-100-0.png "10w-rw-100-0测试结果")  
3）10w-rw-25-75测试结果
![10w-rw-25-75测试结果](https://github.com/Bedmote/DB-Go-Go-Go/raw/master/10w-rw-25-75.png "10w-rw-25-75测试结果")  
4）10w-rw-50-50测试结果
![10w-rw-50-50测试结果](https://github.com/Bedmote/DB-Go-Go-Go/raw/master/10w-rw-50-50.png "10w-rw-50-50测试结果")  
5）10w-rw-75-25测试结果
![10w-rw-75-25测试结果](https://github.com/Bedmote/DB-Go-Go-Go/raw/master/10w-rw-75-25.png "10w-rw-75-25测试结果")  
6）1w-rw-50-50测试结果
![1w-rw-50-50测试结果](https://github.com/Bedmote/DB-Go-Go-Go/raw/master/1w-rw-50-50.png "1w-rw-50-50测试结果")  
7）220w-rw-50-50测试结果
![220w-rw-50-50测试结果](https://github.com/Bedmote/DB-Go-Go-Go/raw/master/220w-rw-50-50.png "220w-rw-50-50测试结果")
#### 3.4.3 运行FPTreeDB和LevelDB的YCSB对比测试
在```/src```目录下，如果```bin```目录不存在，需要先通过命令```mkdir bin```创建一个可执行文件夹后。如果源文件还未编译，则再执行```make all```命令  
编译成功后，
> cd bin
  sudo ./ycsb

注意：由于ycsb.cpp中采用了相对路径对测试文件进行寻址，所以必须在bin目录下执行命令```sudo ./ycsb```才能正确打开测试文件。另外此测试由于使用的NVM挂载在根目录下，所以运行```ycsb```文件时需要在根模式下。除此之外，由于自行分配的NVM的空间可能较小，这时需要相应的修改```ycsb.cpp```代码中的```READ_WRITE_NUM```变量，否则```READ_WRITE_NUM```相对于NVM空间过大时，会出现段错误。  
测试结果（这里由于NVM内存有限，以下的测试结果为```READ_WRITE_NUM=100```情况下）：  
![YCSB对比测试结果图](https://github.com/Bedmote/DB-Go-Go-Go/raw/master/YCSB%E5%AF%B9%E6%AF%94%E6%B5%8B%E8%AF%95.png "YCSB对比测试结果图")
#### 3.4.4 清除数据文件说明
1）如果在运行```ycsb```测试时发生了段错误，需要清除NVM中的数据文件后再重新运行，可以在```/src```目录下，执行命令```sudo make cleand```。若运行成功需要再次测试则无需执行命令删除文件，因为这在```ycsb.cpp```源代码中已做处理。  
2）如果运行```lycsb```测试成功后需要再次运行该测试，需要先清除数据目录```leveldb```，可以在```/src```目录下，执行命令```sudo make cleand```，也可以直接在```bin```目录下执行```rm -r -f leveldb```命令。  
### 3.5 Google Test单元测试
在```/test```目录下，创建可执行文件夹```mkdir bin```，然后执行```make```进行编译。  
注意要先创建文件bin后再执行make，否则编译不能成功。
#### 3.5.1 utility_test
编译成功后，进入根模式，输入命令```./bin/utility_test```，测试结果正确。  
![通过utility_test测试](https://github.com/Bedmote/DB-Go-Go-Go/raw/master/%E9%80%9A%E8%BF%87utility_test%E6%B5%8B%E8%AF%95.png "通过utility_test测试")
#### 3.5.2 fptree_test
编译成功后，进入根模式，输入命令```./bin/fptree_test```，测试结果正确。
![通过fptree_test测试](https://github.com/Bedmote/DB-Go-Go-Go/raw/master/%E9%80%9A%E8%BF%87fptree_test%E6%B5%8B%E8%AF%95.png "通过fptree_test测试")
