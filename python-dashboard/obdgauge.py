import serial
import threading
import math
import tkinter as tk
from tkinter import ttk
from matplotlib.figure import Figure
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
import matplotlib.animation as animation
from collections import deque

# ---- CONFIG ----
SERIAL_PORT = 'COM4'      # Promijeni po potrebi
BAUDRATE    = 115200
MAX_DATA_POINTS = 50      # Broj točaka za prikaz na grafovima

# Globalni podaci
obd_data = {
    'rpm': 0,
    'speed': 0,
    'coolant': 0,
    'intake': 0,
    'throttle': 0,
    'maf': 0,
    'map': 0,
    'lambdaV': 0,
    'runtime': 0,
    'fuelRate': 0
}

# Deque objekti za grafove
data_history = {key: deque(maxlen=MAX_DATA_POINTS) for key in obd_data.keys()}
time_history = deque(maxlen=MAX_DATA_POINTS)

class DashboardApp:
    def __init__(self, root):
        self.root = root
        self.root.title("OBD-II Dashboard")
        self.root.geometry("1200x800")
        
        # Stilizacija
        self.style = ttk.Style()
        self.style.configure('TFrame', background='#f0f0f0')
        self.style.configure('TLabel', background='#f0f0f0', font=('Helvetica', 10))
        self.style.configure('Title.TLabel', font=('Helvetica', 12, 'bold'))
        self.style.configure('Value.TLabel', font=('Helvetica', 14, 'bold'))
        
        # Glavni okvir
        self.main_frame = ttk.Frame(root)
        self.main_frame.pack(fill=tk.BOTH, expand=True)
        
        # Gornji dio - mjerni instrumenti
        self.top_frame = ttk.Frame(self.main_frame)
        self.top_frame.pack(fill=tk.X, padx=10, pady=10)
        
        # RPM mjerač
        self.rpm_frame = ttk.Frame(self.top_frame)
        self.rpm_frame.pack(side=tk.LEFT, padx=10)
        self.rpm_gauge = AnalogGauge(self.rpm_frame, "RPM", 0, 8000, unit="", size=200)
        
        # Brzina
        self.speed_frame = ttk.Frame(self.top_frame)
        self.speed_frame.pack(side=tk.LEFT, padx=10)
        self.speed_gauge = AnalogGauge(self.speed_frame, "Speed", 0, 240, unit="km/h", size=200)
        
        # Temperatura rashladne tekućine
        self.coolant_frame = ttk.Frame(self.top_frame)
        self.coolant_frame.pack(side=tk.LEFT, padx=10)
        self.coolant_gauge = AnalogGauge(self.coolant_frame, "Coolant Temp", -40, 120, unit="°C", size=200)
        
        # Otvor leptira za gas
        self.throttle_frame = ttk.Frame(self.top_frame)
        self.throttle_frame.pack(side=tk.LEFT, padx=10)
        self.throttle_gauge = AnalogGauge(self.throttle_frame, "Throttle", 0, 100, unit="%", size=200)
        
        # Srednji dio - digitalne vrijednosti
        self.middle_frame = ttk.Frame(self.main_frame)
        self.middle_frame.pack(fill=tk.X, padx=10, pady=10)
        
        # Lijevi stupac
        self.left_col = ttk.Frame(self.middle_frame)
        self.left_col.pack(side=tk.LEFT, expand=True)
        
        # Desni stupac
        self.right_col = ttk.Frame(self.middle_frame)
        self.right_col.pack(side=tk.LEFT, expand=True)
        
        # Digitalni prikazi
        self.create_digital_display(self.left_col, "Intake Air Temp", "intake", "°C")
        self.create_digital_display(self.left_col, "MAF", "maf", "g/s")
        self.create_digital_display(self.left_col, "MAP", "map", "kPa")
        
        self.create_digital_display(self.right_col, "Lambda Voltage", "lambdaV", "V")
        self.create_digital_display(self.right_col, "Runtime", "runtime", "s")
        self.create_digital_display(self.right_col, "Fuel Rate", "fuelRate", "L/h")
        
        # Donji dio - grafovi
        self.bottom_frame = ttk.Frame(self.main_frame)
        self.bottom_frame.pack(fill=tk.BOTH, expand=True, padx=10, pady=10)
        
        # RPM i brzina graf
        self.rpm_speed_fig = Figure(figsize=(8, 4), dpi=100)
        self.rpm_speed_ax = self.rpm_speed_fig.add_subplot(111)
        self.rpm_speed_canvas = FigureCanvasTkAgg(self.rpm_speed_fig, self.bottom_frame)
        self.rpm_speed_canvas.get_tk_widget().pack(side=tk.TOP, fill=tk.BOTH, expand=True)
        
        # Temperatura i lambda graf
        self.temp_lambda_fig = Figure(figsize=(8, 4), dpi=100)
        self.temp_lambda_ax = self.temp_lambda_fig.add_subplot(111)
        self.temp_lambda_canvas = FigureCanvasTkAgg(self.temp_lambda_fig, self.bottom_frame)
        self.temp_lambda_canvas.get_tk_widget().pack(side=tk.TOP, fill=tk.BOTH, expand=True)
        
        # Pokretanje serijske komunikacije
        self.start_serial_thread()
        
        # **EVO GDJE UBAČEŠ save_count i cache_frame_data**
        self.ani_rpm_speed = animation.FuncAnimation(
            self.rpm_speed_fig,
            self.update_rpm_speed_graph,
            interval=1000,
            blit=False,
            save_count=MAX_DATA_POINTS,
            cache_frame_data=False
        )
        self.ani_temp_lambda = animation.FuncAnimation(
            self.temp_lambda_fig,
            self.update_temp_lambda_graph,
            interval=1000,
            blit=False,
            save_count=MAX_DATA_POINTS,
            cache_frame_data=False
        )
    
    def create_digital_display(self, parent, title, data_key, unit):
        frame = ttk.Frame(parent)
        frame.pack(fill=tk.X, pady=5)
        
        title_label = ttk.Label(frame, text=title, style='Title.TLabel')
        title_label.pack(side=tk.LEFT, padx=10)
        
        value_label = ttk.Label(frame, text="0", style='Value.TLabel', width=10)
        value_label.pack(side=tk.RIGHT, padx=10)
        
        unit_label = ttk.Label(frame, text=unit)
        unit_label.pack(side=tk.RIGHT)
        
        setattr(self, f"{data_key}_label", value_label)
    
    def start_serial_thread(self):
        self.serial_thread = SerialReader(SERIAL_PORT, BAUDRATE, self.update_data)
        self.serial_thread.start()
    
    def update_data(self, data):
        for key, value in data.items():
            obd_data[key] = value
            data_history[key].append(value)
        time_history.append(obd_data['runtime'])
        self.root.after(0, self.update_gui)
    
    def update_gui(self):
        self.rpm_gauge.set_value(obd_data['rpm'])
        self.speed_gauge.set_value(obd_data['speed'])
        self.coolant_gauge.set_value(obd_data['coolant'])
        self.throttle_gauge.set_value(obd_data['throttle'])
        
        self.intake_label.config(text=f"{obd_data['intake']}")
        self.maf_label.config(text=f"{obd_data['maf']:.2f}")
        self.map_label.config(text=f"{obd_data['map']}")
        self.lambdaV_label.config(text=f"{obd_data['lambdaV']:.2f}")
        self.runtime_label.config(text=f"{obd_data['runtime']}")
        self.fuelRate_label.config(text=f"{obd_data['fuelRate']:.2f}")
    
    def update_rpm_speed_graph(self, frame):
        self.rpm_speed_ax.clear()
        if len(time_history) > 1:
            self.rpm_speed_ax.plot(time_history, data_history['rpm'], 'r-', label='RPM')
            self.rpm_speed_ax.plot(time_history, [s * 50 for s in data_history['speed']], 'b-', label='Speed (x50)')
            self.rpm_speed_ax.set_title('RPM and Speed Over Time')
            self.rpm_speed_ax.set_xlabel('Runtime (s)')
            self.rpm_speed_ax.set_ylabel('Value')
            self.rpm_speed_ax.legend()
            self.rpm_speed_ax.grid(True)
        return []
    
    def update_temp_lambda_graph(self, frame):
        self.temp_lambda_ax.clear()
        if len(time_history) > 1:
            self.temp_lambda_ax.plot(time_history, data_history['coolant'], 'r-', label='Coolant Temp')
            self.temp_lambda_ax.plot(time_history, data_history['intake'], 'b-', label='Intake Temp')
            self.temp_lambda_ax.plot(time_history, [v * 100 for v in data_history['lambdaV']], 'g-', label='Lambda (x100)')
            self.temp_lambda_ax.set_title('Temperatures and Lambda Over Time')
            self.temp_lambda_ax.set_xlabel('Runtime (s)')
            self.temp_lambda_ax.set_ylabel('Value')
            self.temp_lambda_ax.legend()
            self.temp_lambda_ax.grid(True)
        return []

