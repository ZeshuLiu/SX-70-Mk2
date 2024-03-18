"""
Ver 1.1.2 20240314
by: Liu ZS
正常调试版本，适合组装后使用
上传进板子的flash内
"""

from machine import I2C, Pin, PWM, Timer
import tsl2561,time,ssd1306,pcf8575

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

#~ 快门速度定义位置
ev6 = 1050  # 1s
ev7 = 540	# 1/2
ev75 = 470	# 1/3
ev8 = 300	# 1/4
ev85 = 290	# 1/6
ev9 = 175	#* 1/8	OK
ev95 =136	# 1/10
ev10 = 97	#* 1/15	OK
ev105 = 79	# 1/20
ev11 = 60	# 1/30
ev115 = 52	# 1/45
ev12 = 45	# 1/60
ev125 = 41	# 1/90
ev13 = 38	# 1/125
ev135 = 36	# 1/180
ev14 = 32	#& 1/250    10  45
ev145 = 29	#& 1/360    9   34
ev15 = 26	#& 1/500    4   15
ev16 = 24	#& 1/1000

"""
#! 1/1000s 快门慢了一档(照片太亮); 1/250 快门慢1/3以内
ev6 = 1050  # 1s
ev7 = 540	# 1/2
ev75 = 470	# 1/3
ev8 = 300	# 1/4
ev85 = 290	# 1/6
ev9 = 175	# 1/8	OK
ev95 =136	# 1/10
ev10 = 97	# 1/15	OK
ev105 = 79	# 1/20
ev11 = 60	# 1/30
ev115 = 52	# 1/45
ev12 = 45	# 1/60
ev125 = 41	# 1/90
ev13 = 38	# 1/125
ev135 = 36	# 1/180
ev14 = 34	# 1/250
ev145 = 32	# 1/360
ev15 = 29	# 1/500
ev16 = 26	# 1/1000
"""

"""
ev6 = 1020  # 1s
ev7 = 600	# 1/2
ev75 = 455	# 1/3
ev8 = 311	# 1/4
ev85 =210	# 1/6
ev9 = 175	# 1/8	OK
ev95 =130	# 1/10
ev10 = 97	# 1/15	OK
ev105 = 80	# 1/20
ev11 = 63	# 1/30
ev115 = 52	# 1/45
ev12 = 46	# 1/60
ev125 = 41	# 1/90
ev13 = 38	# 1/125
ev135 = 36	# 1/180
ev14 = 34	# 1/250
ev145 = 29	# 1/360
ev15 = 24	# 1/500
ev16 = 22	# 1/1000
"""

ShutterSpeedHuman = {ev6:'1s', ev7:'1/2', ev75:'1/3', ev8:'1/4', ev85:'1/6', ev9:'1/8', ev95:'1/10',
                        ev10:'1/15', ev105:'1/20', ev11:'1/30', ev115:'1/45', ev12:'1/60',
                        ev125:'1/90', ev13:'1/125', ev135:'1/180', ev14:'1/250', ev145:'1/360',
                        ev15:'1/500', ev16:'1/1000'}

M_CMD_Dict = {ev6:'1s', ev7:'1/2', ev75:'1/3', ev8:'1/4', ev85:'1/6', ev9:'1/8', ev95:'1/10',
                ev10:'1/15', ev105:'1/20', ev11:'1/30', ev115:'1/45', ev12:'1/60',
                ev125:'1/90', ev13:'1/125', ev135:'1/180', ev14:'1/250', ev145:'1/360',
                ev15:'1/500', ev16:'1/1000'}

M_CMD_List = [i for i in M_CMD_Dict]
M_CMD_List.sort()

