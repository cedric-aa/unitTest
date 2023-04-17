

test_cases = [
    {
        "name": "Test Get Unit Control",
        "send_command": {"message_type": 0x01, "message_id": 0x03},
        "expected_response": {"message_type": 0x01, "message_id": 0x04, "data": [0, 0]},
    },
]
