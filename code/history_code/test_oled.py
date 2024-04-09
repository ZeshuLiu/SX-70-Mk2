from machine import Pin, SoftI2C  # 导入Pin和软I2C模块
from time import sleep  # 导入时间模块
import ssd1306  # 导入屏幕驱动模块
import sys
 
# 创建i2c对象
i2c = SoftI2C(scl=Pin(19), sda=Pin(18))  # 时钟接Pin22，数据接Pin21
 
# 宽度高度，屏幕宽高为128*64 像素
oled_width = 128  
oled_height = 64
 
# 创建oled屏幕对象
oled = ssd1306.SSD1306_I2C(oled_width, oled_height, i2c,60)  # 设置宽度，高度和I2C通信
 
# 在指定位置处显示文字
oled.text('Shanghai!', 0, 0)  # 在屏幕的左上角开始显示
oled.text('Beijing welcome!', 0, 15) 
oled.text('Guangzhou beautiful!', 0, 25)
        
oled.show()  # 显示文字
print(sys.platform)