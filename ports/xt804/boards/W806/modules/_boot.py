import gc

gc.threshold((gc.mem_free() + gc.mem_alloc()) // 4)

print("boards/W806/_boot.py performed.")