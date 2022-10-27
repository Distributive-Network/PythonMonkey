#ifndef PythonMonkey_Utilities_
#define PythonMonkey_Utilities_

/**
 * @brief 
 * 
 * @tparam Base - child type
 * @tparam T - parent type
 * @param ptr - pointer to parent object
 * @return true - ptr is an instance of Base
 * @return false - ptr is not an instance of Base
 */
template <typename Base, typename T>
inline bool instanceof(const T *ptr) {
  return dynamic_cast<const Base *>(ptr) != nullptr;
}

#endif