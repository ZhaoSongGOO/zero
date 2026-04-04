# User Guide

欢迎使用本自定义脚本语言！这是一种类 C 风格、动态类型，并支持对象映射与高阶函数调用的轻量级脚本语言。

---

## 1. 环境准备

### 从源码构建

ZERO 只包含四个源代码，所以从源码构建是一个非常简单的事情。

- zero.h

- vec.h & vec.c

- map.h & map.c

- main.c

你可以使用 gn 构建.

```bash
gn gen out --args="is_debug=true"
ninja -C out
```

也可以使用 cmake 工具链。

```bash
mkdir cmake_out
cd cmake_out
cmake ..
make
```

然后你就会获得名为 zero 的虚拟机，直接使用 zero -h 获取其使用方法。

```bash
zero -h

* * * * * * * * * * * * * * * * * * * **
*   ______  ______   _____     ____    *
*  |___  / | _____| |  __ \   / __ \   *
*     / /  | |__    | |__) | | |  | |  *
*    / /   |  __|   |  _  /  | |  | |  *
*   / /__  | |____  | | \ \  | |__| |  *
*  /_____| |______| |_|  \_\  \____/   *
*                                   LMM*
* * * * * * * * * * * * * * * * * * * **


Usage: zero [options] <input_file>

Options:
  -h, --help              Show this help message
  -v, --version           Display version information
  -i, --input <file>      Specify the input files

Example:
  zero -i main.z
```

---

## 2. 第一个 Hello World

zero 的源文件以 z 作为后缀，遵循这个约定，在当前目录下创建一个叫 hello_world.z 的源文件。

```bash
touch hello_world.z
```

然后键入如下内容。

```js
func main(){
    print("Hello World!");
}
```

在 zero 中, 和 c 语言类似，你需要提供一个 main 函数作为程序的入口，同时 zero 内置的函数 print 将会进行标准输出。

然后使用 `zero -i hello_world.z` 运行对应的代码。"Hello World!"

--- 

## 3. 全局与局部

<img src="./res/global.png" style="width:150; height:200"></img>


zero 中也提供了全局和局部的概念，对于一个源文件，上图中绿色部分属于全局环境，蓝色部分属于局部环境。

在全局环境中，zero 只允许函数定义，声明变量以及类型定义三种语句。不允许任何的表达式语句。目前甚至只允许字面量初始化，emmm，这很不合理，后续会进行优化。

```js
func f(){ // okay

}

var a = 1; // okay

var a = 1 + 2; // error now, but trust me, i will support.

typedef my_type{ // okay

};
```

你可以在局部环境中声明和全局变量同名变量，当然，他会在局部变量中覆盖全局变量。

```js
var a = 1 * 2;
func main(){
    var a = 2;
    print(a); // 2!!!
}
```

---

## 4. 基础变量与数据类型


本语言使用 `var` 关键字进行变量声明，支持动态类型赋值。


### 普通数值

```js
var a = 1;
var b = false;
var c = 1.2;
var d = "Hello";
```

如果切实需要使用基础类型的引用类型，你需要使用标准库预定的引用类型。

```js
// 在脚本的最上方，你可以使用 `include` 关键字来加载标准库或外部模块：
include("std");

func main(){
    var a_ref = Int(1);
    var b_ref = Int(2);
    swap(a_ref, b_ref);
    print(a_ref); // 2
    print(b_ref); // 1
}

```

### 字典

字典本身是一个引用类型。

```js
var o = {"name":"zero", "age": 2.5};
print(o); // {"name":"zero", "age":2.500000}
```

### 数组

数组本身也是引用类型，但是这里还需要更多的完善，不建议使用。

```js
var o = [1, 2.5, "zero"];
print(o); // [1, 2.500000, "zero"]
```

---


## 3. 函数定义

使用 `func` 关键字定义逻辑块。函数可以接受参数，也可以不带参数。

```javascript
// 无参数函数
func sayHi() {
    print("hi");
}

// 带参数函数
func action(name, num) {
    while (num > 0) {
        print(name);
        num = num - 1;
    }
}

```

---

## 4. 流程控制

### While 循环

目前语言通过 `while` 关键字实现循环控制。

```javascript
func loop(a, b) {
    while (b > 0) {
        print(a * 1);
        b = b - 1; // 暂不支持 b--，请使用显式赋值
    }
}

```

---

## 5. 对象与高级特性

你可以将函数作为值存入对象中，并实现类似于“方法”的调用方式。语言支持通过**点操作符 (`.`)** 进行深度链式访问。

### 示例：嵌套对象与方法调用

```javascript

func action(title, count){
    while (count > 0) {
        print(title);
        count = count - 1; 
    }
}
func main() {
    // 1. 定义包含函数的对象
    var human = {"run" : action};
    
    // 2. 嵌套对象
    var all = {"owner" : human};
    
    // 3. 链式访问并执行
    // 这将寻找 all 下的 owner，再寻找其下的 run 函数并执行
    all.owner.run("owner running", 3);
}

```

---

## 6. 代码注释

为了保持代码的可读性，你可以使用以下两种注释方式：

* **单行注释**: `// 这是单行注释`
* **多行注释**: `/* 这是块注释 */`

---

## 7. std 容器

### 1. List

[List](./test/list_test.z)

### 2. Map

[Map](./test/map_test.z)

### 3. Queue

[Queue](./test/queue_test.z)

### 4. Stack

[Stack](./test/stack_test.z)

