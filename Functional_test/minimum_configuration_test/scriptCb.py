import os
import threading
import sys
import time
import traceback
import msvcrt

import serial
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



calculator = Calculator(Crc16.CCITT)
ser = serial.serial_for_url('COM6', baudrate=115200, timeout=0)



commands = (
    bytes((0x01, 0x01, 0x01, 0x01, 0x02, 20, 0, 24, 0, 19)),    # Set - Unit Control : Cool, On, Fan speed 2, Current Temp: 20.0, Target Temp: 24.0
    bytes([0x01, 0x03]), #Get Unit Control
    bytes((0x04, 0x01, 1, 40)), # Motor Set Id
    bytes((0x04, 0x06, 1)), # Motor Get Id : MotorId = 1 
    bytes([0x04, 0x03]), # Get Motor all
    bytes([0x02, 0x03]), # Get Activation  
    bytes([0x02, 0x01,0x01,0x01, 0x01, 0x01]), # Set Activation    
    
)

def addSequenceNumberAndPackWithAddress(address: int, msgData: bytes, sequenceNumber: int) -> bytes:
    payload = b'\x00' + \
        address.to_bytes(2, 'big') + msgData + \
        sequenceNumber.to_bytes(1, 'big')
    crcBytesLittleEndian = calculator.checksum(payload).to_bytes(2, 'little')

    return b'\x7E' + len(payload).to_bytes(1, 'big') + payload + crcBytesLittleEndian + b'\x7F'


def handle_unit_control(message_id, data_bytes):
    if message_id == 0x01:
        print("Set Unit Control:")
    elif message_id == 0x03:
        print("Get Unit Control:")
    elif message_id == 0x04:
        error_code = data_bytes[0]
        sequence_number = data_bytes[1]
        print("Status Code Unit Control: Error Code = {}, Sequence Number = {}".format(error_code, sequence_number))

def handle_motor_control(message_id, data_bytes):
    if message_id == 0x01:
        print("Motor Set Id:")
    elif message_id == 0x06:
        print("Motor Get Id:")
    elif message_id == 0x03:
        print("Get Motor all:")
    elif message_id == 0x04:
        error_code = data_bytes[0]
        sequence_number = data_bytes[1]
        print("Status Code Motor: Error Code = {}, Sequence Number = {}".format(error_code, sequence_number))

def handle_activation_control(message_id, data_bytes):
    if message_id == 0x03:
        print("Get Activation:")
    elif message_id == 0x01:
        print("Set Activation:")
    elif message_id == 0x04:
        error_code = data_bytes[0]
        sequence_number = data_bytes[1]
        print("Status Code Activation: Error Code = {}, Sequence Number = {}".format(error_code, sequence_number))

def parse_message(message_type, message_id, data_bytes):
    message_type_int = int.from_bytes(message_type, 'big')
    message_id_int = int.from_bytes(message_id, 'big')

    if message_type_int == 0x01:  # Unit Control
        handle_unit_control(message_id_int, data_bytes)
    elif message_type_int == 0x04:  # Motor Control
        handle_motor_control(message_id_int, data_bytes)
    elif message_type_int == 0x02:  # Activation Control
        handle_activation_control(message_id_int, data_bytes)
    else:
        print("Unknown message type:", message_type.hex())



def reader():
    while True:
        # Wait for start byte
        while True:
            startByte = ser.read(1)
            if startByte == b'\x7E':
                break
        # Start byte found, read rest of message
        lengthByte = ser.read(1)
        length = int.from_bytes(lengthByte, 'big')
        uartAck = ser.read(1)
        addressBytes = ser.read(2)
        address = int.from_bytes(addressBytes, 'little')
        messageType = ser.read(1)
        messageId = ser.read(1)
        dataBytes = ser.read(length - 5)
        crcBytes = ser.read(2)
        endByte = ser.read(1)
        if endByte != b'\x7F':
            print("Error: Invalid end byte")
        else:
            message = ' '.join(['0x' + byte.hex() for byte in (startByte, lengthByte, uartAck, addressBytes[::-1], messageType, messageId, dataBytes, crcBytes, endByte)])
         print("receive",message)
            parse_message(messageType, messageId, dataBytes)  # Call parse_message function

def sender():
    sequenceNumber = 0  # Initialize sequence number
    while True:
          # Increment sequence number
        if sequenceNumber > 255:
            sequenceNumber = 0
        for command in commands:
            message = addSequenceNumberAndPackWithAddress(0x9601, command, sequenceNumber)
            hex_message = ' '.join(['0x' + byte.to_bytes(1, 'big').hex() for byte in message])          
            print("send",hex_message)
            ser.write(message)
            time.sleep(5)
            sequenceNumber += 1
        


serialWorker = threading.Thread(target=reader, daemon=True)
serialWorker.start()
serialSender = threading.Thread(target=sender, daemon=True)
serialSender.start()
done = False
while not done:
    # print("ticking\n")
    # time.sleep(1)

    if msvcrt.kbhit():
        print("key pressed, stopping")
        done = True
