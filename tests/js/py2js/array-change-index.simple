/**
 * @file        py2js/array-chnage-index.simple
 *              Simple test which demonstrates modifying a list passed
 *              to JavaScript from Python and returning it to JS.
 * @author      Will Pringle, will@distributive.network
 * @date        July 2023
 */
'use strict';

python.exec(`
def modifyAndReturn(modifierFun):
  numbers = [1,2,3,4,5,6,7,8,9]
  modifierFun(numbers)
  return numbers
`);

const modifyAndReturn = python.eval('modifyAndReturn');
const numbers = modifyAndReturn((array) => {
  array[1] = 999;
});

if (numbers[1] !== 999)
  throw new Error('Python list not modified by JS');

console.log('pass');

