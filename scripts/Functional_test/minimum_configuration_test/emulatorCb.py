import os
import threading
import sys
import time
import traceback
import msvcrt
import serial
import queue
from serial.threaded import ReaderThread
from serial.threaded import LineReader
from crc import Calculator, Crc16


class PrintLines(LineReader):
    def connection_made(self, transport):
        super(PrintLines, self).connection_made(transport)
        sys.stdout.write('port opened\n')

    def handle_line(self, data):
        sys.stdout.write('line received: {}\n'.format(repr(data)))

    def connection_lost(self, exc):
        if exc:
            traceback.print_exc(exc)
        sys.stdout.write('port closed\n')


class UnitControl:
    def __init__(self):      
        self.mode = 0
        self.on_off = 0
        self.fan_speed = 0
        self.current_temp = {'val1': 0, 'val2': 0}
        self.target_temp = {'val1': 0, 'val2': 0}
        self.unit_control_type = 0


class Motor:
    def __init__(self):
        self.levels = [0] * 10  # initialize array with zeros

    def set_level(self, motor_id, level):
        self.levels[motor_id] = level

    def get_level(self, motor_id):
        return self.levels[motor_id]
    
class Activation:
    def __init__(self):
        self.status = 0x00  # in seconds
        self.password = 0  # as a string
        self.lockoutDay = 0x00 # as an integer representing the day of the week (0 = Monday, 1 = Tuesday, etc.)
        self.remainingDay = 0x00



calculator = Calculator(Crc16.CCITT)
ser = serial.serial_for_url('COM6', baudrate=115200, timeout=0)
unit_control = UnitControl()
motor = Motor()
activation = Activation()


def pack_message(address: bytes, message_type: int, message_id: int, msg_data: bytes, sequence_number: bytes, uart_ack_byte: int) -> bytes:
    uart_ack_byte = uart_ack_byte.to_bytes(1, 'big')
    message_type_byte = message_type.to_bytes(1, 'big')
    message_id_byte = message_id.to_bytes(1, 'big')
    
    payload = uart_ack_byte + address + message_type_byte + message_id_byte + msg_data + sequence_number
    crc_bytes_little_endian = calculator.checksum(payload).to_bytes(2, 'little')

    return b'\x7E' + len(payload).to_bytes(1, 'big') + payload + crc_bytes_little_endian + b'\x7F'



#UnitControl
def handle_set_unit_control(address,data_bytes,sequence_number):


    unit_control.mode = data_bytes[0]
    unit_control.on_off = data_bytes[1]
    unit_control.fan_speed = data_bytes[2]
    unit_control.current_temp['val1'] = data_bytes[3]
    unit_control.current_temp['val2'] = data_bytes[4]
    unit_control.target_temp['val1'] = data_bytes[5]
    unit_control.target_temp['val2'] = data_bytes[6]
    unit_control.unit_control_type = data_bytes[7]
    
    message_type = 0x01
    message_id = 0x04
    ErrorCode = 0x00
    
    message = pack_message(address, message_type, message_id, ErrorCode.to_bytes(1, 'big'), sequence_number, 0)

    




    ser.write(message)
    hex_message = ' '.join(['0x' + byte.to_bytes(1, 'big').hex() for byte in message])
    print("send", hex_message)

def handle_get_unit_control(address, sequence_number):
    msg_data = bytearray()

    msg_data.append(unit_control.mode)
    msg_data.append(unit_control.on_off)
    msg_data.append(unit_control.fan_speed)
    msg_data.append(unit_control.current_temp['val1'])
    msg_data.append(unit_control.current_temp['val2'])
    msg_data.append(unit_control.target_temp['val1'])
    msg_data.append(unit_control.target_temp['val2'])
    msg_data.append(unit_control.unit_control_type)

    message_type = 0x01
    message_id = 0x02

    message = pack_message(address, message_type, message_id, msg_data, sequence_number,0 )
    ser.write(message)
    hex_message = ' '.join(['0x' + byte.to_bytes(1, 'big').hex() for byte in message])
    print("send", hex_message)

