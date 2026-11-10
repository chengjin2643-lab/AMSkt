# AMSkt

论文 **AMSkt: Accurate and Private Distinct Counting for Multiple Groups** 的代表性研究代码。

本仓库先提供 FMS 版本，包含 AMSkt、vSkt、rSkt、CMSkt 和预分配独立 FMS。核心实现支持公共组目录、组加盐哈希、Hadamard 分块分配、分区状态合并和联合私有发布。

## 编译和测试

需要 C++20 编译器与 CMake 3.20 或更新版本，无需额外下载数据。

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel 4
ctest --test-dir build -C Release --output-on-failure
./build/amskt_demo
```

Windows 的 MinGW 环境请先将编译器 `bin` 目录加入 `PATH`，首次配置增加 `-G "MinGW Makefiles"`，执行文件使用 `.exe` 后缀。

小示例使用合成数据，演示两分区合并及五种方法的误差比较。它不是论文完整实验的复跑，也不将一次试验作为统计结论。

## 文件说明

- `include/amskt/` 是当前 FMS 核心实现。
- `examples/` 是小型可运行示例。
- `tests/` 检查正交性、重复消除、合并、反演和主实验/消融一致性。
- `experiments/` 提供可选的 Synthetic-10K 运行入口与单次试验参数。
- `docs/` 说明配置和内存口径。

首版不包含论文全文、审稿意见、真实数据集、全部历史代码和大型结果文件。FM/LLC 不在这个精简版本的范围内。

预算指逻辑位图容量，辅助映射、解码表和输出缓存不计入其中。示例中的固定噪声种子只用于合成试验，实际私有发布不能公开噪声种子。项目整体许可证尚未确定，不默认授予开源使用许可。

完整命令与实现说明见 [英文 README](README.md)、[实验说明](docs/EXPERIMENTS.md) 和 [实现说明](docs/IMPLEMENTATION.md)。
