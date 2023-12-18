
#ifndef V4L2_CAMERA_HAL_ARRAY_VECTOR_H_
#define V4L2_CAMERA_HAL_ARRAY_VECTOR_H_

#include <array>
#include <vector>

namespace v4l2_camera_hal {
// ArrayVector behaves like a std::vector of fixed length C arrays,
// with push_back accepting std::arrays to standardize length.
// Specific methods to get number of arrays/number of elements
// are provided and an ambiguous "size" is not, to avoid accidental
// incorrect use.
template <class T, size_t N>
class ArrayVector {
 public:
  const T* data() const { return mItems.data(); }
  // The number of arrays.
  size_t num_arrays() const { return mItems.size() / N; }
  // The number of elements amongst all arrays.
  size_t total_num_elements() const { return mItems.size(); }

  // Access the ith array.
  const T* operator[](int i) const { return mItems.data() + (i * N); }
  T* operator[](int i) { return mItems.data() + (i * N); }

  void push_back(const std::array<T, N>& values) {
    mItems.insert(mItems.end(), values.begin(), values.end());
  }

 private:
  std::vector<T> mItems;
};

}  // namespace v4l2_camera_hal

#endif  // V4L2_CAMERA_HAL_ARRAY_VECTOR_H_
