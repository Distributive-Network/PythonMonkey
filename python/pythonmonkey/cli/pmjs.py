#! /usr/bin/env python3
# @file         pmjs - PythonMonkey REPL
# @author       Wes Garland, wes@distributive.network
# @date         June 2023
# @copyright Copyright (c) 2023 Distributive Corp.

import sys
import os
import signal
import getopt
import readline
import asyncio
import pythonmonkey as pm
from pythonmonkey.lib import pmdb, wtfpm

globalThis = pm.eval("globalThis")
evalOpts = {'filename': __file__, 'fromPythonFrame': True, 'strict': False}  # type: pm.EvalOptions

if (os.getenv('PMJS_PATH')):
  requirePath = list(map(os.path.abspath, os.getenv('PMJS_PATH').split(',')))
else:
  requirePath = False

pm.eval("""'use strict';
const cmds = {};

cmds.help = function help() {
  return '.' +
`exit     Exit the REPL
.help     Print this help message
.load     Load JS from a file into the REPL session
.save     Save all evaluated commands in this REPL session to a file
.python   Evaluate a Python statement, returning result as global variable $n.
          Use '.python reset' to reset back to $1.
          Statement starting with 'from' or 'import' are silently executed.

Press Ctrl+C to abort current expression, Ctrl+D to exit the REPL`
};

cmds.exit = python.exit;
cmds.python = function pythonCmd(...args) {
  var retval;
  const cmd = args.join(' ').trim();

  if (cmd === 'reset')
  {
    for (let i=0; i < pythonCmd.serial; i++)
      delete globalThis['$' + i];
    pythonCmd.serial = 0;
    return;
  }

  if (cmd === '')
    return;

  try
  {
    if (arguments[0] === 'from' || arguments[0] === 'import')
      return python.exec(cmd);
    retval = python.eval(cmd);
  }
  catch(error)
  {
    globalThis._error = error;
    return util.inspect(error);
  }

  pythonCmd.serial = (pythonCmd.serial || 0) + 1;
  globalThis['$' + pythonCmd.serial] = retval;
  python.stdout.write('$' + pythonCmd.serial + ' = ');
  return util.inspect(retval);
};

/**
 * Handle a .xyz repl command. Invokes function cmds[XXX], passing arguments that the user typed as the
 * function arguments. The function arguments are space-delimited arguments; arguments surrounded by
 * quotes can include spaces, similar to how bash parses arguments. Argument parsing cribbed from
 * stackoverflow user Tsuneo Yoshioka, question 4031900.
 *
 * @param {string} cmdLine     the command the user typed, without the leading .
 * @returns {string} to display
 */
globalThis.replCmd = function replCmd(cmdLine)
{
  const cmdName = (cmdLine.match(/^[^ ]+/) || ['help'])[0];
  const args = cmdLine.slice(cmdName.length).trim();
  const argv = args.match(/\\\\?.|^$/g).reduce((p, c) => {
        if (c === '"')
          p.quote ^= 1;
        else if (!p.quote && c === ' ')
          p.a.push('');
        else
          p.a[p.a.length-1] += c.replace(/\\\\(.)/,"$1");
        return  p;
    }, {a: ['']}).a;

  if (!cmds.hasOwnProperty(cmdName))
    return `Invalid REPL keyword`;
  return cmds[cmdName].apply(null, argv);
}

/**
 * Evaluate a complete statement, built by the Python readline loop.
 */
globalThis.replEval = function replEval(statement)
{
  const indirectEval = eval;
  var originalStatement = statement;
  var result;
  var mightBeObjectLiteral = false;

  /* A statement which starts with a { and does not end with a ; is treated as an object literal,
   * and to get the parser in to Expression mode instead of Statement mode, we surround any expression
   * like that which is also a valid compilation unit with parens, then if that is a syntax error,
   * we re-evaluate without the parens.
   */
  if (/^\\s*\\{.*[^;\\s]\\s*$/.test(statement))
  {
    const testStatement = `(${statement})`;
    if (globalThis.python.pythonMonkey.isCompilableUnit(testStatement))
      statement = testStatement;
  }

  try
  {
    try
    {
      result = indirectEval(statement);
    }
    catch(evalError)
    {
      /* Don't retry paren-wrapped statements which weren't syntax errors, as they might have
       * side-effects. Never retry if we didn't paren-wrap.
       */
      if (!(evalError instanceof SyntaxError) || originalStatement === statement)
        throw evalError;
      globalThis._swallowed_error = evalError;
      result = indirectEval(originalStatement);
    }

    globalThis._ = result;
    return util.inspect(result);
  }
  catch(error)
  {
    globalThis._error = error;
    return util.inspect(error);
  }
}
""", evalOpts)


