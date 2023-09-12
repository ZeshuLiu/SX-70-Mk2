"""
Ver 1.0 20230912
正常调试版本，适合组装后使用
直接命名为main.py然后上传进板子的flash里就行
"""
from machine import Pin,PWM,ADC,I2C
import tsl2561,time,ssd1306,pcf8575

# Change Shutter_Delay_Back
# Change Motor Sequence S5


_CAMERA_DBG_ = True
_SAVE_FILE_ = True
Red_Button_Pressed = 1
if_focused = False
flash_connected = False
LM_ADDR = 41

de_bounce_time = 1
de_bounce_count = 10
adc_de_bonuce_count = 70

display_addr = 60
have_disp = False
encoder_addr = 32
have_enc = False
enc_pins = [4,5,6,7]
sec_enc_pins = [0,1,2,3]
# Code Start at "#End of Config"

# Config Output Pins <See Pins.xls>
shutter_pin = 9
apture_pin = 17
motor_pin = 5
LED_Y_PIN = 12
LED_B_PIN = 13
S1F_FBW_PIN = 22
FF_PIN = 11 # Flash Pin High to trigger
# End Config Output

# Config Input Pins <See Pins.xls>
S1F_PIN = 2 # Sw 1 Focus
S1T_PIN = 1 # Sw 1 Take Photo
S2_PIN = 14 # Flash Check Pin | SCL1
S3_PIN = 7
S5_PIN = 6
# End Config Input Pins

# Define Encoder Code
EC_ZERO = '1111'
EC_ONE = '0111'
EC_TWO = '1011'
EC_THREE = '0011'
EC_FOUR = '1101'
EC_FIVE = '0101'
EC_SIX = '1001'
EC_SEVEN = '0001'
EC_EIGHT = '1110'
EC_NINE = '0110'
EC_A = '1010'
EC_B = '0010'
EC_C = '1100'
EC_D = '0100'
EC_E = '1000'
EC_F = '0000'

SEC_EC_ZERO = '1111'
SEC_EC_ONE = '0111'
SEC_EC_TWO = '1011'
SEC_EC_THREE = '0011'
SEC_EC_FOUR = '1101'
SEC_EC_FIVE = '0101'
SEC_EC_SIX = '1001'
SEC_EC_SEVEN = '0001'
SEC_EC_EIGHT = '1110'
SEC_EC_NINE = '0110'

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

ShutterSpeedHuman = {ev7:'1/2', ev75:'1/3', ev8:'1/4', ev85:'1/6', ev9:'1/8', ev95:'1/10', ev10:'1/15', ev105:'1/20', ev11:'1/30', ev115:'1/45', ev12:'1/60',
                     ev125:'1/90', ev13:'1/125', ev135:'1/180', ev14:'1/250', ev145:'1/360', ev15:'1/500', ev16:'1/1000'}
# End Config ShutterDelay
#a=sht1000
#End of Config

def read_enc(enc_p=enc_pins):
    result = ""
    for i in enc_p:
        result += str(pcf.pin(i))
    #print(result,pcf.pin(1))
    return result

def plug_encoder():
    ec = read_enc(enc_pins)
    #ec = EC_ZERO #delete
    #print(ec)
    if ec == EC_ZERO:
        display.text('AUTO', 10, 2, 0)
        return 'A'
    elif ec == EC_ONE:
        display.text('B', 23, 2, 0)
        return 'B'
    elif ec == EC_TWO:
        # display.text('T', 23, 2, 0)
        return read_sec_plug_enc()
    
    elif ec == EC_THREE:
        display.text('1/2', 15, 2, 0)
        return ev7
    elif ec == EC_FOUR:
        display.text('1/4', 15, 2, 0)
        return ev8
    elif ec == EC_FIVE:
        display.text('1/6', 15, 2, 0)
        return ev85
    elif ec == EC_SIX:
        display.text('1/8', 15, 2, 0)
        return ev9
    elif ec == EC_SEVEN:
        display.text('1/10', 10, 2, 0)
        return ev95
    elif ec == EC_EIGHT:
        display.text('1/15', 10, 2, 0)
        return ev10
    elif ec == EC_NINE:
        display.text('1/20', 10, 2, 0)
        return ev105
    elif ec == EC_A:
        display.text('1/30', 10, 2, 0)
        return ev11
    elif ec == EC_B:
        display.text('1/60', 10, 2, 0)
        return ev12
    elif ec == EC_C:
        display.text('1/125', 5, 2, 0)
        return ev13
    elif ec == EC_D:
        display.text('1/250', 5, 2, 0)
        return ev14
    elif ec == EC_E:
        display.text('1/500', 5, 2, 0)
        return ev15
    elif ec == EC_F:
        display.text('1/1000', 1, 2, 0)
        return ev16

