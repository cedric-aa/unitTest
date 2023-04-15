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
        self.target_temp = {'val1': 0, 'val2': 0}
        self.current_temp = {'val1': 0, 'val2': 0}
        self.mode = 0
        self.on_off = 0
        self.fan_speed = 0
        self.unit_control_type = 0


calculator = Calculator(Crc16.CCITT)
ser = serial.serial_for_url('COM17', baudrate=115200, timeout=2)
unit_control = UnitControl()


def pack_message(address: bytes, message_type: int, message_id: int, msg_data: bytes, sequence_number: bytes, uart_ack_byte: int) -> bytes:
    uart_ack_byte = uart_ack_byte.to_bytes(1, 'big')
    message_type_byte = message_type.to_bytes(1, 'big')
    message_id_byte = message_id.to_bytes(1, 'big')
    payload = uart_ack_byte + address + message_type_byte + message_id_byte + msg_data + sequence_number
    crc_bytes_little_endian = calculator.checksum(payload).to_bytes(2, 'little')

    return b'\x7E' + len(payload).to_bytes(1, 'big') + payload + crc_bytes_little_endian + b'\x7F'



def handle_set_unit_control(data_bytes):
    global unit_control
    unit_control.target_temp['val1'] = data_bytes[0]
    unit_control.target_temp['val2'] = data_bytes[1]
    unit_control.mode = data_bytes[4]
    unit_control.on_off = data_bytes[5]
    unit_control.fan_speed = data_bytes[6]
    unit_control.unit_control_type = data_bytes[7]


def handle_get_unit_control(address, sequence_number):
    global unit_control
    msg_data = bytearray()
    msg_data.append(unit_control.target_temp['val1'])
    msg_data.append(unit_control.target_temp['val2'])
    msg_data.append(unit_control.current_temp['val1'])
    msg_data.append(unit_control.current_temp['val2'])
    msg_data.append(unit_control.mode)
    msg_data.append(unit_control.on_off)
    msg_data.append(unit_control.fan_speed)
    msg_data.append(unit_control.unit_control_type)

    message_type = 0x01
    message_id = 0x02

    message = pack_message(address, message_type, message_id, msg_data, sequence_number, 0)
    ser.write(message)
    hex_message = ' '.join(['0x' + byte.to_bytes(1, 'big').hex() for byte in message])
    print("send", hex_message)


def handle_status_code_unit_control(error_code):
    print("Status Code Unit Control: Error")


def handle_unit_control(address, message_id, data_bytes, sequence_number):
    if message_id.hex() == '01':
        print("Set Unit Control:")
        handle_set_unit_control(data_bytes)
    elif message_id.hex() == '03':
        print("Get Unit Control:")
        handle_get_unit_control(address, sequence_number)
    elif message_id.hex() == '04':
        error_code = data_bytes[0]
        handle_status_code_unit_control(error_code)
    else:
        print("Unknown message Id:", message_id.hex())   

def process_message(address_bytes, message_type, message_id, data_bytes, sequence_number):
    if message_type.hex() == '01':  # Unit Control
        handle_unit_control(address_bytes, message_id, data_bytes, sequence_number)
    else:
        print("Unknown message type:", message_type.hex())


def reader():
    while True:
        # Wait for start byte
        while True:
            start_byte = ser.read(1)
            if start_byte == b'\x7E':
                break

        # Start byte found, read rest of message
        length_byte = ser.read(1)
        length = int.from_bytes(length_byte, 'big')
        uart_ack = ser.read(1)
        address_bytes = ser.read(2)
        message_type = ser.read(1)
        message_id = ser.read(1)
        data_bytes = b''
        if length > 6:
            data_bytes = ser.read(length - 4)
        sequence_number = ser.read(1)
        crc_bytes = ser.read(2)
        end_byte = ser.read(1)

        if end_byte != b'\x7F':
            print("Error: Invalid end byte")
        else:
            message = (start_byte + length_byte + uart_ack + address_bytes[::-1] + message_type +
                       message_id + data_bytes + sequence_number + crc_bytes + end_byte)
            hex_message = ' '.join(['0x' + byte.to_bytes(1, 'big').hex() for byte in message])
            print("receive", hex_message)
            process_message(address_bytes, message_type, message_id, data_bytes, sequence_number)


serial_worker = threading.Thread(target=reader, daemon=True)
serial_worker.start()

done = False
while not done:
    if msvcrt.kbhit():
        print("key pressed, stopping")
        done = True

