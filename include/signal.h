#ifndef _SIGNAL_H
#define _SIGNAL_H

#define SIGNAL_COUNT 64 // 信号的种类总数

// 信号掩码的修改方式
#define SIG_BLOCK 0 // 阻塞
#define SIG_UNBLOCK 1   // 解除阻塞
#define SIG_SETMASK 2   // 将信号掩码设置为指定掩码

// 信号掩码
struct sigset_t{
    int sig[2]; //最多 32*2=64 种信号
};
// 信号处理结构体
struct sigaction{
    void (*sa_handler)(int);    // 信号处理函数入口
    sigset_t sa_mask;   // 运行信号处理函数过程中
};

#endif