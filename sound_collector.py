import serial
import csv
import wave
import struct
import time

SERIAL_PORT = 'COM7' 
BAUD_RATE = 921600  

def run_data_exporter():
    raw_samples = []
    start_time = None
    
    try:
        ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=0.01)
        ser.set_buffer_size(rx_size=1024000)
        ser.reset_input_buffer() 
        
        with open('vibration_data.csv', 'w', newline='') as f_csv:
            csv_writer = csv.writer(f_csv)
            # 增加毫秒列 HumanTime_MS
            csv_writer.writerow(['HumanTime_MS', 'UnixTimestamp', 'Value'])
            
            print(">>> 正在录制！按下 Ctrl+C 停止...")
            start_time = time.time()
            
            while True:
                if ser.in_waiting > 0:
                    line = ser.readline().decode('utf-8', errors='ignore').strip()
                    
                    if line.startswith('>V:'):
                        try:
                            val = line.split(':')[1]
                            now = time.time()
                            # 格式化为：时:分:秒.毫秒
                            milli = int((now % 1) * 1000)
                            h_time = time.strftime("%H:%M:%S", time.localtime(now)) + f".{milli:03d}"
                            csv_writer.writerow([h_time, now, val])
                        except: continue
                    
                    if line.startswith('>R:'):
                        try:
                            raw_val = int(line.split(':')[1])
                            raw_samples.append(max(-32768, min(32767, raw_val)))
                        except: continue

    except KeyboardInterrupt:
        duration = time.time() - start_time
        points = len(raw_samples)
        if points > 0:
            actual_hz = int(points / duration)
            print(f"\n录制结束！时长: {duration:.2f}s, 采样率: {actual_hz}Hz")
            with wave.open('sound_monitor.wav', 'wb') as f_wav:
                f_wav.setnchannels(1)
                f_wav.setsampwidth(2)
                f_wav.setframerate(actual_hz)
                for s in raw_samples:
                    f_wav.writeframes(struct.pack('<h', s))
            print("文件已保存。")
    finally:
        if 'ser' in locals(): ser.close()

if __name__ == "__main__":
    run_data_exporter()