#include <env.h>
#include <pmap.h>
#include <printk.h>
#include <trap.h>

extern void handle_int(void);
extern void handle_tlb(void);
extern void handle_sys(void);
extern void handle_mod(void);
extern void handle_reserved(void);
extern void handle_ov(void);

void (*exception_handlers[32])(void) = {
    [0 ... 31] = handle_reserved,
    [0] = handle_int,
    [2 ... 3] = handle_tlb,
#if !defined(LAB) || LAB >= 4
    [1] = handle_mod,
    [8] = handle_sys,
#endif
    [12] = handle_ov
};

/* Overview:
 *   The fallback handler when an unknown exception code is encountered.
 *   'genex.S' wraps this function in 'handle_reserved'.
 */
void do_reserved(struct Trapframe *tf) {
	print_tf(tf);
	panic("Unknown ExcCode %2d", (tf->cp0_cause >> 2) & 0x1f);
}

extern void env_pop_tf(struct Trapframe *tf, u_int asid) __attribute__((noreturn));
void do_ov(struct Trapframe *tf) {
	curenv->env_ov_cnt++;
	u_long va = tf->cp0_epc;
	u_long command = *(u_long*)va;
	u_long first = (command>>28) & 0xf;
	u_long second = command & 0x7ff;
	if (first) {
		int t = (command >> 16) & 0x1f;
		int s = (command >> 21) & 0x1f;
		u_long imm = command & 0xffff;
		tf->regs[t] = (u_long)tf->regs[t] / 2 + imm / 2;
		tf->cp0_epc = va + 4;
		printk("addi ov handled\n");
		env_pop_tf(tf, curenv->env_asid);
	}
	Pte *pt;                                                                   
        Pde* pgdir = &cur_pgdir[PDX(va)];                                                 
        pt = (Pte *)KADDR(PTE_ADDR(*pgdir));                                      
	u_long perm = pt[PTX(va)] & 0xfff;
	struct Page* p = page_lookup(curenv->env_pgdir, va, NULL);
	page_insert(curenv->env_pgdir, curenv->env_asid, p, va, PTE_D | perm);
	if (second == 0x20) {
		*(u_long*)va = (command & ~0x7ff) | 0x21;
		printk("add ov handled\n");
		env_pop_tf(tf, curenv->env_asid);
	}
	*(u_long*)va = (command & ~0x7ff) | 0x23;
	printk("sub ov handled\n");
	env_pop_tf(tf, curenv->env_asid);
}
