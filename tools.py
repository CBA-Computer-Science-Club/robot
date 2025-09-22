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

TOOLS.extend([
    {
        "type": "function",
        "function": {
            "name": "memory_add",
            "description": "Store a short piece of information in the robot's memory.",
            "parameters": {
                "type": "object",
                "properties": {
                    "key": {"type": "string", "description": "Unique key for the memory"},
                    "value": {"type": "string", "description": "The value to remember"}
                },
                "required": ["key", "value"]
            }
        }
    },
    {
        "type": "function",
        "function": {
            "name": "memory_get",
            "description": "Retrieve a memory by key.",
            "parameters": {
                "type": "object",
                "properties": {
                    "key": {"type": "string", "description": "Key of the memory to retrieve"}
                },
                "required": ["key"]
            }
        }
    },
    {
        "type": "function",
        "function": {
            "name": "memory_search",
            "description": "Search memories by a query string.",
            "parameters": {
                "type": "object",
                "properties": {
                    "query": {"type": "string", "description": "Partial text to search for"}
                },
                "required": ["query"]
            }
        }
    },
    {
        "type": "function",
        "function": {
            "name": "memory_forget",
            "description": "Remove a memory by key.",
            "parameters": {
                "type": "object",
                "properties": {
                    "key": {"type": "string", "description": "Key to forget"}
                },
                "required": ["key"]
            }
        }
    },
])
