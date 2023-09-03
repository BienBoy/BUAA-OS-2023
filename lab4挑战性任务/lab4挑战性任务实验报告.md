# Lab4挑战性任务实验报告

## 1. 实现思路

### 1.1 相关数据结构

由于每个进程有自己的信号掩码，可以为信号注册处理函数，因此，将信号相关数据结构放在 env 中，在 env 中添加如下字段：

```C
struct sigaction env_sigactions[SIGNAL_COUNT];	// 每个信号的注册函数指针
sigset_t env_signal_mask;	// 进程的信号掩码，采用位图法，表示需要被阻塞的信号
u_int env_signal;	// 当前在处理的信号，信号编号从1开始
struct Int_queue env_pending_signals;	// 等待处理的信号队列
struct Int_queue env_blocking_signals;	// 被阻塞的信号队列
u_int env_signal_entry;	// 处理信号的函数入口，类似cow_entry，函数内为每个信号调用其注册函数，没有则进行默认操作。
```

与信号相关的类型定义、常量放在头文件 signal.h 中：

```C
#ifndef _SIGNAL_H
#define _SIGNAL_H

#include <queue.h>

#define SIGNAL_COUNT 64 // 信号的种类总数

// 一些信号
#define SIGKILL 9
#define SIGSEGV 11
#define SIGTERM 15

// 信号掩码的修改方式
#define SIG_BLOCK 0 // 阻塞
#define SIG_UNBLOCK 1   // 解除阻塞
#define SIG_SETMASK 2   // 将信号掩码设置为指定掩码

// 信号掩码
typedef struct sigset_t{
    int sig[2]; //最多 32*2=64 种信号
}sigset_t;
// 信号处理结构体
struct sigaction{
    void (*sa_handler)(int);    // 信号处理函数入口
    struct sigset_t sa_mask;   // 运行信号处理函数过程中
};

// 用于实现整数队列的数据结构
struct Int {
	int val;
	TAILQ_ENTRY(Int) link;
};

TAILQ_HEAD(Int_queue, Int);

// 向进程发送信号
int send_signal(u_int envid, int sig);

#endif
```

### 1.2 初始化

信号相关数据结构放在 env 后，需要考虑初始化的问题。

首先，需要在进程创建时初始化，我选择在 env_alloc 中初始化：

```C
// 初始化所有信号的注册函数
memset(e->env_sigactions, 0, sizeof(e->env_sigactions));
// 初始化进程的信号掩码
memset(&(e->env_signal_mask), 0, sizeof(e->env_signal_mask));
// 无正在处理的信号
e->env_signal = 0;
// 初始化等待处理的信号队列
TAILQ_INIT(&(e->env_pending_signals));
// 初始化阻塞信号队列
TAILQ_INIT(&(e->env_blocking_signals));
// 初始化处理信号的函数入口
e->env_signal_entry = NULL;
```

其次，还要考虑 fork 时的继承问题：

```C
// 继承父进程的信号注册函数
memset(e->env_sigactions, curenv->env_sigactions, sizeof(curenv->env_sigactions));
// 继承父进程的信号处理函数入口
e->env_signal_entry = curenv->env_signal_entry;
```

### 1.3 编写系统调用

完成了以上准备工作，需要编写系统调用实现一些接口。

需要用到系统调用的函数有：

```C
// 为信号注册一个处理函数
int sigaction(int signum, const struct sigaction *act, struct sigaction *oldact);
// 修改进程的信号掩码
int sigprocmask(int how, const sigset_t *set, sigset_t *oldset);
// 向进程发送信号
int kill(u_int envid, int sig);
```

此外，为进程设置处理信号的函数入口也需要系统调用。

用到的系统调用有：

```C
// 为进程设置处理信号的函数入口
int syscall_set_signal_entry(u_int envid, void (*func)(int signum, struct Trapframe *tf));
// 为信号注册一个处理函数
int syscall_sigaction(u_int envid, int signum, const struct sigaction *act, struct sigaction *oldact);
// 修改进程的信号掩码
int syscall_sigprocmask(u_int envid, int how, const sigset_t *set, sigset_t *oldset);
// 向进程发送信号
int syscall_kill(u_int envid, int sig);
```

