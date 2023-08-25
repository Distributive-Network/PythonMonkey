# @file     XMLHttpRequest-internal.py
# @brief    internal helper functions for XMLHttpRequest
# @author   Tom Tang <xmader@distributive.network>
# @date     August 2023

import aiohttp
import yarl
from typing import Union, Sequence, Callable, Any

async def request(
    method: str,
    url: str,
    headers: dict,
    body: Union[str, Sequence[int]],
    processResponse: Callable[[Any], None],
    processBodyChunk: Callable[[bytearray], None],
    processEndOfBody: Callable[[], None],
    /
):
    if isinstance(body, str):
        body = bytes(body, "utf-8")

    async with aiohttp.request(method=method,
                               url=yarl.URL(url, encoded=True),
                               headers=dict(headers),
                               data=bytes(body)
    ) as res:
        def getResponseHeader(name: str):
            return res.headers.get(name)
        def getAllResponseHeaders():
            headers = []
            for name, value in res.headers.items():
                headers.append(f"{name.lower()}: {value}")
            headers.sort()
            return "\r\n".join(headers)

        # readyState HEADERS_RECEIVED
        responseData = { # FIXME: PythonMonkey bug: the dict will be GCed if directly as an argument
            'url': str(res.real_url),
            'status': res.status,
            'statusText': res.reason,

            'getResponseHeader': getResponseHeader,
            'getAllResponseHeaders': getAllResponseHeaders,
            
            'contentLength': res.content_length,
        }
        processResponse(responseData)

        # readyState LOADING
        async for data in res.content.iter_any():
            processBodyChunk(bytearray(data)) # PythonMonkey only accepts the mutable bytearray type
        
        # readyState DONE
        processEndOfBody()

def decodeStr(data: bytes, encoding='utf-8'): # XXX: Remove this once we get proper TextDecoder support
    return str(data, encoding=encoding)

# Module exports
exports['request'] = request # type: ignore
exports['decodeStr'] = decodeStr # type: ignore
