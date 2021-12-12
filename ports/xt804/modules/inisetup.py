import os

def check_bootsec(bdev):
    buf = bytearray(bdev.ioctl(5, 0))  # 5 is SEC_SIZE
    bdev.readblocks(0, buf)
    empty = True
    for b in buf:
        if b != 0xFF:
            empty = False
            break
    if empty:
        return True
    fs_corrupted()


def fs_corrupted():
    import time

    while 1:
        print(
            """\
The filesystem appears to be corrupted. If you had important data there, you
may want to make a flash snapshot to try to recover it. Otherwise, perform
factory reprogramming of MicroPython firmware (completely erase flash, followed
by firmware programming).
"""
        )
        time.sleep(15000)


def setup(force=False):
    from xt804 import Partition
    bdev = Partition.find(Partition.TYPE_DATA, label="vfs")
    bdev = bdev[0] if bdev else None

    if not force:
        check_bootsec(bdev)
    print("Performing initial setup...")
    print("Making file system...")
    os.VfsLfs2.mkfs(bdev)
    vfs = os.VfsLfs2(bdev)
    os.mount(vfs, "/")
    print("File system mounted.")
    print("Generating scripts...")
    with open("boot.py", "w") as f:
        f.write(
            """\
# This file is executed on every boot (including wake-boot from deepsleep)
#import webrepl
#webrepl.start()
"""
        )
    print("Initial setup done.")
    return vfs
