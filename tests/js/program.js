#! /usr/bin/env pmjs
/**
 * @file        program.js
 *              A program for use by pmjs *.bash tests which need a basic program module
 * @author      Wes Garland, wes@distributive.network
 * @date        July 2023
 */
'use strict';

if (arguments !== globalThis.arguments)
  throw new Error('arguments free variable is not the global arguments array!');

/* If the module cache is wrong, this will load again after -r of the same module */
require('./modules/print-load');

for (let argument of arguments)
  console.log('ARG', argument);
console.log('ARGC', arguments.length);
console.log('FINISHED - ran program', __filename);