### 1.4 实现相关函数

借助于之前编写的系统调用，完成相关处理函数。

此外，有了信号量机制后，passive_alloc 函数中，对于访问低地址的 panic 操作可以替换为向当前进程发送 SIGSEGV 信号。

### 1.5 何时检查进程是否收到信号

我选择在内核态恢复到用户态时检查当前进程是否收到信号。为此需要修改 ret_from_exception 函数，添加跳转到相关函数的代码。

```C
move    a0, sp
addiu   sp, sp, -8
jal     do_signal  /* 判断有无信号，有则处理，无则返回 */
addiu   sp, sp, 8
```

### 1.6 检查进程是否收到信号

do_signal 是一个检查进程是否收到信号的函数。它先遍历阻塞信号队列，判断有无信号取消屏蔽，将其加入未决信号队列。

若未决信号队列不为空，代表当前进程收到了信号，需要处理。为此需要保存当前进程的现场至**用户异常栈**，然后设置用户现场，将 cpu0 中的 epc 值设置为处理信号的函数入口地址，使其返回用户态后，可以进行信号处理。

保存及设置现场的操作参考了 do_tlb_mod 函数：

```C
// 保存Trapframe
struct Trapframe tmp_tf = *tf;
if (tf->regs[29] < USTACKTOP || tf->regs[29] >= UXSTACKTOP) {
	tf->regs[29] = UXSTACKTOP;
}
tf->regs[29] -= sizeof(struct Trapframe);
*(struct Trapframe *)tf->regs[29] = tmp_tf;

if (curenv->env_signal_entry) {
	// 第一个参数为信号编号
	tf->regs[4] = i->val;、
	// 第二个参数为指向保存的现场的指针，用于将来恢复现场
	tf->regs[5] = tf->regs[29];
	tf->regs[29] -= sizeof(tf->regs[4]) * 2;
	// 设置epc值设置为处理信号的函数入口地址
	tf->cp0_epc = curenv->env_signal_entry;
} else {
	panic("Get signal but no signal entry registered");
}
```

### 1.7 处理信号

获取当前处理的信号，为其调用注册的处理函数，没有则进行默认处理。

### 1.8 恢复现场

处理完成后，需要恢复现场，可调用 syscall_set_trapframe 实现。

由于恢复现场前需要将 curenv->env_signal 清空，所以对其进行了一层封装。

```C
int sys_recover_after_signal(struct Trapframe *tf) {
	curenv->env_signal = 0;
	return sys_set_trapframe(0, tf);
}
```

### 1.9 注册处理信号的函数入口地址

完成了以上内容，还剩下最后一件事：注册处理信号的函数入口地址。

我选择在 libmain 函数中为进程注册处理信号的函数入口地址。

## 2. 测试程序

### 1. 基本信号测试

测试程序：

```C
#include <lib.h>

int global = 0;
void handler(int num) {
    debugf("Reach handler, now the signum is %d!\n", num);
    global = 1;
}

#define TEST_NUM 2
int main(int argc, char **argv) {
    sigset_t set;
    sigemptyset(&set);
    struct sigaction sig;
    sig.sa_handler = handler;
    sig.sa_mask = set;
    panic_on(sigaction(TEST_NUM, &sig, NULL));
    sigaddset(&set, TEST_NUM);
    panic_on(sigprocmask(0, &set, NULL));
    kill(0, TEST_NUM);
    int ans = 0;
    for (int i = 0; i < 10000000; i++) {
        ans += i;
    }
    panic_on(sigprocmask(1, &set, NULL));
    debugf("global = %d.\n", global);
    return 0;
}
```

正确输出：

```
Reach handler, now the signum is 2!
global = 1.
```

### 2. 空指针测试

测试程序：

