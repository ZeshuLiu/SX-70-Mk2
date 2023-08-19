# OPSX-Sonnar
原版相机（不支持声纳对焦）[OPSX 普通版项目](https://github.com/sunyitong/OPSX)\
开源的宝丽来sx-70核心板，可以支持带有声呐对焦的设备。

本项目内容为宝丽来sx-70开发，使用rp2040芯片作为控制器，具有自动测光功能，配备机身控制器后可以实现完全手动曝光控制，且支持长曝光。

本项目仓库的结构：

1. [opsx_x](https://github.com/LiuZSChina/OPSX-Sonnar/tree/main/opsx_x)——文件夹内是新设计的适用于声呐对焦的sx70核心板，使用KiCad绘制。
2. [code](https://github.com/LiuZSChina/OPSX-Sonnar/tree/main/code)——所有的代码。
3. [Flash_Plug](https://github.com/LiuZSChina/OPSX-Sonnar/tree/main/Flash_Plug)——2.5mm和3.5mm闪光灯连接线的转接板。
4. [Body_controller](https://github.com/LiuZSChina/OPSX-Sonnar/tree/main/Body_controller) ——固定在机身上的外接控制板。
5. [Fabrication](https://github.com/LiuZSChina/OPSX-Sonnar/tree/main/Fabrication) ——制造所需要的文件，均为各个组件的最新版本。

## 目录

- [OPSX](#opsx)
  - [硬件](#hardware硬件)
  - [制作](#制造)
  - [软件](#软件)
  - [上传代码](#上传代码)
  - [License](#license)
  - [版本记录](#版本记录)


## 硬件
> 提示：首先你需要一些基本的工具：电烙铁、撬棒和镊子。\
其次大多数sx70相机的螺丝都是非常特殊的1mm x 1mm 宝丽来螺丝，所以你需要使用特殊的螺丝刀才能拆卸，买一把或者自己磨一个都可以。\
在操作之前推荐先浏览以下内容[Disassembly Guide](https://instantphotography.files.wordpress.com/2010/12/polaroid-sx-70-camera-repair-book.pdf) 或者 [SX70 camera 125ASA to 600ASA conversion](https://opensx70.com/tutorials/100-600-conversion/).

### 制造
需要使用 60mm 长度 1mm间距 6p fpc软排线连接主板和机身控制器。

--->等待详细步骤<---

## 软件
Sonar版本的部分代码源自[OPSX 普通版项目](https://github.com/sunyitong/OPSX)。

### 上传代码
把板子上的U+ U- GND VCC焊到usb线上，连接电脑后可以当作树莓派Pico开发板进行操作，具体上传的方法在[这里（树莓派官网）](https://www.raspberrypi.com/documentation/microcontrollers/rp2040.html#raspberry-pi-pico)，或者使用搜索引擎搜索树莓派pico使用方法。

## License
Any distribution or modification based on this project should be clearly attributed to the source.
[GPL 3.0](LICENSE)

## 版本记录
### v0.1 - 初次发布
#### 功能情况:
- 机身控制板工作正常
- 闪光灯正常
- 手动曝光正常

#### 存在问题：
- 自动曝光存在1/3 档偏差

