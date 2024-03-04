# @file     XMLHttpRequest-internal.py
# @brief    internal helper functions for XMLHttpRequest
# @author   Tom Tang <xmader@distributive.network>
# @date     August 2023

import asyncio
import aiohttp
import yarl
import io
import platform
import pythonmonkey as pm
from typing import Union, ByteString, Callable, TypedDict
import traceback

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
    /
):
    #print("request START about to crash, calling gc"); 
    #pm.collect()

    #print("request START print stack");
    #traceback.print_stack()
   
    #print("request START access headers");
    #print("headers['accept'] is ", headers['accept']);
    #print("headers['x-dcp-platform'] is ", headers['x-dcp-platform']);
    #print("request START, headers are ", headers); 
    
    class BytesPayloadWithProgress(aiohttp.BytesPayload):
        _chunkMaxLength = 2**16 # aiohttp default

        async def write(self, writer) -> None:
            print("write START"); 
            buf = io.BytesIO(self._value)
            chunk = buf.read(self._chunkMaxLength)
            while chunk:
                await writer.write(chunk)
                processRequestBodyChunkLength(len(chunk))
                chunk = buf.read(self._chunkMaxLength)
            processRequestEndOfBody()
            print("write END"); 

    if isinstance(body, str):
        body = bytes(body, "utf-8")

    # set default headers
    headers=dict(headers)
    headers.setdefault("user-agent", f"Python/{platform.python_version()} PythonMonkey/{pm.__version__}")
    print("HEADERS after setdefault: ", headers)

    

    print("METHOD is ", method)
    print("URL is ", url)
    print("TIMEOUT is ", timeoutMs)
    #print("BODY is ", body)
    

    if timeoutMs > 0:
        timeoutOptions = aiohttp.ClientTimeout(total=timeoutMs/1000) # convert to seconds
    else:
        timeoutOptions = aiohttp.ClientTimeout() # default timeout

    try:
        print("async with aiohttp.request BEFORE");
        async with aiohttp.request(method=method,
                                url=yarl.URL(url, encoded=True),
                                headers=headers,
                                data=BytesPayloadWithProgress(body) if body else None,
                                timeout=timeoutOptions,
        ) as res:
            print("async with aiohttp.request AFTER");
            def getResponseHeader(name: str):
                print("getAllResponseHeader");
                return res.headers.get(name)
            def getAllResponseHeaders():
                headers = []
                print("getAllResponseHeaders BEFORE"); 
                for name, value in res.headers.items():
                    headers.append(f"{name.lower()}: {value}")
                print("getAllResponseHeaders AFTER");     
                headers.sort()
                print("getAllResponseHeaders sorted"); 
                return "\r\n".join(headers)
            def abort():
                print("abort");
                res.close()

            # readyState HEADERS_RECEIVED
            responseData: XHRResponse = { # FIXME: PythonMonkey bug: the dict will be GCed if directly as an argument
                'url': str(res.real_url),
                'status': res.status,
                'statusText': str(res.reason or ''),

                'getResponseHeader': getResponseHeader,
                'getAllResponseHeaders': getAllResponseHeaders,
                'abort': abort,
                
                'contentLength': res.content_length or 0,
            }
            print("processResponse"); 
            processResponse(responseData)

            # readyState LOADING
            print("readyState LOADING");
            async for data in res.content.iter_any():
                processBodyChunk(bytearray(data)) # PythonMonkey only accepts the mutable bytearray type
            print("readyState DONE");    
            
            # readyState DONE
            processEndOfBody()
    except asyncio.TimeoutError as e:
        print("onTimeoutError " + e);   
        onTimeoutError(e)
        raise # rethrow
    except aiohttp.ClientError as e:
        print("onNetworkError " + e); 
        onNetworkError(e)
        raise # rethrow

def decodeStr(data: bytes, encoding='utf-8'): # XXX: Remove this once we get proper TextDecoder support
    print("decodeStr")
    return str(data, encoding=encoding)

# Module exports
exports['request'] = request # type: ignore
exports['decodeStr'] = decodeStr # type: ignore
