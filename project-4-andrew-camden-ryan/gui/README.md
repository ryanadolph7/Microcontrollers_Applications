# MCU interface GUI

## Installation 

### Python

If you're on Linux or macOS, you likely already have python installed. If you're on Windows, use the latest [Python install manager](https://www.python.org/downloads/windows/).

### Python package manager

I suggest you use [uv](https://docs.astral.sh/uv/) to manage your python environment for this GUI.

### Dependencies

[Pyserial](https://pyserial.readthedocs.io/en/latest/pyserial.html) is the only package needed that isn't the standard library.

If you're using uv, all dependencies will be installed the first time you run the program.

If you're not using uv, you can just install `pyserial` globally using `pip`:
```bash
pip install pyserial
```

## Usage

To the run GUI, you will launch it from the command line.

With `uv`:
```bash
uv run mcu_interface.py
```

If you're not using `uv`, just run directly with python:
```bash
python3 mcu_interface.py
```

## Modifying the code

`mcu_interface_worker()` is the primary function you need to modify. That function is what you will use to read from write to the UART serial port. In essence, that thread is how you will interface with the MCU. You will essentially be doing some serial reads and writes using `pyserial` and putting/getting values from some queues.

Depending on how your MCU expects the date/time to be formatted when being set via UART, you will need to modify the `_sync_date_time_callback` method to format the datetime string how your MCU wants.

Beyond that, you shouldn't have to modify anything unless you want to.

Read through the code and comments for more information. Again, you don't have to worry about the GUI things---the layout, callbacks, etc.---just worry about the `mcu_interface_worker()` thread.