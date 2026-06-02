# log_reader.py
import serial, time, csv, datetime

port = serial.Serial('COM4', 115200, timeout=1)

with open('drive1.csv', 'w', newline='') as f:
    wr = csv.writer(f)
    wr.writerow(["ts","rpm","speed","cool","intake","thr","maf",
                 "map","lambda","run","fr","lat_s","lat_avg",
                 "lat_max","tx","rx"])

    while True:
        line = port.readline().decode(errors='ignore').strip()
        if not line:
            continue

        ts = datetime.datetime.now().isoformat(timespec='milliseconds')

        if line.startswith("RPM:"):
            fields = line.split(',')
            data = dict(kv.split(':') for kv in fields)
            wr.writerow([
                ts, data["RPM"], data["S"].rstrip("km/h"), data["C"].rstrip("°C"),
                data["I"].rstrip("°C"), data["Thr"].rstrip('%'),
                data["MAF"].rstrip("g/s"), data["MAP"].rstrip("kPa"),
                data["λ"], data["Run"].rstrip('s'), data["FR"].rstrip("L/h"),
                "", "", "", "", ""
            ])
        elif line.startswith("LAT"):
            _, n, avg, mx = line.split(',')
            wr.writerow([ts,"","","","","","","","","","", n, avg, mx,"",""])
        elif line.startswith("BUS"):
            _, tx, rx = line.split(',')
            wr.writerow([ts,"","","","","","","","","","", "", "", "", tx, rx])

