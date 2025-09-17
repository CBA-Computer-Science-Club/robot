TOOLS = [
    {
        "type": "function",
        "function": {
            "name": "move_forward",
            "description": "Move the robot forward for a certain number of seconds.",
            "parameters": {
                "type": "object",
                "properties": {
                    "duration": {
                        "type": "number",
                        "description": "How long to move in seconds"
                    }
                },
                "required": ["duration"]
            }
        }
    },
    {
        "type": "function",
        "function": {
            "name": "turn_left",
            "description": "Turn the robot left by a specified number of degrees.",
            "parameters": {
                "type": "object",
                "properties": {
                    "degrees": {
                        "type": "number",
                        "description": "Degrees to turn"
                    }
                },
                "required": ["degrees"]
            }
        }
    },
    {
        "type": "function",
        "function": {
            "name": "reset_conversation",
            "description": "Clear the current conversation history/context and start a new one.",
        }
    }
]
