
add_library(bearssl STATIC IMPORTED)
set_target_properties(bearssl PROPERTIES
  IMPORTED_LOCATION "${CMAKE_SOURCE_DIR}/lib/libbearssl.a"
  INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/inc/bearssl"
)

add_library(mqttpacket STATIC IMPORTED)
set_target_properties(mqttpacket PROPERTIES
  IMPORTED_LOCATION "${CMAKE_SOURCE_DIR}/lib/libMQTTPacketClient.a"
  INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/inc/paho"
)

add_executable(smart_switch
        smart_switch.c
        tcp_layer.c
        ssl_layer.c
        mqtt_layer.c)
        
target_link_libraries(smart_switch pico_stdlib bearssl mqttpacket)

pico_add_extra_outputs(smart_switch)

