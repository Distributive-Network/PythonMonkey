/**
 * @file     dom-exception.js
 *           Polyfill the DOMException interface
 *
 * @author   Tom Tang <xmader@distributive.network>
 * @date     August 2023
 */

// Apply polyfill from core-js
require('core-js/actual/dom-exception');

exports.DOMException = globalThis.DOMException;
