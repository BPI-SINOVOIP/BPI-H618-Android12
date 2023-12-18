# Add pixel common soong config here.
# Set the variables properly in ProductConfig Makefiles for each pixel device

#For sensor
SOONG_CONFIG_sensor += \
        mag_no_temp \
        single_light

SOONG_CONFIG_sensor_mag_no_temp := true
SOONG_CONFIG_sensor_single_light := true
