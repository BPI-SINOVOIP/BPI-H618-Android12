
// Mock for metadata readers.

#ifndef DEFAULT_CAMERA_HAL_METADATA_METADATA_READER_MOCK_H_
#define DEFAULT_CAMERA_HAL_METADATA_METADATA_READER_MOCK_H_

#include <gmock/gmock.h>

#include "metadata_reader.h"

namespace default_camera_hal {

class MetadataReaderMock : public MetadataReader {
 public:
  MetadataReaderMock() : MetadataReader(nullptr){};
  MOCK_CONST_METHOD0(raw_metadata, const camera_metadata_t*());
  MOCK_CONST_METHOD1(Facing, int(int*));
  MOCK_CONST_METHOD1(Orientation, int(int*));
  MOCK_CONST_METHOD1(MaxInputStreams, int(int32_t*));
  MOCK_CONST_METHOD3(MaxOutputStreams, int(int32_t*, int32_t*, int32_t*));
  MOCK_CONST_METHOD1(RequestCapabilities, int(std::set<uint8_t>*));
  MOCK_CONST_METHOD1(StreamConfigurations,
                     int(std::vector<StreamConfiguration>*));
  MOCK_CONST_METHOD1(StreamStallDurations,
                     int(std::vector<StreamStallDuration>*));
  MOCK_CONST_METHOD1(ReprocessFormats, int(ReprocessFormatMap*));
};

}  // namespace default_camera_hal

#endif  // DEFAULT_CAMERA_HAL_METADATA_METADATA_READER_MOCK_H_
