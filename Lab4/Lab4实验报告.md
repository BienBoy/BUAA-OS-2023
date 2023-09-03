# Lab4实验报告

## 思考题

### Thinking 4.1

1. 陷入内核态后，调用了 SAVE_ALL 宏将用户进程的上下文运行环境保存在内核栈中。系统调用结束，返回用户态前调用RESTORE_SOME 将所有通用寄存器的值恢复。
2. 可以
3. 在do_syscall函数中根据\$a0至\$a3及栈指针获取到用户传入的参数，然后在调用sys开头的函数时传入这5个参数。
4. 将epc寄存器内地址加4，使得返回用户态后从syscall的下一条指令执行

### Thinking 4.2

不同的envid可能对应同一个env，但只有满足e->env_id == envid的envid是有效的。

### Thinking 4.3

在函数envid2env中，若envid为0，则转换为当前进程，若存在envid为0的进程，则其无法通过envid2env获取到。

而IPC若要发送信息到envid为0的进程，其最终结果是自己向自己发送信息，无法达到目的。

### Thinking 4.4

C

### Thinking 4.5

应该映射USTACKTOP以下的页面。USTACKTOP为无效内存及异常栈，每个进程间是独立的，所以不能映射。

### Thinking 4.6

```C
#define vpt ((volatile Pte *)UVPT)
#define vpd ((volatile Pde *)(UVPT + (PDX(UVPT) << PGSHIFT)))
```

vpt为用户进程页表起始地址，vpd为用户进程页目录起始地址。

vpt\[i\]表示第i个页表内存储的值，vpd\[i>>10\]为第i个页表对应的页目录项中的值。

vpd是根据自映射关系计算得到的，页表起始地址为第PDX(UVPT)个页表，故页目录的起始地址的相对页表起始地址的偏移为PDX(UVPT)<<PGSHIFT

进程不能根据vpt和vpd修改页表。用户态不能进行页表的修改，需要依靠系统调用实现。

### Thinking 4.7

在用户发生写时复制 COW 引发的缺页中断并进行处理时，可能会再次发生缺页中断，从而出现“中断重入”。

MOS中，对缺页错误的处理在用户态完成，用户进程在处理错误及恢复现场时需要使用 Trapframe。

### Thinking 4.8

减轻内核的工作量，且出错不会导致内核崩溃，提高了操作系统的可靠性。

### Thinking 4.9

在调用 syscall_env_alloc 的过程中也可能需要进行异常处理，在调用 fork 时，有可能当前进程已是之前进程的子进程，从而需要考虑是否会发生写时复制的缺页中断异常处理，如果这时还没有调用过 syscall_set_tlb_mod_entry 则无法处理异常。

触发缺页中断时因中断处理未设置好导致无法进行正常处理，从而导致内核崩溃。

## 难点

### 1. 系统调用

#### 1.1 系统调用流程

1. 用户程序调用syscall_\*函数接口
2. syscall_\*内调用msyscall函数
3. msyscall函数内调用syscall指令陷入内核态
4. 内核使用 SAVE_ALL 宏将用户进程的上下文运行环境保存在内核栈中，取出 CP0_CAUSE 寄存器中的异常码，系统调用对应的异常码为 8 ，以异常码作为索引在 exception_handlers 数组中找到对应的异常处理函数 handle_sys ，转跳至 handle_sys 函数处理用户的系统调用请求。
5. handle_sys跳转到do_syscall函数
6. do_syscall调用相应的系统调用函数，具体流程如下：
	- 判断系统调用号是否可用，不可用直接返回
	- 将epc寄存器内地址加4，即返回用户态后从syscall的下一条指令执行
	- 根据系统调用号在syscall_table获取到相应的系统调用函数
	- 从 Trapframe 结构体中获取到用户传入的参数
	- 调用相应的系统调用函数并将返回值存入$v0
7. 返回到handle_sys函数，调用ret_from_exception从内核态返回用户程序

#### 1.2 基础系统调用函数

##### 1.2.1 syscall_mem_alloc

syscall_mem_alloc对应的内核态主要作用函数为：sys_mem_alloc，用于给该程序所允许的虚拟内存空间显式地分配实际的物理内存，即编写的程序在内存中申请了一片空间。

对于操作系统内核来说，这一系统调用意味着一个进程请求将其运行空间中的某段地址与实际物理内存进行映射，从而可以通过该虚拟页面来对物理内存进行存取访问。

内核将通过传入的进程标识符参数(envid) 来确定发出请求的进程，因此需要一个函数envid2env完成envid到env的转换。

