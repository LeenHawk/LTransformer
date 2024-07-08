## 编译前提

于Github Codespace使用Debian Sid系统测试：

```shell
apt install cmake ninja-build clang-18 clang-tools-18 libllvm18 llvm-18 llvm-18-dev libclang-common-18-dev libclang-18-dev libclang1-18
```

假如不是使用llvm-18工具链，或者自行编译安装请自行修改CMakeLists.txt中的`LLVM_DIR`
## 使用

命令行参数如下：

```shell
-O <filename> The name of output file 
-N <loopnumber> The biggest number of layer recognized
```

输入文件会经过AST分析，将所有层数低于loopnumber的循环外放成为一个打了`#pragma HLS INLINE OFF`函数。