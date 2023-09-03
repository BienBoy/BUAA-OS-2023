# Lab5实验报告

## 思考题

### Thinking 5.1

**读取**：对于设备的读取若通过 Cache 进行，在外设数据已被更新时，有可能读到缓存在 Cache 中的旧数据。

**写入**：对于设备的写入若通过 Cache 进行，根据采用的写回方法，可能在数据换出 Cache 时才写回外设。对于串口设备，会只显示最后写入的字符，中间写入的字符停留时间短而无法识别。

### Thinking 5.2

一个磁盘块中最多能存储 $\frac{4096B}{256B}=16$ 个文件控制块。

一个目录下最多能有 $1024\times16=16384$ 个文件。

单个文件最大为 $1024\times 4KB=4MB$

### Thinking 5.3

1GB

### Thinking 5.4

```
#define PTE_DIRTY 0x0002 // 脏位
#define DISKNO 1 // 超级块所在的磁盘号
#define BY2SECT 512 // 磁盘扇区大小
#define SECT2BLK (BY2BLK / BY2SECT) // 一个磁盘块对应的扇区数目
#define DISKMAP 0x10000000 // 块缓存中磁盘的起始虚拟地址
#define DISKMAX 0x40000000 // 允许的最大磁盘大小

#define BY2BLK BY2PG // 磁盘块大小（B为单位）
#define BIT2BLK (BY2BLK * 8) // 磁盘块大小（bit为单位）
#define MAXNAMELEN 128 // 文件名最大长度
#define MAXPATHLEN 1024 // 路径的最大长度
#define NDIRECT // 文件控制块中直接引用指针的个数
#define NINDIRECT (BY2BLK / 4) // 文件控制块中一级引用指针的个数
#define MAXFILESIZE (NINDIRECT * BY2BLK) // 文件的最大大小
#define BY2FILE 256 // 文件控制块大小
#define FILE2BLK (BY2BLK / sizeof(struct File)) // 一个磁盘块可容纳的文件控制块个数
#define FTYPE_REG 0 // 文件类型：一般文件
#define FTYPE_DIR 1 // 文件类型：目录
#define FS_MAGIC 0x68286097 // 超级块中的魔数
```

### Thinking 5.5

 fork 前后的父子进程会共享文件描述符和定位指针。

```
#include <lib.h>

int main() {
	int r;
	int fdnum;
	if ((r = open("/newmotd", O_RDWR)) < 0) {
		user_panic("open /newmotd: %d", r);
	}
	fdnum = r;
	seek(fdnum, 20020105);
	if (fork() == 0) {
		struct Fd* fd;
		if (fd_lookup(fdnum, &fd) < 0 || fd->fd_offset != 20020105) {
			debugf("不共享文件描述符和定位指针\n");
			return 0;
		}
		debugf("共享文件描述符和定位指针\n");s
	}
}
```

### Thinking 5.6

```C
struct File {
	char f_name[MAXNAMELEN]; // 文件名
	uint32_t f_size; // 文件大小
	uint32_t f_type; // 文件类型，有普通文件和目录两种
	uint32_t f_direct[NDIRECT]; // 文件的直接指针
	uint32_t f_indirect; // 文件的一级指针

	struct File *f_dir; // 指向文件所在目录的指针
	char f_pad[BY2FILE - MAXNAMELEN - (3 + NDIRECT) * 4 - sizeof(void *)]; // 补齐，使得struct File结构体占据256B空间
} __attribute__((aligned(4), packed));
struct Fd {
    u_int fd_dev_id; // 外设id，用于标识外设类型。
    u_int fd_offset; // 当前读写的偏移位置。
    u_int fd_omode; // 文件的打开模式，可实现只读、只写、读写等权限控制。
};

struct Filefd {
    struct Fd f_fd; // 文件描述符
    u_int f_fileid; // 文件的id
    struct File f_file; // 文件控制块
};
```

### Thinking 5.7

实线箭头表示发送消息，虚线表示返回消息。

文件系统进程处于接收消息的状态，普通用户进程要操作文件时，通过 ipc 发送请求，并转换为接收消息状态。文件系统进程收到消息后，执行相应操作，并通过 ipc 发送结果，然后再次回到接收消息的状态。普通用户进程收到返回结果，继续执行。

## 难点

### 1. 外部存储设备驱动

