/**
 * @file TypeEnum.hh
 * @author Caleb Aikens (caleb@distributive.network) & Giovanni Tedesco (giovanni@distributive.network) & Tom Tang (xmader@distributive.network)
 * @brief Enum for every PyType
 * @version 0.1
 * @date 2022-08-08
 *
 * @copyright Copyright (c) 2023 Distributive Corp.
 *
 */

#ifndef PythonMonkey_TypeEnum_
#define PythonMonkey_TypeEnum_

enum class TYPE {
  DEFAULT,
  BOOL,
  INT,
  FLOAT,
  STRING,
  FUNC,
  DICT,
  LIST,
  TUPLE,
  DATE,
  PYTHONMONKEY_PROMISE,
  BUFFER,
  EXCEPTION,
  NONE,
  PYTHONMONKEY_NULL,
};

#endif