# todo 
def read_sec_plug_enc():
    ec = read_enc(sec_enc_pins)
    # print('Sec Enc:', ec)
    one_sec = 1000
    one_min = one_sec*60
    if ec == SEC_EC_ZERO:
        display.text('T', 10, 2, 0)
        return 'T'
    elif ec == SEC_EC_ONE:
        display.text('T1S', 23, 2, 0)
        return 1*one_sec
    elif ec == SEC_EC_TWO:
        display.text('T2S', 23, 2, 0)
        return 2*one_sec
    elif ec == SEC_EC_THREE:
        display.text('T4S', 15, 2, 0)
        return 4*one_sec
    elif ec == SEC_EC_FOUR:
        display.text('T8S', 15, 2, 0)
        return 8*one_sec
    elif ec == SEC_EC_FIVE:
        display.text('T15S', 15, 2, 0)
        return 15*one_sec
    elif ec == SEC_EC_SIX:
        display.text('T30S', 15, 2, 0)
        return 30*one_sec
    elif ec == SEC_EC_SEVEN:
        display.text('T1M', 10, 2, 0)
        return 1*one_min
    elif ec == SEC_EC_EIGHT:
        display.text('T2M', 10, 2, 0)
        return 2*one_min
    elif ec == SEC_EC_NINE:
        display.text('T4M', 10, 2, 0)
        return 4*one_min
def showFrame():
    display.fill(0)
    display.fill_rect(0, 0, 128, 11, 1)
    display.vline(51, 0, 11, 0)
    display.vline(82, 0, 11, 0)
    
def meter():
    sensor.gain(16)
    try:
        lux = sensor.read()
    except ValueError:
        sensor.gain(1)
        lux = sensor.read()
    #print(lux,sensor.gain())
    if _CAMERA_DBG_:
        print("Raw Lux = {0:.2f} \t\tGain = {1:.2f} \t\t".format(lux, sensor.gain()))

    if _SAVE_FILE_:
        da_F = open('sht.txt','a')
        da_F.write("Raw Lux = {0:.2f} \tGain = {1:.2f} \n".format(lux, sensor.gain()))
        da_F.close()

    lux -= 0.4
    #* Magic Number
    lux *= 0.9
    if iso == '600':
        if 0.120<lux<=0.125: 
            return ev75,lux
        elif 0.125<lux<=0.1385:
            return ev8,lux
        elif 0.1385<lux<=0.152:
            return ev85,lux
        elif 0.152<lux<=0.185:
            return ev9,lux
        elif 0.185<lux<=0.22:
            return ev95,lux
        elif 0.22<lux<=0.275:
            return ev10,lux
        elif 0.28<lux<=0.345:
            return ev105,lux
        elif 0.345<lux<=0.468:
            return ev11,lux
        elif 0.468<lux<=0.58:
            return ev115,lux
        elif 0.58<lux<=0.802:
            return ev12,lux
        elif 0.802<lux<=1.115:
            return ev125,lux
        elif 1.115<lux<=1.6:
            return ev13,lux
        elif 1.6<lux<=2.35:
            return ev135,lux
        elif 2.35<lux<=3.4:
            return ev14,lux
        elif 3.4<lux<=4.2:
            return ev145,lux
        elif 4.2<lux<=10:#
            return ev15,lux
        elif 10<lux:#
            return ev16,lux
        else:
            return ev7,lux

