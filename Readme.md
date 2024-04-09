# OPSX-Sonnar

## 介绍
项目参考非声纳型号项目开发（不支持声纳对焦[OPSX 普通版项目](https://github.com/sunyitong/OPSX)），软硬件重构。
可以支持声呐版本的SX70。

## 参数
**快门速度** 1/2000s-1s，支持B门及T门 \
**自拍定时** 3s 5s 10s （可以与T门或B门同时使用减轻机器振动的影响）\
**测光** 仅支持600型相纸，自动测光下最低快门速度1/2s 

改装后机器：\
![改装后的机器](https: "改装后的机器")

## 样片
_拍摄效果受相纸质量影响，当前相纸普遍存在火焰纹，偏色等问题_

###### 自动测光：
![自动测光拍摄](https://github.com/ZeshuLiu/OPSX-Sonnar/blob/main/raw/pics/op-01-5%266.jpg "自动测光拍摄")

###### 手动曝光：
![手动曝光](https://github.com/ZeshuLiu/OPSX-Sonnar/blob/main/raw/pics/op-01-7%268.jpg "手动曝光")

###### 闪光灯：
![闪光灯](https://github.com/ZeshuLiu/OPSX-Sonnar/blob/main/raw/pics/op-01-3.jpg "闪光灯")

###### 1/1000s 快门速度：
![1/1000s](https://github.com/ZeshuLiu/OPSX-Sonnar/blob/main/raw/pics/op-01-4.jpg "1/1000s")


## 仓库结构
1. [opsx_x](https://github.com/ZeshuLiu/OPSX-Sonnar/tree/main/opsx_x)——文件夹内是新设计的适用于声呐对焦的sx70核心板，使用KiCad绘制。
2. [code](https://github.com/ZeshuLiu/OPSX-Sonnar/tree/main/code)——相机控制器代码。
3. [Flash_Plug](https://github.com/ZeshuLiu/OPSX-Sonnar/tree/main/Flash_Plug)——2.5mm和3.5mm闪光灯连接线的转接板。
4. [Body_controller](https://github.com/ZeshuLiu/OPSX-Sonnar/tree/main/Body_controller) ——固定在机身上的外接控制板。
5. [Fabrication](https://github.com/ZeshuLiu/OPSX-Sonnar/tree/main/Fabrication) —— 制造文件，均为各个组件的最新版本。
6. [PowerBack](https://github.com/ZeshuLiu/OPSX-Sonnar/tree/main/PowerBack) —— 机身使用的外接电池，外接电池时可以使用itype相纸

## 硬件制造
PCB制造时选择0.8mm以下厚度，光绘（Gerber）文件可以在[Fabrication](https://github.com/ZeshuLiu/OPSX-Sonnar/tree/main/Fabrication)文件夹中找到。需要使用 60mm 长 1mm间距的 6pin fpc软排线连接主板和机身控制器。

## 软件

把板子上的U+ U- GND VCC焊到usb线上，连接电脑后可以当作树莓派Pico开发板进行操作，具体上传的方法在[这里（树莓派官网）](https://www.raspberrypi.com/documentation/microcontrollers/rp2040.html#raspberry-pi-pico)，或者自行搜索。\
建议在组装之前将micropython固件及代码烧录至控制器中。后续可以通过机身控制器上的USB接口连接至开发板并更新python代码。
将[code](https://github.com/ZeshuLiu/OPSX-Sonnar/tree/main/code)根目录中的所有py文件上传至控制器，**并且将g_main.py命名为 main.py(非常重要，只有这样才能够上电运行)**\
建议针对自己的快门适当调整代码中快门速度的值，可以使用这个[快门速度测试器](https://github.com/ZeshuLiu/OPSX-Sonnar/tree/main/code)来便于测试自己快门的打开时间。

## 组装
> 提示：首先你需要一些基本的工具：电烙铁、撬棒和镊子。\
其次大多数sx70相机的螺丝都是非常特殊的1mm x 1mm 宝丽来螺丝，所以你需要使用特殊的螺丝刀才能拆卸，买一把或者自己磨一个都可以。\
在操作之前推荐先浏览以下内容[Disassembly Guide](https://instantphotography.files.wordpress.com/2010/12/polaroid-sx-70-camera-repair-book.pdf) 或者 [SX70 camera 125ASA to 600ASA conversion](https://opensx70.com/tutorials/100-600-conversion/).

拆卸后对原本主板进行替换即可，如需要机身控制器则将左上角焊盘焊线引出即可通过转接板连接机身控制器。焊接FPC时建议使用含铅焊锡，否则有损伤机器FPC的风险。对于有机身拆装经验的人来说难度不太高\

--->详细步骤待补充<---
### 二次开发
#### 代码部分开发：
_TODO_

#### 硬件部分开发：
_TODO_

#### 存在问题：
- 没有测试无机身控制器时工作情况