def handle_status_code_unit_control(error_code):
    print("Status Code Unit Control: Error")

def handle_unit_control(address, message_id, data_bytes, sequence_number):
    if message_id.hex() == '01':
        print("Set Unit Control:")
        handle_set_unit_control(address,data_bytes,sequence_number)
    elif message_id.hex() == '03':
        print("Get Unit Control:")
        handle_get_unit_control(address, sequence_number)
    elif message_id.hex() == '04':
        error_code = data_bytes[0]
        handle_status_code_unit_control(error_code)
    else:
        print("Unknown message Id:", message_id.hex())   


#Motor
def handle_set_motor_id(address, data_bytes, sequence_number):

    motor_id = data_bytes[0]
    level = data_bytes[1]

    motor.set_level(motor_id, level)

    message_type = 0x04
    message_id = 0x04
    ErrorCode = 0x00

    message = pack_message(address, message_type, message_id, ErrorCode.to_bytes(1, 'big'), sequence_number, 0)
    ser.write(message)
    hex_message = ' '.join(['0x' + byte.to_bytes(1, 'big').hex() for byte in message])
    print("send", hex_message)

def handle_get_motor_id(address, sequence_number, data_bytes):
    global motor
    msg_data = bytearray()

    msg_data.append(motor.get_level(data_bytes[0])) 

    message_type = 0x04
    message_id = 0x05

    message = pack_message(address, message_type, message_id, msg_data, sequence_number,0 )
    ser.write(message)
    hex_message = ' '.join(['0x' + byte.to_bytes(1, 'big').hex() for byte in message])
    print("send", hex_message)

def handle_get_motor_all(address, sequence_number):
    global motor
    msg_data = bytearray()

    for level in motor.levels:
        msg_data.append(level)

    message_type = 0x04
    message_id = 0x02

    message = pack_message(address, message_type, message_id, msg_data, sequence_number,0 )
    ser.write(message)
    hex_message = ' '.join(['0x' + byte.to_bytes(1, 'big').hex() for byte in message])
    print("send", hex_message)

def handle_status_code_motor(error_code):
    print("Status Code Motor: Error")
  
def handle_motor(address, message_id, data_bytes, sequence_number):
    if message_id.hex() == '01':
        print("Set Motor ID:")
        handle_set_motor_id(address, data_bytes, sequence_number)
    elif message_id.hex() == '06':
        print("Get Motor ID:")
        handle_get_motor_id(address,data_bytes, sequence_number)
    elif message_id.hex() == '03':
        print("Get All Motors:")
        handle_get_motor_all(address, sequence_number)
    elif message_id.hex() == '04':
        error_code = data_bytes[0]
        handle_status_code_motor(error_code)
    else:
        print("Unknown message Id:", message_id.hex())   
 
 
 
 
 
#Activation  
def handle_set_activation(address, data_bytes, sequence_number):
    
    
    activation.status = data_bytes[0:1]
    password_int = data_bytes[1:3]
    password = str(int.from_bytes(password_int, byteorder='big') / 1000).replace('.', '')
    activation.password = password
    activation.lockoutDay = data_bytes[3:4]
    activation.remainingDay = activation.lockoutDay
    message_type = 0x02
    message_id = 0x04
    error_code = 0x00
    print("state ", activation.status)
    print("password ",activation.password)
    print("lockoutDay ",activation.lockoutDay )
    print("remainingDay " ,activation.remainingDay)
    
    
    
    
    message = pack_message(address, message_type, message_id, error_code.to_bytes(1, 'big'), sequence_number,0 )
    ser.write(message)
    hex_message = ' '.join(['0x' + byte.to_bytes(1, 'big').hex() for byte in message])
    print("send", hex_message)
    
