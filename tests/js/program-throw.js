/**
 * @file        program-throw.js
 *              Support code program-throw.bash
 * @author      Wes Garland, wes@distributive.network
 * @date        July 2023
 */

setTimeout(() => console.error('goodbye'), 6000)
throw new Error('hello')
