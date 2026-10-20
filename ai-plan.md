# AI 方向：llama.cpp 量化复现（半天任务 · 简历素材）

> 定位：不是项目，是“AI 认知”素材。30 天内抽 2-3 个半天完成，让简历有 AI 相关一行、面试能聊 3 分钟。
> 真正的 AI 项目（向量库 / RAG / 推理引擎）放第 2 个月，用 Linux 服务器项目打底后再做。

## 半天任务清单（在 VM 里做）

1. clone 并编译 llama.cpp（`cmake -B build && cmake --build build -j`）
2. 从 HuggingFace 下载一个小模型的两个量化版本，如 Qwen2.5-0.5B-Instruct 的 `Q4_K_M` 和 `Q8_0` GGUF
3. 分别用 `llama-cli` 跑同一段 prompt，记录：
   - 生成速度（tok/s）
   - 内存占用（RSS）
   - 输出质量粗略对比（同一问题看回答是否正常）
4. 读一篇量化原理文：HuggingFace Quantization 文档 / llama.cpp 的 k-quants 说明
5. 写 100 字总结发笔记

## 简历措辞（放技能区“其他”一行）

> 了解 LLM 推理部署：在 llama.cpp 上复现 Q4_K_M / Q8_0 量化对比（速度、内存、质量）

## 面试 3 分钟大纲（自己讲顺）

1. 量化是什么：把 FP16 权重压到 4/8 bit，省显存/内存；
2. Q4_K_M vs Q8_0 的实测差异：内存减半、速度不一定变快（反量化有开销）、质量轻微下降；
3. 为什么 llama.cpp 能跑在 CPU 上：GGUF 格式 + 量化 + 针对性优化；
4. 顺带能提的：KV cache、tokenizer、采样参数（temperature/top-p）。

## 可能被追问（提前准备）

- 量化为什么可能“不加速”？
- KV cache 是什么？长上下文为什么吃内存？
- GGUF 和 safetensors 的区别？

## 第 2 个月候选（做完 Linux 服务器再选）

- C++ 本地 RAG：llama.cpp 推理 + embedding + HNSW 检索
- 向量数据库 mini 版：HNSW + SIMD 距离计算
- 迷你推理引擎：实现 GGML 风格量化 + KV cache（难度最高）