class Button3D():
    def  __init__(self) -> None:
        self.IF_DBG = True

        self.MAX_MENU = 3
        self.Menu = 0   # 在第几层菜单 0 - AUTO; 1 - T; 2 - B; 3 - M
        self.CAM_MODE = 'A'
        self.M_Pos = 0

        if self.IF_DBG:
            print(self.CAM_MODE)
            print(M_CMD_List)

        self.old_button_value = '111'
        self.thereD_button_debounce_last = time.ticks_ms()
        self.thereD_button_debounce_time = 100

        self.push_down_start_time = time.ticks_ms()

    def if_can_update(self):
        # 如果时间不到防抖时间则直接返回
        if time.ticks_ms() - self.thereD_button_debounce_last < self.thereD_button_debounce_time:
            return 0
        return 1

    def down_button_call(self):
        if self.Menu == 1:
            if self.IF_DBG:
                print("在T门按下了下")

        if self.Menu == 3:
            if self.IF_DBG:
                print("在M挡按下了下")

            if self.M_Pos == 0:
                self.M_Pos = len(M_CMD_List) - 1
            else:
                self.M_Pos -= 1

    def up_button_call(self):
        if self.Menu == 1:
            if self.IF_DBG:
                print("在T门按下了上")

        if self.Menu == 3:
            if self.IF_DBG:
                print("在M挡按下了上")
            self.M_Pos = (self.M_Pos + 1) % (len(M_CMD_List))

    def push_button_short_call(self):
        self.Menu = (self.Menu + 1) % (self.MAX_MENU+1)

        if self.Menu == 0:
            if self.IF_DBG:
                print("进入A挡")
            self.CAM_MODE = 'A'

        elif self.Menu == 1:
            if self.IF_DBG:
                print("进入B挡")
            self.CAM_MODE = 'B'

        elif self.Menu == 2:
            if self.IF_DBG:
                print("进入T挡")
            self.CAM_MODE = 'T'

        elif self.Menu == 3:
            if self.IF_DBG:
                print("进入M挡")
            self.CAM_MODE = M_CMD_List[self.M_Pos]


    def update(self, bt):
        change = 0

        #~ 向下按
        if self.old_button_value[0] == '1' and self.old_button_value[0] != bt[0]:
            if self.IF_DBG:
                print("按下了: 下")
            self.down_button_call()

        #~ 向上按
        if self.old_button_value[1] == '1' and self.old_button_value[1] != bt[1]:
            if self.IF_DBG:
                print("按下了: 上")
            self.up_button_call()

        #~ 向下按，在松开时动作（判断长短按）
        if self.old_button_value[2] == '1' and self.old_button_value[2] != bt[2]:
            if self.IF_DBG:
                print("按下了: 按下")
            self.push_down_start_time = time.ticks_ms()

        if self.old_button_value[2] == '0' and self.old_button_value[2] != bt[2]:
            if time.ticks_ms() - self.push_down_start_time > 1000:
                if self.IF_DBG:
                    print("长按")
            else:
                if self.IF_DBG:
                    print("短按")
                self.push_button_short_call()

        self.old_button_value = bt

        if self.Menu == 3:  # M 挡
            self.CAM_MODE = M_CMD_List[self.M_Pos]

        return self.CAM_MODE

