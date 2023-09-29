/**
 * @file        quint.js
 *              A minimum testing framework with QUnit-like (https://qunitjs.com/) APIs
 * @author      Tom Tang <xmader@distributive.network>
 * @date        Aug 2023
 */

const QUnitAssert = {
  arity(fn, length) 
  {
    if (fn.length !== length) throw new Error(`'${fn}' does not have arity of ${length}`);
  },
  isFunction(x) 
  {
    if (typeof x !== 'function') throw new Error(`'${x}' is not a function`);
  },
  name(x, name)
  {
    if (x.name !== name) throw new Error(`'${x}' does not have a name of ${name}`);
  },
  true(x)
  {
    if (x !== true) throw new Error(`'${x}' is not true`);
  },
  false(x)
  {
    if (x !== false) throw new Error(`'${x}' is not false`);
  },
  same(a, b)
  {
    if (a !== b) throw new Error(`'${a}' does not equal to '${b}'`);
  },
  arrayEqual(a, b)
  {
    if (JSON.stringify(a) !== JSON.stringify(b)) throw new Error(`'${a}' does not equal to '${b}'`);
  },
  throws(fn, error)
  {
    try
    {
      fn();
    } 
    catch (err)
    {
      if (!err.toString().includes(error)) throw new Error(`'${fn}' throws '${err}' but expects '${error}'`);
      return;
    }
    throw new Error(`'${fn}' does not throw`);
  },
  looksNative(fn)
  {
    if (!fn.toString().includes('[native code]')) throw new Error(`'${fn}' does not look native`);
  },
  enumerable(obj, propertyName)
  {
    const descriptor = Object.getOwnPropertyDescriptor(obj, propertyName);
    if (!descriptor.enumerable) throw new Error(`'${obj[Symbol.toStringTag]}.${propertyName}' is not enumerable`);
  },
};

const QUnit = {
  test(name, callback)
  {
    callback(QUnitAssert);
  },
  skip(name, callback)
  {
    // no op
  }
};

module.exports = QUnit;
