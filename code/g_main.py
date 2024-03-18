"""
Ver 2.0.0 20240307
正常调试版本，适合组装后使用
直接命名为main.py然后上传进板子的flash里就行
"""
from camera_driver import SX70

cam = SX70()
cam.camera_init()
cam.Cam_Operation()