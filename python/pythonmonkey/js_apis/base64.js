// @ts-check
///<reference path="./global.d.ts"/>

/*!
 * Modified from https://github.com/zloirock/core-js/blob/44cf9e8/packages/core-js/modules/web.atob.js and
 *               https://github.com/zloirock/core-js/blob/44cf9e8/packages/core-js/modules/web.btoa.js
 * core-js
 * MIT License, Copyright (c) 2014-2023 Denis Pushkarev
 */

var uncurryThis = function (fn) {
  // Modified from https://github.com/zloirock/core-js/blob/44cf9e8/packages/core-js/internals/
  return function () {
    return Function.prototype.call.apply(fn, arguments);
  };
};
var toString = (arg) => String(arg);
var hasOwn = Object.hasOwn;

// Taken from https://github.com/zloirock/core-js/blob/44cf9e8/packages/core-js/internals/validate-arguments-length.js
function validateArgumentsLength(passed, required) {
  if (passed < required) throw TypeError('Not enough arguments');
  return passed;
};

// Taken from https://github.com/zloirock/core-js/blob/44cf9e8/packages/core-js/internals/base64-map.js
var itoc = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=';
var ctoi = {};
for (var index = 0; index < 66; index++) ctoi[itoc.charAt(index)] = index;

var disallowed = /[^\d+/a-z]/i;
var whitespaces = /[\t\n\f\r ]+/g;
var finalEq = /[=]{1,2}$/;

var fromCharCode = String.fromCharCode;
var charAt = uncurryThis(''.charAt);
var charCodeAt = uncurryThis(''.charCodeAt);
var replace = uncurryThis(''.replace);
var exec = uncurryThis(disallowed.exec);

function atob(data) {
  validateArgumentsLength(arguments.length, 1);
  var string = replace(toString(data), whitespaces, '');
  var output = '';
  var position = 0;
  var bc = 0;
  var chr, bs;
  if (string.length % 4 == 0) {
    string = replace(string, finalEq, '');
  }
  if (string.length % 4 == 1 || exec(disallowed, string)) {
    // throw new (getBuiltIn('DOMException'))('The string is not correctly encoded', 'InvalidCharacterError');
    throw new Error('InvalidCharacterError: The string is not correctly encoded');
  }
  while (chr = charAt(string, position++)) {
    if (hasOwn(ctoi, chr)) {
      bs = bc % 4 ? bs * 64 + ctoi[chr] : ctoi[chr];
      if (bc++ % 4) output += fromCharCode(255 & bs >> (-2 * bc & 6));
    }
  } return output;
}

function btoa(data) {
  validateArgumentsLength(arguments.length, 1);
  var string = toString(data);
  var output = '';
  var position = 0;
  var map = itoc;
  var block, charCode;
  while (charAt(string, position) || (map = '=', position % 1)) {
    charCode = charCodeAt(string, position += 3 / 4);
    if (charCode > 0xFF) {
      // throw new (getBuiltIn('DOMException'))('The string contains characters outside of the Latin1 range', 'InvalidCharacterError');
      throw new Error('InvalidCharacterError: The string contains characters outside of the Latin1 range');
    }
    block = block << 8 | charCode;
    output += charAt(map, 63 & block >> 8 - position % 1 * 8);
  } return output;
}

defineGlobal("atob", atob)
defineGlobal("btoa", btoa)
