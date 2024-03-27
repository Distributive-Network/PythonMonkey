/**
 * @file    base64.d.ts
 * @brief   TypeScript type declarations for base64.py
 * @author  Tom Tang <xmader@distributive.network>
 * @date    July 2023
 * 
 * @copyright Copyright (c) 2023 Distributive Corp.
 */

/**
 * Decode base64 string
 * @param b64 A string containing base64-encoded data.
 * @see https://html.spec.whatwg.org/multipage/webappapis.html#dom-atob-dev
 */
export declare function atob(b64: string): string;

/**
 * Create a base64-encoded ASCII string from a binary string
 * @param data The binary string to encode.
 * @see https://html.spec.whatwg.org/multipage/webappapis.html#dom-btoa-dev
 */
export declare function btoa(data: string): string;
