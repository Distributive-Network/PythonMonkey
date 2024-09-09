# @file     XMLHttpRequest-internal.py
# @brief    internal helper functions for XMLHttpRequest
# @author   Tom Tang <xmader@distributive.network>
# @date     August 2023
# @copyright Copyright (c) 2023 Distributive Corp.

import asyncio
import aiohttp
import yarl
import io
import platform
import pythonmonkey as pm
from typing import Union, ByteString, Callable, TypedDict

keepAliveConnector: Union[aiohttp.TCPConnector, None] = None


class XHRResponse(TypedDict, total=True):
  """
  See definitions in `XMLHttpRequest-internal.d.ts`
  """
  url: str
  status: int
  statusText: str
  contentLength: int
  getResponseHeader: Callable[[str], Union[str, None]]
  getAllResponseHeaders: Callable[[], str]
  abort: Callable[[], None]


async def request(
    method: str,
    url: str,
    headers: dict,
    body: Union[str, ByteString],
    timeoutMs: float,
    # callbacks for request body progress
    processRequestBodyChunkLength: Callable[[int], None],
    processRequestEndOfBody: Callable[[], None],
    # callbacks for response progress
    processResponse: Callable[[XHRResponse], None],
    processBodyChunk: Callable[[bytearray], None],
    processEndOfBody: Callable[[], None],
    # callbacks for known exceptions
    onTimeoutError: Callable[[asyncio.TimeoutError], None],
    onNetworkError: Callable[[aiohttp.ClientError], None],
    # the debug logging function, see `pm.bootstrap.require("debug")`
    debug: Callable[[str], Callable[..., None]],
    /
):

  # to support HTTP-Keep-Alive
  global keepAliveConnector
  if (not keepAliveConnector):
    # seconds before closing Keep-Alive connection.
    keepAliveConnector = aiohttp.TCPConnector(keepalive_timeout=5)  # 5s is the default for Node.js's `http.globalAgent`

  class BytesPayloadWithProgress(aiohttp.BytesPayload):
    _chunkMaxLength = 2**16  # aiohttp default

    async def write(self, writer) -> None:
      debug('xhr:io')('begin chunked write')
      buf = io.BytesIO(self._value)
      chunk = buf.read(self._chunkMaxLength)
      while chunk:
        debug('xhr:io')('  writing', len(chunk), 'bytes')
        await writer.write(chunk)
        processRequestBodyChunkLength(len(chunk))
        chunk = buf.read(self._chunkMaxLength)
      processRequestEndOfBody()
      debug('xhr:io')('finish chunked write')

  if isinstance(body, str):
    body = bytes(body, "utf-8")

  # set default headers
  headers.setdefault("user-agent", f"Python/{platform.python_version()} PythonMonkey/{pm.__version__}")
  debug('xhr:headers')('after set default\n', headers)

  if timeoutMs > 0:
    timeoutOptions = aiohttp.ClientTimeout(total=timeoutMs / 1000)  # convert to seconds
  else:
    timeoutOptions = aiohttp.ClientTimeout()  # default timeout

  try:
    debug('xhr:aiohttp')('creating request for', url)
    async with aiohttp.request(method=method,
                               url=yarl.URL(url, encoded=True),
                               headers=headers,
                               data=BytesPayloadWithProgress(body) if body else None,
                               timeout=timeoutOptions,
                               connector=keepAliveConnector,
                               ) as res:
      debug('xhr:aiohttp')('got', res.content_type, 'result')

      def getResponseHeader(name: str):
        return res.headers.get(name)

      def getAllResponseHeaders():
        headers = []
        for name, value in res.headers.items():
          headers.append(f"{name.lower()}: {value}")
        headers.sort()
        return "\r\n".join(headers)

      def abort():
        debug('xhr:io')('abort')
        res.close()

      # readyState HEADERS_RECEIVED
      processResponse({
          'url': str(res.real_url),
          'status': res.status,
          'statusText': str(res.reason or ''),

          'getResponseHeader': getResponseHeader,
          'getAllResponseHeaders': getAllResponseHeaders,
          'abort': abort,
          'contentLength': res.content_length or 0,
      })

      async for data in res.content.iter_any():
        processBodyChunk(bytearray(data))  # PythonMonkey only accepts the mutable bytearray type
      # readyState DONE
      processEndOfBody()
  except asyncio.TimeoutError as e:
    onTimeoutError(e)
    raise  # rethrow
  except aiohttp.ClientError as e:
    onNetworkError(e)
    raise  # rethrow


def decodeStr(data: bytes, encoding='utf-8'):  # XXX: Remove this once we get proper TextDecoder support
  return str(data, encoding=encoding)


# Module exports
exports['request'] = request  # type: ignore
exports['decodeStr'] = decodeStr  # type: ignore
