file(GLOB my_srcs 
main.cpp *.c *.cpp hw/*.c hw/*.cpp dsp/*.c dsp/*.cpp dkr/*.c dkr/*.cpp v1/*.c v1/*.cpp
mi/clouds/*.cc mi/clouds/drivers/*.cc mi/clouds/dsp/*.cc mi/clouds/dsp/pvoc/*.cc mi/stmlib/dsp/*.cc mi/stmlib/utils/*.cc)

idf_component_register(
SRCS ${my_srcs}
INCLUDE_DIRS "." "hw" "dsp" "dkr" "v1" "mi" "mi/clouds" "mi/clouds/drivers" "mi/clouds/dsp" "mi/clouds/dsp/fx" "mi/clouds/dsp/pvoc" "mi/stmlib" "mi/stmlib/dsp" "mi/stmlib/fft" "mi/stmlib/utils"
REQUIRES freertos bootloader_support nvs_flash driver spi_flash heap esp_rom fatfs sdmmc vfs app_update esp32)
