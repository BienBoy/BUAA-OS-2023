# Lab 1 Extra

## 准备工作：创建并切换到 lab1-extra 分支

请在自动初始化分支后，在开发机依次执行以下命令：

```
$ cd ~/学号
$ git fetch
$ git checkout lab1-extra
```
初始化的 lab1-extra 分支基于课下完成的 lab1 分支，并且在 tests 目录下添加了lab1_sprintf 样例测试目录。

## 题目背景 & 函数声明

课下我们在 MOS 系统中实现了 printk 函数，使得内核可以将信息输出到控制台。

现在你需要仿照 printk 函数在 lib/string.c 中实现 sprintf 函数，使得调用该函数可以将格式化的结果输出到内存缓冲区中，成为一个 C 字符串。

>int sprintf(char *buf, const char *fmt, ...);
>
>参数：
>
>- buf ：指向目标缓冲区的指针，需要向其中写入格式化得到的 C 字符串（包含字符串末尾的 '\0'）。参数保证目标缓冲区的有效长度足够容纳正确的格式化结果。
>
>- fmt ：格式字符串，与 printk 函数中的定义相同。
>
>返回值 ：返回写入的字符总数，不计追加在字符串末尾的 '\0'。

## 题目要求

在 include/string.h 中声明以下函数：

>int sprintf(char *buf, const char *fmt, ...);
>

在 lib/string.c 中实现上述函数。

>提示
>
>可以调用 vprintfmt 完成格式化字符串的解析，这时需要在 lib/string.c 的顶部包含相应的头文件。
>
>在 void vprintfmt(fmt_callback_t out, void *data, const char *fmt, va_list ap) 中， out 、 data 分别叫做回调函数、回调上下文。 printk 函数直接输出到控制台，因此可以不使用 data ；在实现 sprintf 函数时，你可以使用自定义的结构保存当前输出位置的相关信息，将其地址作为参数 data 传入 vprintfmt 函数，并仿照 outputk 函数实现自定义的回调函数。

## 样例输出 & 本地测试

对于下列样例：

```C
char str[100];
sprintf(str, "%d\n", 12321);
printk("%s", str);
sprintf(str, "%c\n", 97);
printk("%s", str);
```

应当输出：

```
12321
a
```

你可以使用 `make test lab=1_sprintf && make run` 在本地测试上述样例，或者在 init/init.c 的 mips_init() 函数中自行编写测试代码并使用 `make && make run` 测试。

## 提交评测 & 评测标准

请在开发机中执行下列命令后，在课程网站上提交评测。

```shell
$ cd ~/学号
$ git add -A
$ git commit -m "message"  # 请将 message 改为有意义的信息
$ git push
```

在线评测时，所有的 .mk 文件、所有的 Makefile 文件、init/init.c 以及 tests/ 和 tools/ 目录下的所有文件都可能被替换为标准版本，因此请同学们在本地开发时，不要在这些文件中编写实际功能所依赖的代码。

测试用例保证向 sprintf 传入的缓冲区 buf 的有效大小足够容纳正确的格式化结果。

测试点和分数说明如下：

|测试点序号|评测内容|分数|
|---|---|---|
|1	|样例	|5
|2	|sprintf 的可变长参数列表中只有一个数字	|20
|3	|sprintf 的可变长参数列表中可能有多个数字、字符或字符串	|25
|4	|只检查 sprintf 函数的返回值是否正确	|25
|5	|综合评测	|25