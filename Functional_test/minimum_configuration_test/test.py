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
from test_cases import test_cases

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
response_queue = queue.Queue()  # Create a queue to store received data.

sequence_number = 0  # Initialize sequence number

def addSequenceNumberAndPackWithAddress(address: int, msgData: bytes, sequenceNumber: int) -> bytes:
    payload = b'\x00' + \
        address.to_bytes(2, 'big') + msgData + \
        sequenceNumber.to_bytes(1, 'big')
    crcBytesLittleEndian = calculator.checksum(payload).to_bytes(2, 'little')

    return b'\x7E' + len(payload).to_bytes(1, 'big') + payload + crcBytesLittleEndian + b'\x7F'



def send_command(command):
    message_type_bytes = command["message_type"].to_bytes(1, 'big')
    message_id = command["message_id"].to_bytes(1, 'big')
    data = b''.join([byte.to_bytes(1, 'big') for byte in command.get("data", [])])

    msgData = message_type_bytes + message_id + data
    message = addSequenceNumberAndPackWithAddress(0x9601, msgData, sequence_number)
    hex_message = ' '.join(['0x' + byte.to_bytes(1, 'big').hex() for byte in message])

    #print("send", hex_message)
    ser.write(message)




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
        error_code = data_bytes[0:1]
        sequence_number = data_bytes[1:2]
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
  #  message_type_int = int.from_bytes(message_type, 'big')
  #  message_id_int = int.from_bytes(message_id, 'big')

    if message_type == 0x01:  # Unit Control
        handle_unit_control(message_id, data_bytes)
    elif message_type == 0x04:  # Motor Control
        handle_motor_control(message_id, data_bytes)
    elif message_type == 0x02:  # Activation Control
        handle_activation_control(message_id, data_bytes)
    else:
        print("Unknown message type:", message_type.hex())
        



def wait_for_response(timeout=2):
    try:
        response = response_queue.get(timeout=timeout)
    except queue.Empty:
        print("Response not received within the timeout")
        return None

    # Process the response and return it as a dictionary.
    # You need to implement this function based on the format of the received data.
    return process_response(response)

def process_response(response):
    #print("Received response:", response)

    start_byte = response[0:1]
    length_byte = response[1:2]
    uart_ack_byte = response[2:3]
    address_bytes = response[3:5]
    message_type = int.from_bytes(response[5:6], 'big')  # Extract message type as an integer
    message_id = int.from_bytes(response[6:7], 'big')  # Extract message ID as an integer
    data_length = int.from_bytes(length_byte, 'big')
    data_bytes = response[7:2+data_length]

    data_list = list(data_bytes)
    response_dict = {"message_type": message_type, "message_id": message_id, "data": data_list}

    parse_message(message_type, message_id, data_bytes)

    return response_dict


def run_test_case(test_case):
    global sequence_number 
    print("Running test:", test_case["name"])
    send_command(test_case["send_command"])
    response = wait_for_response()

    # Check if message_type, message_id fields match, and the first value of data is the sequence number
    
    if response is None:
        print("\033[31mTest failed: No response received within the timeout message SequenceNumber:",sequence_number," \033[0m")
        return
    print("ok",sequence_number,"--",response["data"][1])
    if (response["message_type"] == test_case["expected_response"]["message_type"] and
            response["message_id"] == test_case["expected_response"]["message_id"] and
            response["data"][1] == sequence_number):
        print("\033[32mTest passed",response,"\033[0m")
    else:
        print("\033[31mTest failed: Expected:", test_case["expected_response"], "Received:", response,"\033[0m")
        
    
    



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
            message = (startByte + lengthByte + uartAck + addressBytes[::-1] + messageType +
                       messageId + dataBytes + crcBytes + endByte)
            response_queue.put(message)


def sender():
    global sequence_number  # Declare sequence_number as a global variable
    sequence_number = 0  # Initialize sequence number
    os.system('cls' if os.name == 'nt' else 'clear')
    while True:
        if sequence_number > 255:
            sequence_number = 0
        for test_case in test_cases:
                run_test_case(test_case)
                time.sleep(2) # Pause between test cases
                sequence_number = sequence_number + 1
                
                 
                
            

        


serialWorker = threading.Thread(target=reader, daemon=True)
serialWorker.start()
serialSender = threading.Thread(target=sender, daemon=True)
serialSender.start()
done = False 
while not done:
  
    if msvcrt.kbhit():
        print("key pressed, stopping")
        done = True