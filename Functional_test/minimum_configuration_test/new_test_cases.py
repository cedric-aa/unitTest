

test_cases = [
    {
        "name": "Test Set Unit Control",
        "send_command": {"uart_ack": 0x00,"message_type": 0x01, "message_id": 0x01,"data": [0x01, 0x01, 0x02,0x01, 0x01, 0x02,0x01, 0x01],"sequence_number": 0x00},
        "expected_response": {"message_type": 0x01, "message_id": 0x04, "error_code": 0x00,"sequence_number": 0x00},
    },
    {
        "name": "Test Get Unit Control",
        "send_command": {"uart_ack": 0x00,"message_type": 0x01, "message_id": 0x03,"data": [],"sequence_number": 0x00},
        "expected_response": {"message_type": 0x01, "message_id": 0x04, "error_code": 0x00,"sequence_number": 0x00},
    },
     {
        "name": "Test Motor Set ID",
        "send_command": {"uart_ack": 0x00, "message_type": 0x04, "message_id": 0x01, "data": [0x01, 0x28], "sequence_number": 0x00},
        "expected_response": {"message_type": 0x04, "message_id": 0x04, "error_code": 0x00, "sequence_number": 0x00},
    },
    {
        "name": "Test Motor Get ID",
        "send_command": {"uart_ack": 0x00, "message_type": 0x04, "message_id": 0x06, "data": [0x01], "sequence_number": 0x00},
        "expected_response": {"message_type": 0x04, "message_id": 0x04, "error_code": 0x00, "sequence_number": 0x00},
    },
    {
        "name": "Test Get Motor All",
        "send_command": {"uart_ack": 0x00, "message_type": 0x04, "message_id": 0x03, "data": [], "sequence_number": 0x00},
        "expected_response": {"message_type": 0x04, "message_id": 0x04, "error_code": 0x00, "sequence_number": 0x00},
    },
    {
        "name": "Test Get Activation",
        "send_command": {"uart_ack": 0x00, "message_type": 0x02, "message_id": 0x03, "data": [], "sequence_number": 0x00},
        "expected_response": {"message_type": 0x02, "message_id": 0x04, "error_code": 0x00, "sequence_number": 0x00},
    },
        {
        "name": "Test Set Activation",
        "send_command": {"uart_ack": 0x00, "message_type": 0x02, "message_id": 0x01, "data": [0x01,0x01, 0x01, 0x01], "sequence_number": 0x00},
        "expected_response": {"message_type": 0x02, "message_id": 0x04, "error_code": 0x00, "sequence_number": 0x00},
    },

]


