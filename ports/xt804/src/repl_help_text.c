
const char xt804_help_text[] = 
    "Welcome to MicroPython!\n"
    "\n"
    "For online help please visit https://micropython.org/help/.\n"
    "\n"
    "For access to the hardware use the 'machine' module. XT804 specific commands\n"
    "are in the 'xt804' module.\n"
    "\n"
    "Quick overview of some objects:\n"
    "  machine.Pin(pin) -- get a pin, eg machine.Pin(0)\n"
    "  machine.Pin(pin, m, [p]) -- get a pin and configure it for IO mode m, pull mode p\n"
    "    methods: init(..), value([v]), high(), low(), irq(handler)\n"
    "  machine.ADC(pin) -- make an analog object from a pin\n"
    "    methods: read_u16()\n"
    "  machine.PWM(pin) -- make a PWM object from a pin\n"
    "    methods: deinit(), freq([f]), duty_u16([d]), duty_ns([d])\n"
    "  machine.I2C(id) -- create an I2C object (id=0,1)\n"
    "    methods: readfrom(addr, buf, stop=True), writeto(addr, buf, stop=True)\n"
    "             readfrom_mem(addr, memaddr, arg), writeto_mem(addr, memaddr, arg)\n"
    "  machine.SPI(id, baudrate=1000000) -- create an SPI object (id=0,1)\n"
    "    methods: read(nbytes, write=0x00), write(buf), write_readinto(wr_buf, rd_buf)\n"
    "  machine.Timer(freq, callback) -- create a software timer object\n"
    "    eg: machine.Timer(freq=1, callback=lambda t:print(t))\n"
    "\n"
    "Pins are numbered 0-29, and 26-29 have ADC capabilities\n"
    "Pin IO modes are: Pin.IN, Pin.OUT, Pin.ALT\n"
    "Pin pull modes are: Pin.PULL_UP, Pin.PULL_DOWN\n"
    "\n"
    "Useful control commands:\n"
    "  CTRL-C -- interrupt a running program\n"
    "  CTRL-D -- on a blank line, do a soft reset of the board\n"
    "  CTRL-E -- on a blank line, enter paste mode\n"
    "\n"
    "For further help on a specific object, type help(obj)\n"
    "For a list of available modules, type help('modules')\n"
;
