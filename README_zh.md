# RISC-V64 Inline Hook

[**English**](README.md)

适用于 RISC-V 64 的轻量级 Inline Hook 库

> 此项目用到了 RISC-V 向量指令集, 需要 CPU 支持向量扩展才可以使用

## 特征
 * 对同一个函数进行多次 `hook`, 每个用户 `hook` 函数均可生效
 * 支持对函数进行插桩, 在其调用 前/后, 读取/修改 寄存器上下文
 * 可对一个函数同时进行 Inline Hook 与函数插桩, 二者均可生效 (类似于`Xposed`)

## 开始使用
此项目同时提供了`C/C++`两种借口, `C`只提供基础功能, 建议使用`C++`

假设我们有一个函数`add`需要使其返回`114514`, 可以使用以下三种方法
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

 * Inline Hook
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
将打印`add(114000, 514) = 114514`

 * 函数插桩
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
将打印`add(114000, 514) = 114514`

 * 函数插桩2
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
将打印`add(999, 666) = 114514`

## 致谢
 * [berberis](https://android.googlesource.com/platform/frameworks/libs/binary_translation)
 * [sifive-libc](https://github.com/sifive/sifive-libc)
 * [open-bsd](https://www.openbsd.org)

## 许可证
[![LGPL v3](https://www.gnu.org/graphics/lgplv3-with-text-154x68.png)](https://www.gnu.org/licenses/lgpl-3.0.txt)

*我这辈子做过的最有成就感的事莫过于在 18.7 岁为开源社区贡献了第一份 RISC-V64 Inline Hook*