```C
#include <lib.h>

int a[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
int *test = NULL;
void sgv_handler(int num) {
    debugf("Segment fault appear!\n");
    test = &a[0];
    debugf("test = %d.\n", *test);
    exit();
}

int main(int argc, char **argv) {
    sigset_t set;
    sigemptyset(&set);
    struct sigaction sig;
    sig.sa_handler = sgv_handler;
    sig.sa_mask = set;
    panic_on(sigaction(11, &sig, NULL));
    *test = 10;
    debugf("test = %d.\n", *test);
    return 0;
}
```

正确输出：

```
Segment fault appear!
test = 1.
```

### 3. 写时复制

测试程序：

```C
#include <lib.h>

sigset_t set2;

int main(int argc, char **argv) {
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, 1);
    sigaddset(&set, 2);
    panic_on(sigprocmask(0, &set, NULL));
    sigdelset(&set, 2);
    int ret = fork();
    if (ret != 0) {
        panic_on(sigprocmask(0, &set2, &set));
        printf("Father: %d.\n", sigismember(&set, 2));
    } else {
        printf("Child: %d.\n", sigismember(&set, 2));
    }
    return 0;
}
```

正确输出：

```
Father: 1.
Child: 0.
```

### 4. 信号屏蔽

测试程序：

```C
// 信号阻塞和解除阻塞测试
#include <lib.h>

void handler(int num) {
    debugf("Reach handler, now the signum is %d!\n", num);
}

int main(int argc, char **argv) {
    sigset_t set;
    sigemptyset(&set);
    struct sigaction sig;
    sig.sa_handler = handler;
    sig.sa_mask = set;
    panic_on(sigaction(3, &sig, NULL));
    panic_on(sigaction(4, &sig, NULL));
    
    // 阻塞信号3、4
    sigaddset(&set, 3);
    sigaddset(&set, 4);
    panic_on(sigprocmask(SIG_BLOCK, &set, NULL));

    // 发送信号3，但信号被屏蔽，处理函数不会被执行
    kill(0, 3);
    kill(0, 4);
    
    debugf("Signal 3 and signal 4 are blocked and not currently being processed.\n");
    // 解除对信号3、4的阻塞
    debugf("Unblock signal 3 and signal 4 and handle them accordingly.\n");
    panic_on(sigprocmask(SIG_UNBLOCK, &set, NULL));
    
    return 0;
}
```

正确输出：

```
Signal 3 and signal 4 are blocked and not currently being processed.
Unblock signal 3 and signal 4 and handle them accordingly.
Reach handler, now the signum is 3!
Reach handler, now the signum is 4!
```

### 5. 信号集操作

测试程序：

```C
// 信号集操作测试
#include <lib.h>
#define assert(x)                                                                                  \
	do {                                                                                       \
		if (!(x)) {                                                                        \
			user_panic("assertion failed: %s", #x);                                         \
		}                                                                                  \
	} while (0)

int main(int argc, char **argv) {
    sigset_t set1, set2, set3;
    sigemptyset(&set1);
    sigemptyset(&set2);
    sigemptyset(&set3);
    
    // 向set1添加信号1和信号2
    sigaddset(&set1, 1);
    sigaddset(&set1, 2);
    
    // 将set1复制给set2
    set2 = set1;
    
    // 从set2删除信号1
    sigdelset(&set2, 1);
    
    // 检查信号1和信号2是否在set1和set2中
    int is_member1 = sigismember(&set1, 1);
    assert(is_member1);
    int is_member2 = sigismember(&set1, 2);
    assert(is_member2);
    int is_member3 = sigismember(&set2, 1);
    assert(!is_member3);
    int is_member4 = sigismember(&set2, 2);
    assert(is_member4);

    
    // 清空set1
    sigemptyset(&set1);
    
    // 检查信号1和信号2是否在set1中
    int is_member5 = sigismember(&set1, 1);
    assert(!is_member5);
    int is_member6 = sigismember(&set1, 2);
    assert(!is_member6);
    
    // 设置信号集set3全为1
    sigfillset(&set3);
    // 检查信号1和信号2是否在set3中
    int is_member7 = sigismember(&set3, 1);
    assert(is_member7);
    int is_member8 = sigismember(&set3, 2);
    assert(is_member8);

    debugf("Pass signal set operations test.\n");
    return 0;
}
```

正确输出：

