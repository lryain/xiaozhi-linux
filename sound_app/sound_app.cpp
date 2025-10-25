// SPDX-License-Identifier: GPL-3.0-only
/*
 * Copyright (c) 2008-2023 100askTeam : Dongshan WEI <weidongshan@100ask.net> 
 * Discourse:  https://forums.100ask.net
 */
 
/*  Copyright (C) 2008-2023 深圳百问网科技有限公司
 *  All rights reserved
 *
 * 免责声明: 百问网编写的文档, 仅供学员学习使用, 可以转发或引用(请保留作者信息),禁止用于商业用途！
 * 免责声明: 百问网编写的程序, 用于商业用途请遵循GPL许可, 百问网不承担任何后果！
 * 
 * 本程序遵循GPL V3协议, 请遵循协议
 * 百问网学习平台   : https://www.100ask.net
 * 百问网交流社区   : https://forums.100ask.net
 * 百问网官方B站    : https://space.bilibili.com/275908810
 * 本程序所用开发板 : Linux开发板
 * 百问网官方淘宝   : https://100ask.taobao.com
 * 联系我们(E-mail) : weidongshan@100ask.net
 *
 *          版权所有，盗版必究。
 *  
 * 修改历史     版本号           作者        修改内容
 *-----------------------------------------------------
 * 2025.03.20      v01         百问科技      创建文件
 *-----------------------------------------------------
 */
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <iostream>
#include <thread>
#include <alsa/asoundlib.h>
#include <pthread.h>
#include <opus/opus.h>
#include <vector>
#include <mutex>
#include <condition_variable>

#include "aplay.h"
#include "record.h"
#include "opus.h"

#include "ipc_udp.h"
#include "cfg.h"

#define BUFFER_SIZE (1024*30)  /* 上传60ms的数据,以441000的采样率,双通道,16bit,最大数据量:44100*2*2*60/1000=10584=10K, 给它3倍 */
#define OPUS_BUF_SIZE (1024*5) /* 60ms的OPUS数据, 5K足够了 */

// Global buffer to hold audio data
static char audio_buffer[BUFFER_SIZE];

static unsigned char g_opus_record_buffer[OPUS_BUF_SIZE]; /* 把录音数据编码为OPUS后存在这里 */
static unsigned char g_record_buffer[BUFFER_SIZE]; /* 读取到的录音原始数据 */
static int g_record_buffer_offset;
static int g_originalPCMDataSize;
static int g_totalPCMDataSize;

static unsigned char g_opus_play_buffer[OPUS_BUF_SIZE]; /* 把要播放的OPUS码流存在这里 */
static unsigned char g_play_buffer[BUFFER_SIZE]; /* OPUS解码后得到的PCM数据,暂存在这里 */

// Ring buffer for decoded PCM data between receiver thread and playback callback
static std::vector<unsigned char> g_ring_buffer;
static size_t g_ring_capacity = BUFFER_SIZE * 4; // make it larger to absorb jitter
static size_t g_ring_read = 0;
static size_t g_ring_write = 0;
static size_t g_ring_count = 0;
static std::mutex g_ring_mutex;
static std::condition_variable g_ring_cv;

// Forward declarations for ring buffer helpers
static void ring_write(const unsigned char* data, size_t len);
static size_t ring_read(unsigned char* out, size_t len);

static int file_number = 1;
static p_ipc_endpoint_t g_ipc_ep;

// Callback function for recording
void record_callback(unsigned char *buffer, size_t size, void *user_data) {
    int opussize = 0;
    static int cnt = 0;
    static int init = 0;

    g_totalPCMDataSize += size;    

    if (!init)
    {
        unsigned int inputSampleRate;
        unsigned int inputChannels;
        snd_pcm_format_t inputFormat;
    
        get_actual_record_settings(&inputSampleRate, &inputChannels, &inputFormat);
        init_opus_encoder(inputSampleRate, inputChannels, 60, 16000, 1);

        // 要上传60ms的数据，计算它的大小
        g_originalPCMDataSize = inputSampleRate * 60 / 1000 * inputChannels * sizeof(opus_int16);

        printf("inputSampleRate = %d, inputChannels = %d, inputFormat = %d, g_originalPCMDataSize = %d\n", inputSampleRate, inputChannels, inputFormat, g_originalPCMDataSize);

        init = 1;
    }

    // 将数据存入g_record_buffer
    memcpy(g_record_buffer + g_record_buffer_offset, buffer, size);
    g_record_buffer_offset += size;

    // 当g_record_buffer_offset大于或等于g_originalPCMDataSize时，调用pcm2opus来处理
    int i = 0;
    int pcmsize = 0;
    if (g_record_buffer_offset >= g_originalPCMDataSize) {        
        while (i < g_record_buffer_offset) {
            pcmsize = g_record_buffer_offset - i;
            if (pcmsize > g_originalPCMDataSize)
                pcmsize = g_originalPCMDataSize;
            else {
                // 如果剩余的数据不足一个Opus包，则保留下来移动到最前面
                memcpy(g_record_buffer, g_record_buffer + i, pcmsize);
                break;
            }
            pcm2opus(g_record_buffer + i, pcmsize, g_opus_record_buffer, &opussize);

            if (opussize) {
#if 0
                // 构造文件名
                char filename[20];
                snprintf(filename, sizeof(filename), "test%03d.opus", file_number);

                // 打开文件
                FILE *file = fopen(filename, "wb");
                if (file) {
                    // 写入Opus数据
                    fwrite(g_opus_record_buffer, 1, opussize, file);
                    fclose(file);
                    file_number++; // 增加文件编号
                } else {
                    fprintf(stderr, "Failed to open file %s for writing\n", filename);
                }      
#endif                      
                g_ipc_ep->send(g_ipc_ep, (const char*)g_opus_record_buffer, opussize);
            }

            i += pcmsize;
        }

        // 重置g_record_buffer_offset
        g_record_buffer_offset -= i;
    }
}

