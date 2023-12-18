add_executable(
  gfxstream-generic-apigen
  ApiGen.cpp EntryPoint.cpp getopt.c main.cpp Parser.cpp strUtils.cpp TypeFactory.cpp)
target_include_directories(gfxstream-generic-apigen PRIVATE ../..)
install(TARGETS gfxstream-generic-apigen DESTINATION ${PROJECT_BINARY_DIR})
