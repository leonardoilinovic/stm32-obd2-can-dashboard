# Python Dashboard

This folder contains the PC-side dashboard and serial logger for the STM32 OBD-II/CAN vehicle diagnostics project.

## Files

- `obdgauge.py` - Tkinter and Matplotlib dashboard for live visualization of OBD-II data
- `log_reader.py` - serial logger that stores received vehicle data into CSV files

## Requirements

Install the required Python packages:

```bash
pip install -r requirements.txt
```

Serial Port Configuration

The scripts currently use:

SERIAL_PORT = 'COM4'
BAUDRATE = 115200

Change the serial port if needed.

On Windows, the port usually looks like COM4, COM5, etc.
On Linux, it may look like /dev/ttyACM0 or /dev/ttyUSB0.

Notes

The STM32 firmware sends decoded OBD-II data over UART / USB virtual COM port.
The Python scripts parse this serial stream and display or log the values.
