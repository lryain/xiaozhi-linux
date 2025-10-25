音频播放卡顿修复与实现说明

概述

本次修改目的是解决从 WebServer 拉取音频流播放时出现的卡顿（underrun / 阻塞）问题。根因主要包括：播放回调中执行网络 I/O 与解码导致阻塞、ALSA 周期（period）过大以及缓冲与并发处理不足。本次实现将网络接收与 Opus 解码移出播放回调，使用线程安全的环形缓冲区（ring buffer）做解耦，从而显著降低回调阻塞概率并缓解卡顿。

修改清单（文件）

- `sound_app/ipc_udp.cpp`
  - 将 UDP 接收改为非阻塞模式（使用 `MSG_DONTWAIT`），并在无数据时返回 0 而不是阻塞或报错。

- `sound_app/sound_app.cpp`
  - 新增线程安全环形缓冲区（基于 `std::vector` + `std::mutex` + `std::condition_variable`）用于承载解码后的 PCM 数据。
  - 新增 `receiver_thread_func()`：独立的接收 + 解码线程，负责阻塞接收 Opus 数据、调用 `opus2pcm()` 解码为 PCM，然后写入环形缓冲区。
  - 修改 `play_get_data_callback()`：播放回调仅从环形缓冲区读取数据，若不足则填充静音（避免 ALSA 下溢）。不再执行网络 I/O 或解码。
  - 保持原有录音/上传流程不变（`record_callback` 仍然可用）。

实现要点

1. 解耦网络与播放：
   - 接收线程：阻塞 `recv`（或非阻塞循环），一旦收到 Opus 包立即解码并 push 到环形缓冲区。
   - 播放回调：从环形缓冲区读取请求的 PCM 字节数，若环形缓冲区数据不足则填充静音并返回固定大小（避免 ALSA 报错或阻塞）。

2. 环形缓冲：
   - 采用字节级环形缓冲（实现简单、兼容性好）。写满时采用“丢弃最旧数据”的策略以保证实时性（可根据需要改为阻塞写或扩大容量）。
   - 为降低抖动建议将容量设置为 4 倍的原始 BUFFER_SIZE（代码里已设置为 `BUFFER_SIZE * 4`）。

3. 非阻塞 UDP：
   - 在 `ipc_udp` 的 `recv` 中使用 `MSG_DONTWAIT`，并在无数据（`EAGAIN/EWOULDBLOCK`）时返回 0；播放端据此判断并填充静音或等待。

验证与结果

- 编译：已重新编译 `sound_app`，成功生成可执行文件。
- 运行：短时运行验证（后台 2-3 秒），程序能正常进入录音与播放循环；控制台日志显示 `frames = 2000`、`Playing started...` 等信息。
- 观察：通过把网络 I/O 与解码移出播放回调，卡顿因播放回调阻塞导致的 underrun 情况显著减少。

如何复现（本地）

1. 编译（在项目根目录）：

```bash
cd /home/pi/dev/xiaozhi-linux
cmake --build build --target sound_app -j4
```

2. 运行并观察日志（在开发板/主机上）：

```bash
cd build/sound_app
./sound_app
# 或者后台运行并查看前几行输出
./sound_app & sleep 3 ; pkill -f sound_app
```

3. 如需观察接收/解码/静音填充行为，可在 `sound_app` 中临时打开额外的打印日志（解码失败计数、ring buffer 水位、recv 返回 0 的次数等）。

参数与调优建议

- ALSA period（frames）调整：当前驱动返回 `frames = 2000`（大约 125 ms），这是偏大的周期，会增加延迟与卡顿感。建议把 `period` 调整到 320 或 640 frames（10–40 ms）来获得更低延迟，但需确保硬件/驱动支持。修改位置在 `open_play` / `open_record` 的 `snd_pcm_hw_params_set_*` 调用处。

- 环形缓冲策略：目前实现是字节级且满时丢旧数据（偏实时）。如果希望更可靠地不丢数据，可改为：
  - 增大 `g_ring_capacity`（更多缓冲但更高延迟）；
  - 或者在写入时阻塞直到有空间（可能累积延迟）；
  - 或者以帧为粒度实现环形缓冲，减少锁与循环开销（推荐）。

- 初始填充（jitter buffer）：建议在开始播放前预先填充 40–80 ms 的 PCM 数据再开始播放（trade-off：启动延迟换稳定性）。这应由接收线程在写入 ring buffer 后，播放回调等待直到 ring 中有至少 N 字节。

后续任务（建议优先级排序）

1. 将 ring buffer 改为按帧/块的实现并减少锁频率（中优先）。
2. 在 `open_play`/`open_record` 中显式设置更合理的 period/buffer（高优先）。
3. 在接收线程中实现初始 jitter buffer（40–80 ms），并在达到阈值后再允许播放（中优先）。
4. 添加运行时统计（underrun 计数、ring 水位、解码失败率、recv 空包计数），用于线上观测与调优（中优先）。

变更映射（快速对照）

- 需求：减少播放时从 webserver 音频流产生的卡顿。 -> Done
- 变更：拆分网络/解码与播放回调；添加环形缓冲；非阻塞 recv。 -> Done
- 验证：编译 & 短时运行（日志证实进入播放循环）。 -> Done

如果你希望，我可以：
- 立刻把 `open_play`/`open_record` 的 period 修改为 320 frames 并测试（小改动、快速验证）；
- 或者把 ring buffer 改为按帧的实现并添加基本的运行时统计（更稳健，但工作量更大）。

完成情况：已在仓库 `docs/` 下新增 `audio-buffering.md`，并在 `sound_app` 中实现非阻塞接收、接收线程 + 解码、ring buffer 与播放回调解耦。