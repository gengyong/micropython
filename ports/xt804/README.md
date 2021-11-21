# [中文说明]

## 如何使用

### 使用预置资源

```python
import board
# 板子上有三盏灯
# 开灯
board.led0.on()
board.led1.on()
board.led2.on()
# 关灯
board.led0.off()
board.led1.off()
board.led2.off()
```

### GPIO

```python
import machine
# PB0 脚, 输出, 默认高电平
pb0 = machine.Pin('B0', machine.Pin.OUT, value=1)
# 输出低电平
pb0.off()

# PA0 脚, 输入
pa0 = machine.Pin('A0', machine.Pin.IN)
# 读取
pa0.value()
```

Pin 名称 'A0', 'A1'..., B0', 'B1'...
 不区分大小写, 也就是 'a0', 'b0' 同样工作。
另外 Pin 名称映射到序号，从0 - 31对应A0 - A31, 32 - 63对应 B0 - B31。
注意：有些针脚实际并不存在，使用这些不存在的针脚时会抛出异常。

所以，以下代码，操作的是同一个 Pin：

```python
pb0 = machine.Pin('B0', machine.Pin.OUT)
pb0 = machine.Pin('b0', machine.Pin.OUT)
pb0 = machine.Pin('32', machine.Pin.OUT)
```

### xt804 模块

```python
import xt804
# 打开彩色输出（需终端支持）
xt804.color_print(1)
# 关闭彩色输出
xt804.color_print(0)
```

其它模块参考 micropython 官网文档，接口会尽量保持与 esp32 port 版本一致。

## 配置本地 Toolchain 和 SDK 路径
因为某些原因，工具链和SDK需要本地设置。
工具链到 https://occ.t-head.cn/community/download?id=3885366095506644992 下载。

注意有四个库版本，1) glibc 2) uclibc 3) minilibc 4) newlib。

当前只能使用 minilibc 这个版本，其他库版本暂时有很多问题。
minilibc 也是平头哥 CDK IDE 在用的库。

此时最新版本是20210423，你看到的时候可能已经更新，文件名末尾版本号会有不同。
有三个包， 对应不同开发平台：
64位 Linux: csky-elfabiv2-tools-x86_64-minilibc-20210423.tar
32位 Linux: csky-elfabiv2-tools-i386-minilibc-20210423
MinGW 环境：csky-elfabiv2-tools-mingw-minilibc-20210423

SDK 到 https://h.hlktech.com/Mobile/download/fdetail/143.html 下载。
在"通用软件"页面下，下载 WM_SDK_W806(.rar)。

有同学在 github 上为此 SDK 建了repo, 也可以从此处克隆。
感谢 IOSetting 同学！
https://github.com/IOsetting/wm-sdk-w806

工具链和SDK下载好之后，解压到指定目录，然后修改 Makefile 配置工具链和SDK路径。

Makefile 在 ports/xt804 下。

Makefile 文件前部， 修改这两处：
1) ToolChainPath: 工具链根目录
2）SDKPath: SDK 根目录
其它项不用修改，ToolChainName 跟工具链库版本有关，minilibc库指定 csky-elfabiv2。

另外，产出 rom 文件时会调用crc32以打印rom的 CRC32 值，这个不是必须的。
如果需要的话，linux下可以安装 libarchive-zip-perl_1.60-1_all。

## 编译
命令行进入 ports/xt804目录下:

    $ cd ports/xt804

第一次编译时，先执行以下命令，下载依赖模块：

    $ make submodules


然后， 执行下边命令编译并生成最终rom文件：

    $ make

生成的文件在 ports/xt804/build 目录下, 名为 firmware.unsign.fls, 用 Upgrader tool 刷到板上就好。

如果要签名的fls文件，make 时加上参数 sign=1, 如下：

    $make sign=1

生成的文件名为 firmware_sign.fls。

最终看到的编译输出如下：

    firmware        [UNENCRYPT][UNSIGNED][UNZIPPED]
    *  build/firmware.bin(234420)
    -> build/firmware.img(234484)
    -> build/firmware_unsign.fls(266128)
    141aa9d0        build/firmware.elf
    6225f8ed        build/firmware.bin
    77c293ac        build/firmware.img
    92ab06a6        ./sdk/WM_SDK_W806/tools/W806/W806_secboot.img
    ae0214ea        build/firmware_unsign.fls

## 进入 REPL
W806 没有网络，只能通过串口连接。
打开任意串口工具(比如putty), 波特率 115200，数据位8，停止位1, 连接即可。


## 项目当前进度
python 跑了起来，正在移植各种模块及硬件器件。

[发布](https://github.com/gengyong/micropython/releases)里提供了编译好的rom供尝鲜，现在除了做算术题啥也不是，可以下载试试。

# The xt804 port

This port is intended to be a minimal MicroPython port that actually runs.
It can run under Linux (or similar) and on any STM32F4xx MCU (eg the pyboard).

## Building and running Linux version

When you first time running build:

    $ make submodules


By default the port will be built for the host machine:

    $ make


## Building for an xt804 MCU

The Makefile has the ability to build for a Cortex-M CPU, and by default
includes some start-up code for an STM32F4xx MCU and also enables a UART
for communication.  To build:

    $ make

If you previously built the Linux version, you will need to first run
`make clean` to get rid of incompatible object files.