class AnalogGauge(tk.Canvas):
    def __init__(self, parent, title, min_val, max_val, unit="", size=200):
        super().__init__(parent, width=size, height=size+40, bg='white', highlightthickness=0)
        self.size = size
        self.center = (size/2, size/2)
        self.radius = size * 0.4
        self.min_val = min_val
        self.max_val = max_val
        self.unit = unit
        self.title = title
        
        # Naslov
        self.create_text(size/2, 20, text=title, font=('Helvetica', 12, 'bold'))
        
        # Digitalni prikaz ispod kazaljke
        self.value_text = self.create_text(size/2, size-10, text="0", font=('Helvetica', 14, 'bold'))
        if unit:
            self.create_text(size/2 + 40, size-10, text=unit, font=('Helvetica', 10))
        
        # Crtanje lica mjerača
        self.draw_face()
        
        # Inicijalizacija kazaljke
        self.needle = self.create_line(0, 0, 0, 0, width=3, fill='red')
        
        self.pack()
    
    def draw_face(self):
        cx, cy = self.center
        r = self.radius
        
        # Vanjski krug
        self.create_oval(cx-r, cy-r, cx+r, cy+r, width=2)
        
        # Podjela - svakih 10% opsega
        range_val = self.max_val - self.min_val
        for i in range(0, 11):
            value = self.min_val + (i * range_val / 10)
            angle = 180 - (i * 180 / 10)
            angle_rad = math.radians(angle)
            inner = r * 0.85 if i % 2 else r * 0.8
            outer = r * 0.95 if i % 2 else r * 0.9
            x0 = cx + math.cos(angle_rad) * inner
            y0 = cy - math.sin(angle_rad) * inner
            x1 = cx + math.cos(angle_rad) * outer
            y1 = cy - math.sin(angle_rad) * outer
            width = 2 if i % 2 else 1
            self.create_line(x0, y0, x1, y1, width=width)
            if i % 2 == 0:
                tx = cx + math.cos(angle_rad) * (r * 0.70)
                ty = cy - math.sin(angle_rad) * (r * 0.70)
                self.create_text(tx, ty, text=str(int(value)), font=('Helvetica', 10))
    
    def set_value(self, value):
        value = max(self.min_val, min(value, self.max_val))
        self.itemconfig(self.value_text, text=f"{value:.0f}")
        normalized = (value - self.min_val) / (self.max_val - self.min_val)
        angle = 180 - (normalized * 180)
        angle_rad = math.radians(angle)
        cx, cy = self.center
        r = self.radius
        x = cx + math.cos(angle_rad) * r * 0.8
        y = cy - math.sin(angle_rad) * r * 0.8
        self.coords(self.needle, cx, cy, x, y)

