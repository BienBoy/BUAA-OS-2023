// 信号处理相关函数

#include <lib.h>

// 信号的注册函数
int sigaction(int signum, const struct sigaction *act, struct sigaction *oldact) {
    debugf("开始为signal %d注册函数\n", signum);
    return syscall_sigaction(0, signum, act, oldact);
}

// 修改进程的信号掩码
int sigprocmask(int how, const sigset_t *set, sigset_t *oldset) {
    return syscall_sigprocmask(0, how, set, oldset);
}

// 清空信号集，将所有位都设置为 0
void sigemptyset(sigset_t *set) {
    set->sig[0] = 0;
    set->sig[1] = 0;
}

// 设置信号集，即将所有位都设置为 1
void sigfillset(sigset_t *set) {
    set->sig[0] = -1;
    set->sig[1] = -1;
}

// 向信号集中添加一个信号，即将指定信号的位设置为 1
void sigaddset(sigset_t *set, int signum) {
    set->sig[(signum - 1) / 32] |= (1 << (signum - 1) % 32);
}

// 检查一个信号是否在信号集中，如果在则返回 1，否则返回 0
void sigdelset(sigset_t *set, int signum) {
    set->sig[(signum - 1) / 32] &= ~(1 << ((signum - 1) % 32));
}

// 向信号集中添加一个信号，即将指定信号的位设置为 1
int sigismember(const sigset_t *set, int signum) {
    return !!(set->sig[(signum - 1) / 32] & (1 << ((signum - 1) % 32)));
}

// 发送信号
int kill(u_int envid, int sig) {
    return syscall_kill(envid, sig);
}