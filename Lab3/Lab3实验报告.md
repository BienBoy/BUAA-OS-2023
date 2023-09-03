# Lab3实验报告

## 思考题

### Thinking 3.1

UVPT为页目录起始虚拟地址，e->env_pgdir[PDX(UVPT)]
= PADDR(e->env_pgdir) | PTE_V将e->env_pgdir[PDX(UVPT)]对应的页目录项内的物理地址设置为页目录的起始物理地址，实现了自映射。

### Thinking 3.2

有四种情况：

- 起始地址位于页内，终止地址为页终止地址
- 终止地址位于页内，起始地址为页起始地址
- 起始地址、终止地址均位于页内
- 起始地址、终止地址分别为页起始地址、页终止地址

### Thinking 3.4

虚拟地址

### Thinking 3.5

- handle_int：kern/genex.S
- handle_mod：kern/genex.S内通过宏BUILD_HANDLER定义，主要处理函数为do_tlb_mod，位于kern/tlbex.c
- handle_tlb：kern/genex.S内通过宏BUILD_HANDLER定义，主要处理函数为do_tlb_refill，位于kern/tlb_asm.S
- handle_sys：kern/genex.S内通过宏BUILD_HANDLER定义，主要处理函数为do_syscall，位于kern/do_syscall.c

### Thinking3.6

```assembly
LEAF(enable_irq)
	li      t0, (STATUS_CU0 | STATUS_IM4 | STATUS_IEc) // 获取CP0允许始中断的状态值
	mtc0    t0, CP0_STATUS   // 设置CP0允许中断
	jr      ra   // 返回
END(enable_irq)
```

```assembly
timer_irq:
	// 向KSEG1 | DEV_RTC_ADDRESS | DEV_RTC_HZ 位置写入0，即关闭时钟中断
	sw      zero, (KSEG1 | DEV_RTC_ADDRESS | DEV_RTC_INTERRUPT_ACK) 
	li      a0, 0 // 设置传入schedule的参数为0
	j       schedule // 调用schedule函数
```



### Thinking 3.7

在进程使用完时间片后，将会产生时钟中断。MOS切换至内核态，根据CP0中存储的中断信息，调用相应的中断处理函数，在中断处理函数中，进一步调用schedule函数实现进程的切换。

## 难点分析

### 1. 进程

#### 1.1 进程管理初始化

主要用到两个函数：

- map_segment：将一段物理地址（即一个或多个连续的物理页面）映射到一段虚拟地址（即一个或多个连续的虚拟页面），并设置指定权限。通过循环调用page_insert函数实现。
- env_init：初始化进程控制块。主要工作为：
	- 将所有进程控制块状态设为ENV_FREE，并添加至空闲进程控制块链表env_free_list
	- 构造模板页表：通过page_alloc申请一个页表，将其作为页目录base_pgdir，在其中调用map_segment建立UPAGES到pages数组、UENVS到envs数组的映射，UPAGES和UENVS均为用户态可访问的地址，以便用户进程可以通过模板页表访问pages数组和envs数组。

#### 1.2 创建进程

##### 1.2.1 获取空闲进程控制块

主要用到以下函数：

- env_setup_vm：初始化用户进程的虚拟地址空间。主要工作为：

	- 通过page_alloc申请一个页表，用作该进程的页目录。

	- 将模板页目录base_pgdir内的UTOP至UVPT间的映射关系复制到页目录（这一区间内的映射关系就是env_init中建立的UPAGES到pages数组、UENVS到envs数组的映射），之后用户进程可以通过页表访问pages数组和envs数组。

	- 建立自映射关系，即在页目录中设置到页目录的映射。

- asid_alloc：分配一个asid
- mkenvid：获取进程的envid，envid可以和env相互转换。
- env_alloc：建立进程环境。主要工作为：
	- 在env_free_list中取出一个空闲的进程控制块
	- 使用env_setup_vm初始化进程的虚拟地址空间
	- 初始化进程控制块的字段，如：通过asid_alloc获取asid，通过mkenvid获取envid，设置parent_id、cp0_status等。

##### 1.2.2 加载二进制镜像

将程序加载到新进程的地址空间，主要用到以下函数：

- load_icode：加载可执行文件binary 到进程e 的内存中，主要工作如下：
	- 调用elf_from找到 ELF 文件中的所有要加载的segment信息
	- 通过ELF_FOREACH_PHDR_OFF遍历所有要加载的segment，对每个可加载的程序段，调用elf_load_seg将其加载到内存中
	- 将进程控制块的env_tf.cp0_epc设置为程序入口
