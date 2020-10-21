idf_component_register(SRCS "main.cpp"
							"wifihandler.cpp"
							"networkhandler.cpp"
							"EEPROMConfig.cpp"
							"camerahandler.cpp"
							"I2CHandler.cpp"
                    INCLUDE_DIRS ""
                    REQUIRES AsyncTCP lwip arduino-esp32 nvs_flash esp32-camera)

add_subdirectory(argus-utils/argus-netbuffer)

target_link_libraries(${COMPONENT_LIB} PRIVATE argus-libnetbuffer)
target_include_directories(${COMPONENT_LIB} PRIVATE argus-utils/argus-netbuffer)