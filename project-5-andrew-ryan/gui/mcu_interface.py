import tkinter as tk
from tkinter import ttk

from datetime import datetime
import time
import threading
import queue

# You will need to use the serial package to communicate with the MCU
import serial



class App(tk.Tk):
    def __init__(
        self,
        gui_rtc_date_time_queue,
        mcu_rtc_date_time_queue,
        moving_avg_window_size_queue,
        temperature_queue,
    ):
        super().__init__()
        self.title("EELE 465 MCU interface")

        # Resize logic is too complicated for me... :'(
        # Thus the window is not resizable ¯\_(ツ)_/¯
        self.resizable(False, False)

        self._create_widgets()

        self._gui_rtc_date_time_queue = gui_rtc_date_time_queue
        self._mcu_rtc_date_time_queue = mcu_rtc_date_time_queue
        self._moving_avg_window_size_queue = moving_avg_window_size_queue
        self._temperature_queue = temperature_queue

        # Update the temperature display every 500 ms. The gui needs to check the
        # temperature queue manually because GUI functions can't be called by the
        # mcu interface thread
        self._TEMPERATURE_UPDATE_PERIOD_MS = 500
        self.after(self._TEMPERATURE_UPDATE_PERIOD_MS, self._update_temperature)

        # Same thing as above for updating the date/time.
        self._DATE_TIME_UPDATE_PERIOD_MS = 500
        self.after(self._DATE_TIME_UPDATE_PERIOD_MS, self._update_rtc_date_time)

    def _create_widgets(self):
        PADDING = 10

        # This is configured for the following date format: yyyy/mm/dd hh:mm:ss
        DATE_TIME_WIDTH = 19

        # These are for display purposes on startup only. These values have no effect on the MCU
        DEFAULT_TEMPERATURE_VALUE = -99.0
        DEFAULT_RTC_DATE_TIME = "Waiting for RTC..."
        DEFAULT_MOVING_AVG_WINDOW_SIZE = 5

        root_frame = ttk.Frame(self, padding=PADDING)

        date_time_frame = ttk.Frame(root_frame, padding=PADDING)

        current_date_time_label = ttk.Label(date_time_frame, text="Current date/time:")

        self._rtc_date_time = tk.StringVar(value=DEFAULT_RTC_DATE_TIME)
        rtc_date_time_label = ttk.Label(
            date_time_frame, textvariable=self._rtc_date_time, width=DATE_TIME_WIDTH
        )

        sync_date_time_button = ttk.Button(
            date_time_frame,
            text="Sync date/time from computer",
            command=self._sync_date_time_callback,
        )

        current_date_time_label.grid(column=0, row=0)
        rtc_date_time_label.grid(column=1, row=0)
        sync_date_time_button.grid(column=2, row=0)

        temperature_frame = ttk.Frame(root_frame, padding=PADDING)

        temperature_label = ttk.Label(temperature_frame, text="Temperature:")

        self._temperature = tk.DoubleVar(value=DEFAULT_TEMPERATURE_VALUE)
        temperature_display = ttk.Label(
            temperature_frame, textvariable=self._temperature
        )

        temperature_unit = ttk.Label(temperature_frame, text="°C")

        temperature_label.grid(column=0, row=0)
        temperature_display.grid(column=1, row=0)
        temperature_unit.grid(column=2, row=0)

        moving_avg_window_frame = ttk.Frame(root_frame, padding=PADDING)

        moving_avg_window_size_label = ttk.Label(
            moving_avg_window_frame, text="Moving average window size"
        )

        self._moving_avg_window_size = tk.IntVar(value=DEFAULT_MOVING_AVG_WINDOW_SIZE)
        moving_avg_window_spinbox = ttk.Spinbox(
            moving_avg_window_frame,
            from_=1,
            to=16,
            width=2,
            textvariable=self._moving_avg_window_size,
        )

        set_moving_avg_window_size_button = ttk.Button(
            moving_avg_window_frame,
            text="Set",
            command=self._set_moving_avg_window_size_callback,
        )

        moving_avg_window_size_label.grid(column=0, row=1, padx=(0, PADDING))
        moving_avg_window_spinbox.grid(column=1, row=1, padx=(0, PADDING))
        set_moving_avg_window_size_button.grid(column=2, row=1)

        date_time_frame.pack()
        temperature_frame.pack()
        moving_avg_window_frame.pack()
        root_frame.pack()

    def register_close_callback(self, close_callback):
        """Register window close callback

        close_callback:
            Function to be called upon window close.
        """
        self.protocol("WM_DELETE_WINDOW", close_callback)

    def _update_temperature(self):
        """Fetch the most recent temperature from the temperature_queue"""
        if not self._temperature_queue.empty():
            temperature = self._temperature_queue.get()
            self._temperature.set(temperature)

        # We have to recurisvely trigger the "after" timer so this function is
        # called every _TEMPERATURE_UPDATE_PERIOD_MS instead of only once.
        self.after(self._TEMPERATURE_UPDATE_PERIOD_MS, self._update_temperature)

    def _update_rtc_date_time(self):
        """Fetch the most recent RTC date/time from the mcu_rtc_date_timequeue"""
        if not self._mcu_rtc_date_time_queue.empty():
            rtc_date_time = self._mcu_rtc_date_time_queue.get()
            self._rtc_date_time.set(rtc_date_time)

        # We have to recurisvely trigger the "after" timer so this function is
        # called every _DATE_TIME_UPDATE_PERIOD_MS instead of only once.
        self.after(self._DATE_TIME_UPDATE_PERIOD_MS, self._update_rtc_date_time)

    def _sync_date_time_callback(self):
        """Send the computer's date/time to the gui_rtc_date_time_queue

        You will likely need to modify this method to format the datetime
        in the way the MCU wants it.
        """
        # Grab the current date and time
        date_time = datetime.now()

        # Format according to how the MCU wants the date/time formatted
        formatted_datetime = date_time.strftime("%Y/%m/%d %H:%M:%S")

        # Push date/time string into the queue for the other thread to process
        self._gui_rtc_date_time_queue.put(formatted_datetime)

    def _set_moving_avg_window_size_callback(self):
        """Send the new moving average window size to the moving_avg_window_size_queue"""
        self._moving_avg_window_size_queue.put(self._moving_avg_window_size.get())