- int envid2env(u_int envid, struct Env **penv, int checkperm)：
	-  如果envid为 0,则将当前进程控制块的地址写入到penv指向的地址
	-  如果checkperm为1，则env需要满足：env是当前进程，即env == curenv或env是当前进程的子进程

- int sys_mem_alloc(u_int envid, u_int va, u_int perm)：申请一个物理页，将va映射过去，并设置权限为perm
	- 判断va是否有效
	- 使用envid2env获取对应的ebv
	- 使用page_alloc申请一个物理页
	- 使用page_insert将va映射到申请的物理页

##### 1.2.2 syscall_mem_map

该系统调用将源进程地址空间中的相应内存映射到目标进程的相应地址空间的相应虚拟内存中去，即两者共享一页物理内存。

- int sys_mem_map(u_int srcid, u_int srcva, u_int dstid, u_int dstva, u_int perm)：
	- 判断srcva及dstva的有效性
	- 将srcid及dstid转换为对应的env
	- 使用page_lookup获取到srcva映射到的物理页
	- 使用page_insert将dstva以权限perm映射到上一步获取到物理页

##### 1.2.3 syscall_mem_upmap

该系统调用可以解除某个进程地址空间虚拟内存和物理内存之间的映射关系。

- int sys_mem_unmap(u_int envid, u_int va)：
	- 判断va的有效性
	- 将envid转换为对应的env
	- 使用page_remove一处vad映射关系

##### 1.2.4 syscall_yield

该系统调用放弃占用CPU，调度其他进程

- void __attribute__((noreturn)) sys_yield(void)：
	- 调用schedule(1)完成进程调度

### 2. 进程间通信机制(IPC)

基本原理为：发送方进程可以将数据以系统调用的形式存放在进程控制块中，接收方进程同样以系统调用的方式在进程控制块中找到对应的数据，读取并返回。

#### 2.1 相关数据结构

需要在Env中添加字段以支持IPC

```C
struct Env {
    .......
    u_int env_ipc_value; // 收到的具体数值
    u_int env_ipc_from; // 发送方的进程ID
    u_int env_ipc_recving; // 1：等待接受数据中；0：不可接受数据
    u_int env_ipc_dstva; // 接收到的页面需要与自身的哪个虚拟页面完成映射
    u_int env_ipc_perm; // 传递的页面的权限位设置
    .......
}
```

#### 2.2 发送消息

发送消息的系统调用为syscall_ipc_try_send，对应的内核态主要作用函数为：

- int sys_ipc_try_send(u_int envid, u_int value, u_int srcva, u_int perm)：发送value到指定进程，若srcva不为零则同时发送页面。
	- 若有srcva，则判断其有效性
	- 将envid转换为env，若目标进程未处于接收状态，即env_ipc_recving==0，则直接返回
	- 设置目标进程env_ipc_value为value，env_ipc_from为curenv->env_id，env_ipc_perm为perm|PTE_V，env_ipc_recving为0
	- 修改目标进程的进程状态为ENV_RUNNABLE，使接受数据的进程可继续运行，并将其插入调度链表env_sched_list
	- 若srcva不为0，则调用page_insert将目标进程env_ipc_dstva对应的虚拟映射到srcva对应的物理页完成共享内存

#### 2.3 接收消息

接收消息的系统调用为syscall_ipc_recv，对应的内核态主要作用函数为：

- int sys_ipc_recv(u_int dstva)：
	- 若dstva不为0，则判断其有效性
	- 将当前进程env_ipc_recving设置为1，表明该进程准备接受发送方的消息
	- 设置env_ipc_dstva为dstva
	- 阻塞当前进程，即把当前进程的状态置为不可运行（ENV_NOT_RUNNABLE），并将其从调度链表env_sched_list中移除
	- 通过schedule(1)进行进程调度

### 3. Fork

#### 3.1 Fork流程

##### 3.1.1 父进程

- 调用 fork 函数
- 调用 syscall_set_tlb_mod_entry 设置当前进程 TLB Mod 异常处理函数 
- 调用 syscall_exofork 函数为子进程分配进程控制块 
- 调用duppage函数将当前进程低于 USTACKTOP 的有效虚拟页映射到子进程虚拟地址空间
- 调用 syscall_set_tlb_mod_entry 设置子进程 TLB Mod 异常处理函数 
- 调用 syscall_set_env_status 进程设置子进程状态为ENV_RUNNABLE并将其加入了 env_sched_list

##### 3.1.2 子进程

- 子进程被 env_run 函数启动
- 子进程中的 syscall_exofork 返回 0 到 fork 函数中
- 子进程中的 fork 函数设置好自己的 env 变量，然后返回 0

#### 3.2 创建子进程

主要系统调用处理为：sys_exofork

sys_exofork：

