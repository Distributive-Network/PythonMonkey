# @file     WebSocket-internal.py
# @brief    internal helper functions for WebSocket, backed by aiohttp
# @author   (local patch, this session)
# @date     September 2026

import asyncio
import aiohttp
from typing import Callable, Union, List


async def wsConnect(
    url: str,
    protocols: List[str],
    headers: dict,
    onOpen: Callable[[Callable, Callable, Callable], None],
    onMessage: Callable[[Union[str, bytearray], bool], None],
    onError: Callable[[str], None],
    onClose: Callable[[int, str], None],
    debug: Callable[[str], Callable[..., None]],
    /
):
  """
  NOTE: onOpen is called with (sendText, sendBinary, close) as arguments,
  rather than having this whole function return them once the coroutine
  finishes -- the JS side needs those functions available in the SAME
  synchronous callback that fires the 'open' event, not one microtask
  later. A real client (dcp-client included) commonly sends its first
  message immediately in reaction to 'open'; if the send/close functions
  only became available via this async function's eventual return value
  (resolved on a later microtask), that first send would silently race
  against them not being wired up yet and get dropped -- confirmed
  empirically as a silent hang (no error, no crash, connection just never
  progresses) the first time this was tried without this fix.
  """
  session = aiohttp.ClientSession()

  try:
    ws = await session.ws_connect(url, protocols=tuple(protocols) if protocols else (), headers=headers or {})
  except Exception as e:
    try:
      onError(str(e))
    finally:
      await session.close()
    return

  async def sendText(data: str):
    if not ws.closed:
      await ws.send_str(data)

  async def sendBinary(data):
    if not ws.closed:
      await ws.send_bytes(bytes(data))

  async def closeConn(code: int, reason: str):
    if not ws.closed:
      await ws.close(code=code or 1000, message=(reason or '').encode('utf-8'))

  onOpen(sendText, sendBinary, closeConn)

  close_code = 1006
  close_reason = ''
  try:
    async for msg in ws:
      debug('ws:io')('received', msg.type, 'len=', len(msg.data) if hasattr(msg.data, '__len__') else None)
      if msg.type == aiohttp.WSMsgType.TEXT:
        onMessage(msg.data, False)
      elif msg.type == aiohttp.WSMsgType.BINARY:
        onMessage(bytearray(msg.data), True)
      elif msg.type == aiohttp.WSMsgType.ERROR:
        onError(str(ws.exception()))
      elif msg.type in (aiohttp.WSMsgType.CLOSE, aiohttp.WSMsgType.CLOSING, aiohttp.WSMsgType.CLOSED):
        break
    close_code = ws.close_code if ws.close_code is not None else 1000
  except Exception as e:
    onError(str(e))
  finally:
    try:
      await session.close()
    except Exception:
      pass
    onClose(close_code, close_reason)


# Module exports
exports['wsConnect'] = wsConnect  # type: ignore