async def repl():
  """
  Start a REPL to evaluate JavaScript code in the extra-module environment. Multi-line statements and
  readline history are supported. ^C support is sketchy. Exit the REPL with ^D or ".quit".
  """

  print('Welcome to PythonMonkey v' + pm.__version__ + '.')
  print('Type ".help" for more information.')
  readline.parse_and_bind('set editing-mode emacs')
  histfile = os.getenv('PMJS_REPL_HISTORY') or os.path.expanduser('~/.pmjs_history')
  if (os.path.exists(histfile)):
    try:
      readline.read_history_file(histfile)
    except BaseException:
      pass

  got_sigint = 0
  statement = ''
  readline_skip_chars = 0
  inner_loop = False

  def save_history():
    nonlocal histfile
    readline.write_history_file(histfile)

  import atexit
  atexit.register(save_history)

  def quit():
    """
    Quit the REPL. Repl saved by atexit handler.
    """
    globalThis.python.exit()  # need for python.exit.code in require.py

  def sigint_handler(signum, frame):
    """
    Handle ^C by aborting the entry of the current statement and quitting when double-struck.

    Sometimes this happens in the main input() function. When that happens statement is "", because
    we have not yet returned from input(). Sometimes it happens in the middle of the inner loop's
    input() - in that case, statement is the beginning of a multiline expression. Hitting ^C in the
    middle of a multiline express cancels its input, but readline's input() doesn't return, so we
    have to print the extra > prompt and fake it by later getting rid of the first readline_skip_chars
    characters from the input buffer.
    """
    nonlocal got_sigint
    nonlocal statement
    nonlocal readline_skip_chars
    nonlocal inner_loop

    got_sigint = got_sigint + 1
    if (got_sigint > 1):
      raise EOFError

    if (not inner_loop):
      if (got_sigint == 1 and len(readline.get_line_buffer()) == readline_skip_chars):
        # First ^C with nothing in the input buffer
        sys.stdout.write("\n(To exit, press Ctrl+C again or Ctrl+D or type .exit)")
      elif (got_sigint == 1 and readline.get_line_buffer() != ""):
        # Input buffer has text - clear it
        got_sigint = 0
        readline_skip_chars = len(readline.get_line_buffer())
    else:
      if (got_sigint == 1 and statement == "" and len(readline.get_line_buffer()) == readline_skip_chars):
        # statement == "" means that the inner loop has already seen ^C and is now faking the outer loop
        sys.stdout.write("\n(To exit, press Ctrl+C again or Ctrl+D or type .exit)")
      elif (got_sigint == 1 and statement != ""):
        # ^C happened on inner loop while it was still thinking we were doing a multiline-expression; since
        # we can't break the input() function, we set it up to return an outer expression and fake the outer loop
        got_sigint = 0
        readline_skip_chars = len(readline.get_line_buffer())

    sys.stdout.write("\n> ")
    statement = ""
  signal.signal(signal.SIGINT, sigint_handler)

  # Main Loop
  #
  # Read lines entered by the user and collect them in a statement. Once the statement is a candiate
  # for JavaScript evaluation (determined by pm.isCompilableUnit(), send it to replEval(). Statements
  # beginning with a . are interpreted as REPL commands and sent to replCmd().
  #
  # Beware - extremely tricky interplay between readline and the SIGINT handler. This is largely because we
  # we can't clear the pending line buffer, so we have to fake it by re-displaying the prompt and subtracting
  # characters. Another complicating factor is that the handler will suspend and resume readline, but there
  # is no mechanism to force readline to return before the user presses enter.
  #
  while got_sigint < 2:
    try:
      await asyncio.sleep(0)
      inner_loop = False
      if (statement == ""):
        statement = input('> ')[readline_skip_chars:]
      readline_skip_chars = 0

      if (len(statement) == 0):
        continue
      if (statement[0] == '.'):
        cmd_output = globalThis.replCmd(statement[1:])
        if (cmd_output is not None):
          print(cmd_output)
        statement = ""
        continue
      if (pm.isCompilableUnit(statement)):
        print(globalThis.replEval(statement))
        statement = ""
        got_sigint = 0
      else:
        got_sigint = 0
        # This loop builds a multi-line statement, but if the user hits ^C during this build, we
        # abort the statement. The tricky part here is that the input('... ') doesn't quit when
        # SIGINT is received, so we have to patch things up so that the next-entered line is
        # treated as the input at the top of the loop.
        while (got_sigint == 0):
          await asyncio.sleep(0)
          inner_loop = True
          lineBuffer = input('... ')
          more = lineBuffer[readline_skip_chars:]
          readline_skip_chars = 0
          if (got_sigint > 0):
            statement = more
            break
          statement = statement + '\n' + more
          if (pm.isCompilableUnit(statement)):
            print(globalThis.replEval(statement))
            statement = ""
            break
    except EOFError:
      print()
      quit()


