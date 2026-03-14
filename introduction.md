# User Guide

欢迎使用本自定义脚本语言！这是一种类 C 风格、动态类型，并支持对象映射与高阶函数调用的轻量级脚本语言。

---

## 1. 环境准备

在脚本的最上方，你可以使用 `include` 关键字来加载标准库或外部模块：

```javascript
include("std");

```

---

## 2. 变量与数据类型

本语言使用 `var` 关键字进行变量声明，支持动态类型赋值。

* **数值**: `var a = 1;`
* **字符串**: `"hi"`
* **对象 (Dictionary)**: 使用 `{ "key" : value }` 格式定义。

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

## 7. 语法清单 (Quick Cheat Sheet)

| 功能 | 语法示例 |
| --- | --- |
| **导入** | `include("filename");` |
| **定义变量** | `var name = value;` |
| **定义函数** | `func name(args) { ... }` |
| **循环** | `while (condition) { ... }` |
| **对象访问** | `object.property` 或 `object.method()` |
| **标准输出** | `print(content);` |
