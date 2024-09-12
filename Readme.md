# SX-70 Mk2

## 介绍
针对SX-70 Sonnar相机的现代化改造，在保留原本机身的自动功能的情况下，提供更直接的手动控制方式，能够更加自由地进行拍摄。使用测光表可以更加自由地对曝光进行控制，从而控制自己的拍摄效果。**机身控制器的存在使得手动控制更加直观、方便** 

### 项目组成

项目中包含几个部分的硬件：

1. [相机主控板](https://github.com/ZeshuLiu/SX-70-Mk2/tree/main/Hardwares/MainController)：替换相机机头内原装的控制器，接管相机的控制。
2. [机身控制器](https://github.com/ZeshuLiu/SX-70-Mk2/tree/main/Hardwares/Body_controller)：显示当前相机工作状态并提供用于交互的按键。粘贴在机身背部，不会增加相机高度，因此大部分相机包依旧可以通用。
3. [闪光灯接口](https://github.com/ZeshuLiu/SX-70-Mk2/tree/main/Hardwares/Flash_Plug)：**(还未实际测试)**，从原本的闪光灯排上引出3.5mm接口，用于外置闪光灯。
4. [机身电池](https://github.com/ZeshuLiu/SX-70-Mk2/tree/main/Hardwares/PowerBack)：**(正在开发中)**，通过相机底部的两个触点为相机供电，可以使用iType相纸进行拍摄。

### 开发

大部分印制线路板使用KiCad8进行绘制，极少部分使用其它软件，仓库均提供了相应的工程和光绘(gerber)文件。

Rev1.x - Rev2.x 版本的主控使用RP2040，通过micropython编程。由于micropython中硬件定时器最小单位为1ms，后续考虑换用C语言获得更高定时器配置精度。

### 项目使用的开源软件

1. 项目受到了[OPSX ](https://github.com/sunyitong/OPSX)和[openSX70](https://github.com/openSX70)的启发。
2. Rev1.x - Rev2.x 版本的128x32OLED屏幕代码：[micropython-adafruit-ssd1306 ](https://github.com/adafruit/micropython-adafruit-ssd1306)
3. 机身控制器按键使用的PCF8575：[micropython-pcf8575 ](https://github.com/mcauser/micropython-pcf8575)
4. Rev1.x-Rev2.x 主控使用的tsl2561光线传感器：[micropython-adafruit-tsl256](https://github.com/adafruit/micropython-adafruit-tsl2561)



## 性能参数

本项目具有若干独立的硬件，其中有一些可以单独改装，有一些则依赖于其它硬件。

### 相机主控

可以单独改装，推荐配合机身控制器使用。

**快门速度：** 1/2000s-1s，支持B门及T门 \
**自拍定时：** 3s 5s 10s （可以与T门或B门同时使用减轻机器振动的影响）\
**测光：** 目前软件仅支持600型相纸，自动测光下最低快门速度1/2s  \



## 样片
_第一个版本拍摄的第一盒相纸_

###### 自动测光：
![自动测光拍摄](https://github.com/ZeshuLiu/SX-70-Mk2/blob/main/Demos/pics/op-01-5%266.jpg "自动测光拍摄")

###### 手动曝光：
![手动曝光](https://github.com/ZeshuLiu/SX-70-Mk2/blob/main/Demos/pics/op-01-7%268.jpg "手动曝光")

###### 闪光灯：
![闪光灯](https://github.com/ZeshuLiu/SX-70-Mk2/blob/main/Demos/pics/op-01-3.jpg "闪光灯")

###### 1/1000s 快门速度：
![1/1000s](https://github.com/ZeshuLiu/SX-70-Mk2/blob/main/Demos/pics/op-01-4.jpg "1/1000s")

## 硬件制造
PCB制造时选择0.8mm以下厚度，光绘（Gerber）文件可以在[Fabrication](https://github.com/ZeshuLiu/OPSX-Sonnar/tree/main/Fabrication)文件夹中找到。需要使用 60mm 长 1mm间距的 6pin fpc软排线连接主板和机身控制器。

## 软件

把板子上的U+ U- GND VCC焊到usb线上，连接电脑后可以当作树莓派Pico开发板进行操作，具体上传的方法在[这里（树莓派官网）](https://www.raspberrypi.com/documentation/microcontrollers/rp2040.html#raspberry-pi-pico)，或者自行搜索。 \
建议在组装之前将micropython固件及代码烧录至控制器中。后续可以通过机身控制器上的USB接口连接至开发板并更新python代码。
将[code](https://github.com/ZeshuLiu/OPSX-Sonnar/tree/main/code)根目录中的所有python文件上传至控制器（文件夹不需要），**并且将g_main.py命名为 main.py(非常重要，只有这样才能够上电运行)** \
建议针对自己的快门适当调整代码中快门速度的值，可以使用这个[快门速度测试器](https://github.com/ZeshuLiu/ShutterSpeedCalibrator)来测试机器的快门速度，便于调整。

## 组装
> 提示：首先你需要一些基本的工具：电烙铁、撬棒和镊子。 \
其次大多数sx70相机的螺丝都是非常特殊的1mm x 1mm 宝丽来螺丝，所以你需要使用特殊的螺丝刀才能拆卸，买一把或者自己磨一个都可以。 \
在操作之前推荐先浏览以下内容[Disassembly Guide](https://instantphotography.files.wordpress.com/2010/12/polaroid-sx-70-camera-repair-book.pdf) 或者 [SX70 camera 125ASA to 600ASA conversion](https://opensx70.com/tutorials/100-600-conversion/).

拆卸后对原本主板进行替换即可，如需要机身控制器则将左上角焊盘焊线引出即可通过转接板连接机身控制器。焊接FPC时建议使用含铅(低温)焊锡，否则有损伤机器FPC的风险。对于有机身拆装经验的人来说难度不高。只需要将原本的拆掉并且替换上去即可使用。