#### 1.1 内存映射I/O (MMIO)

通过读写设备上的寄存器可与外设进行数据通信，这些寄存器被映射到指定的物理地址空间。因此，通过直接读写 kseg1 段的指定地址处的数据，可以实现读写外设。

用户态进程若是直接读写内核虚拟地址将会由处理器引发一个地址错误。所以对于设备的读写必须通过系统调用来实现。

- int sys_write_dev(u_int va, u_int pa, u_int len)：将 [va, va+len) 间的数据写入指定外设物理地址 [pa, pa+len)。
	- 判断 va 是否有效：调用 is_illegal_va_range 
	- 判断 pa 是否有效：判断 [pa, pa+len) 是否在有效范围内。在实验中允许访问的地址范围为：
		- console：[0x10000000, 0x10000020)
		- disk：[0x13000000, 0x13004200)
		- rtc：[0x15000000,
			0x15000200)
	- 通过 memcpy(KSEG1 + pa, va, len) 写入数据。
- int sys_read_dev(u_int va, u_int pa, u_int len)：将外设物理地址 [pa, pa+len) 间数据读入 [va, va+len)。
	- 判断 va 是否有效：调用 is_illegal_va_range 
	- 判断 pa 是否有效：判断 [pa, pa+len) 是否在有效范围内。在实验中允许访问的地址范围为：
		- console：[0x10000000, 0x10000020)
		- disk：[0x13000000, 0x13004200)
		- rtc：[0x15000000,
			0x15000200)
	- 通过 memcpy(va, KSEG1 + pa, len) 读取数据。

#### 1.2 IDE 磁盘操作

IDE 磁盘的物理地址范围为：[0x13000000, 0x13004200)，功能表如下：

| 偏移          | 效果                                                         | 数据带宽 |
| :------------ | :----------------------------------------------------------- | :------- |
| 0x0000        | 写：设置下一次读写操作时的磁盘镜像偏移的字节数               | 4 字节   |
| 0x0008        | 写：设置高32 位的偏移的字节数                                | 4 字节   |
| 0x0010        | 写：设置下一次读写操作的磁盘编号                             | 4 字节   |
| 0x0020        | 写：开始一次读写操作（写0 表示读操作，1 表示写操作）         | 4 字节   |
| 0x0030        | 读：获取上一次操作的状态返回值（读 0 表示失败，非 0 则<br/>表示成功） | 4 字节   |
| 0x4000-0x41ff | 读/写：512 字节的读写缓存                                    |          |

读写 IDE 磁盘的函数如下：

- void ide_read(u_int diskno, u_int secno, void *dst, u_int nsecs)：读 IDE 磁盘。diskno 为磁盘号，secno 为起始扇区号，dst 为存储读出的数据的目标区域的起始地址，nsecs 为要读取的扇区数目。函数内遍历读取从 secno 至 secno + nsecs 的扇区 sec，过程如下：
	- 通过系统调用 syscall_write_dev 在 0x13000000 + 0x0010 写入操作的磁盘编号 diskno。
	- 通过系统调用 syscall_write_dev 在 0x13000000 + 0x0000 写入读取数据的位置相对起始位置的偏移 sec \* 512。
	- 通过系统调用 syscall_write_dev 在 0x13000000 + 0x0020 写入 0，开始读取。
	- 通过系统调用 syscall_read_dev 读取 0x13000000 + 0x0030 的值，判断读取是否成功。
	- 通过系统调用 syscall_read_dev 读取从 0x13000000 + 0x4000 开始的 512 字节数据到 dst + (sec - secno) \* 512。
- void ide_write(u_int diskno, u_int secno, void *src, u_int nsecs)：写 IDE 磁盘。diskno 为磁盘号，secno 为起始扇区号，src 为要写入的数据所在区域的起始地址，nsecs 为要写入的扇区数目。函数内遍历写入从 secno 至 secno + nsecs 的扇区 sec，过程如下：
	- 通过系统调用 syscall_write_dev 在 0x13000000 + 0x0010 写入操作的磁盘编号 diskno。
	- 通过系统调用 syscall_write_dev 在 0x13000000 + 0x0000 写入读取数据的位置相对起始位置的偏移 sec \* 512。
	- 通过系统调用 syscall_write_dev 在 0x13000000 + 0x4000 写入 src + (sec - secno) \* 512 起始的 512 字节的数据。
	- 通过系统调用 syscall_write_dev 在 0x13000000 + 0x0020 写入 1，开始写入。
	- 通过系统调用 syscall_read_dev 读取 0x13000000 + 0x0030 的值，判断写入是否成功。

