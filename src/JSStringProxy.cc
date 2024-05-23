/**
 * @file JSStringProxy.cc
 * @author Caleb Aikens (caleb@distributive.network)
 * @brief JSStringProxy is a custom C-implemented python type that derives from str. It acts as a proxy for JSStrings from Spidermonkey, and behaves like a str would.
 * @date 2024-05-15
 *
 * @copyright Copyright (c) 2024 Distributive Corp.
 *
 */

#include "include/JSStringProxy.hh"

void JSStringProxyMethodDefinitions::JSStringProxy_dealloc(JSStringProxy *self)
{
  delete self->jsString;
}