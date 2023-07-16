# @file         event-loop-asyncio.py
#               Interface to the asyncio module for the event-loop library. We can't use JavaScript for
#               some of this code, because PythonMonkey can't transport things like asyncio.TimerHandler
#               and event loops as of version 0.2.0.
# @author       Wes Garland, wes@distributive.network
# @date         July 2023
#
import asyncio

loop = False

def enqueueWithDelay(callback, delay):
    """
    Schedule a callback to run after a certain amount of time. Delay is in seconds.
    """
    return { 'timer': loop.call_later(delay, callback) }

def enqueue(callback):
    """
    Schedule a callback to run as soon as possible.
    """
    return { 'timer': loop.call_soon(callback) }

def cancelTimer(pyHnd):
    """
    Cancel a timer that was previously enqueued. The pyHnd argument is the return value from one of the
    enqueue functions.
    """
    loop.pyHnd['timer'].cancel()

def getLoop():
    """
    Get the current Python event loop used by this code. If no loop has been specified and this is the
    first time this function is run, we start using the event loop that invoked this call.
    """
    global loop
    if (loop == False):
        loop = asyncio.get_running_loop()
    return loop

def setLoop(newLoop):
    """
    Set the event loop this code will use. Replacing a running loop is not allowed.
    """
    global loop
    if (loop != False and loop != newLoop):
        raise Except("can't set loop - event loop already exists")
    loop = newLoop
    
def makeLoop():
    """
    Make a new event loop.
    """
    global loop
    if (loop != False):
        raise Except("can't make loop - event loop already exists")
    loop = asyncio.new_event_loop()
    return loop

def endLoop():
    """
    End the event loop. This will throw if there are pending events.
    """
    global loop
    if (loop != False):
        loop.stop()
        loop = False

def uptime():
    """
    Return the number of seconds this event loop has been running.
    """
    return loop.time()

exports['enqueueWithDelay'] = enqueueWithDelay
exports['enqueue']          = enqueue
exports['getLoop']          = getLoop    
exports['setLoop']          = setLoop
exports['makeLoop']         = makeLoop
exports['endLoop']          = endLoop
