import serial
import numpy as np
import matplotlib.pyplot as plt

frequency, impedance, phase = [], [], []

def get_reading(frequency):
    ser = serial.Serial('/dev/ttyACM0', 230400, timeout = 1)
    ser.write(f"{frequency}\n".encode('utf-8'))
    ser.flush()
    try:
        data = ser.readline().decode('utf-8').strip()
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


    