# Inital camera
def camera_init():
    global sensor,plugI2c,plugscan,pcf,display,have_disp,have_enc,motor,shutter,apture,LED_Y,LED_B,FF,s2,s3,s5,s1t,s1f,S1F_FBW,ADC0,ADC1,iso
    lm_i2c = I2C(0,scl=Pin(21), sda=Pin(20), freq=400000)
    sensor = tsl2561.TSL2561(lm_i2c,41)
    plugI2c = I2C(1,scl=Pin(19), sda=Pin(18), freq=400000)
    
    plug_i2c_add = plugI2c.scan()
    plugscan = bool(plug_i2c_add) and len(plug_i2c_add)<=5
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
    FF = Pin(FF_PIN, Pin.OUT,value = 0)
    S1F_FBW = Pin(S1F_FBW_PIN, Pin.OUT,value = 0)
    
    s2 = Pin(S2_PIN,Pin.IN,Pin.PULL_UP)
    s3 = Pin(S3_PIN,Pin.IN,Pin.PULL_UP)
    s5 = Pin(S5_PIN,Pin.IN,Pin.PULL_UP)
    s1f = Pin(S1F_PIN,Pin.IN)
    s1t = Pin(S1T_PIN,Pin.IN)
    
    f = open('./dat/iso.txt')
    iso = f.readline()
    iso.replace(" ","")
    f.close()
    if _CAMERA_DBG_:
        print("ISO at %s!"%iso)
    if iso == '600':
        LED_Y.value(0)
        #display.text('600', 55, 2, 0)
    else:
        LED_B.value(0)
        #display.text('70', 60, 2, 0)
    
    if plugscan and encoder_addr in plug_i2c_add:
        pcf = pcf8575.PCF8575(plugI2c, encoder_addr)
        pcf.port = 0x0ff
        print("have plug")
        have_enc = True
        
    if plugscan and display_addr in plug_i2c_add:
        
        display = ssd1306.SSD1306_I2C(128, 32, plugI2c,addr=display_addr) # 设置宽度，高度和I2C通信
        have_disp = True
        display.contrast(1)
        print("have disp")
        showFrame()
        display.text('OFF', 94, 2, 0)
        if iso == '600':
            display.text('600', 55, 2, 0)
        else:
            display.text('70', 60, 2, 0)
        
        if have_enc:
            """
            enc = plug_encoder()
            if True or enc == 'A':
                shut_speed,raw = meter()                 
                display.text(str(ShutterSpeedHuman[shut_speed])+'s', 8, 23)
                if len(str(raw))>7:
                    raw = str(raw)[0:7]
                display.text(str(raw), 64, 23)
            elif enc == 'B':
                func = 'B'
            elif enc == 'T':
                func = 'T'
            """
            pass
        else:
            if not have_enc:
                display.text('Auto', 10, 2, 0)
            shut_speed,raw = meter()                 
            display.text(str(ShutterSpeedHuman[shut_speed])+'s', 8, 23)
            if len(str(raw))>7:
                raw = str(raw)[0:7]
            display.text(str(raw), 64, 23)
        display.show()
        

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
        
        
def shut(Shutter_Delay, f="1"):
    
    #f = open('sht_cal.txt','a')
    #f.write("{0:.2f}\n".format(Shutter_Delay))
    #f.close()
    if _SAVE_FILE_:
        da_F = open('sht.txt','a')
        da_F.write("Mode: " + f)
        da_F.write(" Shutter Delay = {0:.2f}\n\n".format(Shutter_Delay))
        da_F.close()
    
    if _CAMERA_DBG_:
        print("Taking Picture")
        print("Shutter start to close!")
    # Shutter Close
    shutter.duty_u16(65535)
    time.sleep_ms(30)
    shutter.duty_u16(30000)
    if _CAMERA_DBG_:
        print("Shutter closed!")

    time.sleep_ms(30) # wait for shutter to fully close
    #time.sleep_ms(3000) #Delete

    # Motor starts moving
    motor.value(1)
    if _CAMERA_DBG_:
        print("Motor Start Moving")

    # Move until mirror reach correct position
    while True:
        if s3.value() == 1:
            motor.value(0)
            if _CAMERA_DBG_:
                print("Motor Stoped!")
            break
        
    if f=='0': #Engage apture 
        apture_engage()
        if _CAMERA_DBG_:
            print("apture_engage")
    
    time.sleep_ms(18) # Y delay
    
    shutter.duty_u16(0) #open shutter
    if _CAMERA_DBG_:
        print("Shutter Start to Open, Exposure Starts")

    # Start Exposure
    if f == '0': #Flash
        if _CAMERA_DBG_:
            print("Flash Mode!")
        gap = int(Shutter_Delay- 47)
        if gap < 0:
            gap = 0
        time.sleep_ms(47)
        FF.value(1)
        apture_disengage() # Apture goes back
        FF.value(0)
        time.sleep_ms(gap)
    
    elif f == '1': #Normal
        if _CAMERA_DBG_:
            print("Normal Mode!")
        start = time.ticks_ms()
        if _CAMERA_DBG_:
            print("Start at", start)

        while time.ticks_ms() - start < Shutter_Delay:
            continue
        
        # time.sleep_ms(Shutter_Delay)
        if _CAMERA_DBG_:
            print("End at", time.ticks_ms(), "Dura", time.ticks_ms()-start)
    
    elif f == 'B': #B mode
        if _CAMERA_DBG_:
            print("B Mode!")
        time.sleep_ms(15)
        while True:
            dbpv = de_bounce_read_pins([s1f,s1t])
            foc = dbpv[0]
            tak = dbpv[1]
            time.sleep_ms(3)
            if tak != Red_Button_Pressed:
                break

    elif f == 'T': # T mode
        if _CAMERA_DBG_:
            print("T Mode!")
        
        # Wait until Button Released
        while True:
            dbpv = de_bounce_read_pins([s1f,s1t])
            foc = dbpv[0]
            tak = dbpv[1]
            if tak != Red_Button_Pressed:
                break
        # Wait until Button Pressed Again
        while True:
            dbpv = de_bounce_read_pins([s1f,s1t])
            foc = dbpv[0]
            tak = dbpv[1]
            time.sleep_ms(3)
            if tak == Red_Button_Pressed:
                break
    # End Exposure
    
    #time.sleep_ms(3000) #Delete

    # Close Shutter
    shutter.duty_u16(65535) # Close Shutter
    if _CAMERA_DBG_:
        print("Shutter Closing!")
    time.sleep_ms(30) # wait until shutter fully closed
    shutter.duty_u16(30000) # Keep Shutter Closed
    if _CAMERA_DBG_:
        print("Shutter Closd. Exposure Finished")
    time.sleep_ms(18)

    #start ejacting film
    motor.value(1)
    if _CAMERA_DBG_:
        print("Motor Working!")
    
    #time.sleep_ms(2000) #Delete

    # wait until film fully ejact
    while True:
        if s5.value() == 0:
        #if s5.value() == 1:#Delete
            motor.value(0)
            shutter.duty_u16(0)
            break


