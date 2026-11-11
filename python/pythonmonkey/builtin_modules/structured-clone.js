/**
 * @file     structured-clone.js
 *           Polyfill the structuredClone() global
 *
 * @author   Dan Desjardins <dan@distributive.network>
 * @date     November 2026
 *
 * @copyright Copyright (c) 2026 Distributive Corp.
 */

// Apply polyfill from core-js
require('./dom-exception');
require('core-js/actual/structured-clone');

exports.structuredClone = globalThis.structuredClone;
