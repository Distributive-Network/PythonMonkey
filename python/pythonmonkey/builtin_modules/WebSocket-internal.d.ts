/**
 * @file    WebSocket-internal.d.ts
 * @brief   TypeScript type declarations for the internal WebSocket helpers
 * @author  Dan Desjardins <dan@distributive.network>
 * @date    September 2026
 *
 * @copyright Copyright (c) 2026 Distributive Corp.
 */

/**
 * Open a WebSocket connection and pump its messages into the callbacks.
 * Resolves once the connection has closed; `onClose` is called exactly once.
 */
export declare function wsConnect(
  url: string,
  protocols: string[],
  // called before the 'open' event, with the negotiated subprotocol and the send/close functions
  onOpen: (
    protocol: string,
    sendText: (data: string) => Promise<void>,
    sendBinary: (data: Uint8Array) => Promise<void>,
    close: (code: number, reason: string) => Promise<void>,
  ) => void,
  onMessage: (data: string | Uint8Array, isBinary: boolean) => void,
  onError: (message: string) => void,
  onClose: (code: number, reason: string) => void,
  // the debug logging function
  /** See `pm.bootstrap.require("debug")` */
  debug: (selector: string) => ((...args: string[]) => void),
): Promise<void>;
