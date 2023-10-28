import minimalmodbus
import time
import serial

not_in_op = 0
left_in_op = 1
right_in_op = 2

doWeld = 0             #Triggers the weld cycle
stopWeld = 1
safe = 2           #Read only, is set when all doors / latches are closed / latched
# Safety inputs (0 or 1):
leftDoorLatched = 3
rightDoorLatched = 4
stationInOperation = 5 # 1 for left, 2 for right, 0 for neither
# Determines the size of the modbus registers array associated with this enum:
TOTAL_REGS_SIZE = 6
controller = None
try:
    controller = minimalmodbus.Instrument('COM21', 1)
except serial.serialutil.SerialException as e:
    print(e)
    quit()
controller.serial.timeout = .2
controller.serial.baudrate = 115200
controller.serial.bytesize = 8
controller.serial.parity = 'N'
controller.serial.stopbits = 1

while True:
    for i in range(116):
        try:
            print('reg ' + str(i) + ": " + str(controller.read_register(0)))
        except minimalmodbus.NoResponseError as e:
            print('no response on register ' + str(i))
        except serial.serialutil.SerialException as e:
            print('Port not open')
            quit()
        except KeyboardInterrupt:
            quit()