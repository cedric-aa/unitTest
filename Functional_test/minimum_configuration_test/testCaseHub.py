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
from new_test_cases import test_cases

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
ser = serial.serial_for_url('COM10', baudrate=115200, timeout=2)
response_queue = queue.Queue()  # Create a queue to store received data.
cond = threading.Condition()

def close_serial_port():
    if ser is not None and ser.isOpen():
        ser.close()



def packWithAddress(address: int, msgData: bytes, sequenceNumber: bytes, uart_ack_byte: bytes) -> bytes:
    payload = uart_ack_byte + address.to_bytes(2, 'big') + msgData + sequenceNumber
    crcBytesLittleEndian = calculator.checksum(payload).to_bytes(2, 'little')

    return b'\x7E' + len(payload).to_bytes(1, 'big') + payload + crcBytesLittleEndian + b'\x7F'

def send_command1(command):
    message_type_bytes = command["message_type"].to_bytes(1, 'big')
    message_id = command["message_id"].to_bytes(1, 'big')
    data = b''.join([byte.to_bytes(1, 'big') for byte in command.get("data", [])])
    sequence_number = command["sequence_number"].to_bytes(1, 'big')
    uart_ack = command["uart_ack"].to_bytes(1, 'big')

    msgData = message_type_bytes + message_id + data
    message = packWithAddress(0x9601, msgData, sequence_number,uart_ack)
    hex_message = ' '.join(['0x' + byte.to_bytes(1, 'big').hex() for byte in message])
    
    
    # Retry sending the data until it's successfully sent
    while True:
        try:
            print("send",hex_message)
            ser.write(message)
            break  # Data was sent successfully, exit the loop
        except Exception as e:
            print("Error writing data to serial port:", e)
            time.sleep(0.1)  # Wait for a short time before retrying





def handle_unit_control(message_id, data_bytes,sequence_number):
    if message_id == 0x01:
        print("Set Unit Control:")
    elif message_id == 0x03:
        print("Get Unit Control:")
    elif message_id == 0x04:
        error_code = data_bytes[0]
        print("Status Code Unit Control: Error Code = ",error_code,"Sequence Number =", int.from_bytes(sequence_number))

def handle_motor_control(message_id, data_bytes,sequence_number):
    if message_id == 0x01:
        print("Motor Set Id:")
    elif message_id == 0x06:
        print("Motor Get Id:")
    elif message_id == 0x03:
        print("Get Motor all:")
    elif message_id == 0x04:
        error_code = data_bytes[0]
        print("Status Code Motor: Error Code = ",error_code,"Sequence Number =", int.from_bytes(sequence_number))

def handle_activation_control(message_id, data_bytes,sequence_number):
    if message_id == 0x03:
        print("Get Activation:")
    elif message_id == 0x01:
        print("Set Activation:")
    elif message_id == 0x04:
        error_code = data_bytes[0]
        print("Status Code Activation: Error Code = ",error_code,"Sequence Number =", int.from_bytes(sequence_number))

def parse_message(message_type, message_id, data_bytes,sequence_number):
  #  message_type_int = int.from_bytes(message_type, 'big')
  #  message_id_int = int.from_bytes(message_id, 'big')

    if message_type == 0x01:  # Unit Control
        handle_unit_control(message_id, data_bytes,sequence_number)
    elif message_type == 0x04:  # Motor Control
        handle_motor_control(message_id, data_bytes,sequence_number)
    elif message_type == 0x02:  # Activation Control
        handle_activation_control(message_id, data_bytes,sequence_number)
    else:
        print("Unknown message type:", message_type.hex())
        
def wait_for_response(timeout=5):
    with cond:
        try:
            response = response_queue.get(timeout=timeout)
        except queue.Empty:
            print("Response not received within the timeout")
            return None
    return process_response(response)

def process_response(response):
   # print("Received response:", response)

    start_byte = response[0:1]
    length_byte = response[1:2]
    uart_ack_byte = response[2:3]
    address_bytes = response[3:5]
    message_type = int.from_bytes(response[5:6], 'big')  # Extract message type as an integer
    message_id = int.from_bytes(response[6:7], 'big')  # Extract message ID as an integer
    data_length = int.from_bytes(length_byte, 'big')
    data_bytes = response[7:2+data_length-1]
    sequence_number = response[2+data_length-1:2+data_length]
    data_list = list(data_bytes)
    response_dict = {"message_type": message_type, "message_id": message_id, "data": data_list,"sequence_number":int.from_bytes(sequence_number)}

    parse_message(message_type, message_id, data_bytes,sequence_number)

    return response_dict




def run_test_case(test_case,sequence_number):
    
    print("Running test:", test_case["name"])
    test_case["send_command"]["sequence_number"] = sequence_number
    test_case["expected_response"]["sequence_number"]=sequence_number
    
    response_queue.queue.clear()
    send_command1(test_case["send_command"])
    response = wait_for_response()

    if response is None:
        print("\033[31mTest failed: No response received within the timeout message SequenceNumber:",sequence_number," \033[0m")
        return
    if (response["message_type"] == test_case["expected_response"]["message_type"] and
            response["message_id"] == test_case["expected_response"]["message_id"] and
            response["sequence_number"] == sequence_number):
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
            hex_message = ' '.join(['0x' + byte.to_bytes(1, 'big').hex() for byte in message])
            print("receive",hex_message)
            response_queue.put(message)

def sender():
    sequence_number = 1  # Initialize sequence number
    while True:
        for test_case in test_cases:
            if sequence_number > 255:
                sequence_number = 0
            run_test_case(test_case,sequence_number)
            sequence_number = sequence_number + 1
            time.sleep(10)
        time.sleep(120)
            
                
                 
                
            

        

serialSender = threading.Thread(target=sender, daemon=True)
serialSender.start()
serialWorker = threading.Thread(target=reader, daemon=True)
serialWorker.start()

done = False 
while not done:
  
    if msvcrt.kbhit():
        print("key pressed, stopping")
        done = True
        close_serial_port()