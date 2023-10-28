import minimalmodbus
import time
import serial
from ctypes import (Union, Array, c_uint16, c_float)

controller = None
try:
    controller = minimalmodbus.Instrument('COM12', 1)
except serial.serialutil.SerialException as e:
    print(e)
    quit()
controller.serial.timeout = 3
controller.serial.baudrate = 115200
controller.serial.bytesize = 8
controller.serial.parity = 'N'
controller.serial.stopbits = 1

class c_uint16_array(Array):
    _type_ = c_uint16
    _length_ = 2

class c_union(Union):
    _fields_ = ("i", c_uint16_array), ("f", c_float)

converter = c_union()

converter.f = 6517900.0

f = open('register_defaults.txt')
defaults_lines = f.readlines()

for line in defaults_lines:
    data = line.split('=')
    reg:int = int(data[0].replace('register[', '').replace(']', '').strip())
    value:int = int(data[1].strip())
    print(f'register[{reg}] = {value}')
    if reg != 34 and reg != 35 and reg < 80:
        controller.write_register(reg, value)

quit()

for i in range(81):
    value:int = controller.read_register(i)
    print(f'register[{i}] = {value}')