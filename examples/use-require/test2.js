'use strict'

exports.makeOutput = function makeOutput()
{
  const argv = Array.from(arguments);
  argv.unshift('TEST OUTPUT: ');
  debugPrint.apply(null, argv);
}

