//
// Created by 16182 on 12/6/2020.
//

#ifndef V8DEBUGGER_EMBEDBUILTINS_H
#define V8DEBUGGER_EMBEDBUILTINS_H

#include<v8.h>
#include<vector>
#include "../v8-runtime/v8Runtime.h"

v8::Local<v8::ObjectTemplate> global_builtins(v8::Isolate * isolate);
void set_context_builtins(v8Runtime *runtime);
void set_linked_builtins(v8Runtime * authoring, v8Runtime * rendering);
v8::Local<v8::Value> msg_to_value(v8::Local<v8::Context> ctx, message msg);

#endif //V8DEBUGGER_EMBEDBUILTINS_H
