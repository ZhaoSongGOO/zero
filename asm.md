# Zero VM Instruction Set Architecture (ISA) Reference

This document provides a technical specification for the Zero VM assembly instructions. The Virtual Machine operates on a **Stack-Based Architecture** where most instructions pop operands from the top of the stack and push results back.

## 1. Memory Operations

### STORE

**Description**: Pops the top value from the stack and stores it in a specified location.

* `STORE <index>`: Pops the value and stores it at the absolute stack index.
* `STORE [*]`: Pops the value into the **ei** (Return Value) register.
* `STORE #<offset>`: Pops the value and stores it at the relative position `bp + offset` (Base Pointer offset).

### LOAD

**Description**: Loads a value from a specified location and pushes it onto the stack.

* `LOAD <index>`: Loads data from the absolute stack index.
* `LOAD [*]`: Loads data from the **ei** register.
* `LOAD #<offset>`: Loads data from the relative position `bp + offset`.
* `LOAD .<name>`: Loads the function pointer associated with `name` from the Symbol Table.

---

## 2. Immediate Constants (Push Operations)

These instructions push literal values of specific types onto the stack.

| Instruction | Type | Example | Description |
| --- | --- | --- | --- |
| **PUSH_D** | Integer | `PUSH_D 1` | Push a 64-bit integer. |
| **PUSH_F** | Float | `PUSH_F 1.2` | Push a double-precision floating point. |
| **PUSH_B** | Boolean | `PUSH_B 1` | Push `true` (1) or `false` (0). |
| **PUSH_S** | String | `PUSH_S "text"` | Push a string constant. |
| **PUSH_N** | Null | `PUSH_N` | Push a `null` value. |

---

## 3. Function Calls & Stack Management

### CALL

**Description**: Transfers control to a function.

* `CALL <name>`: Calls the function by its identifier.
* `CALL -1`: Dynamic call. Pops the value at `top - 1` and treats it as a function pointer to execute.

### FREE

**Description**: Cleans up the stack.

* `FREE <n>`: Removes `n` elements from the top of the stack.

### ACCESS

**Description**: Property access for objects.

* Pops `a` (attribute name) and `b` (object).
* Pushes the value of `b.a` onto the stack. Requires `b` to be an `Object` type.

### ASSIGN

**Description**: Direct assignment.

* Pops value `a`. Assigns it to the element at `top - 1`.
* **Requirement**: The element at `top - 1` must be a reference type.

---

## 4. Arithmetic & Logical Instructions

Unless otherwise specified, these instructions pop two values: `a` (top) and `b` (top-1), then push the result of `b [op] a`.

| Mnemonic | Operation | Description |
| --- | --- | --- |
| **ADD** | `b + a` | Addition. |
| **MINUS** | `b - a` | Subtraction. |
| **MULTI** | `b * a` | Multiplication. |
| **DIV** | `b / a` | Division. |
| **NEGATE** | `-a` | Pops `a`, pushes its arithmetic negation. |

### Comparison

* **G / GE**: Greater Than (`b > a`) / Greater or Equal (`b >= a`).
* **L / LE**: Less Than (`b < a`) / Less or Equal (`b <= a`).
* **E / NE**: Equal (`b == a`) / Not Equal (`b != a`).
* **NOT**: Logical NOT (`!a`). Pops `a`, pushes inverted boolean result.

---

## 5. Control Flow (Branching)

### JUMP

**Description**: Unconditional relative jump.

* `JUMP <offset>`: Moves the Program Counter (PC) by the specified offset (e.g., `PC = PC + offset`).

### JF (Jump if False)

**Description**: Conditional relative jump.

* Pops the top value. If it is `false`, `PC = PC + offset`. Otherwise, proceeds to the next instruction.

