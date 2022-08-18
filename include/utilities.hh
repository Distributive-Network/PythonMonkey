#ifndef Bifrost_Utilities_
#define Bifrost_Utilities_

template <typename Base, typename T>
inline bool instanceof(const T *ptr) {
  return dynamic_cast<const Base *>(ptr) != nullptr;
}
#endif