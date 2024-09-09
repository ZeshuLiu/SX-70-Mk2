import pcf8575,ssd1306
from machine import I2C, Pin



# TinyPICO (ESP32)
i2c = I2C(1,scl=Pin(19), sda=Pin(18),freq=400000)
pcf = pcf8575.PCF8575(i2c, 0x20)

def plug():
    a = i2c.readfrom_mem(32, 0x00, 2)
    one = "%8d" %int(bin(a[0])[2:])
    two = "%8d" %int(bin(a[1])[2:])
    one = one.replace(" ","0")
    return one+two

# read pin 2
print(pcf.pin(2))

# set pin 3 HIGH
pcf.pin(3, 1)

# set pin 4 LOW
pcf.pin(4, 0)
# toggle pin 5
pcf.toggle(5)

# set all pins at once with 16-bit int
pcf.port = 0x0ff

# read all pins at once as 16-bit int
print(pcf.port)

#基本定义
EC_ZERO = '1111'
EC_ONE = '0111'
EC_TWO = '1101'
EC_THREE = '0101'
EC_FOUR = '1110'
EC_FIVE = '0110'
EC_SIX = '1100'
EC_SEVEN = '0100'
EC_EIGHT = '1011'
EC_NINE = '0011'
EC_A = '1001'
EC_B = '0001'
EC_C = '1010'
EC_D = '0010'
EC_E = '1000'
EC_F = '0000'

# 宽度高度，屏幕宽高为128*64 像素
oled_width = 128  
oled_height = 64
 
# 创建oled屏幕对象
display = ssd1306.SSD1306_I2C(128, 32, i2c) # 设置宽度，高度和I2C通信
display.contrast(0)
#plug()


display.fill(0)
display.fill_rect(0, 0, 128, 11, 1)
display.vline(51, 0, 11, 0)
display.vline(82, 0, 11, 0)
#plug()
display.text('AUTO', 10, 2, 0)
display.text('OFF', 94, 2,0)
display.text('600', 55, 2,0)
display.show()
