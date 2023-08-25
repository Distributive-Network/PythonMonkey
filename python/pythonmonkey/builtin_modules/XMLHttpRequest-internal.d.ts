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
  processResponse: (response: any) => void,
  processBodyChunk: (bytes: Uint8Array) => void,
  processEndOfBody: () => void,
): string;
