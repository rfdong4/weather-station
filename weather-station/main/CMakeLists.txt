# CMakeLists.txt in the main/ directory
idf_component_register(SRCS "weather-station.c"
                       INCLUDE_DIRS ".")

# Embed certificate files
target_add_binary_data(${COMPONENT_TARGET} "certs/client.crt" "BINARY")
target_add_binary_data(${COMPONENT_TARGET} "certs/client.key" "BINARY")
target_add_binary_data(${COMPONENT_TARGET} "certs/server.crt" "BINARY")