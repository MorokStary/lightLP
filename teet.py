print("""
[82/101] Building C object esp-idf/net/CMakeFiles/__idf_net.dir/web_server.c.obj
[83/101] Building C object esp-idf/storage/CMakeFiles/__idf_storage.dir/storage.c.obj
[84/101] Building C object esp-idf/ota/CMakeFiles/__idf_ota.dir/ota_service.c.obj
[85/101] Building C object esp-idf/ota/CMakeFiles/__idf_ota.dir/ota_update.c.obj
[86/101] Linking C static library esp-idf/sensors/libsensors.a
[87/101] Linking C static library esp-idf/control/libcontrol.a
[88/101] Linking C static library esp-idf/storage/libstorage.a
[89/101] Linking C static library esp-idf/net/libnet.a
[90/101] Linking C static library esp-idf/ota/libota.a
[91/101] Linking C executable esp32_lighting_control.elf
[92/101] Generating binary image from built executable
[93/101] Running objcopy on esp32_lighting_control.elf
[94/101] Generating flash image...
[95/101] Running esptool.py to convert output
esptool.py v4.5
Generated binary image: build/esp32_lighting_control.bin
Generated ELF file: build/esp32_lighting_control.elf
Generated map file: build/esp32_lighting_control.map
Project build complete. To flash, run this command:

  idf.py -p /dev/ttyUSB0 flash
""")