class SerialReader(threading.Thread):
    def __init__(self, port, baudrate, callback):
        super().__init__(daemon=True)
        self.port = port
        self.baudrate = baudrate
        self.callback = callback
        self.running = True
    
    def run(self):
        try:
            with serial.Serial(self.port, self.baudrate, timeout=1) as ser:
                while self.running:
                    line = ser.readline().decode('utf-8', errors='ignore').strip()
                    if line:
                        self.parse_line(line)
        except serial.SerialException as e:
            print(f"Serial error: {e}")
    
    def parse_line(self, line):
        try:
            if not line.startswith('RPM:'):
                return
            parts = line.split(',')
            data = {}
            for part in parts:
                if ':' in part:
                    key, value = part.split(':', 1)
                    value = ''.join(c for c in value if c.isdigit() or c in {'.', '-'})
                    if key == 'RPM':
                        data['rpm'] = int(value) if value else 0
                    elif key == 'S':
                        data['speed'] = int(value) if value else 0
                    elif key == 'C':
                        data['coolant'] = int(value) if value else 0
                    elif key == 'I':
                        data['intake'] = int(value) if value else 0
                    elif key == 'Thr':
                        data['throttle'] = int(value) if value else 0
                    elif key == 'MAF':
                        data['maf'] = float(value) if value else 0.0
                    elif key == 'MAP':
                        data['map'] = int(value) if value else 0
                    elif key == 'λ':
                        data['lambdaV'] = float(value) if value else 0.0
                    elif key == 'Run':
                        data['runtime'] = int(value) if value else 0
                    elif key == 'FR':
                        data['fuelRate'] = float(value) if value else 0.0
            if data:
                self.callback(data)
        except Exception as e:
            print(f"Error parsing line: {e}")

if __name__ == '__main__':
    root = tk.Tk()
    app = DashboardApp(root)
    root.mainloop()
