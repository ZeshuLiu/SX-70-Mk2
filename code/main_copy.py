from machine import Pin,PWM,ADC,I2C
import tsl2561,time

# Change Shutter_Delay_Back
# Change Motor Sequence S5


_CAMERA_DBG_ = True
Red_Button_Pressed = 1
if_focused = False
flash_connected = False
de_bounce_time = 1
de_bounce_count = 10
adc_de_bonuce_count = 70
# Code Start at "#End of Config"

# Config Output Pins <See Pins.xls>
shutter_pin = 9
apture_pin = 17
motor_pin = 5
LED_Y_PIN = 12
LED_B_PIN = 13
S1F_FBW_PIN = 22
FF_PIN = 18 # Flash Pin High to trigger
# End Config Output

# Config Input Pins <See Pins.xls>
S1F_PIN = 2 # Sw 1 Focus
S1T_PIN = 1 # Sw 1 Take Photo
S2_PIN = 19 # Flash Check Pin | SCL1
S3_PIN = 7
S5_PIN = 6
ADC_STAGE1_PIN = 27
ADC_STAGE2_PIN = 28
# End Config Input Pins


# Config ShutterDelay  sht(1/n) or sht(n)s
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
# End Config ShutterDelay
#a=sht1000
#End of Config

# Inital camera
def camera_init():
    global sensor,plugI2c,plugscan,motor,shutter,apture,LED_Y,LED_B,FF,s2,s3,s5,s1t,s1f,S1F_FBW,ADC0,ADC1,iso
    lm_i2c = I2C(0,scl=Pin(21), sda=Pin(20), freq=400000)
    sensor = tsl2561.TSL2561(lm_i2c,41)
    plugI2c = I2C(1,scl=Pin(19), sda=Pin(18), freq=400000)
    tmp = plugI2c.scan()
    plugscan = bool(tmp) and len(tmp)<=5
    print(plugI2c.scan())
    
    shutter = PWM(Pin(shutter_pin))
    shutter.freq(20000)
    shutter.duty_u16(0)
    apture = PWM(Pin(apture_pin))
    apture.freq(70000)
    apture.duty_u16(0)
    
    motor = Pin(motor_pin,Pin.OUT, value=0)
    LED_Y = Pin(LED_Y_PIN, Pin.OUT,value = 1)
    LED_B = Pin(LED_B_PIN, Pin.OUT,value = 1)
    if not plugscan:
        FF = Pin(FF_PIN, Pin.OUT,value = 0)
    S1F_FBW = Pin(S1F_FBW_PIN, Pin.OUT,value = 0)
    
    if not plugscan:
        s2 = Pin(S2_PIN,Pin.IN,Pin.PULL_UP)
    s3 = Pin(S3_PIN,Pin.IN,Pin.PULL_UP)
    s5 = Pin(S5_PIN,Pin.IN,Pin.PULL_UP)
    s1f = Pin(S1F_PIN,Pin.IN)
    s1t = Pin(S1T_PIN,Pin.IN)
    
    ADC0 = ADC(Pin(ADC_STAGE1_PIN))  # 初始化ADC
    ADC1 = ADC(Pin(ADC_STAGE2_PIN))  # 初始化ADC
    
    f = open('./dat/iso.txt')
    iso = f.readline()
    iso.replace(" ","")
    f.close()
    if _CAMERA_DBG_:
        print("ISO at %s!"%iso)
    if iso == '600':
        LED_Y.value(0)
    else:
        LED_B.value(0)

def close_led():
    LED_Y.value(1)
    LED_B.value(1)
    
def led_iso():
    if iso == '600':
        LED_Y.value(0)
    else:
        LED_B.value(0)

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

def apture_engage():
    apture.duty_u16(65535)
    
def apture_disengage():
    apture.duty_u16(0)

def meter():
    sensor.gain(16)
    try:
        lux = sensor.read()
    except ValueError:
        sensor.gain(1)
        lux = sensor.read()
    #print(lux,sensor.gain())
    if _CAMERA_DBG_:
        print("Lux = {0:.2f} \t\tGain = {1:.2f} \t\t".format(lux, sensor.gain()))
    lux -= 0.4
    if iso == '600':
        if 0.120<lux<=0.125: 
            return ev75
        elif 0.125<lux<=0.1385:
            return ev8
        elif 0.1385<lux<=0.152:
            return ev85
        elif 0.152<lux<=0.185:
            return ev9
        elif 0.185<lux<=0.22:
            return ev95
        elif 0.22<lux<=0.275:
            return ev10
        elif 0.28<lux<=0.345:
            return ev105
        elif 0.345<lux<=0.468:
            return ev11
        elif 0.468<lux<=0.58:
            return ev115
        elif 0.58<lux<=0.802:
            return ev12
        elif 0.802<lux<=1.115:
            return ev125
        elif 1.115<lux<=1.6:
            return ev13
        elif 1.6<lux<=2.35:
            return ev135
        elif 2.35<lux<=3.4:
            return ev14
        elif 3.4<lux<=4.2:
            return ev145
        elif 4.2<lux<=10:#
            return ev15
        elif 10<lux:#
            return ev16
        else:
            return ev7
        
        
