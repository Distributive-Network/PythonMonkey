/**
 * @file        timer-reject.js
 *              Support code timer-reject.bash
 * @author      Wes Garland, wes@distributive.network
 * @date        July 2023
 */

setTimeout(async () => { throw new Error('goodbye') }, 600);
setTimeout(async () => { console.warn('this should not fire') }, 2000);
console.error('hello');
