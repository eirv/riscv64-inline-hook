# RISC-V64 Inline Hook

[**中文**](README_zh.md)

A Lightweight Inline Hook Library for RISC-V 64

> This project utilizes RISC-V Vector Instruction Set and requires CPU support for **V**ector Extension

## Features
 * Support multiple `hook` operations on the same function, with all user's `hook` functions taking effect
 * Inline instrumentation support to read/modify register context before/after function calls
 * Simultaneous inline hook and inline instrumentation on the same function (Similar to `Xposed` framework behavior)

## Getting Started
This project provides both `C/C++` interfaces. The C interfaces offers basic functionality only; C++ implementation is recommended.

Suppose we have a function `add` that needs to return `114514`. Here are three implementation approaches:
```cpp
#include <print>

int add(int x, int y) {
  std::print("add({}, {})", x, y);
  return x + y;
}

void do_hook();

int main() {
  do_hook();

  auto result = add(1, 2);
  std::println(" = {}", result);
}
```

### Inline Hook Approach
```cpp
#include <rv64hook.h>

void do_hook() {
  using namespace rv64hook;
  static decltype(add)* add_backup = nullptr;

  ScopedRWXMemory unused(add);
  InlineHook(
      add,
      [](auto x, auto y) {
        x = 114000;
        y = 514;
        return add_backup(x, y);
      },
      &add_backup);
}
```
Output: `add(114000, 514) = 114514`

### Inline Instrumentation Approach 1
```cpp
#include <rv64hook.h>

void do_hook() {
  using namespace rv64hook;
  static decltype(add)* add_backup = nullptr;
  static struct {
    int x;
    int y;
  } data{114000, 514};

  ScopedRWXMemory unused(add);
  InlineInstrument(
      add,
      {.pre =
           [](RegisterContext* ctx, auto, auto data) {
             ctx->SetArg<0, int>(data->x);
             ctx->SetArg<1, int>(data->y);
           }},
      &data,
      &add_backup);
}
```
Output: `add(114000, 514) = 114514`

### Inline Instrumentation Approach 2
```cpp
#include <rv64hook.h>

void do_hook() {
  using namespace rv64hook;
  static decltype(add)* add_backup = nullptr;

  ScopedRWXMemory unused(add);
  InlineInstrument<void>(
      add,
      {.pre =
           [](auto ctx, auto, auto) {
             ctx->a0 = 999;
             ctx->a1 = 666;
           },
       .post =
           [](RegisterContext* ctx, auto, auto) {
             ctx->ReturnValue<int>(114514);
           }});
}
```
Output: `add(999, 666) = 114514`

## Acknowledgements
 * [berberis](https://android.googlesource.com/platform/frameworks/libs/binary_translation)
 * [sifive-libc](https://github.com/sifive/sifive-libc)
 * [open-bsd](https://www.openbsd.org)

## License
[![LGPL v3](https://www.gnu.org/graphics/lgplv3-with-text-154x68.png)](https://www.gnu.org/licenses/lgpl-3.0.txt)

*The most rewarding thing I've ever done in my life was contributing the first RISC-V64 Inline Hook to the open source community at 18.7 years old*
