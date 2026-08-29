import serial
import matplotlib.pyplot as plt

frequency, impedance, phase = [], [], []

ser = serial.Serial('/dev/ttyACM0', 230400, timeout = 1)

try:
    while True:
        data = ser.readline().decode('utf-8').strip()
        if data == "Done":
            ser.close()
            break
        try:
            a, b, c = data.split(', ')
            frequency.append(float(a))
            impedance.append(float(b))
            phase.append(float(c))
        except ValueError:
            pass
finally:
    ser.close()
plt.scatter(frequency, impedance)
plt.show()


        
            




