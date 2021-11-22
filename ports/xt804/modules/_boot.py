import gc
import uos
from flashbdev import bdev

try:
    if bdev:
        print("uos.mount...")
        uos.mount(bdev, "/")
        print("mount done")
        uos.listdir()
        print("list done.")
except OSError:
    print("OSError...")
    import inisetup
    print("setup vfs...")
    vfs = inisetup.setup()

gc.collect()
