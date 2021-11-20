

class _internal_led:
    def __init__(self, pin):
        from machine import Pin
        self.pin = Pin(pin, Pin.OUT, value=1)

    def on(self):
        self.pin.off()

    def off(self):
        self.pin.on()

led0 = _internal_led('B0')
led1 = _internal_led('B1')
led2 = _internal_led('B2')