def handle_get_activation(address, sequence_number):
    
    
    msg_data = bytearray()

    msg_data += activation.status
    msg_data += activation.remainingDay
    
    message_type = 0x02
    message_id = 0x02
    
    message = pack_message(address, message_type, message_id, msg_data, sequence_number,0 )
    ser.write(message)
    hex_message = ' '.join(['0x' + byte.to_bytes(1, 'big').hex() for byte in message])
    print("send", hex_message)

   
        
def handle_activation(address, message_id, data_bytes, sequence_number):
    if message_id.hex() == '01':
        print("Set Activation:")
        handle_set_activation(address, data_bytes, sequence_number)
    elif message_id.hex() == '03':
        print("Get Activation:")
        handle_get_activation(address, sequence_number)
    else:
        print("Unknown message Id:", message_id.hex())

def handle_status_code_activation(error_code):
    print("Status Code Activation: Error")


def process_message(address_bytes, message_type, message_id, data_bytes, sequence_number):
    
    if message_type.hex() == '01':  # Unit Control
        handle_unit_control(address_bytes, message_id, data_bytes, sequence_number)
    elif message_type.hex() == '04':
        handle_motor(address_bytes, message_id, data_bytes, sequence_number)
    elif message_type.hex() == '02':
        handle_activation(address_bytes, message_id, data_bytes, sequence_number)
    else:
        print("Unknown message type:", message_type.hex())


def reader():
    while True:
        # Wait for start byte
        while True:
            start_byte = ser.read(1)
            if start_byte == b'\x7E':
                break

        # Start byte found, read length byte
        length_byte = ser.read(1)
        length = int.from_bytes(length_byte, 'big')

        # Read the entire message based on the length
        message_bytes = ser.read(length + 3)  # +2 for end byte and CRC
        end_byte = message_bytes[-1:]

        if end_byte != b'\x7F':
            print("Error: Invalid end byte")
            print("Received end byte:", end_byte)
            message = start_byte + length_byte + message_bytes
            hex_message = ' '.join(['0x' + byte.to_bytes(1, 'big').hex() for byte in message])
            print("ERROR", hex_message)
        else:
            # Process the message
            uart_ack = message_bytes[0:1]
            address_bytes = message_bytes[1:3]
            message_type = message_bytes[3:4]
            message_id = message_bytes[4:5]
            data_bytes = message_bytes[5:-4]  # Exclude sequence number, CRC, and end byte
            sequence_number = message_bytes[-4:-3]
            crc_bytes = message_bytes[-2:-1]


            message = start_byte + length_byte + uart_ack + address_bytes[::-1] + message_type + message_id + data_bytes + sequence_number + crc_bytes + end_byte
            hex_message = ' '.join(['0x' + byte.to_bytes(1, 'big').hex() for byte in message])
            print("receive", hex_message)
            message =  address_bytes[::-1] 
            hex_message = ' '.join(['0x' + byte.to_bytes(1, 'big').hex() for byte in message])
            print("address_bytes", hex_message)
            message =  message_type 
            hex_message = ' '.join(['0x' + byte.to_bytes(1, 'big').hex() for byte in message])
            print("message_type", hex_message)
            message =  message_id 
            hex_message = ' '.join(['0x' + byte.to_bytes(1, 'big').hex() for byte in message])
            print("message_id", hex_message)
            message =  data_bytes 
            hex_message = ' '.join(['0x' + byte.to_bytes(1, 'big').hex() for byte in message])
            print("data_bytes", hex_message)
            message =  sequence_number
            hex_message = ' '.join(['0x' + byte.to_bytes(1, 'big').hex() for byte in message])
            print("sequence_number", hex_message)
            
            
            process_message(address_bytes, message_type, message_id, data_bytes, sequence_number)



serial_worker = threading.Thread(target=reader, daemon=True)
serial_worker.start()

done = False
while not done:
    if msvcrt.kbhit():
        print("key pressed, stopping")
        done = True

