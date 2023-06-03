import pcf8575
from machine import I2C, Pin

# TinyPICO (ESP32)
i2c = I2C(1,scl=Pin(19), sda=Pin(18))
pcf = pcf8575.PCF8575(i2c, 0x20)

# read pin 2
print(pcf.pin(2))

# set pin 3 HIGH
pcf.pin(3, 1)

# set pin 4 LOW
pcf.pin(4, 0)
# toggle pin 5
pcf.toggle(5)

# set all pins at once with 16-bit int
pcf.port = 0xffff

# read all pins at once as 16-bit int
print(pcf.port)