def shut(Shutter_Delay, f="1"):
    
    #file = open('sht_cal.txt','a')
    #file.write("{0:.2f}\n".format(Shutter_Delay))
    #file.close()
    
    if _CAMERA_DBG_:
        print("Taking Picture")
    if _CAMERA_DBG_:
        print("Shutter start to close!")
    shutter.duty_u16(65535)
    time.sleep_ms(30)
    shutter.duty_u16(30000)
    time.sleep_ms(30)
    #time.sleep_ms(3000) #Delete
    motor.value(1)
    if _CAMERA_DBG_:
        print("Motor Start Moving")
    while True:
        if s3.value() == 1:
            motor.value(0)
            if _CAMERA_DBG_:
                print("Motor Stoped!")
            break
        
    if f=='0': #Engage apture
        apture_engage() 
    time.sleep_ms(18) # Y delay
    
    shutter.duty_u16(0) #open shutter
    if _CAMERA_DBG_:
        print("Shutter Start to Open, Exposure Starts")

    # Start Exposure
    if f == '0': #Flash
        if _CAMERA_DBG_:
            print("Flash Mode!")
        FF.value(1)
        gap=int(Shutter_Delay*0.3)
        time.sleep_ms(gap)
        time.sleep_ms(gap)
        FF.value(0)
        apture_disengage() # Apture goes back
        time.sleep_ms(gap)
    if f == '1': #Normal
        time.sleep_ms(Shutter_Delay)
    if f == 'B': 
        time.sleep_ms(15)
        while True:
            time.sleep_ms(3)
            if btn.value() == 1:
                break
    if f == 'T':
        time.sleep(100)
        while True:
            time.sleep(3)
            if btn.value() == 0:
                break
    
    # End Exposure
    
    shutter.duty_u16(65535) # Close Shutter
    if _CAMERA_DBG_:
        print("Shutter Closing!")
    time.sleep_ms(30)
    shutter.duty_u16(30000) # Keep Shutter Closed
    if _CAMERA_DBG_:
        print("Shutter Closd. Exposure Finished")
    time.sleep_ms(18)
    motor.value(1)
    if _CAMERA_DBG_:
        print("Motor Working!")
    
    #time.sleep_ms(2000) #Delete
    while True:
        if s5.value() == 0:
        #if s5.value() == 1:#Delete
            motor.value(0)
            shutter.duty_u16(0)
            break
    #time.sleep_ms(3000) #Delete
        

def test_cam():
    time.sleep_ms(3800)
    LED_Y.value(1)
    shutter.duty_u16(65535)
    time.sleep_ms(3800)
    shutter.duty_u16(0)
    motor.value(1)
    LED_Y.value(0)
    
def test_cam1():
    global if_focused, CAMERA_DBG
    while True:
        # See if flash is connected
        if not plugscan:
            flash_connected = not s2.value()
        else:
            flash_connected = False
        
        # Get Redbutton Value
        dbpv = de_bounce_read_pins([s1f,s1t])
        foc = dbpv[0]
        tak = dbpv[1]
        if foc == Red_Button_Pressed and if_focused == False:
            S1F_FBW.value(1);
            if not flash_connected:
                St = meter()
                if St >= ev7:
                    LED_B.value(0)
                else:
                    LED_B.value(1)
            else:
                St = ev11
            if_focused = True
            print("Focusing!")
            if _CAMERA_DBG_:
                print(dbpv)
        if foc != Red_Button_Pressed and if_focused == True:
            S1F_FBW.value(0);
            if_focused = False
            print("Stop Focus")
            if _CAMERA_DBG_:
                print(dbpv)
        if tak == Red_Button_Pressed:
            #meter()
            LED_Y.value(1)
            LED_B.value(1)
            print("EXP Time:",str(St)," Flash:",str(flash_connected))
            if not flash_connected:
                shut(St,'1');
            else:
                shut(St,'0');
            print("Taken!")
            #led_iso()
            LED_Y.value(0)
            
            if _CAMERA_DBG_:
                print(dbpv)
            #return
    
if __name__ == "__main__":
    camera_init()
    #test_cam()
    #shut(1000)
    test_cam1()
    #apture_engage()
    #LED_Y.value(1)
    #LED_B.value(1)
    #shut(a)
    while True:
        #meter()
        #time.sleep_ms(1000)
        #test_cam1()
        #time.sleep_ms(2000)
        #shut(meter())
        break

