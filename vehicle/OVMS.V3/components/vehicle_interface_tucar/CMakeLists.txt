set(srcs)
set(include_dirs)

list(APPEND srcs "vehicle_interface_tucar.cpp")
list(APPEND include_dirs "src")

# requirements can't depend on config
idf_component_register(SRCS ${srcs}
                       INCLUDE_DIRS ${include_dirs}
                       PRIV_REQUIRES "main"
                       WHOLE_ARCHIVE)
