#
# Main component makefile.
#
# This Makefile can be left empty. By default, it will take the sources in the 
# src/ directory, compile them and link them into lib(subdirectory_name).a 
# in the build directory. This behaviour is entirely configurable,
# please read the ESP-IDF documents if you need to do this.
#
COMPONENT_INCLUDES += $(GLO_PATH)/main
COMPONENT_INCLUDES += $(GLO_PATH)/main/hw
COMPONENT_INCLUDES += $(GLO_PATH)/main/dsp
COMPONENT_INCLUDES += $(GLO_PATH)/main/dkr
COMPONENT_INCLUDES += $(IDF_PATH)/components
