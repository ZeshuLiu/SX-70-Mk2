from machine import Pin,ADC
import utime


ADC0 = ADC(Pin(27))  # 通过GPIO27初始化ADC
ADC1 = ADC(Pin(28))  # 通过GPIO28初始化ADC
sensor_temp = ADC(4) # 指定初始化ADC通道4，其对应片内温度传感器
LED0 = Pin(12,Pin.OUT)
LED1 = Pin(13,Pin.OUT)
led_value_ = 1
de_bounce = 1000
cnt = 0
read_voltage0 = 0
read_voltage1 = 0
while True:
    
    LED0.value(led_value_)
    LED1.value(not led_value_)
    led_value_ = not led_value_
    read_voltage0 += ADC0.read_u16()*3300/65535   # 读取ADC通道0的数值并根据ADC电压计算公式得到GPIO26引脚上的电压
    read_voltage1 += ADC1.read_u16()*3300/65535   # 读取ADC通道0的数值并根据ADC电压计算公式得到GPIO26引脚上的电压
    read_temp_voltage = sensor_temp.read_u16()*3/65535    # 计算出ADC通道4上的电压
    temperature = 27 - (read_temp_voltage - 0.706)/0.001721   # 温度计算公式，即可计算出当前温度
    if cnt == de_bounce:
        cnt = 0
        print("ADC0 voltage = {0:.2f}mV \t\t  ADC1 voltage = {1:.2f}mV \t\t  temperature = {2:.2f}℃ \r\n".format(read_voltage0/de_bounce, read_voltage1/de_bounce, temperature))    # 将GPIO27\28上的电压输出到控制台，将当前温度输出到控制台
        read_voltage0 = 0
        read_voltage1 = 0
    utime.sleep_ms(1)
    cnt += 1