class SX70():
    def __init__(self) -> None:
        #~ -------------------------宏定义---------------------------------
        self._CAMERA_DBG_ = True
        self._SAVE_FILE_ = False


        #~ ------------------------I2C总线设备-----------------------------
        self.lm_i2c = I2C(0,scl=Pin(21), sda=Pin(20), freq=400000)  # 测光表的I2C
        self.plugI2c = I2C(1,scl=Pin(19), sda=Pin(18), freq=400000) # 外接控制器的I2C
        self.sensor = tsl2561.TSL2561(self.lm_i2c,41)               # 测光表的类实例
        # I2c总线设备地址
        self.display_addr = 60
        self.encoder_addr = 32
        self.LM_ADDR = 41


        #~ ------------------------I2C显示设备-----------------------------
        self.have_disp = False              # I2c显示设备是否存在指示
        self.display = None                 # I2c显示设备类


        #~ ------------------------I2C控制旋钮-----------------------------
        self.pcf = None                         # PCF8575
        self.have_enc = False                   # I2c旋钮是否存在指示
        self.enc_pins = [4,5,6,7]               # 主旋扭使用PCF引脚列表 #! 质量不行，未来准备弃用
        self.sec_enc_pins = [0,1,2,3]           # 副旋钮使用PCF引脚列表 #! 质量不行，未来准备弃用
        self.thereD_button_pins = [12,10,11]    # 三维按键 下，上，按下 顺序
        self.Button_3D = Button3D()

        #~ -------------------------机身设备-------------------------------
        # 机身输出引脚 <具体连接方式与硬件有关，参见： Pins.xls>
        self.shutter_pin = 9
        self.apture_pin = 17
        self.motor_pin = 5
        self.LED_Y_PIN = 12
        self.LED_B_PIN = 13
        self.S1F_FBW_PIN = 22
        self.FF_PIN = 11            # Flash Pin High to trigger

        # 机身输入引脚 <具体连接方式与硬件有关，参见： Pins.xls>
        self.S1F_PIN = 2        # 半按红色旋钮（对焦）
        self.S1T_PIN = 1        # 完全按红色旋钮（拍摄）
        self.S2_PIN = 14        # 闪光灯是否插入 Flash Check Pin | SCL1
        self.S3_PIN = 7
        self.S5_PIN = 6

        # 输出引脚类的实例 - type(machine.Pin)
        self.shutter = None     # 快门螺线管输出
        self.apture = None      # 光圈螺线管输出
        self.motor = None       # 机身马达控制
        self.LED_Y = None       # LED 黄
        self.LED_B = None       # LED 蓝
        self.FF = None          # FF线 控制闪光灯闪光
        self.S1F_FBW = None     # S1F线 控制自动对焦

        # 机身设备状态指示
        self.Red_Button_Pressed = 1
        self.if_focused = False
        self.flash_connected = False


        #~ -------------------------工作参数定义------------------------------
        self.de_bounce_time = 1
        self.de_bounce_count = 10
        self.adc_de_bonuce_count = 70
        self.iso = '600'      # 默认为600
        self.light_meter_count = 7      # 测光时连续读7个数

    def de_bounce_read_pins(self, pin_list):
        pin_value = [0]*len(pin_list)
        for i in range(self.de_bounce_count):
            for j in range(len(pin_list)):
                pin_value[j] += pin_list[j].value()
            time.sleep_ms(self.de_bounce_time)
        for i in range(len(pin_value)):
            pin_value[i] = round(pin_value[i]/self.de_bounce_count)
            if i >1:
                i = 1
        return pin_value

    def read_enc(self, enc_p=None):
        if enc_p == None:
            enc_p = self.enc_pins
        result = ""
        for i in enc_p:
            result += str(self.pcf.pin(i))
        return result

    def showFrame(self):
        self.display.fill(0)
        self.display.fill_rect(0, 0, 128, 11, 1)
        self.display.vline(51, 0, 11, 0)
        self.display.vline(82, 0, 11, 0)

    def get_cmd_plug_encoder(self):
        ec = self.read_enc(self.enc_pins)
        #ec = EC_ZERO #delete
        #print(ec)
        if ec == EC_ZERO:
            self.display.text('AUTO', 10, 2, 0)
            return 'A'
        elif ec == EC_ONE:
            self.display.text('B', 23, 2, 0)
            return 'B'
        elif ec == EC_TWO:
            # self.display.text('T', 23, 2, 0)
            return self.read_sec_plug_enc()

        elif ec == EC_THREE:
            self.display.text('1/2', 15, 2, 0)
            return ev7
        elif ec == EC_FOUR:
            self.display.text('1/4', 15, 2, 0)
            return ev8
        elif ec == EC_FIVE:
            self.display.text('1/6', 15, 2, 0)
            return ev85
        elif ec == EC_SIX:
            self.display.text('1/8', 15, 2, 0)
            return ev9
        elif ec == EC_SEVEN:
            self.display.text('1/10', 10, 2, 0)
            return ev95
        elif ec == EC_EIGHT:
            self.display.text('1/15', 10, 2, 0)
            return ev10
        elif ec == EC_NINE:
            self.display.text('1/20', 10, 2, 0)
            return ev105
        elif ec == EC_A:
            self.display.text('1/30', 10, 2, 0)
            return ev11
        elif ec == EC_B:
            self.display.text('1/60', 10, 2, 0)
            return ev12
        elif ec == EC_C:
            self.display.text('1/125', 5, 2, 0)
            return ev13
        elif ec == EC_D:
            self.display.text('1/250', 5, 2, 0)
            return ev14
        elif ec == EC_E:
            self.display.text('1/500', 5, 2, 0)
            return ev15
        elif ec == EC_F:
            self.display.text('1/1000', 1, 2, 0)
            return ev16

    def read_sec_plug_enc(self):
        ec = self.read_enc(self.sec_enc_pins)
        # print('Sec Enc:', ec)
        one_sec = 1000
        one_min = one_sec*60
        if ec == SEC_EC_ZERO:
            self.display.text('T', 10, 2, 0)
            return 'T'
        elif ec == SEC_EC_ONE:
            self.display.text('T1S', 23, 2, 0)
            return 1.1*one_sec
        elif ec == SEC_EC_TWO:
            self.display.text('T2S', 23, 2, 0)
            return 2.2*one_sec
        elif ec == SEC_EC_THREE:
            self.display.text('T4S', 15, 2, 0)
            return 4.4*one_sec
        elif ec == SEC_EC_FOUR:
            self.display.text('T8S', 15, 2, 0)
            return 8*2*one_sec
        elif ec == SEC_EC_FIVE:
            self.display.text('T15S', 15, 2, 0)
            return 15*one_sec
        elif ec == SEC_EC_SIX:
            self.display.text('T30S', 15, 2, 0)
            return 30*one_sec
        elif ec == SEC_EC_SEVEN:
            self.display.text('T1M', 10, 2, 0)
            return 1*one_min
        elif ec == SEC_EC_EIGHT:
            self.display.text('T2M', 10, 2, 0)
            return 2*one_min
        elif ec == SEC_EC_NINE:
            self.display.text('T4M', 10, 2, 0)
            return 4*one_min

    def mode_disp(self):
        if self.cmd == 'A':
            self.display.text('AUTO', 10, 2, 0)

        elif self.cmd == 'B':
            self.display.text('B', 23, 2, 0)

        elif self.cmd == 'T':
            self.display.text('T', 23, 2, 0)

        elif self.cmd == ev6:
            self.display.text('1s', 15, 2, 0)

        elif self.cmd == ev7:
            self.display.text('1/2', 15, 2, 0)

        elif self.cmd == ev75:
            self.display.text('1/3', 15, 2, 0)

        elif self.cmd == ev8:
            self.display.text('1/4', 15, 2, 0)

        elif self.cmd == ev85:
            self.display.text('1/6', 15, 2, 0)

        elif self.cmd == ev9:
            self.display.text('1/8', 15, 2, 0)

        elif self.cmd == ev95:
            self.display.text('1/10', 10, 2, 0)

        elif self.cmd == ev10:
            self.display.text('1/15', 10, 2, 0)

        elif self.cmd == ev105:
            self.display.text('1/20', 10, 2, 0)

        elif self.cmd == ev11:
            self.display.text('1/30', 10, 2, 0)

        elif self.cmd == ev115:
            self.display.text('1/45', 10, 2, 0)

        elif self.cmd == ev12:
            self.display.text('1/60', 10, 2, 0)

        elif self.cmd == ev125:
            self.display.text('1/90', 10, 2, 0)

        elif self.cmd == ev13:
            self.display.text('1/125', 5, 2, 0)

        elif self.cmd == ev135:
            self.display.text('1/180', 5, 2, 0)

        elif self.cmd == ev14:
            self.display.text('1/250', 5, 2, 0)

        elif self.cmd == ev145:
            self.display.text('1/360', 5, 2, 0)

        elif self.cmd == ev15:
            self.display.text('1/500', 5, 2, 0)

        elif self.cmd == ev16:
            self.display.text('1/1000', 1, 2, 0)


    def get_cmd_3D_button(self):
        if not self.Button_3D.if_can_update():
            return self.Button_3D.CAM_MODE

        self.cmd = self.Button_3D.update(self.read_enc(self.thereD_button_pins))

        self.mode_disp()

        return self.cmd

    def meter(self):
        # 自动增益
        self.sensor.gain(16)
        try:
            lux = self.sensor.read()
        except ValueError:
            self.sensor.gain(1)
            lux = self.sensor.read()

        # 读取滤波
        for i in range(self.light_meter_count-1):
            time.sleep_ms(1)
            lux += self.sensor.read()
        lux = lux/self.light_meter_count

        # 打印调试信息
        if self._CAMERA_DBG_:
            print("Raw Lux = {0:.2f} \t\tGain = {1:.2f} \t\t".format(lux, self.sensor.gain()))

        # 保存测光信息，用于使用中标定
        if self._SAVE_FILE_:
            da_F = open('sht.txt','a')
            da_F.write("Raw Lux = {0:.2f} \tGain = {1:.2f} \n".format(lux, self.sensor.gain()))
            da_F.close()

        # 对参数进行调整
        lux -= 0.4
        #* Magic Number
        lux *= 0.9

        # 判断使用的拍摄参数
        if self.iso == '600':
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

    def close_led(self):
        self.LED_Y.value(1)
        self.LED_B.value(1)

    def led_iso(self):
        if self.iso == '600':
            self.LED_Y.value(0)
        else:
            self.LED_B.value(0)

    def apture_engage(self):
        self.apture.duty_u16(65535)

    def apture_disengage(self):
        self.apture.duty_u16(0)

    def close_shutter(self, timer = None):
        self.shutter.duty_u16(65535) # Close Shutter

    def camera_init(self):
        plug_i2c_add = self.plugI2c.scan()
        plugscan = bool(plug_i2c_add) and len(plug_i2c_add)<=5
        print(self.plugI2c.scan())

        self.shutter = PWM(Pin(self.shutter_pin))
        self.shutter.freq(20000)
        self.shutter.duty_u16(0)
        self.apture = PWM(Pin(self.apture_pin))
        self.apture.freq(70000)
        self.apture.duty_u16(0)

        self.motor = Pin(self.motor_pin,Pin.OUT, value=0)
        self.LED_Y = Pin(self.LED_Y_PIN, Pin.OUT,value = 1)
        self.LED_B = Pin(self.LED_B_PIN, Pin.OUT,value = 1)
        self.FF = Pin(self.FF_PIN, Pin.OUT,value = 0)
        self.S1F_FBW = Pin(self.S1F_FBW_PIN, Pin.OUT,value = 0)

        self.s2 = Pin(self.S2_PIN,Pin.IN,Pin.PULL_UP)
        self.s3 = Pin(self.S3_PIN,Pin.IN,Pin.PULL_UP)
        self.s5 = Pin(self.S5_PIN,Pin.IN,Pin.PULL_UP)
        self.s1f = Pin(self.S1F_PIN,Pin.IN)
        self.s1t = Pin(self.S1T_PIN,Pin.IN)

        f = open('./dat/iso.txt')
        self.iso = f.readline()
        self.iso.replace(" ","")
        f.close()
        if self._CAMERA_DBG_:
            print("ISO at %s!"%self.iso)
        if self.iso == '600':
            self.LED_Y.value(0)
            #display.text('600', 55, 2, 0)
        else:
            self.LED_B.value(0)
            #display.text('70', 60, 2, 0)

        if plugscan and self.encoder_addr in plug_i2c_add:
            self.pcf = pcf8575.PCF8575(self.plugI2c, self.encoder_addr)
            self.pcf.port = 0xffff
            print("have plug")
            self.have_enc = True

        if plugscan and self.display_addr in plug_i2c_add:

            self.display = ssd1306.SSD1306_I2C(128, 32, self.plugI2c,addr=self.display_addr) # 设置宽度，高度和I2C通信
            self.have_disp = True
            self.display.contrast(1)
            if self._CAMERA_DBG_:
                print("have disp")
            self.showFrame()
            self.display.text('OFF', 94, 2, 0)
            if self.iso == '600':
                self.display.text('600', 55, 2, 0)
            else:
                self.display.text('70', 60, 2, 0)

            if self.have_enc:
                pass
            else:
                if not self.have_enc:
                    self.display.text('Auto', 10, 2, 0)
                shut_speed,raw = self.meter()
                self.display.text(str(ShutterSpeedHuman[shut_speed])+'s', 8, 23)
                if len(str(raw))>7:
                    raw = str(raw)[0:7]
                self.display.text(str(raw), 64, 23)
            self.display.show()

    def shut(self, Shutter_Delay, f="1"):
        Shutter_Delay = int(Shutter_Delay)

        #~ ------------------------拍摄前准备-------------------------------
        if self._SAVE_FILE_:
            da_F = open('sht.txt','a')
            da_F.write("Mode: " + f)
            if Shutter_Delay in ShutterSpeedHuman:
                da_F.write(" Speed: "+ShutterSpeedHuman[Shutter_Delay])
            da_F.write(" Shutter Delay = {0:.2f}\n\n".format(Shutter_Delay))
            da_F.close()

        if self._CAMERA_DBG_:
            print("Taking Picture")
            print("Shutter start to close!")


        #~ ------------------------关闭快门--------------------------------
        self.shutter.duty_u16(65535)
        time.sleep_ms(30)
        self.shutter.duty_u16(30000)
        if self._CAMERA_DBG_:
            print("Shutter closed!")

        time.sleep_ms(30) # 等快门完全关闭

        #~ --------------------电机启动,带动反光板上升------------------------
        self.motor.value(1)
        if self._CAMERA_DBG_:
            print("Motor Start Moving")

        # 直到反光板就位
        while True:
            if self.s3.value() == 1:
                self.motor.value(0)
                if self._CAMERA_DBG_:
                    print("Motor Stoped!")
                break

        #~ ----------------Y Delay 此时如果使用闪光灯则光圈就位------------------
        if f=='0': #光圈就位
            self.apture_engage()
            if self._CAMERA_DBG_:
                print("apture_engage")

        time.sleep_ms(18) # Y delay

        #~ ---------------------开启快门 曝光开始-------------------------------
        if self._CAMERA_DBG_:
            print("Shutter Start to Open, Exposure Starts")
        #! 此处不再调用 self.shutter.duty_u16(0) 各模式判断后调用

        #~ ---------------------根据不同模式开始工作-------------------------------
        if f == '0': #~ 闪光灯模式
            if self._CAMERA_DBG_:
                print("Flash Mode!")
            gap = int(Shutter_Delay- 47)
            if gap < 0:
                gap = 0
            self.shutter.duty_u16(0)    #& 开启快门
            time.sleep_ms(47)
            self.FF.value(1)
            self.FF.value(0)
            time.sleep_ms(gap)

        elif f == '1': #~ 普通曝光模式
            ti = Timer()    # 启动定时器以完成精确曝光
            self.shutter.duty_u16(0)    #& 开启快门
            ti.init(mode = Timer.ONE_SHOT, period = Shutter_Delay, callback = self.close_shutter)

            if self._CAMERA_DBG_:
                print("Normal Mode!")

            time.sleep_ms(Shutter_Delay+100) # 防止干扰定时器工作

        elif f == 'B': #~ 长曝光模式
            if self._CAMERA_DBG_:
                print("B Mode!")
            self.shutter.duty_u16(0)    #& 开启快门
            time.sleep_ms(15)
            while True:
                dbpv = self.de_bounce_read_pins([self.s1f, self.s1t])
                foc = dbpv[0]
                tak = dbpv[1]
                time.sleep_ms(3)
                if tak != self.Red_Button_Pressed:
                    break

        elif f == 'T': #~ T门模式
            if self._CAMERA_DBG_:
                print("T Mode!")

            self.shutter.duty_u16(0)    #& 开启快门
            # Wait until Button Released
            while True:
                dbpv = self.de_bounce_read_pins([self.s1f, self.s1t])
                foc = dbpv[0]
                tak = dbpv[1]
                if tak != self.Red_Button_Pressed:
                    break
            # Wait until Button Pressed Again
            while True:
                dbpv = self.de_bounce_read_pins([self.s1f, self.s1t])
                foc = dbpv[0]
                tak = dbpv[1]
                time.sleep_ms(3)
                if tak == self.Red_Button_Pressed:
                    break

        #~ ---------------------关闭快门 曝光结束-------------------------------
        self.close_shutter()    #& 关闭快门
        self.apture_disengage() # 光圈归位

        if self._CAMERA_DBG_:
            print("Shutter Closing!")
        time.sleep_ms(30) # wait until shutter fully closed
        self.shutter.duty_u16(30000) # Keep Shutter Closed
        if self._CAMERA_DBG_:
            print("Shutter Closd. Exposure Finished")
        time.sleep_ms(18)

        #~ ---------------------电机启动 开始吐片-------------------------------
        self.motor.value(1)
        if self._CAMERA_DBG_:
            print("Motor Working!")

        while True:
            if self.s5.value() == 0:
                self.motor.value(0)
                self.shutter.duty_u16(0)
                break

    def Take_Photo(self, St, Shut_Mode):
        self.LED_Y.value(1)
        self.LED_B.value(1)
        if self._CAMERA_DBG_:
            print("EXP Time:",str(St)," Flash:",str(Shut_Mode))

        self.shut(St,Shut_Mode);
        if self._CAMERA_DBG_:
            print("Taken!")

        self.LED_Y.value(0)

    def Focus_Flash_work(self, enc):
        shut_mode = '0'
        if (not self.have_enc) or enc =='A':
            St = ev11
            self.display.text(str(ShutterSpeedHuman[St])+'s', 8, 23)
            _,raw = self.meter()
            if len(str(raw))>7:
                raw = str(raw)[0:7]
            self.display.text(str(raw), 64, 23)
            self.display.text(str(ShutterSpeedHuman[St])+'s', 8, 23)
        elif enc != 'B' and enc != 'T':
            # St = self.get_cmd_plug_encoder()
            St = self.get_cmd_3D_button()
            if St < ev11:
                St = ev11
            _,raw = self.meter()
            if len(str(raw))>7:
                raw = str(raw)[0:7]
            self.display.text(str(raw), 64, 23)
            self.display.text(str(ShutterSpeedHuman[St])+'s', 8, 23)
        else:
            if self.have_disp:
                self.display.text('---', 94, 2, 0)
                self.display.fill_rect(0,12,128,52,0)
                self.display.text('Not Support', 8, 23)
            return shut_mode, St

        if self.have_disp:
            self.display.text('FLASH', 86, 2, 0)
            self.display.text('1/30', 10, 2, 0)

        return shut_mode, St

    def Focus_A_work(self):
        if not self.have_enc and self.have_disp:
            self.display.text('Auto', 10, 2, 0)
        if self.have_disp:
            self.display.text('OFF', 94, 2, 0)

        # 进行测光
        if self._SAVE_FILE_:
            da_F = open('sht.txt','a')
            da_F.write("Operation Mode = A\n")
            da_F.close()

        St,raw = self.meter()

        shut_mode = '1' # '0'-flash; '1'-normal; 'B'- B; 'T' - T
        if self.have_disp:
            if len(str(raw))>7:
                raw = str(raw)[0:7]
            self.display.text(str(raw), 64, 23)
            self.display.text(str(ShutterSpeedHuman[St])+'s', 8, 23)

        return shut_mode, St

    def Focus_B_work(self):
        shut_mode = 'B'
        if self._SAVE_FILE_:
            da_F = open('sht.txt','a')
            da_F.write("Operation Mode = B\n")
            da_F.close()
        if not self.have_enc and self.have_disp:
                #display.text('B', 10, 2, 0)
                pass
        if self.have_disp:
            self.display.text('OFF', 94, 2, 0)
            self.display.fill_rect(0,12,128,52,0)
            #display.text('Shutter open until button release', 8, 23)
            self.display.text('Open When Press', 8, 23)
        return shut_mode, ev11

    def Focus_T_work(self):
        if self._SAVE_FILE_:
            da_F = open('sht.txt','a')
            da_F.write("Operation Mode = T\n")
            da_F.close()
        shut_mode = 'T'
        if not self.have_enc and self.have_disp:
                #display.text('B', 10, 2, 0)
                pass
        if self.have_disp:
            self.display.text('OFF', 94, 2, 0)
            self.display.fill_rect(0,12,128,52,0)
            #display.text('Shutter open until button release', 8, 23)
            self.display.text('Press to Stop', 8, 23)
        return shut_mode, ev11

    def Focus_TP_work(self, enc):
        shut_mode = '1'
        if self._SAVE_FILE_:
            da_F = open('sht.txt','a')
            da_F.write("Operation Mode = T(select time)\n")
            da_F.close()
        St = enc
        _,raw = self.meter()
        if self.have_disp:
            self.display.text('OFF', 94, 2, 0)
            self.display.fill_rect(0,12,128,52,0)
            if len(str(raw))>7:
                raw = str(raw)[0:7]
            self.display.text(str(raw), 64, 23)
            self.display.text(str(St/1000)+'s', 8, 23)

        return shut_mode, St

    def Focus_M_work(self):
        # St = self.get_cmd_plug_encoder()
        St = self.get_cmd_3D_button()
        if self._SAVE_FILE_:
            da_F = open('sht.txt','a')
            da_F.write("Operation Mode = M\n")
            da_F.close()
        _,raw = self.meter()
        shut_mode = '1' # '0'-flash; '1'-normal; 'B'; 'T'
        if self.have_disp:
            self.display.text('OFF', 94, 2, 0)
            self.display.fill_rect(0,12,128,52,0)
            if len(str(raw))>7:
                raw = str(raw)[0:7]
            self.display.text(str(raw), 64, 23)
            self.display.text(str(ShutterSpeedHuman[St])+'s', 8, 23)

        return shut_mode, St

    def Focus(self, cmd):
        mode = None
        st = None
        self.S1F_FBW.value(1)
        if self.have_disp:
            self.showFrame()
            if self.iso == '600':
                self.display.text('600', 55, 2, 0)
            else:
                self.display.text('70', 60, 2, 0)

        if self.flash_connected:    # 有闪光灯
            return self.Focus_Flash_work(cmd)

        else:                       # 无闪光灯
            if (not self.have_enc) or cmd == 'A':
                mode, st = self.Focus_A_work()

            elif cmd == 'B':
                mode, st = self.Focus_B_work()

            elif cmd == 'T' :
                mode, st = self.Focus_T_work()

            elif cmd > ev7: # 长于1/2秒的模式
                mode, st = self.Focus_TP_work(cmd)

            else:   # 少于1/2秒的时间
                mode, st = self.Focus_M_work()

            if self.have_disp:
                self.display.show()
            self.if_focused = True

            if self._CAMERA_DBG_:
                print("Focusing!")

            return mode, st

    def Cam_Operation(self):
        while True:
            #~ ------------------判断是否连接了闪光灯------------------
            flash_connected = not self.s2.value()

            #~ ---------------------获取按钮的信息--------------------
            dbpv = self.de_bounce_read_pins([self.s1f,self.s1t])
            foc = dbpv[0]
            tak = dbpv[1]
            if self.have_enc:
                self.display.fill_rect(0, 0, 50, 11, 1)
                # cmd = self.get_cmd_plug_encoder()
                self.cmd = self.get_cmd_3D_button()
                self.display.show()

            #~ ---------------如果半按快门，但是还未对焦----------------
            # 负责对焦+测光
            if foc == self.Red_Button_Pressed and self.if_focused == False:
                shut_mode, St = self.Focus(self.cmd)

            #~ ---------------松开半按快门，但是已经对焦----------------
            if foc != self.Red_Button_Pressed and self.if_focused == True:
                self.S1F_FBW.value(0)
                self.if_focused = False
                if self._CAMERA_DBG_:
                    print("Stop Focus")
                if self._CAMERA_DBG_:
                    print(dbpv)

            #~ ---------------全按快门，拍摄照片----------------
            if tak == self.Red_Button_Pressed:
                self.Take_Photo(St, shut_mode)
                if self._CAMERA_DBG_:
                    print(dbpv)

if __name__ == "__main__":
    a = SX70()
    a.camera_init()
    while 0:
        print(a.read_enc(a.thereD_button_pins), a.pcf._port[1])
        time.sleep(1)

    a.Cam_Operation()