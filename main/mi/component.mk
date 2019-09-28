#
# Main component makefile.
#
# This Makefile can be left empty. By default, it will take the sources in the 
# src/ directory, compile them and link them into lib(subdirectory_name).a 
# in the build directory. This behaviour is entirely configurable,
# please read the ESP-IDF documents if you need to do this.
#
COMPONENT_INCLUDES += E:/dev/ESP32/workspace/whale-v150/main/mi
COMPONENT_INCLUDES += E:/dev/ESP32/workspace/whale-v150/main/mi/clouds
COMPONENT_INCLUDES += E:/dev/ESP32/workspace/whale-v150/main/hw
COMPONENT_INCLUDES += E:/dev/ESP32/workspace/whale-v150/main
COMPONENT_INCLUDES += E:/dev/ESP32/esp-idf/components

COMPONENT_SRCDIRS := clouds clouds/drivers clouds/dsp clouds/dsp/fx clouds/dsp/pvoc stmlib stmlib/dsp stmlib/fft stmlib/utils