// Callback function for playing
int play_get_data_callback(unsigned char *buffer, size_t size) {
    static int init = 0;

    if (!init)
    {
        unsigned int outputSampleRate;
        unsigned int outputChannels;
        snd_pcm_format_t outputFormat;
    
        get_actual_play_settings(&outputSampleRate, &outputChannels, &outputFormat);
        init_opus_decoder(16000, 1, 60, outputSampleRate, outputChannels);

        // initialize ring buffer
        g_ring_buffer.resize(g_ring_capacity);

        init = 1;
    }

    // Try to read from ring buffer
    size_t read = ring_read(buffer, size);
    if (read < size) {
        // fill rest with silence
        memset(buffer + read, 0, size - read);
    }

    return size;
}

void handle_signal(int sig) {
    printf("Received signal %d, exiting..., g_totalPCMDataSize = %d, file_number = %d\n", sig, g_totalPCMDataSize, file_number);
}

// Helper: write PCM data into ring buffer (non-blocking, drops oldest if full)
static void ring_write(const unsigned char* data, size_t len) {
    std::unique_lock<std::mutex> lk(g_ring_mutex);
    for (size_t i = 0; i < len; ++i) {
        if (g_ring_count == g_ring_capacity) {
            // drop oldest byte to make room
            g_ring_read = (g_ring_read + 1) % g_ring_capacity;
            g_ring_count--;
        }
        g_ring_buffer[g_ring_write] = data[i];
        g_ring_write = (g_ring_write + 1) % g_ring_capacity;
        g_ring_count++;
    }
    lk.unlock();
    g_ring_cv.notify_one();
}

// Helper: read up to len bytes from ring buffer into out; returns number read
static size_t ring_read(unsigned char* out, size_t len) {
    std::unique_lock<std::mutex> lk(g_ring_mutex);
    size_t toread = std::min(len, g_ring_count);
    for (size_t i = 0; i < toread; ++i) {
        out[i] = g_ring_buffer[g_ring_read];
        g_ring_read = (g_ring_read + 1) % g_ring_capacity;
    }
    g_ring_count -= toread;
    return toread;
}

// Receiver + decoder thread: blocks on UDP recv, decodes Opus to PCM and pushes into ring buffer
static void receiver_thread_func() {
    while (1) {
        int opus_data_size = 0;
        int recv_res = g_ipc_ep->recv(g_ipc_ep, g_opus_play_buffer, OPUS_BUF_SIZE, &opus_data_size);
        if (recv_res != 0) {
            // recv error - wait a bit and retry
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }
        if (opus_data_size <= 0) {
            // no data - small sleep
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
            continue;
        }

        // decode opus to temporary buffer
        int pcm_data_size = 0;
        opus2pcm(g_opus_play_buffer, opus_data_size, g_play_buffer, &pcm_data_size);
        if (pcm_data_size > 0) {
            ring_write(g_play_buffer, pcm_data_size);
        }
    }
}

int main() {

    //signal(SIGINT, handle_signal);

    g_ipc_ep = ipc_endpoint_create_udp(AUDIO_PORT_DOWN, AUDIO_PORT_UP, NULL, NULL);
    if (!g_ipc_ep) {
        fprintf(stderr, "Failed to create IPC endpoint\n");
        return -1;
    }

    // Create a thread for recording
    pthread_t record_thread = create_record_thread(record_callback, NULL);
    if (!record_thread) {
        fprintf(stderr, "Failed to create recording thread\n");
        return -1;
    }

    // Create a thread for playing
    pthread_t play_thread = create_play_thread(play_get_data_callback, NULL);
    if (!play_thread) {
        fprintf(stderr, "Failed to create playing thread\n");
        return -1;
    }

    // Create a thread for receiver + decoder
    std::thread receiver_thread(receiver_thread_func);

    // Wait for either thread to exit
    int record_thread_status;
    int play_thread_status;

    if (pthread_join(record_thread, (void**)&record_thread_status) != 0) {
        fprintf(stderr, "Failed to join recording thread\n");
    }

    if (pthread_join(play_thread, (void**)&play_thread_status) != 0) {
        fprintf(stderr, "Failed to join playing thread\n");
    }

    receiver_thread.join(); // wait for receiver thread to finish

    // Check the exit status of the threads
    if (record_thread_status != 0 || play_thread_status != 0) {
        fprintf(stderr, "One of the threads exited with an error\n");
        return -1;
    }

    return 0;
}
