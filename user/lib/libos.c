#include <env.h>
#include <lib.h>
#include <mmu.h>

void exit(void) {
	// After fs is ready (lab5), all our open files should be closed before dying.
#if !defined(LAB) || LAB >= 5
	close_all();
#endif

	syscall_env_destroy(0);
	user_panic("unreachable code");
}

volatile struct Env *env;
extern int main(int, char **);

// 处理信号的函数
static void __attribute__((noreturn)) signal_entry(int signum, struct Trapframe *tf) {
    int r;
	void (*func)(int);

	func = env->env_sigactions[signum - 1].sa_handler;
	debugf("将要处理signal %d\n", signum);
	if (!func) {
		switch (signum) {
		case SIGKILL:
			debugf("收到SIGKILL信号，强制结束程序的运行\n");
			exit();
			break;
		case SIGSEGV:
			debugf("收到SIGSEGV信号，引用了无效内存，结束程序的运行\n");
			exit();
			break;
		case 15:
			debugf("收到SIGTERM信号，终止进程\n");
			exit();
			break;
		}
	} else {
		func(signum);
	}
    r = syscall_recover_after_signal(tf);
	user_panic("syscall_recover_after_signal returned %d", r);
}

void libmain(int argc, char **argv) {
	// set env to point at our env structure in envs[].
	env = &envs[ENVX(syscall_getenvid())];

	// 设置信号处理函数入口
	if (env->env_signal_entry != (u_int)signal_entry) {
		try(syscall_set_signal_entry(0, signal_entry));
	}

	// call user main routine
	main(argc, argv);

	// exit gracefully
	exit();
}
