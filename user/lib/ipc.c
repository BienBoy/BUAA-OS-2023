// User-level IPC library routines

#include <env.h>
#include <lib.h>
#include <mmu.h>

// Send val to whom.  This function keeps trying until
// it succeeds.  It should panic() on any error other than
// -E_IPC_NOT_RECV.
//
// Hint: use syscall_yield() to be CPU-friendly.
void ipc_send(u_int whom, u_int val, const void *srcva, u_int perm) {
	int r;
	while ((r = syscall_ipc_try_send(whom, val, srcva, perm)) == -E_IPC_NOT_RECV) {
		syscall_yield();
	}
	user_assert(r == 0);
}

// Receive a value.  Return the value and store the caller's envid
// in *whom.
//
// Hint: use env to discover the value and who sent it.
u_int ipc_recv(u_int *whom, void *dstva, u_int *perm) {
	int r = syscall_ipc_recv(dstva);
	if (r != 0) {
		user_panic("syscall_ipc_recv err: %d", r);
	}

	if (whom) {
		*whom = env->env_ipc_from;
	}

	if (perm) {
		*perm = env->env_ipc_perm;
	}

	return env->env_ipc_value;
}

int sem_init(const char *name, int init_value, int checkperm) {
	return syscall_sem_init(name, init_value, checkperm);
}
int sem_wait(int sem_id) {
	int value;
	if ((value = syscall_sem_getvalue(sem_id))<0)
		return value;
	while (!syscall_sem_add(sem_id, -1)) {
		syscall_yield();
	}
	return 0;
}
int sem_post(int sem_id) {
	int value;
	if ((value = syscall_sem_getvalue(sem_id))<0)
		return value;
	syscall_sem_add(sem_id, 1);
	return 0;
}
int sem_getvalue(int sem_id) {
	return syscall_sem_getvalue(sem_id);
}
int sem_getid(const char *name) {
	return syscall_sem_getid(name);
}
