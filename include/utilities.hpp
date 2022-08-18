#ifndef UTILITIES_HPP
#define UTILITIES_HPP
template <typename Base, typename T>
inline bool instanceof(const T *ptr) {
  return dynamic_cast<const Base *>(ptr) != nullptr;
}
#endif