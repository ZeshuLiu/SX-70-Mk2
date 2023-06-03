from machine import Pin,ADC
import time

_CAMERA_DBG_ = True
S1F_PIN = 2 # Sw 1 Focus
LED_B_PIN = 13
LED_Y_PIN = 12
LED_B = Pin(LED_B_PIN, Pin.OUT,value = 0)
LED_Y = Pin(LED_Y_PIN, Pin.OUT,value = 1)
adc_de_bonuce_count = 70
Red_Button_Pressed = 1
if_focused = False
de_bounce_time = 1
de_bounce_count = 10
ADC_STAGE1_PIN = 27
ADC_STAGE2_PIN = 28
s1f = Pin(S1F_PIN,Pin.IN)
ADC0 = ADC(Pin(ADC_STAGE1_PIN))  # 初始化ADC
ADC1 = ADC(Pin(ADC_STAGE2_PIN))  # 初始化ADC
if_focused = False


def de_bounce_read_pins(pin_list):
    global de_bounce_time,de_bounce_count
    pin_value = [0]*len(pin_list)
    for i in range(de_bounce_count):
        for j in range(len(pin_list)):
            pin_value[j] += pin_list[j].value()
        time.sleep_ms(de_bounce_time)
    for i in range(len(pin_value)):
        pin_value[i] = round(pin_value[i]/de_bounce_count)
        if i >1:
            i = 1
    return pin_value

def meter_cal():
    read_voltage0 = 0
    read_voltage1 = 0
    for i in range(adc_de_bonuce_count):
        read_voltage0 += ADC0.read_u16()*3000/65535   # 读取ADC通道0的数值并根据ADC电压计算公式得到GPIO26引脚上的电压
        read_voltage1 += ADC1.read_u16()*3000/65535   # 读取ADC通道0的数值并根据ADC电压计算公式得到GPIO26引脚上的电压
        time.sleep_ms(1)
    read_voltage0 = read_voltage0/adc_de_bonuce_count
    read_voltage1 = read_voltage1/adc_de_bonuce_count
    if _CAMERA_DBG_:
        print("1st stage voltage = {0:.2f}mV \t\t  2nd stage voltage = {1:.2f}mV \t\t".format(read_voltage0, read_voltage1))
    
    f = open('meter_cal.txt','a')
    f.write("{0:.2f};{1:.2f}\n".format(read_voltage0, read_voltage1))
    f.close()
    
        
while True:
    foc = de_bounce_read_pins([s1f])[0]
    if foc == Red_Button_Pressed and if_focused == False:
        LED_B.value(1)
        #S1F_FBW.value(1);
        meter_cal()
        if_focused = True
        print("Focusing!")
        
    if foc != Red_Button_Pressed and if_focused == True:
        #S1F_FBW.value(0);
        if_focused = False
        LED_B.value(0)
        print("Stop Focus")
    
    

