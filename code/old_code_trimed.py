from machine import Pin, I2C, PWM, ADC
import time


def shut(apture, f="1"):
    sht.duty_u16(65535)
    time.sleep_ms(18)
    sht.duty_u16(10000)
    motor.value(1)
    while True:
        if s3.value() == 1:
            motor.value(0)
            break

    sht.duty_u16(0)
    if f == '0':
        gap=int(apture*0.3)
        time.sleep_ms(gap)
        fl.value(1)
        time.sleep_ms(gap)
        fl.value(0)
        time.sleep_ms(gap)
    elif f == '1':
        time.sleep_ms(apture)
    elif f == 'B':
        time.sleep_ms(15)
        while True:
            time.sleep_ms(3)
            if btn.value() == 1:
                break
    elif f == 'T':
        time.sleep(100)
        while True:
            time.sleep(3)
            if btn.value() == 0:
                break
        
    sht.duty_u16(65535)
    time.sleep_ms(20)
    sht.duty_u16(10000)
    motor.value(1)
    while True:
        if s5.value() == 0:
            motor.value(0)
            sht.duty_u16(0)
            break

    
def lux(iso=1):
    h = i2c.readfrom_mem(74, 0x03, 1)
    l = i2c.readfrom_mem(74, 0x04, 1)
    luxb = bin(((h[0] << 4) | l[0]) | 4096)
    expo = 8*int(luxb[3]) + 4*int(luxb[4]) + 2*int(luxb[5]) + int(luxb[6])
    manti = 128*int(luxb[7]) + 64*int(luxb[8]) + 32*int(luxb[9]) + 16*int(luxb[10]) + 8*int(luxb[11]) + 4*int(luxb[12]) + 2*int(luxb[13]) + int(luxb[14])
    lux = 2**expo * manti * 0.045
    if iso == 0:
        if 2.5<lux<=3.52:
            return ev75
        elif 3.52<lux<=4.99:
            return ev8
        elif 4.99<lux<=7.12:
            return ev85
        elif 7.12<lux<=10.13:
            return ev9
        elif 10.13<lux<=14.21:
            return ev95
        elif 14.21<lux<=19.79:
            return ev10
        elif 19.79<lux<=27.62:
            return ev105
        elif 27.62<lux<=39.01:
            return ev11
        elif 39.01<lux<=55.48:
            return ev115
        elif 55.48<lux<=78.07:
            return ev12
        elif 78.07<lux<=105.48:
            return ev125
        elif 105.48<lux<=150:
            return ev135
        elif 150<lux<=180:
            return ev145
        elif 180<lux<=200:
            return ev15
        elif 200<lux:
            return ev16
        else:
            return ev7
    else:
        if 0.3<lux<=0.5: 
            return ev75
        elif 0.5<lux<=0.9:
            return ev8
        elif 0.9<lux<=1.2:
            return ev85
        elif 1.2<lux<=1.74:
            return ev9
        elif 1.74<lux<=2.5:
            return ev95
        elif 2.5<lux<=3.52:
            return ev10
        elif 3.52<lux<=4.99:
            return ev105
        elif 4.99<lux<=7.12:
            return ev11
        elif 7.12<lux<=10.13:
            return ev115
        elif 10.13<lux<=14.21:
            return ev12
        elif 14.21<lux<=19.79:
            return ev125
        elif 19.79<lux<=27.62:
            return ev13
        elif 27.62<lux<=39.01:
            return ev135
        elif 39.01<lux<=55.48:
            return ev14
        elif 55.48<lux<=78.07:
            return ev145
        elif 78.07<lux<=105.48:
            return ev15
        elif 105.48<lux:
            return ev16
        else:
            return ev7


#基本定义
ev7 = 600
ev75 = 440
ev8 = 280
ev85 =210
ev9 = 155
ev95 =120
ev10 = 97
ev105 = 70
ev11 = 48
ev115 = 40
ev12 = 33
ev125 = 29
ev13 = 25
ev135 = 23
ev14 = 21
ev145 = 20
ev15 = 19
ev16 = 18

#基本初始化
s8 = Pin(17, Pin.IN)
s9 = Pin(16, Pin.IN)
motor = Pin(15,Pin.OUT, value=0)
s3 = Pin(14, Pin.IN, Pin.PULL_UP)
s5 = Pin(13, Pin.IN, Pin.PULL_UP)
btn = Pin(18, Pin.IN, Pin.PULL_UP)
fl = Pin(22, Pin.OUT)
sht = PWM(Pin(4))
sht.freq(20000)
sht.duty_u16(0)
apt = Pin(19, Pin.OUT, value=0)
while True:
    enc = lux()
    for i in range(300):
        if btn.value()==0:
            shut(enc)

