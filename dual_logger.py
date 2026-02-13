import serial
import struct
import wave
import csv
import time
from datetime import datetime

PORT = "COM7"
BAUD = 921600
SAMPLE_RATE = 16000
CHUNK_SAMPLES = 256
CHUNK_BYTES = CHUNK_SAMPLES * 2

ser = serial.Serial(PORT, BAUD, timeout=1)

wav_samples = []

csv_file = open("vibration.csv", "w", newline="")
csv_writer = csv.writer(csv_file)
csv_writer.writerow(["time_ms", "amplitude", "frequency"])

print("Recording... Press Ctrl+C to stop")

start_time = time.time()

try:
    while True:

        header = ser.read(4)

        # ---------- 音频 ----------
        if header == b"AUD0":

            ts_bytes = ser.read(8)
            timestamp = struct.unpack('<q', ts_bytes)[0]

            data = ser.read(CHUNK_BYTES)
            if len(data) == CHUNK_BYTES:
                samples = struct.unpack('<256h', data)
                wav_samples.extend(samples)

        # ---------- 振动 ----------
        else:
            line = header + ser.readline()
            line = line.decode(errors="ignore").strip()

            if line.startswith("VIB"):
                parts = line.split(",")
                if len(parts) == 4:
                    timestamp_us = int(parts[1])
                    amp = float(parts[2])
                    freq = float(parts[3])

                    timestamp_str = datetime.fromtimestamp(timestamp_us/1e6)\
                                    .strftime("%Y-%m-%d %H:%M:%S.%f")[:-3]

                    csv_writer.writerow([timestamp_str, amp, freq])
                    print(timestamp_str, amp, freq)

except KeyboardInterrupt:
    print("Stopping...")

# 保存 WAV
print("Saving WAV...")
with wave.open("recorded.wav", 'w') as wf:
    wf.setnchannels(1)
    wf.setsampwidth(2)
    wf.setframerate(SAMPLE_RATE)
    wf.writeframes(struct.pack('<' + 'h'*len(wav_samples), *wav_samples))

csv_file.close()
ser.close()

print("Done.")
