print("========================================")
print("frozen scripts...")

freeze("$(PORT_DIR)/modules")
#freeze("$(MPY_DIR)/tools", ("upip.py", "upip_utarfile.py"))
#freeze("$(MPY_DIR)/drivers/dht", "dht.py")
#freeze("$(MPY_DIR)/drivers/onewire")
#include("$(MPY_DIR)/extmod/uasyncio/manifest.py")
#include("$(MPY_DIR)/extmod/webrepl/manifest.py")
#include("$(MPY_DIR)/drivers/neopixel/manifest.py")

print ("frozen scripts done.")
print("========================================")