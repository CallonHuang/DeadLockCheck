# DeadLockCheck
文件 | 作用
-|-
deadLockCheck.c | 核心的检测程序和算法，采用两个哈希实现请求锁和占有锁的信息数据存储，采用双哈希循环查找实现死锁环的检测 |
deadLockExternal.h | 外部包含的头文件，用于替换mutex lock相关的函数 |
deadLockInternal.h | 内部包含的头文件，核心的宏定义和数据结构定义 |
memCache.c | 为了优化速度同时减少malloc和free次数而设计的内存链表 |
memCache.h | 内存链表结构定义等 |
list.c | 双向链表，尾节点会循环回链表头 |
list.h | 链表的结点定义和函数声明 |
main.c | 测试demo，有两种类型的demo（双死锁和死锁后访问），通过CONFIG_DOUBLE_DEAD_LOCK来配置 |
build.sh | 懒于写Makefile而写的简单编译脚本，动态库的拷贝可换成设置LD_LIBRARY_PATH环境变量来实现 |