### 2. 文件系统

#### 2.1 空闲磁盘块管理

MOS 的文件系统中通过位图法管理空闲磁盘块。

- void free_block(u_int blockno)：将指定磁盘块标记为空闲状态，主要实现为：
	- bitmap[blockno / 32] |= 1 << (blockno % 32)

#### 2.2 块缓存

块缓存指的是借助虚拟内存来实现磁盘块缓存的设计。

MOS 中将 DISKMAP 到
DISKMAP+DISKMAX 这一段虚存地址空间 (0x10000000-0x4fffffff) 作为缓冲区，对应硬盘。

- void *diskaddr(u_int blockno)：计算磁盘块 blockno 对应的虚拟地址。
	- 判断 blockno 是否有效。
	- 返回 blockno 对应的虚拟地址 DISKMAP + BY2BLK * blockno。
- void write_block(u_int blockno)：写回指定编号的磁盘块。
	- 检查 blockno 对应的磁盘块是否在内存中。
	- 调用 ide_write 写回磁盘。
- int read_block(u_int blockno, void **blk, u_int *isnew)：将指定编号的磁盘块读入到内存中。
	- 检查 blockno 是否有效，并检查对应的磁盘块是否空闲。
	- 调用 block_is_mapped 检查这块磁盘块是否已经在内存。
	- 如果磁盘块未读入内存，通过系统调用 syscall_mem_alloc 分配一页物理内存，然后调用 ide_read 来读取磁盘上的数据到对应的虚存地址 va 处。
	- 若 blk 不为空指针，将 va 赋值到 \*blk。
- int map_block(u_int blockno)：检查指定的磁盘块是否已经映射到内存，如果没有，
	分配一页内存来保存磁盘上的数据。
	- 调用 block_is_mapped 检查 blockno 对应的磁盘块是否已经映射到内存，已映射则直接返回。
	- 通过系统调用 syscall_mem_alloc 分配一页内存，用于映射指定的磁盘块。
- void unmap_block(u_int blockno)：解除磁盘块和物理内存之间的映射关系，回收内存。
	- 调用 block_is_mapped 检查 blockno 对应的磁盘块是否已经映射到内存，已映射则可获取到对应的虚拟地址 va；未映射则直接返回。
	- 如果 blockno 对应的磁盘块不是空闲磁盘块且被更改（is dirty），则调用 write_block 将其写回。
	- 通过系统调用 syscall_mem_unmap 解除 va 的映射。
- int file_get_block(struct File *f, u_int filebno, void **blk)：将某个指定的文件指向的磁盘块读入内存。
	- 调用 file_map_block 获取文件 f 对应的 第 filebno 个磁盘块所对应的编号 diskbno。
	- 调用 read_block 将磁盘块 diskbno 的数据读到 blk 指向的内存位置。
- int dir_lookup(struct File *dir, char *name, struct File **file)：查找某个目录下是否存在指定的文件。
	- 根据目录 dir 的大小计算其中的磁盘块的数目 nblock：nblock = ROUND(dir->f_size, BY2BLK) / BY2BLK。
	- 依次遍历目录中的磁盘块：
		- 调用 file_get_block 将磁盘块数据读入内存。
		- 遍历读取磁盘块内的文件控制块信息（struct File\*），判断是否为目标文件名 name。

### 3. 文件系统的用户接口

#### 3.1 文件描述符

文件描述符用来存储文件的基本信息和用户进程中关于文件的状态，文件描述符也起到描述用户对于文件操作的作用。

当用户进程向文件系统发送打开文件的请求时，文件系统进程会将这些基本信息记录在内存中，
然后由操作系统将用户进程请求的地址映射到同一个存储了文件描述符的物理页上，因此一个文件描述符至少需要独占一页的空间。

文件描述符涉及两个结构体：

```C
struct Fd {
    u_int fd_dev_id;
    u_int fd_offset;
    u_int fd_omode;
};

struct Filefd {
    struct Fd f_fd;
    u_int f_fileid;
    struct File f_file;
};
```

