# Lab 1 Exam

准备工作：创建并切换到 lab1-exam 分支

请在自动初始化分支后，在开发机依次执行以下命令：


```Shell
$ cd ~/学号
$ git fetch
$ git checkout lab1-exam
```

初始化的 lab1-exam 分支基于课下完成的 lab1 分支，并且在 tests 目录下添加了 lab1_range 样例测试目录。

## 题目要求

本题要求修改 lib/print.c 中的 vprintfmt 函数，实现对格式化说明符 R 的支持，使得 printk 可以输出 “Range”。同时，你实现的 Range 也应当满足课下任务中所要求的 printk 格式。即 `%[flags][width][length]specifier`。

`%[flags][width][length]R`的格式化行为如下：读取两个变长参数，产生文本 (<a>,<b>) ，其中 <a> 和 <b> 应为两个参数在 `%[flags][width][length]d` 下的格式化结果。当 [length] 部分为 l 时，两个变长参数的类型都为 long int；否则，两个变长参数的类型都为 int 。

具体来说，对于下列代码：

printk("This is a range: %8R", 2023, 2023);

其正确输出应为：

This is a range: (   2023,   2023)

样例输出 & 本地测试

对于下列样例：

printk("%s%R%s", "This is a testcase: ", 2023, 2023, "\n");

printk("the range is %R, size = %d\n", 1, 9, 9 - 1);

其应当输出：

This is a testcase: (2023,2023)

the range is (1,9), size = 8

你可以使用 make test lab=1_range && make run 在本地测试上述样例，或者在 init/init.c 的 mips_init() 函数中自行编写测试代码并使用 make && make run 测试。

## 提交评测 & 评测标准

请在开发机中执行下列命令后，在课程网站上提交评测。

```shell
$ cd ~/学号/
$ git add -A
$ git commit -m "message" # 请将 message 改为有意义的信息
$ git push
```

具体要求和分数分布如下：

|测试点序号| 评测内容| 分值|
|---|---|---|
|1  |与题面中的样例相同|  10 分
|2  |Range 不涉及对于 flags 、 width 和 length 的评测，Range 对应的两个变长参数均为正整数 |10 分
|3  |Range 不涉及对于 flags 、 width 和 length 的评测  |20 分
|4  |Range 不涉及对于 flags 、 width 的评测  |20 分
|5 | 综合评测   |40 分

测试用例补充说明：

说明符 R 对应的两个参数均为 int 或均为 long int 类型，具体类型取决于说明符 R 前是否有 l。

对于第一个测试点，其测试内容不包含样例外的任何其他内容。

对于后四个测试点，测试数据可能会包含任何课下已经实现的功能。

在线评测时，所有的 .mk 文件、所有的 Makefile 文件、init/init.c 以及 tests/ 和 tools/ 目录下的所有文件都可能被替换为标准版本，因此请同学们本地开发时，不要在这些文件中编写实际功能所依赖的代码。