'use strict'

exports.makeOutput = function makeOutput()
{
  const argv = Array.from(arguments);
  argv.unshift('TEST OUTPUT: ');
  python.print.apply(null, argv);
}