Filefd 结构体的第一个成员就是 Fd，因此指向 Filefd 的指针同样指向这个 Fd
的起始位置，故可以进行强制转换，将 struct Fd\* 指针转换为struct Filefd\* 指针。

#### 3.2 用户接口

常用接口如下：

- int open(const char *path, int mode)：打开文件或目录。接受文件路径 path 和模式 mode 作为输入参数，并返回文件描述符的编号。
	- 通过 fd_alloc 及 fsipc_open 分配一个文件描述符。
	- 通过 fd2data 函数获取文件描述符对应的数据缓存页地址，并从 Filefd 结构体中获取文件的 size 和 fileid 属性。
	- 遍历文件内容，使用 fsipc_map 函数为文件内容分配页面并映射。
	- 使用 fd2num 函数获取文件描述符的编号，并返回。

- int read(int fdnum, void *buf, u_int n)：写文件。接收一个文件描述符编号 fdnum ，一个缓冲区指针 buf 和一个最大读取字节数 n 作为输入参数，返回成功读取的字节数并更新文件描述符的偏移量。
	- 使用 fd_lookup 和 dev_lookup 函数获取文件描述符 fd 和设备结构体 dev。如果查找失败，则返回错误码。
	- 检查文件描述符 fd 的打开模式是否允许读取操作。
	- 调用设备的读取函数 dev_read 从文件当前的偏移位置读取数据到缓冲区中。
	- 如果读取操作成功，则函数更新文件描述符的偏移量 fd->fd_offset 并返回成功读取的字节数。
- int write(int fdnum, const void *buf, u_int n)：写文件。
	- 使用 fd_lookup 和 dev_lookup 函数获取文件描述符 fd 和设备结构体 dev。如果查找失败，则返回错误码。
	- 检查文件描述符 fd 的打开模式是否允许写入操作。
	- 调用设备的写入函数 dev_write 将缓冲区中数据写入到文件当前的偏移位置。
	- 如果写入操作成功，则函数更新文件描述符的偏移量 fd->fd_offset 并返回成功写入的字节数。

- int seek(int fdnum, u_int offset)：更改文件描述符的偏移量 fd->fd_offset 为指定值 offset 。
	- 使用 fd_lookup 函数获取文件描述符 fd。如果查找失败，则返回错误码。
	- 设置 fd->fd_offset 为 offset。

#### 3.3 文件系统服务

MOS 操作系统中的文件系统服务通过 IPC 的形式供其他进程调用，进行文件读写操作。

在内核开始运行时，就启动了文件系统服务进程 ENV_CREATE(fs_serv)，用户进程需要进行文件操作时，使用 ipc_send、ipc_recv 与fs_serv 进行交互，完成操作。

fs/serv.c 中
服务进程的主函数首先调用了 serve_init 函数准备好全局的文件打开记录表opentab，然后调用 fs_init 函数来初始化文件系统。fs_init 函数首先通过读取超级块的内容获知磁盘的基本信息，然后检查磁盘是否能够正常读写，最后调用 read_bitmap 函数检查磁盘块上的位图是否正确。执行完文件系统的初始化后，调用serve 函数，文件系统服务开始运行，等待其他程序的请求。

以文件的删除操作为例。

- int remove(const char *path)：删除文件。
	- 调用 fsipc_remove 发送删除文件的请求。
- int fsipc_remove(const char *path)：发送删除文件的请求。
	- 判断 path 长度是否合法。
	- 将 IPC 缓冲区 fsipcbuf 视为一个 Fsreq_remove 结构体，代表将要发送的是 remove 请求，并将路径字符串复制到结构体的路径字段中。
	- 调用 fsipc 函数将删除请求发送到 fs_serv 进程，并返回删除操作的结果。
- static int fsipc(u_int type, void *fsreq, void *dstva, u_int *perm)：向文件系统服务发送消息。
	- 通过 ipc_send 发送消息。
	- 通过 ipc_recv 接收结果并返回。

之后，文件系统服务收到消息，判断操作类型并执行操作，最终返回操作的结果。

## 感想

文件系统部分较复杂，且比较综合，涉及外设读写、文件系统的实现、文件系统的用户接口、IPC 通信等多方面内容，代码量很大，要完全理解，除了实验中需要自己实现的部份外，还需要阅读其他部分的代码。

在实验中，常常搞混 struct File、struct Fd、struct Filefd 三个结构体……需要区分好文件的存储与操作。
