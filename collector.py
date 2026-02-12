import serial
import csv
import time

# 配置串口参数 (根据你的端口修改，如 'COM3' 或 '/dev/ttyUSB0')
SERIAL_PORT = 'COM7' 
BAUD_RATE = 115200

def start_collecting():
    try:
        ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)
        print(f"正在连接 {SERIAL_PORT}...")
        
        # 创建 CSV 文件
        with open('vibration_data.csv', 'w', newline='') as f:
            writer = csv.writer(f)
            # 写入表头
            writer.writerow(['Timestamp', 'Amplitude', 'Frequency'])
            
            print("正在采集数据... 按 Ctrl+C 停止")
            while True:
                line = ser.readline().decode('utf-8', errors='ignore').strip()
                
                # 识别震动数据
                if line.startswith("VIBE_DATA:"):
                    data_str = line.replace("VIBE_DATA:", "")
                    parts = data_str.split(',')
                    if len(parts) == 2:
                        amp, freq = parts[0], parts[1]
                        timestamp = time.strftime("%H:%M:%S", time.localtime())
                        writer.writerow([timestamp, amp, freq])
                        print(f"[震动] 时间:{timestamp} 幅度:{amp} 频率:{freq}Hz")

    except KeyboardInterrupt:
        print("\n采集停止，文件已保存。")
    except Exception as e:
        print(f"错误: {e}")
    finally:
        if 'ser' in locals(): ser.close()

if __name__ == "__main__":
    start_collecting()