def mcu_interface_worker(
    gui_rtc_date_time_queue: queue.Queue,
    mcu_rtc_date_time_queue: queue.Queue,
    moving_avg_window_size_queue: queue.Queue,
    temperature_queue: queue.Queue,
    quit_event,
):
    """Thread for interfacing with the MCU's UART.

    This thread is where you will interface with the MCU's UART. You will
    need to read whatever the MCU is sending, parse it, then use the queues
    passed in as arguments to send the updated information to the GUI's thread.
    You will also need to check queues for updates from the GUI so you can send
    the date/time and window size to the MCU.

    This function is the primary thing you need to modify.

    Args:
        gui_rtc_date_time_queue:
            Queue the GUI uses to send us the RTC to set.
            The values in this queue will be sent to the MCU,
            which will then set the RTC registers accordingly.
        mcu_rtc_date_time_queue:
            Queue this thread uses to send the current RTC date and time to the GUI.
        moving_avg_window_size_quque:
            Queue for setting the moving average window size. The GUI puts values
            in this queue for us to send to the MCU.
        temperature_queue:
            Queue this thread uses to send the current temperature to the GUI.
        quit_event:
            threading.Event used to indicate that the program is being closed;
            this signals that we need to finish the thread and clean up so the
            main loop can join this thread and quit the program gracefully.
    """
    TEMPERATURE_UPDATE_PERIOD = 1
    last_temperature_update_time = 0
    temperature = 1.0

    rtc_date_time = None
    PORT = "COM5"
    #PORT = "/dev/cu.usbserial-10"
    BAUD_RATE = 115200

    ser = serial.Serial(PORT, BAUD_RATE, timeout=.5)

    while not quit_event.is_set():
        # Dummy for updating the temperature and time every 1 second.
        # Most things in this if statement can be deleted.
        current_time = time.time()

        # update current temperature reading from MCU
        try: 
            print(float(ser.readline().decode(errors="ignore").strip()))
            temperature = float(ser.readline().decode(errors="ignore").strip())
        except ValueError:
            print("Invalid temperature recieved, skipping...")
            continue
        if current_time - last_temperature_update_time >= TEMPERATURE_UPDATE_PERIOD:
            last_temperature_update_time = current_time
            temperature = (temperature) % 100

            # Send the most recent temperature to the GUI.
            temperature_queue.put(temperature)

            # You don't have to worry about what's happening here. This is just for testing.
            # You should fetch the time from UART stream
            if rtc_date_time:
                date_time = datetime.strptime(rtc_date_time, "%Y/%m/%d %H:%M:%S")

                # Make time count backwards just for fun
                # why did you do this, i just did extra debugging for this...
                # seconds = date_time.second - 1
                seconds = date_time.second + 1
                if seconds > 59:
                    seconds = 0
                date_time = date_time.replace(second=seconds)

                rtc_date_time = date_time.strftime("%Y/%m/%d %H:%M:%S")

                # Send the most recent RTC date/time to the GUI.
                mcu_rtc_date_time_queue.put(rtc_date_time)

        if not gui_rtc_date_time_queue.empty():
            # We've received an updated date/time from the GUI.
            # Send the updated date/time to the MCU.
            rtc_date_time = gui_rtc_date_time_queue.get()
            print(rtc_date_time)

        if not moving_avg_window_size_queue.empty():
            # We've reaceived an updated moving avg window size from the GUI.
            # Send the updated window size to the MCU.
            moving_avg_window_size = moving_avg_window_size_queue.get()
            message = f"WIN:{moving_avg_window_size}\n"
            ser.write(message.encode("ascii"))
            #ser.flush()
            print(f"window size = {moving_avg_window_size}")

    # Gracefully close serial port here...
    print("gracefully handling shutdown... :)")
    ser.close()


if __name__ == "__main__":
    # These queues are for passing data between the serial port thread and the GUI's event loop
    gui_rtc_date_time_queue = queue.Queue()
    mcu_rtc_date_time_queue = queue.Queue()
    moving_avg_window_size_queue = queue.Queue()
    temperature_queue = queue.Queue()

    quit_event = threading.Event()

    # Set up the mcu interface serial port thread
    mcu_interface_thread = threading.Thread(
        target=mcu_interface_worker,
        args=(
            gui_rtc_date_time_queue,
            mcu_rtc_date_time_queue,
            moving_avg_window_size_queue,
            temperature_queue,
            quit_event,
        ),
    )
    mcu_interface_thread.start()

    app = App(
        gui_rtc_date_time_queue,
        mcu_rtc_date_time_queue,
        moving_avg_window_size_queue,
        temperature_queue,
    )

    # Gracefully handle program close. This callback ensures that the
    # mcu interface thread gracefully finishes before the program exits.
    def close_callback():
        quit_event.set()
        mcu_interface_thread.join()
        app.destroy()

    app.register_close_callback(close_callback)

    app.mainloop()