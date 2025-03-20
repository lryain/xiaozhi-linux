set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR arm)

set(tools "/home/ubuntu/100ask_imx6ull-sdk/Buildroot_2020.02.x/output/host")
set(CMAKE_C_COMPILER ${tools}/bin/arm-buildroot-linux-gnueabihf-gcc)
set(CMAKE_CXX_COMPILER ${tools}/bin/arm-buildroot-linux-gnueabihf-g++)
