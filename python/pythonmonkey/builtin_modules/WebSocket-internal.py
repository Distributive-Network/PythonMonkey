# @file     WebSocket-internal.py
# @brief    internal helper functions for WebSocket, backed by aiohttp
# @author   Dan Desjardins <dan@distributive.network>
# @date     September 2026
# @copyright Copyright (c) 2026 Distributive Corp.

import aiohttp
from typing import Callable, List, Union


async def wsConnect(
    url: str,
    protocols: List[str],
    onOpen: Callable[[str, Callable, Callable, Callable], None],
    onMessage: Callable[[Union[str, bytearray], bool], None],
    onError: Callable[[str], None],
    onClose: Callable[[int, str], None],
    debug: Callable[[str], Callable[..., None]],
    /
):
  """
  Open a WebSocket and pump its messages into the JS-side callbacks.

  onOpen receives the negotiated subprotocol and the send/close functions in
  the same synchronous call that precedes the JS 'open' event, so a message
  sent from an 'open' handler can never race the functions being wired up.
  Every path ends with exactly one onClose call.
  """
  log = debug('ws:io')
  session = aiohttp.ClientSession()

  try:
    ws = await session.ws_connect(url, protocols=tuple(protocols))
  except Exception as e:
    onError(str(e))
    await session.close()
    onClose(1006, '')
    return

  def guard(fn):
    # JS calls these without awaiting the returned promise, so failures are
    # reported through onError instead of becoming unhandled rejections.
    async def wrapper(*args):
      if ws.closed:
        return
      try:
        await fn(*args)
      except Exception as e:
        onError(str(e))
    return wrapper

  sendText = guard(ws.send_str)
  sendBinary = guard(lambda data: ws.send_bytes(bytes(data)))
  closeConn = guard(lambda code, reason: ws.close(code=code, message=reason.encode('utf-8')))

  onOpen(ws.protocol or '', sendText, sendBinary, closeConn)

  close_reason = ''
  try:
    while True:
      # receive() rather than `async for`, which swallows the CLOSE frame's reason
      msg = await ws.receive()
      log('received', msg.type.name)
      if msg.type == aiohttp.WSMsgType.TEXT:
        onMessage(msg.data, False)
      elif msg.type == aiohttp.WSMsgType.BINARY:
        onMessage(bytearray(msg.data), True)
      elif msg.type == aiohttp.WSMsgType.ERROR:
        onError(str(ws.exception()))
        break
      elif msg.type == aiohttp.WSMsgType.CLOSE:
        close_reason = msg.extra or ''
        break
      elif msg.type == aiohttp.WSMsgType.CLOSED:
        break
      # CLOSING means our own close() is in flight; the next receive() finishes
      # the handshake and reports CLOSED with the real close code
  except Exception as e:
    onError(str(e))
  finally:
    await session.close()
    # close_code is unset when the connection dropped without a close frame
    onClose(ws.close_code or 1006, close_reason)


# Module exports
exports['wsConnect'] = wsConnect  # type: ignore
