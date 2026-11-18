import asyncio
import socket
import aiohttp
import aiohttp.web
import pythonmonkey as pm


def free_port():
  with socket.socket() as s:
    s.bind(('127.0.0.1', 0))
    return s.getsockname()[1]


async def ws_handler(request):
  ws = aiohttp.web.WebSocketResponse(protocols=('chat',))
  await ws.prepare(request)
  async for msg in ws:
    if msg.type == aiohttp.WSMsgType.TEXT:
      if msg.data == 'close-me':
        await ws.close(code=4000, message=b'bye')
      else:
        await ws.send_str('echo:' + msg.data)
    elif msg.type == aiohttp.WSMsgType.BINARY:
      await ws.send_bytes(bytes(reversed(msg.data)))
  return ws


def test_websocket():
  async def async_fn():
    port = free_port()
    app = aiohttp.web.Application()
    app.router.add_get('/ws', ws_handler)
    runner = aiohttp.web.AppRunner(app)
    await runner.setup()
    await aiohttp.web.TCPSite(runner, '127.0.0.1', port).start()

    # text and binary round trips, subprotocol negotiation, server-initiated close
    log = await pm.eval("""
      (port) => new Promise((resolve, reject) => {
        const log = [];
        const ws = new WebSocket(`ws://127.0.0.1:${port}/ws`, ['chat']);
        log.push('state=' + ws.readyState);
        ws.onopen = () => {
          log.push(`open protocol=${ws.protocol} state=${ws.readyState}`);
          ws.send('hello');
        };
        ws.onmessage = (ev) => {
          if (typeof ev.data === 'string') {
            log.push('text:' + ev.data);
            ws.send(new Uint8Array([1, 2, 3]));
          } else {
            log.push('binary:' + Array.from(new Uint8Array(ev.data)).join(','));
            ws.send('close-me');
          }
        };
        ws.onerror = () => log.push('error');
        ws.onclose = (ev) => {
          log.push(`close code=${ev.code} reason=${ev.reason} clean=${ev.wasClean} state=${ws.readyState}`);
          resolve(log);
        };
        setTimeout(() => reject(new Error('timeout: ' + log.join(' | '))), 5000);
      })
    """)(port)
    assert list(log) == [
        'state=0',
        'open protocol=chat state=1',
        'text:echo:hello',
        'binary:3,2,1',
        'close code=4000 reason=bye clean=true state=3',
    ]

    # refused connection fires error then close, and ends CLOSED
    log = await pm.eval("""
      (port) => new Promise((resolve, reject) => {
        const log = [];
        const ws = new WebSocket(`ws://127.0.0.1:${port}/`);
        ws.onopen = () => log.push('open');
        ws.onerror = () => log.push('error state=' + ws.readyState);
        ws.onclose = (ev) => {
          log.push(`close code=${ev.code} clean=${ev.wasClean} state=${ws.readyState}`);
          resolve(log);
        };
        setTimeout(() => reject(new Error('timeout: ' + log.join(' | '))), 5000);
      })
    """)(free_port())
    assert list(log) == ['error state=0', 'close code=1006 clean=false state=3']

    # close() while still connecting never fires 'open'
    log = await pm.eval("""
      (port) => new Promise((resolve, reject) => {
        const log = [];
        const ws = new WebSocket(`ws://127.0.0.1:${port}/ws`);
        ws.close();
        log.push('state=' + ws.readyState);
        ws.onopen = () => log.push('open');
        ws.onclose = (ev) => {
          log.push(`close code=${ev.code} state=${ws.readyState}`);
          resolve(log);
        };
        setTimeout(() => reject(new Error('timeout: ' + log.join(' | '))), 5000);
      })
    """)(port)
    assert list(log) == ['state=2', 'close code=1000 state=3']

    await runner.cleanup()
  asyncio.run(async_fn())
