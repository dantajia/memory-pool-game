# Memory Pool Game — 对象池优化的控制台飞机大战

一个 Windows 控制台飞机大战小游戏（发射子弹 / 敌人碰撞 / 粒子特效），**核心不是游戏而是内存**：针对游戏主角频繁创建/销毁的对象（子弹、敌人、粒子），设计并实现了一个 **C++ 对象池（Object Pool）模板类**，通过严谨的 **benchmark 框架** 对比 `new/delete` 与对象池的分配性能与内存行为，用数据证明优化的有效性。

## 性能实测（内存池 vs 原生 new/delete）

（Windows 10 + MSVC Release，Particle 对象，50 外圈 × 200 内圈 = 10,000 次分配/释放，3 次 warm-up + 10 次正式测试取平均，随机种子固定保证公平）

| 指标 | 裸 `new/delete` | 对象池 | 提升 |
|---|---|---|---|
| 平均每次分配耗时 | **60.0 ns** | **30.0 ns** | **2.0× 加速** |
| 测试总耗时 | 0.600 ms | 0.300 ms | 50% ↓ |
| 测试期间增量内存（工作集） | 20 KB | **0 KB**（全量回收复用） | — |
| 耗时标准差 | 0.490 ms | 0.458 ms | 稳定性相当 |

完整 JSON 数据见 [docs/memorypool.json](docs/memorypool.json)。

> 为什么 pool 模式下增量内存为 0：对象销毁后不交还给操作系统，而是归还到池的空闲链表，后续分配直接复用已分配块，`total new` 调用被拦截，工作集不再增长——这是对象池在游戏/高并发服务的的本质优势。

## 对象池设计（`src/object_pool.h`）

- **空闲链表（free list）结构**：已释放节点直接复用，分配 = 摘队首 + placement new 原地构造
- **构造/析构语义正确**：`T* allocate()` 内 `new(obj) T()` 原地构造；`deallocate()` 显式析构后挂回链表，**类型安全、无对象生命周期错误**
- **容量自增长**：空池时按需 `new`，运行期记录 `totalAllocated`、峰值 `maxPreallocated`、空闲量 `free_count`
- **模板化通用**：`ObjectPool<T>` 可用于任何对象类型（游戏里 Bullet / Enemy / Particle 各自一个池）

## Benchmark 框架为什么可信（`src/benchmark.cpp`）

- **公平对比**：同一固定随机种子 `mt19937 rng(12345)` 生成两组相同的操作序列，变量只有"用不用池"
- **防编译器过度优化**：`volatile int global_dummy` 累加粒子成员，确保分配结果不被优化掉
- **严谨抽样**：3 次 warm-up + 10 次正式测试 → 输出 **avg / stddev / min / max**，并检测两组方差（防"某一组跑了更快的偶然"）
- **系统级监控**：`GetProcessMemoryInfo + WorkingSetSize` 实时采集 Windows 进程工作集，量化内存行为而不只是时间
- **结果持久化**：统一 JSON 格式落盘，可复现、可放进 CI 的回归对比

```json
{
  "test_info": { "test_runs": 10, "warmup_runs": 3, "total_allocations": 10000 },
  "without_pool": { "avg_alloc_ns": 60.00, "stddev_ms": 0.490, "peak_memory_kb": 20 },
  "with_pool":    { "avg_alloc_ns": 29.97, "stddev_ms": 0.458, "peak_memory_kb": 0 },
  "speedup": 2.0
}
```

## 游戏本体（控制台渲染）

- 40×25 字符网格 2D 渲染（`std::cout` + 延迟帧率 40ms loop）
- 子弹-敌人-粒子 三类对象的生成/碰撞/销毁，正是热点分配来源
- 运行时面板实时输出 "[Technical Summary] Total system new calls intercepted: N times"，**把池的拦截效应可视化**

## 构建运行

```bash
# CMake + VS2022 / MinGW
cmake -G "Visual Studio 17 2022" -A x64 .
cmake --build . --config Release

./memorypool.exe            # 无参数 = 跑 benchmark
./memorypool.exe game       # 任意参数 = 进入游戏模式
```

## 项目可以怎么被深挖

- 对象池的**异常安全**（构造抛异常时 free list 的一致性）
- 扩展到**多线程池**：thread-local 池 vs 全局池 + 锁，怎么选？
- 对于变长对象（例如不同尺寸的子弹/敌人），**segmented pool** 的设计思路
- 与 `std::pmr::memory_resource` 官方"内存资源"接口的对比与对接
