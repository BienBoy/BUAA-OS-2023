#ifndef _SIGNAL_H
#define _SIGNAL_H

#include <queue.h>

#define SIGNAL_COUNT 64 // 信号的种类总数
#define MAX_BLOCKING_SIGNAL 64 // 允许阻塞的最大信号数量

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


struct Int {
	int val;
	TAILQ_ENTRY(Int) link;
};

TAILQ_HEAD(Int_queue, Int);

// 向进程发送信号
int send_signal(u_int envid, int sig);

#endif