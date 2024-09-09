/**
 * @file JSStringProxy.cc
 * @author Caleb Aikens (caleb@distributive.network) and Philippe Laporte (plaporte@distributive.network)
 * @brief JSStringProxy is a custom C-implemented python type that derives from str. It acts as a proxy for JSStrings from Spidermonkey, and behaves like a str would.
 * @date 2024-05-15
 *
 * @copyright Copyright (c) 2024 Distributive Corp.
 *
 */

#include "include/JSStringProxy.hh"

#include "include/StrType.hh"

std::unordered_set<JSStringProxy *> jsStringProxies;
extern JSContext *GLOBAL_CX;


void JSStringProxyMethodDefinitions::JSStringProxy_dealloc(JSStringProxy *self)
{
  jsStringProxies.erase(self);
  delete self->jsString;
}

PyObject *JSStringProxyMethodDefinitions::JSStringProxy_copy_method(JSStringProxy *self) {
  JS::RootedString selfString(GLOBAL_CX, ((JSStringProxy *)self)->jsString->toString());
  JS::RootedValue selfStringValue(GLOBAL_CX, JS::StringValue(selfString));
  return StrType::proxifyString(GLOBAL_CX, selfStringValue);
}