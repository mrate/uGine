import math

steps = 90
step = 2 * math.pi / steps

for i in range(90):
    print("float4(", math.sin(i*step), math.cos(i*step), ", 0.0, 1.0),")