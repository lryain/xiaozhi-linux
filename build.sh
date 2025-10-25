#!/bin/bash

# 多线程编译脚本
# 用于同时编译 control_center 和 sound_app

set -e

echo "=== 多线程编译 xiaozhi-linux 项目 ==="
echo "检测到 $(nproc) 个 CPU 核心"

# 编译 control_center
echo "编译 control_center..."
cd control_center
make fast
cd ..

# 编译 sound_app
echo "编译 sound_app..."
cd sound_app
make fast
cd ..

echo "=== 编译完成 ==="
echo "可执行文件位置:"
echo "  - control_center/control_center"
echo "  - sound_app/sound_app"