from pyModbusTCP.client import ModbusClient
import time
from ctypes import (Union, Array, c_uint16, c_float)

c = ModbusClient(host="192.168.4.1", auto_open=True)

class c_uint16_array(Array):
    _type_ = c_uint16
    _length_ = 2

class c_union(Union):
    _fields_ = ("i", c_uint16_array), ("f", c_float)

converter = c_union()

converter.f = 6517900.0

values = [converter.i[0], converter.i[1]]

c.write_multiple_registers(2, [converter.i[0], converter.i[1]])

print(f'wrote {values} ({converter.f})')
print(f'readback: {c.read_holding_registers(2, 2)}')