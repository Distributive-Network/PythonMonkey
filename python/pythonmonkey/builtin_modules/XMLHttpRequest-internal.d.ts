/**
 * @file    XMLHttpRequest-internal.d.ts
 * @brief   TypeScript type declarations for the internal XMLHttpRequest helpers
 * @author  Tom Tang <xmader@distributive.network>
 * @date    August 2023
 */

/**
 * Send request
 */
export declare function request(
  method: string,
  url: string,
  headers: Record<string, string>,
  body: string | Uint8Array,
  // callbacks for request body progress
  processRequestBodyChunkLength: (bytesLength: number) => void,
  processRequestEndOfBody: () => void,
  // callbacks for response progress
  processResponse: (response: any) => void,
  processBodyChunk: (bytes: Uint8Array) => void,
  processEndOfBody: () => void,
): string;

/**
 * Decode data using the codec registered for encoding.
 */
export declare function decodeStr(data: Uint8Array, encoding?: string): string;
