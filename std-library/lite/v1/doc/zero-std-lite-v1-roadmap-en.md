# Zero Std RoadMap

In this cycle, the Zero Standard Library defines itself as a **Lite** version. Instead of building an "all-encompassing" library, we focus on the current scale of the language and virtual machine to establish a small, stable, and closed-loop collection of minimum standard libraries capable of supporting simple scripts and small tool development.

For this reason, the current version is codenamed **Lite**.

## Design Principles

### Minimum Viable Principle
Prioritize supporting the most common and fundamental script development needs rather than attempting to cover a large number of advanced features at once.

### Avoid Standard Library "Kidnapping" Language Design
The standard library should serve the language core. We must avoid introducing complex runtime mechanisms prematurely just for the sake of library expansion.

### Consistency First, New Capabilities Second
Before further expanding the standard library, unify the semantics and behaviors of existing APIs.

### Purpose-Driven Features
Every new capability must have a clear use case. If a feature does not significantly improve the user experience, it should not be included in the current version.

---

## Current Status

### Basic Functions
### Basic Wrapper Types
### Basic Containers
### Basic File I/O

These capabilities form an extensible prototype, but there is still a clear gap before achieving the goal of "writing small tools."

---

## Lite Version Scope

### Resolving Technical Debt
1. **Unify Container Return Semantics**: Ensure consistent return values across different data structures.
2. **Correct Map Logic**: Fix underlying logic issues within the Map implementation.
3. **Clarify `null` Semantics**: Define exactly how `null` behaves across the library.

### Completing Minimum Core Capabilities

#### 1. String Operations (`String`)
- `String(a)`: Conversion
- `len(s)`: Length
- `substr(str, start, size)`: Substring
- `split(s, sep)`: Split
- `join(list, sep)`: Join
- `trim(s)`: Trimming
- `toInt(s)`: Type conversion
- `startsWith` / `endsWith` / `indexOf`: Search and validation

#### 2. Basic Output
- `print(x)` / `println(x)`: Standard output
- `printfmt("xxx${x}")`: String interpolation/formatting

#### 3. File I/O
- `open` / `close`
- `read` / `write`

#### 4. Minimum Error Handling
- `assert(cond, msg)`: Assertions
- `panic(msg)`: Immediate termination

### Minimum File System Capabilities
- `exists(path)`: Check existence
- `remove(path)`: Delete file/directory
- `mkdir(path)`: Create directory

### Containers
In the Lite version, only the following three containers will be provided. Existing `Queue` and `Stack` implementations will be removed:
- `vector`
- `list`
- `map`
