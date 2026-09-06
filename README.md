# Memory Pool Game

Windows 控制台飞机大战小游戏，使用自研 ObjectPool 对象池管理频繁创建/销毁的子弹、敌人、粒子对象，并通过基准测试量化对象池相对原生 new/delete 的性能差异。


## 功能

1. 控制台飞机大战：发射子弹接触敌人，含粒子爆炸效果
2. ObjectPool<T> 模板对象池：placement new 原地构造、空闲链表回收复用
3. 基准测试：固定随机序列、预热轮 → 多轮正测取均值，量化耗时与进程工作集内存，输出 JSON 报告

## 目录结构

| 路径 | 说明 |
|---|---|
| src/object_pool.h | 对象池模板实现 |
| src/game_def.h | 游戏对象定义（Bullet/Enemy/Particle） |
| src/game_core.h/.cpp | 游戏渲染、输入、逻辑主循环 |
| src/benchmark.h/.cpp | 基准测试框架 |
| src/main.cpp | 程序入口（无参跑基准，带参进游戏） |
| docs/memorypool.json | 一次基准测试的完整输出 |

## 实测结果（Windows 10 x64，MSVC Release）

每组测试对 Particle 对象做 50 外圈 × 200 内圈 = 10,000 次分配/释放，先跑 3 次预热，再取 10 次正式均值（相同随机种子保证两组路径一致）。

| 指标 | 原生 new/delete | 对象池 |
|---|---|---|
| 单次分配耗时（平均） | 60.0 ns | 30.0 ns |
| 测试总耗时 | 0.600 ms | 0.300 ms |
| 进程工作集增量 | 20 KB | 0 KB |

完整数据与 stddev/min/max 见 [docs/memorypool.json](docs/memorypool.json)。

## 依赖与构建

Windows，Visual Studio 2022（C++ 桌面开发负载），CMake 3.15+

    # 构建
    cmake -G "Visual Studio 17 2022" -A x64 .
    cmake --build . --config Release

    # 跑基准测试（无参数）
    ./memorypool.exe

    # 游戏模式（任意参数）
    ./memorypool.exe game

## 后续计划

- 异常安全：原地构造抛异常时 free list 的一致性
- 多线程场景的对象池选型（thread-local pool vs 全局池 + 锁）
- 对接 std::pmr::memory_resource，作为标准容器的自定义分配器使用
