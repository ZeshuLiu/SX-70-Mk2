# OPSX-Sonnar

## 目录

- [OPSX](#opsx)
  - [项目介绍](#项目介绍)
  - [仓库结构](#仓库结构)
  - [硬件](#hardware硬件)
  - [制作](#制造)
  - [软件](#软件)
  - [上传代码](#上传代码)
  - [二次开发](#二次开发)
  - [License](#license)
  - [版本记录](#版本记录)

## 项目介绍
项目参考原版相机方案开发（不支持声纳对焦[OPSX 普通版项目](https://github.com/sunyitong/OPSX)）
可以支持带有声呐对焦的设备。\
使用rp2040芯片作为控制器，具有自动测光功能，配备机身控制器后可以实现完全手动曝光控制，且支持长曝光。

目前改装相机可以正常使用，具体可用性内容：[版本记录](#版本记录)\

#### 改装后的机器：
![改装后的机器](https: "改装后的机器")\

#### 本项目改装后拍摄的图片
_因为相纸过期了，药水挤得不均匀导致有些斑块_
##### 第一盒相纸（过期黑白）：

###### 全家福
![改装后第一盒-bw](https://github.com/ZeshuLiu/OPSX-Sonnar/blob/main/raw/pics/op-01-all.jpg "改装后第一盒-bw")
_前两张黏上了导致没有吐出来_

###### 自动测光：
![自动测光拍摄](https://github.com/ZeshuLiu/OPSX-Sonnar/blob/main/raw/pics/op-01-5%266.jpg "自动测光拍摄")

###### 手动曝光：
![手动曝光](https://github.com/ZeshuLiu/OPSX-Sonnar/blob/main/raw/pics/op-01-7%268.jpg "手动曝光")

###### 闪光灯：
![闪光灯](https://github.com/ZeshuLiu/OPSX-Sonnar/blob/main/raw/pics/op-01-3.jpg "闪光灯")

###### 1/1000s 快门速度：
![1/1000s](https://github.com/ZeshuLiu/OPSX-Sonnar/blob/main/raw/pics/op-01-4.jpg "1/1000s")


## 仓库结构
1. [opsx_x](https://github.com/LiuZSChina/OPSX-Sonnar/tree/main/opsx_x)——文件夹内是新设计的适用于声呐对焦的sx70核心板，使用KiCad绘制。
2. [code](https://github.com/LiuZSChina/OPSX-Sonnar/tree/main/code)——所有的代码。
3. [Flash_Plug](https://github.com/LiuZSChina/OPSX-Sonnar/tree/main/Flash_Plug)——2.5mm和3.5mm闪光灯连接线的转接板。
4. [Body_controller](https://github.com/LiuZSChina/OPSX-Sonnar/tree/main/Body_controller) ——固定在机身上的外接控制板。
5. [Fabrication](https://github.com/LiuZSChina/OPSX-Sonnar/tree/main/Fabrication) ——制造所需要的文件，均为各个组件的最新版本。

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

### 二次开发
#### 代码部分开发：
_TODO_

#### 硬件部分开发：
_TODO_

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
- 没有测试无机身控制器时工作情况
- T门可以直接选择至多4分钟曝光时常功能未开启

