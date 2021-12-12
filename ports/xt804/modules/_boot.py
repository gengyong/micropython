import gc
import os

try:
    from xt804 import Partition
    bdev = Partition.find(Partition.TYPE_DATA, label="vfs")
    bdev = bdev[0] if bdev else None
    if bdev:
        print("os.mount...")
        os.mount(bdev, "/")
        print("mount done")
        os.listdir()
        print("list done.")
except OSError:
    print("OSError...")
    import inisetup
    print("setup vfs...")
    vfs = inisetup.setup()

gc.collect()
