title:The Lil Virtual Machine

The Lil Virtual Machine
=======================
Lil is implemented as a stack-based virtual machine. This document describes the virtual machine's architecture, opcodes, and how various language features are implemented.

Stack-Based Architecture
------------------------
The Lil VM is a stack-based virtual machine, meaning that operations primarily manipulate values on a stack. The VM maintains several stacks:

- _Parameter stack_: Holds values that are being operated on
- _Environment stack_: Holds lexical environments for variable lookup
- _Task stack_: Holds blocks of code to be executed
- _Program counter stack_: Holds the current execution position in each task

Opcodes
-------
The VM supports the following opcodes:

| Opcode | Description |
|--------|-------------|
| `JUMP` | Unconditional jump to a target address |
| `JUMPF` | Jump to a target address if the top value on the stack is falsey |
| `LIT` | Push a literal value onto the stack |
| `DUP` | Duplicate the top value on the stack |
| `DROP` | Remove the top value from the stack |
| `SWAP` | Swap the top two values on the stack |
| `OVER` | Copy the second value on the stack to the top |
| `BUND` | Create a list from N values on the stack |
| `OP1` | Apply a unary operator to the top value on the stack |
| `OP2` | Apply a binary operator to the top two values on the stack |
| `OP3` | Apply a ternary operator to the top three values on the stack |
| `GET` | Look up a variable in the current environment and push its value |
| `SET` | Assign a value to a variable in the current environment |
| `LOC` | Create a local variable in the current environment |
| `AMEND` | Modify a value at a specific index |
| `TAIL` | Perform a tail call optimization |
| `CALL` | Call a function |
| `BIND` | Create a function closure |
| `ITER` | Prepare for iteration |
| `EACH` | Begin an iteration loop |
| `NEXT` | Continue an iteration loop |
| `COL` | Process a column in a query |
| `IPRE` | Pre-process an index operation |
| `IPOST` | Post-process an index operation |
| `FIDX` | Filter by index |
| `FMAP` | Map a function over a collection |
| `LINE` | Source code line number (for debugging) |

Handling Literals
-----------------
Literals in Lil are handled using the `LIT` opcode. When the compiler encounters a literal value (number, string, etc.), it adds the value to a literal table in the bytecode block and emits a `LIT` instruction with an index into this table.

For example, the Lil code:

```lil
42
```

Compiles to:

```
LIT 42
```

And a string literal:

```lil
"hello"
```

Compiles to:

```
LIT "hello"
```

Function Calls
--------------
Function calls in Lil are implemented using the `CALL` opcode. When a function is called:

1. The function and its arguments are pushed onto the stack
2. The `CALL` opcode is executed
3. A new environment is created, binding the function's parameters to the arguments
4. The function's body is executed
5. The result is pushed onto the parameter stack

For example, the Lil code:

```lil
foo[42]
```

Compiles to:

```
GET "foo"    # Push the function onto the stack
LIT 42        # Push the argument onto the stack
BUND 1        # Bundle the arguments into a list
CALL          # Call the function
```

Tail Call Optimization
----------------------
Lil supports tail call optimization through the `TAIL` opcode. When a function call is the last operation in a function, the `TAIL` opcode is used instead of `CALL`. This avoids growing the call stack for recursive functions.

For example, the Lil code:

```lil
on factorial n do
  if n <= 1
    1
  else
    n * factorial[n-1]
  end
end
```

The recursive call to `factorial` is not in tail position because the result is multiplied by `n`. However, if we rewrite it in tail-recursive form:

```lil
on factorial_tail n acc do
  if n <= 1
    acc
  else
    factorial_tail[n-1 n*acc]
  end
end
```

The call to `factorial_tail` is in tail position and will be compiled using the `TAIL` opcode.

Closures
--------
Closures in Lil are implemented using the `BIND` opcode. A closure captures the current lexical environment, allowing functions to access variables from their defining scope.

For example, the Lil code:

```lil
on counter start do
  on increment do
    start: start + 1
  end
end
```

The `increment` function captures the `start` variable from its enclosing scope. When `counter` is called, it returns a closure that has access to its own private `start` variable.

Let's look at some examples of Lil code and their corresponding VM assembly.

Simple Assignment
-----------------
```lil
x: 42
```

Compiles to:

```
LIT 42
SET "x"
```

Function Definition
-------------------
```lil
on add x y do
  x + y
end
```

Compiles to:

```
LIT "function:add" {
  // Arguments: x y
  GET "x"
  GET "y"
  OP2 "+"
}
BIND
```

Conditional
-----------
```lil
if x > 10
  "large"
else
  "small"
end
```

Compiles to:

```
GET "x"
LIT 10
OP2 ">"
JUMPF L0
LIT "large"
JUMP L1
L0:
LIT "small"
L1:
```

Loop
----
```lil
each x in 1,2,3
  x * 2
end
```

Compiles to:

```
LIT 1
LIT 2
LIT 3
BUND 3
ITER
L0:
LIT "x"
EACH L1
GET "x"
LIT 2
OP2 "*"
NEXT L0
L1:
```

Conclusion
----------
The Lil virtual machine is a simple yet powerful stack-based VM that efficiently implements the Lil language. Its design allows for features like closures, tail call optimization, and a clean separation between the compiler and the runtime.
