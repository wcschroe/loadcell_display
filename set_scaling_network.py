from pyModbusTCP.client import ModbusClient
import time
from ctypes import (Union, Array, c_uint16, c_float)

c = ModbusClient(host="10.10.3.3", auto_open=True)

class c_uint16_array(Array):
    _type_ = c_uint16
    _length_ = 2

class c_union(Union):
    _fields_ = ("i", c_uint16_array), ("f", c_float)

converter = c_union()

converter.f = 6517900.0


for i in range(88):
    value:int = c.read_holding_registers(i)
    print(f'register[{i}] = {value[0]}')