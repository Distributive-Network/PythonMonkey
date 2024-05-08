/**
 * @file     url.js
 *           Polyfill the URL and URLSearchParams interfaces 
 *
 * @author   Tom Tang <xmader@distributive.network>
 * @date     August 2023
 * 
 * @copyright Copyright (c) 2023 Distributive Corp.
 */

// Apply polyfills from core-js
require('./dom-exception');
require('core-js/actual/url');
require('core-js/actual/url-search-params');

exports.URL = globalThis.URL;
exports.URLSearchParams = globalThis.URLSearchParams;
