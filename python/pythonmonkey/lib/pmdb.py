# @file     pmdb - A gdb-like JavaScript debugger interface
# @author   Tom Tang <xmader@distributive.network>
# @date     July 2023
# @copyright Copyright (c) 2023 Distributive Corp.

import pythonmonkey as pm


def debuggerInput(prompt: str):
  try:
    return input(prompt)  # blocking
  except KeyboardInterrupt:
    print("\b\bQuit")  # to match the behaviour of gdb
    return ""
  except Exception as e:
    print(e)
    return ""


def enable(debuggerGlobalObject=pm.eval("debuggerGlobal")):
  if debuggerGlobalObject._pmdbEnabled:
    return  # already enabled, skipping

  debuggerGlobalObject._pmdbEnabled = True
  pm.eval("debuggerGlobal.eval")("""(mainGlobal, debuggerInput, _pythonPrint, _pythonExit) => {
  const dbg = new Debugger()
  const mainDebuggee = dbg.addDebuggee(mainGlobal)
  dbg.uncaughtExceptionHook = (e) => {
    _pythonPrint(e)
  }

  function makeDebuggeeValue (val) {
    if (val instanceof Debugger.Object) {
      return dbg.adoptDebuggeeValue(val)
    } else {
      // See https://firefox-source-docs.mozilla.org/js/Debugger/Debugger.Object.html#makedebuggeevalue-value
      return mainDebuggee.makeDebuggeeValue(val)
    }
  }

  function print (...args) {
    const logger = makeDebuggeeValue(mainGlobal.console.log)
    logger.apply(logger, args.map(makeDebuggeeValue))
  }

  function printErr (...args) {
    const logger = makeDebuggeeValue(mainGlobal.console.error)
    logger.apply(logger, args.map(makeDebuggeeValue))
  }

  function printSource (frame, location) {
    const src = frame.script.source.text
    const line = src.split('\\n').slice(location.lineNumber-1, location.lineNumber).join('\\n')
    print(line)
    print(" ".repeat(location.columnNumber) + "^") // indicate column position
  }

  function getCommandInputs () {
    const input = debuggerInput("(pmdb) > ") // blocking
    const [_, command, rest] = input.match(/\\s*(\\w+)?(?:\\s+(.*))?/)
    return { command, rest }
  }

  function enterDebuggerLoop (frame, checkIsBreakpoint = false) {
    const metadata = frame.script.getOffsetMetadata(frame.offset)
    if (checkIsBreakpoint && !metadata.isBreakpoint) {
      // This bytecode offset does not qualify as a breakpoint, skipping
      return
    }

    blockingLoop: while (true) {
      const { command, rest } = getCommandInputs() // blocking
      switch (command) {
        case "b":
        case "break": {
          // Set breakpoint on specific line number
          const lineNum = Number(rest)
          if (!lineNum) {
            print(`"break <lineNumber>" command requires a valid line number argument.`)
            continue blockingLoop;
          }

          // find the bytecode offset for possible breakpoint location
          const bp = frame.script.getPossibleBreakpoints({ line: lineNum })[0]
          if (!bp) {
            print(`No possible breakpoint location found on line ${lineNum}`)
            continue blockingLoop;
          }

          // add handler
          frame.script.setBreakpoint(bp.offset, (frame) => enterDebuggerLoop(frame))

          // print breakpoint info
          print(`Breakpoint set on line ${bp.lineNumber} column ${bp.columnNumber+1} in "${frame.script.url}" :`)
          printSource(frame, bp)

          continue blockingLoop;
        }
        case "c":
        case "cont":
          // Continue execution until next breakpoint or `debugger` statement
          frame.onStep = undefined // clear step next handler
          break blockingLoop;
        case "n":
        case "next":
          // Step next
          frame.onStep = function () { enterDebuggerLoop(this, /*checkIsBreakpoint*/ true) } // add handler
          break blockingLoop;
        case "bt":
        case "backtrace":
          // Print backtrace of current execution frame
          // FIXME: we currently implement this using Error.stack
          print(frame.eval("(new Error).stack.split('\\\\n').slice(1).join('\\\\n')").return)
          continue blockingLoop;
        case "l":
        case "line": {
          // Print current line
          printSource(frame, metadata)
          continue blockingLoop;
        }
        case "p":
        case "exec":
        case "print": {
          // Execute an expression in debugging script's context and print its value
          if (!rest) {
            print(`"print <expr>" command requires an argument.`)
            continue blockingLoop;
          }
          const result = frame.eval(rest)
          if (result.throw) printErr(result.throw) // on error
          else print(result.return) // on success
          continue blockingLoop;
        }
        case "q":
        case "quit":
        case "kill":
          // Force exit the program
          _pythonExit(127)
          break blockingLoop;
        case "h":
        case "help":
          // Print help message
          // XXX: keep this in sync with the actual implementation
          print([
            "List of commands:",
            "• b <lineNumber>/break <lineNumber>: Set breakpoint on specific line",
            "• c/cont: Continue execution until next breakpoint or debugger statement",
            "• n/next: Step next",
            "• bt/backtrace: Print backtrace of current execution frame",
            "• l/line: Print current line",
            "• p <expr>/print <expr>/exec <expr>: Execute an expression in debugging script's context and print its value",
            "• q/quit/kill: Force exit the program",
            "• h/help: Print help message",
          ].join("\\n"))
          continue blockingLoop;
        case "":
        case undefined:
          // no-op
          continue blockingLoop;
        default:
          print(`Undefined command: "${command}". Try "help".`)
      }
    }
  }

  // Enter debugger on uncaught exceptions
  dbg.onExceptionUnwind = (frame, err) => {
    const isUncaught = !frame.script.isInCatchScope(frame.offset) // not in a catch block
                    && frame.older == null                        // this is the outermost frame
    if (isUncaught) {
      printErr("Uncaught exception:")
      printErr(err)
      enterDebuggerLoop(frame)
    }
  }

  // Enter debugger on `debugger;` statement
  dbg.onDebuggerStatement = (frame) => enterDebuggerLoop(frame)

  }""")(pm.globalThis, debuggerInput, print, lambda status: exit(int(status)))