- 通过env_alloc函数申请新的进程控制块
- 设置子进程的 Trapframe 为父进程系统调用时的 Trapframe
- 将子进程的 Trapframe 中的 v0 寄存器值设为 0，即在子进程中syscall_exofork返回值为0
- 设置子进程状态为ENV_NOT_RUNNABLE，优先级为当前进程（父进程）的优先级
- 返回子进程的envid

#### 3.3 地址空间的准备

duppage：将指定页表与子进程共享， 对于可写(PTE_D)及未设置共享权限(PTE_LIBRARY)的页表项，在父进程和子进程都需要加上PTE_COW 标志位，同时取消PTE_D
标志位，以实现写时复制保护。

- static void duppage(u_int envid, u_int vpn)：envid为子进程id，vpn为要共享的页表号。
	- 获取页表项地址：vpn<<PGSHIFT
	- 判断页表项权限，如果有PTE_D且无PTE_LIBRARY，则设置PTE_COW权限，且取消PTE_D权限
	- 调用syscall_mem_map将指定页表映射到子进程
	- 若设置了PTE_COW权限，则需要更改当前进程指定页表的权限

#### 3.4 写时复制

处理页写入异常的大致流程可以概括为：

- 用户进程触发页写入异常，陷入到内核中的handle_mod，再跳转到do_tlb_mod 函数。
- do_tlb_mod 函数负责将当前现场保存在异常处理栈中，并设置a0 和EPC 寄存器的值，使得从异常恢复后能够以异常处理栈中保存的现场（Trapframe）为参数，跳转到
	env_user_tlb_mod_entry 域存储的用户异常处理函数的地址。
- 从异常恢复到用户态，跳转到用户异常处理函数中，由用户程序完成写时复制等自定义处理。

##### 3.4.1 do_tlb_mod

中断处理函数，负责处理页写入异常。

- void do_tlb_mod(struct Trapframe *tf)：
	- 将栈指针指向用户异常处理栈
	- 将当前的 Trapframe 压入异常处理栈
	- 如果用户已经注册了 TLB Mod 异常处理函数，则：
		- 将指向当前 Trapframe 的指针放入 a0，并在栈上为该参数分配空间
		- 将 EPC 置为用户态 TLB Mod 异常处理函数

之后，将返回用户态的 TLB Mod 异常处理函数处理页写入异常。

##### 3.4.2 cow_entry

处理页写入异常的用户态函数。

- static void \_\_attribute\_\_((noreturn)) cow_entry(struct Trapframe *tf)：
	- 根据vpt中发生页写入异常的地址va所在页的页表项，判断其标志位是否包含PTE_COW，是则进行下一步，否则调用user_panic() 报错。
	- 分配一个新的临时物理页到临时地址UCOW，使用memcpy 将va所在页的数据拷贝到刚刚分配的页中。
	- 将发生页写入异常的地址va映射到临时页面上，注意设定好对应的页面标志位（即去除
		PTE_COW 并恢复PTE_D），然后解除临时地址UCOW 的内存映射。
	- 通过syscall_set_trapframe恢复现场

##### 3.4.3 sys_set_trapframe

设置指定进程的trapframe。

- int sys_set_trapframe(u_int envid, struct Trapframe \*tf)：
	- 判断tf是否有效
	- 通过envid2env获取envid对应进程块
	- 判断目标进程是否为当前进程
		- 如果目标进程为当前进程：
			- 设置\*((struct Trapframe \*)KSTACKTOP - 1)为\*tf
			- 返回tf->regs[2]
		- 如果目标进程为其它进程：
			- 设置目标进程env->env_tf为\*tf
			- 返回0

##### 3.4.4 sys_set_tlb_mod_entry

设置目标进程的页写入异常处理函数。

- int sys_set_tlb_mod_entry(u_int envid, u_int func)：
	- 通过envid2env获取envid对应进程块env
	- 设置env->env_user_tlb_mod_entry为func

### 3.5 其他

- int sys_set_env_status(u_int envid, u_int status)：设置目标进程的状态
	- 判断status是否为ENV_RUNNABLE或ENV_NOT_RUNNABLE，不是则返回
	- 通过envid2env获取envid对应进程块env
	- 设置env->env_status为status
		- 若status为ENV_RUNNABLE且env当前状态不是ENV_RUNNABLE，需将env添加至调度队列
		- 若status为ENV_NOT_RUNNABLE且env当前状态不是ENV_NOT_RUNNABLE，需将env移除调度队列

## 实验体会

本次实验主要实现系统调用及fork，fork及页写入错误在用户态处理，中间借助系统调用完成一些操作，涉及到用户态和内核态的相互转换，这部分较难理解。