```
Pass signal set operations test.
```

### 6. 多进程信号处理

```C
// 多进程信号处理测试
#include <lib.h>

void handler(int num) {
    debugf("Reach handler in process %d, now the signum is %d!\n", syscall_getenvid(), num);
}

int main(int argc, char **argv) {
    sigset_t set;
    sigemptyset(&set);
    struct sigaction sig;
    sig.sa_handler = handler;
    sig.sa_mask = set;
    panic_on(sigaction(4, &sig, NULL));
    
    int ret = fork();
    if (ret != 0) {
        // 父进程
        debugf("Parent process (envid: %d)\n", syscall_getenvid());
        kill(ret, 4); // 向子进程发送信号4
        debugf("Parent process finishes\n");
    } else {
        // 子进程
        syscall_yield();
        debugf("Child process (envid: %d)\n", syscall_getenvid());
        debugf("Child process finishes\n");
    }
    
    return 0;
}
```

正确输出：

```
Parent process (envid: 2048)
Parent process finishes
[00000800] destroying 00000800
[00000800] free env 00000800
i am killed ... 
Reach handler in process 4097, now the signum is 4!
Child process (envid: 4097)
Child process finishes
```

### 7. 信号重入

测试程序：

```C
// 信号处理函数中的信号重入测试
#include <lib.h>

volatile int count = 0;

void handler(int num) {
    debugf("Reach handler, now the signum is %d!\n", num);
    count++;
    if (count < 3) {
        // 发送信号3，触发信号处理函数重入
        kill(0, 3);
    }
    debugf("Handler ends with count: %d\n", count);
}

int main(int argc, char **argv) {
    sigset_t set;
    sigemptyset(&set);
    struct sigaction sig;
    sig.sa_handler = handler;
    sig.sa_mask = set;
    panic_on(sigaction(3, &sig, NULL));
    
    // 发送信号3，触发信号处理函数
    kill(0, 3);
    
    debugf("Count: %d\n", count);
    
    return 0;
}
```

正确输出：

```
Reach handler, now the signum is 3!
Reach handler, now the signum is 3!
Reach handler, now the signum is 3!
Handler ends with count: 3
Handler ends with count: 3
Handler ends with count: 3
Count: 3
```

### 8. 信号默认操作

测试程序：

```C
// 信号的默认操作
#include <lib.h>

int main(int argc, char **argv) {
    // 发送未注册的信号，该信号的默认处理动作是忽略
    kill(0, 2);

    debugf("Do nothing now.\n");
    
    // 发送SIGTERM信号，该信号的默认处理动作是结束程序的运行
    kill(0, 15);
    
    debugf("You can't reach here!\n");

    return 0;
}
```

正确输出：

```
Do nothing now.
收到SIGTERM信号，终止进程
```

没有You can't reach here!即可

## 3. 遇到的问题及相应的解决方案

### 保存进程收到的信号的问题

信号可能被屏蔽，参考 Linux 的信号处理机制，设置两个队列——阻塞信号队列及未决信号队列。

### 写时复制问题

sigaction 和 sigprocmask 涉及到在内核态下向用户地址空间写入数据的问题，如果用户地址空间为 COW 权限，会出错。

解决方法是系统调用之前，先使用 memset 函数写一次目标地址，提前解决写时复制问题。

### 什么时候检查是否有信号

参照 Linux 的检查时机，选择在每次从内核态恢复至用户态时检查。

### 怎么跳转到信号处理函数

参考了 do_tlb_mod 函数实现跳转：保存当前进程的现场至用户异常栈，然后设置用户现场，将 epc 值设置为处理信号的函数入口地址，使其返回用户态后，可以进行信号处理。

### 怎么恢复现场

调用处理写时复制后的恢复现场的系统调用 syscall_set_trapframe 可恢复现场。

### 信号重入出现了问题

在测试时，在某些情况下信号重入失败，经过测试发现 ret_from_exception 中传入 do_signal 的现场中的 epc 可能存放内核态的地址，没找到具体的原因，但进行一下特判，判断为用户地址空间地址后在进行后续操作解决了问题。