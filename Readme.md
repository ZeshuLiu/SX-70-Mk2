# OPSX
An open sourced Polaroid SX-70 core board with sonnar support.\
开源的宝丽来sx-70核心板，可以支持带有声呐对焦的设备。

This is an open sourced Polaroid SX-70 instant camera core board with the Raspberry Pi RP2040 as the MCU. The aim of this project is to provide a fully resource accessible and hobbyist friendly replacement core board. It offers more extensibility and hacking ideas while implementing all the features of the original camera.\
本项目是开源的宝丽来sx-70核心板，该项目使用树莓派rp2040构建而成，本项目的目的是为爱好者提供一个可以完全自主掌控的核心板，在保留原机器各种功能的同时可以发挥自己的创意进行功能拓展。

The structure of the repository:\
本项目仓库的结构：

1. The [opsx_x](https://github.com/LiuZSChina/OPSX/tree/master/Sonnar) file contains design for sonnar sx-70.Designed using KiCad.\
[opsx_x](https://github.com/LiuZSChina/OPSX-Sonnar/tree/main/opsx_x)文件夹内是新设计的适用于声呐对焦的sx70核心板，使用KiCad绘制。
2. The [code](https://github.com/LiuZSChina/OPSX-Sonnar/tree/main/code) Code for main board.\
[code](https://github.com/LiuZSChina/OPSX-Sonnar/tree/main/code)所有的代码。
3. The [Flash_Plug](https://github.com/LiuZSChina/OPSX-Sonnar/tree/main/Flash_Plug) Flash adapter to 2.5mm and 3.5mm flash cable.\
[Flash_Plug](https://github.com/LiuZSChina/OPSX-Sonnar/tree/main/Flash_Plug)2.5mm和3.5mm闪光灯连接线的转接板。
4. The [Body_controller](https://github.com/LiuZSChina/OPSX-Sonnar/tree/main/Body_controller) Controller fixed on camera body.\
[Body_controller](https://github.com/LiuZSChina/OPSX-Sonnar/tree/main/Body_controller) 固定在机身上的外接控制板。
5. The [Fabrication](https://github.com/LiuZSChina/OPSX-Sonnar/tree/main/Fabrication) Latest files for fabrication.\
[Fabrication](https://github.com/LiuZSChina/OPSX-Sonnar/tree/main/Fabrication) 制造所需要的文件，均为各个组件的最新版本。

## Table of Contents 目录

- [OPSX](#opsx)
  - [Hardware 硬件](#hardware硬件)
    - [Fabrication 制作](#fabrication制造)
  - [Software 软件](#software)
    - [Uploading 上传代码](#uploading上传代码)
  - [License](#license)


## Hardware硬件
> Remander: You are going to need some “basic” tools:   soldering iron with a fine tip, a prying tool and a tweezer.\
Aside from the usual tools you are going to need what is called a “Polaroid screwdriver” or “SX-70 screwdriver”, since most of the cameras use a special “square 1mm x 1mm” screws. You could either buy one or fabricate one yourself.\
please read the [Disassembly Guide](https://instantphotography.files.wordpress.com/2010/12/polaroid-sx-70-camera-repair-book.pdf) or [SX70 camera 125ASA to 600ASA conversion](https://opensx70.com/tutorials/100-600-conversion/) first.\
提示：首先你需要一些基本的工具：电烙铁、撬棒和镊子。\
其次大多数sx70相机的螺丝都是非常特殊的1mm x 1mm 宝丽来螺丝，所以你需要使用特殊的螺丝刀才能拆卸，买一把或者自己磨一个都可以。\
在操作之前推荐先浏览以下内容[Disassembly Guide](https://instantphotography.files.wordpress.com/2010/12/polaroid-sx-70-camera-repair-book.pdf) 或者 [SX70 camera 125ASA to 600ASA conversion](https://opensx70.com/tutorials/100-600-conversion/).

### Fabrication制造
Please select a **dual copper layer PCB** with a thickness of **0.8 mm** or less for fabrication.\
制造时请选择 FR-4基板，双层板，厚度0.8mm或者更小（会贵）。\
You need additional FPC 1.0mm 6p to connect main board with body-controller.\
需要使用 60mm 长度 1mm间距 6p fpc软排线连接主板和机身控制器。

--->等待详细步骤<---

## Software软件
Some codes for Sonar camera is based on [OPSX original project](https://github.com/sunyitong/OPSX)\
Sonar版本的部分代码源自[OPSX 普通版项目](https://github.com/sunyitong/OPSX)。

### Uploading上传代码
Solder U+ U- GND VCC to a standard usb cable to connect to your computer. Then you can treat it as normal raspberry pico device. See [here](https://www.raspberrypi.com/documentation/microcontrollers/rp2040.html#raspberry-pi-pico) for more information about uploading.\
把板子上的U+ U- GND VCC焊到usb线上，连接电脑后可以当作树莓派Pico开发板进行操作，具体上传的方法在[这里（树莓派官网）](https://www.raspberrypi.com/documentation/microcontrollers/rp2040.html#raspberry-pi-pico)，或者使用搜索引擎搜索树莓派pico使用方法。

## License
Any distribution or modification based on this project should be clearly attributed to the source.
[GPL 3.0](LICENSE)