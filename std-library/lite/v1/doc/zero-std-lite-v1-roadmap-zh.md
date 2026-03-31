# Zero Std RoadMap

在本周期，Zero 标准库将自己定义为一个 Lite 版本，不构建一个 “大而全” 的库，而是围绕当前语言与虚拟机体量，先建立一个小而稳，闭环，可支持简单脚本和小工具开发的最小标准库合集。

正因为此，当前版本的代号为 lite。

## 设计原则

### 最小可用原则

优先支持最常见、最基础的脚本开发需求，而不是一次性覆盖大量高级特性。

### 不让标准库反向绑架语言设计

标准库应该服务语言核心，而不是为了标准库扩展，过早引入复杂运行时机制。

### 优先修一致性，再加新能力

在继续扩展标准库之前，先统一已有 API 的语义和行为。

### 每个新增能力都要有明确使用场景

如果某个能力不能明显提升用户体验，就不应进入当前版本。

--- 

## 当前已有能力

### 基础函数

### 基础包装类型

### 基础容器

### 基础文件 IO

这些能力已经构成了一个可扩展的雏形，但距离 “写小工具“ 还有明显差距。

--- 

## Lite 版本范围

### 修复历史债务

1. 统一容器返回值语义

2. 修正 Map 基础逻辑

3. 明确 null 的语义

### 补齐最小核心能力

1. 字符串 `String`

- `String(a)`
- `len(s)`
- `substr(str, start, size)`
- `split(s, sep)`
- `join(list, sep)`
- `trim(s)`
- `toInt(s)`
- `startsWith`
- `endsWith`
- `indexOf`

2. 基础输出能力

- `print(x)`
- `println(x)`
- `printfmt("xxx${x}")`

3. 文件 IO

- `open`
- `close`
- `read`
- `write`

4. 最小错误处理能力

- `assert(cond, msg)`
- `panic(msg)`

### 最小文件系统能力

- `exists(path)`
- `remove(path)`
- `mkdir(path)`


### 容器

1. Lite 版本容器只提供下面三个，移除现在的 Queue 和 Stack

- `vector`
- `list`
- `map`
