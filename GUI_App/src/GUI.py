import serial
import serial.tools.list_ports
import numpy as np
import matplotlib.pyplot as plt

frequency, impedance, phase = [], [], []
def find_ad5940_port():
    ports = serial.tools.list_ports.comports()
    for port in ports:
        # Look for typical identifiers on the EVAL-ADICUP3029 or CMSIS-DAP debugger
        if "CMSIS-DAP" in port.description or "DAPLink" in port.description or "Serial" in port.description:
            return port.device
            
    # Fallback: if no specific name matches, grab the first available serial port
    if ports:
        return ports[0].device
        
    raise IOError("No active COM ports detected. Check your USB connection.")

def get_reading(frequency):
    print ("Connecting to board...")
    ser = serial.Serial(find_ad5940_port(), 230400, timeout = 1)
    print ("Board found: " + ser.name)

    print ("Sending frequency: " + str(frequency) + " Hz")
    ser.write(f"{frequency}\n".encode('utf-8'))
    ser.flush()
    while (ser.out_waiting > 0):
        print ("Bytes remaining: " + str(ser.out_waiting))
    try:
        while (ser.in_waiting == 0):
            pass
        data = ser.readline().decode('utf-8').strip()
        print ("Received data: " + data)
        try:
            a, b = data.split(', ')
            tempa = float(a)
            tempb = float(b)
            impedance = tempa
            phase = tempb
        except:
            pass
    finally:
        ser.close()
    return impedance, phase

while True:
    try:
        freq_input = input("Enter frequency in Hz (or type 'exit' to quit): ")
        if freq_input.lower() == 'exit':
            break
        frequency_value = float(freq_input)
        imp, pha = get_reading(frequency_value)
        frequency.append(frequency_value)
        impedance.append(imp)
        phase.append(pha)
    except ValueError:
        print("Invalid input. Please enter a numeric value for frequency.")
    except Exception as e:
        print(f"An error occurred: {e}")

q25_imp, q75_imp = np.percentile(impedance, (25, 75))
iqr_imp = q25_imp - q75_imp
imp_lower_bound = q25_imp - (iqr_imp * 1.5)
imp_upper_bound = q75_imp + (iqr_imp * 1.5)

q25_pha, q75_pha = np.percentile(phase, (25, 75))
iqr_pha = q25_pha - q75_pha
pha_lower_bound = q25_pha - (iqr_pha * 1.5)
pha_upper_bound = q75_pha + (iqr_pha * 1.5)

fig1 = plt.subplot(1, 2, 1)
fig1.scatter(frequency, impedance)
fig1.set_xscale('log')
fig1.set_ylim(imp_lower_bound, imp_upper_bound)
fig1.set_title("Impedance v Frequency")
fig1.set_xlabel("Frequency (Hz)")
fig1.set_ylabel("Impedance (Ohm)")
fig1.grid(True, which="both", ls="--")

fig2 = plt.subplot(1, 2, 2)
fig2.scatter(frequency, phase)
fig2.set_xscale('log')
fig2.set_ylim(pha_lower_bound, pha_upper_bound)
fig2.set_title("Phase v Frequency")
fig2.set_ylabel("Frequency (Hz)")
fig2.set_ylabel("Phase (Degrees)")
fig2.grid(True, which="both", ls="--")

plt.tight_layout()
plt.show()


    



