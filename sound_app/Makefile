# sudo apt-get install libopus-dev libavresample-dev libswresample-dev libspeexdsp-dev

objs := sound_app.o aplay.o record.o opus.o ipc_udp.o

app = sound_app

# 默认并行度，可以通过命令行覆盖，如 make -j4
JOBS ?= $(shell nproc 2>/dev/null || echo 2)

all: ${app}

${app}: $(objs)
	${CROSS_COMPILE}g++ -o $@ $^ -lasound -pthread -lopus -lspeexdsp

# 并行编译目标
parallel: $(objs)
	@echo "Using $(JOBS) parallel jobs"
	${MAKE} -j$(JOBS) ${app}

# 快速并行编译
fast: clean
	@echo "Fast parallel build with $(JOBS) jobs"
	${MAKE} -j$(JOBS) all


# 需要判断是否存在依赖文件
# .main.o.d .sub.o.d
dep_files := $(foreach f, $(objs), .$(f).d)
dep_files := $(wildcard $(dep_files))

# 把依赖文件包含进来
ifneq ($(dep_files),)
  include $(dep_files)
endif


%.o : %.c
	${CROSS_COMPILE}gcc -I ./ -Wp,-MD,.$@.d -g -c -o $@  $<

%.o : %.cpp
	${CROSS_COMPILE}g++ -I ./ -Wp,-MD,.$@.d -g -c -o $@  $<

clean:
	rm *.o ${app} -f

distclean:
	rm  $(dep_files) *.o ${app} -f

opus_test: opus.cpp
	g++ -DTEST -I ./ -o $@ $^ -lopus  -lspeexdsp
