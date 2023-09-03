# Lab6实验报告

## 思考题

### Thinking 6.1

```C
#include <stdlib.h>
#include <unistd.h>

int fildes[2];
char buf[100];
int status;

int main() {

    status = pipe(fildes);

    if (status == -1) {
        printf("error\n");
    }

    switch (fork()) {
        case -1:
            break;
        case 0: 										/* 子进程- 作为管道的读者*/
            close(fildes[0]); 							/* 关闭不用的读端*/
            write(fildes[1], "Hello world\n", 12); 		/* 向管道中写数据*/
            close(fildes[1]); 							/* 写入结束，关闭写端*/
            exit(EXIT_SUCCESS);
        default: 										/* 父进程- 作为管道的写者*/
            close(fildes[1]); 							/* 关闭不用的写端*/
            read(fildes[0], buf, 100); 					/* 从管道中读数据*/
            printf("parent-process read:%s", buf); 		/* 打印读到的数据*/
            close(fildes[0]); 							/* 读取结束，关闭读端*/
            exit(EXIT_SUCCESS);
    }
}
```

### Thinking 6.2

dup 函数可以建立文件描述符 newfdnum 到文件描述符 oldfdnum 所对应的内容的映射，并共享文件描述符 oldfdnum 所对应的 data。

实现时，先建立的 newfdnum 到 oldfdnum 文件描述符的映射，后共享 data，由于不是原子操作，若两个操作间出现进程切换，可能出现预想之外的情况。

### Thinking 6.3

系统调用会陷入内核，此时会关闭中断，因此系统调用都是原子操作。

### Thinking 6.4

可以解决。在 pipe 未关闭时，pipe 的 pageref 大于 fd 的， 先对 fd 的 pageref 减 1，不会改变其大小关系。

在 dup 增加引用次数时，先增加 pipe 的引用次数，则可以保证 pipe 的 pageref 大于 fd 的。

### Thinking 6.5

spawn 函数、通过和文件系统交互，取得文件描述块，进而找到 ELF 在“硬盘”中的位置，进而读取。

在文件中， 因为 bss 段的变量的值都为 0，故只需记录实际大小，而不需实际占用相应空间，读入内存后，再分配实际大小的空间，并清 0。

### Thinking 6.6

在 user/init.c中。

### Thinking 6.7

外部命令。

Linux 的 cd 指令用于改变当前的工作目录，若是外部指令，则只能改变子 shell 的工作目录。

### Thinking 6.8

两次。ls.b 一个，cat.b 一个。

四次。

## 难点

\_pipe\_is\_closed 不是原子操作，为了保证执行结果的正确性，需要判断进程是否曾切换。

pipe\_close 函数也不是原子操作，需要正确调整取消映射的顺序，保证 \_pipe\_is\_closed 函数不会返回错误的结果。

## 感想

本次实验相比  Lab4、Lab5 内容较少，同时需要自己补充完整的部分也较为简单，提示很直接。但是要理清 pipe、shell 的实现还要查阅了解其他相关函数。实现管道与 Lab5 息息相关；实现加载并运行可执行文件涉及到 Lab4、Lab5 的相关内容，较为综合。

