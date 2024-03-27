/**
 * @file     dom-exception.js
 *           Polyfill the DOMException interface
 *
 * @author   Tom Tang <xmader@distributive.network>
 * @date     August 2023
 * 
 * @copyright Copyright (c) 2023 Distributive Corp.
 */

// Apply polyfill from core-js
require('core-js/actual/dom-exception');

exports.DOMException = globalThis.DOMException;