#Taking Photo, St = Shutter delay
def Take_Photo(St,Shut_Mode):
    #meter()
    LED_Y.value(1)
    LED_B.value(1)
    print("EXP Time:",str(St)," Flash:",str(Shut_Mode))
    
    """
    if not flash_connected:
        shut(St);
    else:
        shut(St,'0');
    """
    shut(St,Shut_Mode);
    print("Taken!")
    #led_iso()
    LED_Y.value(0)
    #return
#time.sleep_ms(1000)

def test_cam():
    time.sleep_ms(3800)
    LED_Y.value(1)
    shutter.duty_u16(65535)
    time.sleep_ms(3800)
    shutter.duty_u16(0)
    motor.value(1)
    LED_Y.value(0)
    
def Cam_Operation():
    global if_focused, CAMERA_DBG
    while True:
        # See if flash is connected
        flash_connected = not s2.value()
        #print(flash_connected, s2.value())
        # Get Redbutton Value
        dbpv = de_bounce_read_pins([s1f,s1t])
        foc = dbpv[0]
        tak = dbpv[1]
        if have_enc:      
            display.fill_rect(0, 0, 50, 11, 1)  
            enc = plug_encoder()
            display.show()
        # If button pressed half way and not focused
        if foc == Red_Button_Pressed and if_focused == False:
            S1F_FBW.value(1);  #delete
            if have_disp:
                showFrame()
                if iso == '600':
                    display.text('600', 55, 2, 0)
                else:
                    display.text('70', 60, 2, 0)
            
            # Meter Mode Select
            #Flash inserted
            if flash_connected: 
                shut_mode = '0'
                if (not have_enc) or enc =='A':
                    St = ev11
                    display.text(str(ShutterSpeedHuman[St])+'s', 8, 23)
                    _,raw = meter()
                    if len(str(raw))>7:
                        raw = str(raw)[0:7]
                    display.text(str(raw), 64, 23)
                    display.text(str(ShutterSpeedHuman[St])+'s', 8, 23)
                elif enc != 'B' and enc != 'T':
                    St = plug_encoder()
                    if St < ev11:
                        St = ev11
                    _,raw = meter()
                    if len(str(raw))>7:
                        raw = str(raw)[0:7]
                    display.text(str(raw), 64, 23)
                    display.text(str(ShutterSpeedHuman[St])+'s', 8, 23)
                else:
                    if have_disp:
                        display.text('---', 94, 2, 0)
                        display.fill_rect(0,12,128,52,0)
                        #display.text('Shutter open until button release', 8, 23)
                        display.text('Not Support', 8, 23)
                    continue

                if have_disp:
                    display.text('FLASH', 86, 2, 0)
                    display.text('1/30', 10, 2, 0)
            
            # No Flash
            else:
            # No Selector or Set to AUTO
                if (not have_enc) or enc == 'A': 
                    if not have_enc and have_disp:
                        display.text('Auto', 10, 2, 0)
                    if have_disp:
                        display.text('OFF', 94, 2, 0)
                    
                    #meter
                    if _SAVE_FILE_:
                        da_F = open('sht.txt','a')
                        da_F.write("Operation Mode = A\n")
                        da_F.close()

                    St,raw = meter()
                    
                    shut_mode = '1' # '0'-flash; '1'-normal; 'B'- B; 'T' - T
                    if have_disp:
                        if len(str(raw))>7:
                            raw = str(raw)[0:7]
                        display.text(str(raw), 64, 23)
                        display.text(str(ShutterSpeedHuman[St])+'s', 8, 23)
                    """
                    if St >= ev7:
                        LED_B.value(0)
                    else:
                        LED_B.value(1)
                    """
                
                # Select B Mode
                elif enc == 'B':
                    shut_mode = 'B'
                    if _SAVE_FILE_:
                        da_F = open('sht.txt','a')
                        da_F.write("Operation Mode = B\n")
                        da_F.close()
                    St = ev11
                    if not have_enc and have_disp:
                            #display.text('B', 10, 2, 0)
                            pass
                    if have_disp:
                        display.text('OFF', 94, 2, 0)
                        display.fill_rect(0,12,128,52,0)
                        #display.text('Shutter open until button release', 8, 23)
                        display.text('Open When Press', 8, 23)
                
                # Select T Mode
                elif enc == 'T' :
                    #sec_enc = read_sec_plug_enc()
                    if _SAVE_FILE_:
                        da_F = open('sht.txt','a')
                        da_F.write("Operation Mode = T\n")
                        da_F.close()
                    shut_mode = 'T'
                    St = ev11
                    if not have_enc and have_disp:
                            #display.text('B', 10, 2, 0)
                            pass
                    if have_disp:
                        display.text('OFF', 94, 2, 0)
                        display.fill_rect(0,12,128,52,0)
                        #display.text('Shutter open until button release', 8, 23)
                        display.text('Press to Stop', 8, 23)
                
                # Long exposure mode
                elif enc > ev7:
                    shut_mode = '1'
                    if _SAVE_FILE_:
                        da_F = open('sht.txt','a')
                        da_F.write("Operation Mode = T(select time)\n")
                        da_F.close()
                    St = enc
                    _,raw = meter()
                    if have_disp:
                        display.text('OFF', 94, 2, 0)
                        display.fill_rect(0,12,128,52,0)
                        if len(str(raw))>7:
                            raw = str(raw)[0:7]
                        display.text(str(raw), 64, 23)
                        display.text(str(St/1000)+'s', 8, 23)

                else:
                    St = plug_encoder()
                    if _SAVE_FILE_:
                        da_F = open('sht.txt','a')
                        da_F.write("Operation Mode = M\n")
                        da_F.close()
                    _,raw = meter()
                    shut_mode = '1' # '0'-flash; '1'-normal; 'B'; 'T'
                    if have_disp:
                        display.text('OFF', 94, 2, 0)
                        display.fill_rect(0,12,128,52,0)
                        if len(str(raw))>7:
                            raw = str(raw)[0:7]
                        display.text(str(raw), 64, 23)
                        display.text(str(ShutterSpeedHuman[St])+'s', 8, 23)

            if have_disp: 
                #display.text(str(ShutterSpeedHuman[St])+'s', 8, 23)
                display.show()
            if_focused = True
            print("Focusing!")
            if _CAMERA_DBG_:
                print(dbpv)
        
        if foc != Red_Button_Pressed and if_focused == True:
            S1F_FBW.value(0); #delete
            if_focused = False
            print("Stop Focus")
            if _CAMERA_DBG_:
                print(dbpv)

        # Button Fully Pressed -> Take Photo
        if tak == Red_Button_Pressed:
            Take_Photo(St, shut_mode) 
            #Take_Photo(ev7, shut_mode) #delete
            #Take_Photo(St, 'B') #delete
            #Take_Photo(St, 'T') #delete
            if _CAMERA_DBG_:
                print(dbpv)
    

if __name__ == "__main__":
    camera_init()
    #test_cam()
    #shut(1000)
    Cam_Operation()
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

