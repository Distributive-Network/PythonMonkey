/**
 * @file        throw-filename.js
 *              A helper that throws when loaded, so that we can test that loading things throws with the right filename.
 * @author      Wes Garland, wes@distributive.network
 * @date        July 2023
 */
try
{
  throw new Error('lp0 on fire');
}
catch(error)
{
  console.log(error.stack);
}
