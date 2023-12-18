add_library(
  lz4 lz4.c lz4frame.c lz4hc.c xxhash.c)
target_include_directories(lz4 PUBLIC .)
