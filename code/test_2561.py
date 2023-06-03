import tsl2561
from machine import I2C, Pin
i2c = I2C(0,scl=Pin(21), sda=Pin(20), freq=400000)
sensor = tsl2561.TSL2561(i2c,41)
print(i2c.scan())
print(sensor.read())