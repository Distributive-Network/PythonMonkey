/**
 * @file        timer-throw.js
 *              Support code timer-throw.bash
 * @author      Wes Garland, wes@distributive.network
 * @date        July 2023
 */

setTimeout(() => { throw new Error('goodbye') }, 600);
console.error('hello');