- load_icode_mapper(void *data, u_long va, size_t offset, u_int perm, const void *src, size_t len)：作为回调函数传入elf_load_seg，用于处理单个页面的加载过程。
	- 通过page_alloc分配一页物理页面
	- 如果 src 不是 NULL ，就将 src 开始的 len 字节长度的一段数据复制到新分配的物理页面中 offset 开始的空间
	- 将新分配的物理页面映射到进程虚拟地址空间中 va 处，并将权限设为 perm

##### 1.2.3 创建进程

实现对上述个别函数的封装，分配一个新的Env 结构体，设置进程控制块，并将程序
载入到该进程的地址空间，具体实现函数为：

- env_create：
	- 调用env_alloc分配空闲进程控制块
	- 设置进程优先级，设置进程状态为ENV_RUNNABLE
	- 调用load_icode加载二进制镜像
	- 将进程控制块添加至调度队列env_sched_list

#### 1.3 进程的运行与切换

切换当前运行进程，主要用到以下函数：

- env_run：
	- 保存当前进程的上下文信息（若当前有正在运行的进程）
	- 将 curenv 改为新进程的控制块指针
	- 自增 env_runs，于Lab6使用
	- 设置全局变量cur_pgdir 为当前进程页目录地址，在TLB 重填时将用到该全局变量
	- 调用 env_pop_tf，恢复现场、异常返回。

- env_pop_tf：
	- 设置 EntryHi 寄存器的 ASID 部分。
	- 将 sp 寄存器的值设置为 env_tf 的地址，以便 ret_from_exception() 利用
	- 将 sp 寄存器的值设置为 env_tf 的地址，以便 ret_from_exception() 利用

- ret_from_exception：从异常返回，详见中断部分。

#### 1.4 进程调度

暂停当前进程的运行，选取其他进程运行。进程调度主要发生在时钟中断后，时钟中断处理函数会跳转至进程调度函数实现进程调度，主要用到以下函数：

- schedule(int yield)：实现进程调度。
	- 在尚未调度过任何进程、当前进程已经用完了时间片、当前进程不再就绪（如被阻塞或退出）或yield 参数指定必须发生切换时，需要暂停当前进程的运行，选取其他进程运行。否则不进行进程切换。
	- 判断当前进程是否仍然就绪，如果是则将其移动到调度链表的尾部，否则从调度链表中删去。
	- 从调度链表头部取出一个新的进程来调度，如果没有队列中没有可用的进程，则调用panic，内核崩溃。
	- 设置count（时间片）为当前进程的优先级。
	- count 自减 1
	- 调用 env_run() 函数，实现进程的切换。

### 2. 中断与异常

#### 2.1 异常的分发

当发生异常时，处理器会进入一个用于分发异常的程序，这个程序的作用就是检测发生了哪
种异常，并调用相应的异常处理程序。分发程序由操作系统提供。主要用到以下函数：

- exc_gen_entry：MIPS编写，根据异常类型，跳转至相应的异常处理函数
	- 使用SAVE_ALL 宏将当前上下文保存到内核的异常栈中。
	- 将Cause寄存器的内容拷贝到t0寄存器中。
	- 获取异常码（Cause寄存器中的2~6 位）
	- 以得到的异常码作为索引在exception_handlers 数组中找到对应的中断处理函数
	- 跳转到对应的中断处理函数

#### 2.2 时钟中断

init/init.c 中的mips_init 函数在完成初始化并创建进程后，需要调用kclock_init 函数完成时钟中断的初始化，然后调用kern/env_asm.S 中的enable_irq函数开启中断。

- kclock_init：初始化时钟中断。通过向KSEG1 | DEV_RTC_ADDRESS
	| DEV_RTC_HZ 位置写入200，设置时钟中断的频率为1秒钟中断200 次.
- enable_irq：允许中断。

#### 2.3 从异常返回

异常处理完毕后，需要返回用户态继续执行程序。主要用到以下函数：

-  ret_from_exception：
	- 调用 RESTORE_SOME 宏进行寄存器的恢复。
	- 将 EPC 寄存器的内容读取到 k0 寄存器中，以便跳转。
	- 将该进程真正的栈指针 sp 读取到 sp 寄存器中。
	- 通过jr指令跳转到EPC
	- 通过rfe指令对SR（Status）寄存器进行恢复。它将在延迟槽中执行，这是MIPS的规定。

## 实验体会

Lab3实现的进程的管理，个人感觉这次实验的内容与内存管理相比更易理解。经过Lab3的实践，我对进程的管理以及中断机制有了进一步的认识。同时，在完成Lab3的过程中，初步掌握了GXemul调试程序的方法。