def usage():
  print("""Usage: pmjs [options] [ script.js ] [arguments]

Options:
  -                    script read from stdin (default if no file name is provided, interactive mode if a tty)
  --                   indicate the end of node options
  -e, --eval=...       evaluate script
  -h, --help           print pnode command line options (currently set)
  -i, --interactive    always enter the REPL even if stdin does not appear to be a terminal
  -p, --print [...]    evaluate script and print result
  -r, --require...     module to preload (option can be repeated)
  -v, --version        print PythonMonkey version
  --use-strict         evaluate -e, -p, and REPL code in strict mode
  --inspect            enable pmdb, a gdb-like JavaScript debugger interface
  --wtf                enable WTFPythonMonkey, a tool that can detect hanging timers when Ctrl-C is hit

Environment variables:
TZ                            specify the timezone configuration
PMJS_PATH                     ':'-separated list of directories prefixed to the module search path
PMJS_REPL_HISTORY             path to the persistent REPL history file"""
        )


def initGlobalThis():
  """
  Initialize globalThis for pmjs use in the extra-module context (eg -r, -e, -p). This context
  needs a require function which resolves modules relative to the current working directory at pmjs
  launch. The global require is to the JS function using a trick instead of a JS-wrapped-Python-wrapped function
  """
  global requirePath

  require = pm.createRequire(os.path.abspath(os.getcwd() + '/__pmjs_virtual__'), requirePath)
  globalThis.require = require
  globalInitModule = require(
      os.path.realpath(
          os.path.dirname(__file__) +
          "/../lib/pmjs/global-init"))  # module load has side-effects
  globalThis.arguments = sys.argv
  return globalInitModule


def main():
  """
  Main program entry point
  """
  enterRepl = sys.stdin.isatty()
  forceRepl = False
  globalInitModule = initGlobalThis()
  global requirePath

  try:
    opts, args = getopt.getopt(sys.argv[1:], "hie:p:r:v", ["help", "eval=", "print=",
                               "require=", "version", "interactive", "use-strict", "inspect", "wtf"])
  except getopt.GetoptError as err:
    # print help information and exit:
    print(err)  # will print something like "option -a not recognized"
    usage()
    sys.exit(2)
  output = None
  verbose = False
  enableWTF = False
  for o, a in opts:
    if o in ("-v", "--version"):
      print(pm.__version__)
      sys.exit()
    elif o in ("--use-strict"):
      evalOpts['strict'] = True
    elif o in ("-h", "--help"):
      usage()
      sys.exit()
    elif o in ("-i", "--interactive"):
      forceRepl = True
    elif o in ("-e", "--eval"):
      async def runEval():
        pm.eval(a, evalOpts)
        await pm.wait()
      asyncio.run(runEval())
      enterRepl = False
    elif o in ("-p", "--print"):
      async def runEvalPrint():
        ret = pm.eval(a, evalOpts)
        pm.eval("ret => console.log(ret)", evalOpts)(ret)
        await pm.wait()
      asyncio.run(runEvalPrint())
      enterRepl = False
    elif o in ("-r", "--require"):
      globalThis.require(a)
    elif o in ("--inspect"):
      pmdb.enable()
    elif o in ("--wtf"):
      enableWTF = True
    else:
      assert False, "unhandled option"

  if (len(args) > 0):
    async def runJS():
      hasUncaughtException = False
      loop = asyncio.get_running_loop()

      def exceptionHandler(loop, context):
        "See https://docs.python.org/3.11/library/asyncio-eventloop.html#error-handling-api"
        error = context["exception"]
        try:
          globalInitModule.uncaughtExceptionHandler(error)
        except SystemExit:  # the "exception" raised by `sys.exit()` call
          pass
        finally:
          pm.stop()  # unblock `await pm.wait()` to gracefully exit the program
          nonlocal hasUncaughtException
          hasUncaughtException = True
      loop.set_exception_handler(exceptionHandler)

      def cleanupExit(code=0):
        pm.stop()
        realExit(code)
      realExit = globalThis.python.exit
      globalThis.python.exit = cleanupExit

      try:
        globalInitModule.patchGlobalRequire()
        pm.runProgramModule(args[0], args, requirePath)
        await pm.wait()  # blocks until all asynchronous calls finish
        if hasUncaughtException:
          sys.exit(1)
      except Exception as error:
        print(error, file=sys.stderr)
        sys.exit(1)
    try:
      asyncio.run(runJS())
    except KeyboardInterrupt:
      print()  # silently going to end the program instead of printing out the Python traceback
      if enableWTF:
        wtfpm.printTimersDebugInfo()
  elif (enterRepl or forceRepl):
    async def runREPL():
      globalInitModule.initReplLibs()
      await repl()
      await pm.wait()
    asyncio.run(runREPL())

  globalThis.python.exit()  # need for python.exit.code in require.py


if __name__ == "__main__":
  main()
