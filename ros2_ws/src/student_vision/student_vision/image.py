"""Decode a small, explicit subset of raw ROS Image encodings without cv_bridge."""
from dataclasses import dataclass


@dataclass(frozen=True)
class DecodedImage:
    width: int
    height: int
    bgr: bytes


def decode_image(message):
    encoding = message.encoding.lower()
    if encoding not in ("rgb8", "bgr8", "mono8"):
        raise ValueError("Unsupported image encoding: " + encoding)
    width, height, step = message.width, message.height, message.step
    channels = 1 if encoding == "mono8" else 3
    if width < 1 or height < 1 or width > 4096 or height > 4096 or step < width * channels or step > 4096 * 8:
        raise ValueError("Invalid image dimensions or step")
    data = bytes(message.data)
    if len(data) != step * height:
        raise ValueError("Invalid image data length")
    output = bytearray(width * height * 3)
    for y in range(height):
        row = data[y * step:y * step + width * channels]
        for x in range(width):
            i = (y * width + x) * 3
            if encoding == "mono8":
                output[i:i + 3] = row[x:x + 1] * 3
            elif encoding == "rgb8":
                output[i:i + 3] = row[x * 3:x * 3 + 3][::-1]
            else:
                output[i:i + 3] = row[x * 3:x * 3 + 3]
    return DecodedImage(width, height, bytes(output))
