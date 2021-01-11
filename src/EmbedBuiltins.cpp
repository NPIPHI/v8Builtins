//
// Created by 16182 on 12/6/2020.
//

#include "../include/v8Builtins/EmbedBuiltins.h"
#include "log.h"
#include <v8Core/v8Runtime.h>
#include <AndroidWebsocket/Message.h>
#include <AndroidWebsocket/Websocket.h>
#include "glue.h"

//TODO find a more elegant solution
struct {
    std::mutex m;
    std::unordered_set<int> cleared_callbacks;
    int next_index = 0;
} setTimeoutInfo;

struct {
    std::mutex m;
    std::unordered_set<int> cleared_callbacks;
    int next_index = 0;
} setIntervalInfo;

std::string stringify(const v8::FunctionCallbackInfo<v8::Value> & values){
    int length = values.Length();
    std::string message;
    for(int i = 0; i < length; i++){
        v8::String::Value str(values.GetIsolate(), values[i]);
        message += std::string{*str, *str + str.length()} + ' ';
    }
    message.pop_back(); //remove trailing space
    return message;
}

v8::Local<v8::Value> msg_to_value(v8::Local<v8::Context> ctx, message msg){
    v8::Context::Scope context_scope(ctx);
    if (msg.type() == MSG_TYPE::STRING_UTF8) {
        return v8::String::NewFromUtf8(ctx->GetIsolate(), (char*)msg.begin(), v8::NewStringType::kNormal, msg.size()).ToLocalChecked();
    } else if(msg.type() == MSG_TYPE::STRING_UTF16) {
        return v8::String::NewFromTwoByte(ctx->GetIsolate(), (uint16_t*)msg.begin(), v8::NewStringType::kNormal, msg.size()/2).ToLocalChecked();
    } else if(msg.type() == MSG_TYPE::BINARY){
        return v8::ArrayBuffer::New(ctx->GetIsolate(), (void *) msg.begin(), msg.size());
    } else {
        assert(false && "unexpected message type");
    }
}

void v8Log(const v8::FunctionCallbackInfo<v8::Value> & values){
    LOG_I("jsLog: %s", stringify(values).c_str());
}

void v8Error(const v8::FunctionCallbackInfo<v8::Value> & values){
    LOG_I("jsError: %s", stringify(values).c_str());
}

void v8Warn(const v8::FunctionCallbackInfo<v8::Value> & values){
    LOG_I("jsWarn: %s", stringify(values).c_str());
}

void v8Info(const v8::FunctionCallbackInfo<v8::Value> & values){
    LOG_I("jsInfo: %s", stringify(values).c_str());
}

void v8SetTimeout(const v8::FunctionCallbackInfo<v8::Value> & values){
    assert(values.Length() == 2 && values[0]->IsFunction() && values[1]->IsInt32() && "takes a function and a number");

    auto runtime = (v8Runtime*) values.Data().As<v8::External>()->Value();
    auto isolate = values.GetIsolate();
    auto context = isolate->GetCurrentContext();
    auto persistent_context = std::make_shared<v8::Persistent<v8::Context>>(isolate, context);

    int delay = values[1]->ToInt32(context).ToLocalChecked()->Value();
    auto callback = std::make_shared<v8::Persistent<v8::Function>>(values.GetIsolate(), values[0].As<v8::Function>());
    std::lock_guard<std::mutex> l(setTimeoutInfo.m);
    int index = setTimeoutInfo.next_index++;
    runtime->post_task_delayed(
            [=]{
                auto local_context = persistent_context->Get(isolate);
                std::lock_guard<std::mutex> l(setTimeoutInfo.m);
                v8::Context::Scope scope(local_context);
                auto canceled = setTimeoutInfo.cleared_callbacks.find(index);
                if(canceled != setTimeoutInfo.cleared_callbacks.end()){
                    setTimeoutInfo.cleared_callbacks.erase(canceled);
                } else {
                    callback->Get(local_context->GetIsolate())->Call(local_context, local_context->Global(), 0, nullptr);
                }
            },
            delay);
    values.GetReturnValue().Set(index);
}

void v8ClearTimeout(const v8::FunctionCallbackInfo<v8::Value> & values){
    assert(values.Length() == 1 && values[0]->IsInt32() && "takes the setTimeout identifying number");

    auto isolate = values.GetIsolate();
    auto context = isolate->GetCurrentContext();

    int cancel_index = values[0]->ToInt32(context).ToLocalChecked()->Value();
    std::lock_guard<std::mutex> l(setTimeoutInfo.m);
    if(cancel_index < setTimeoutInfo.next_index){
        setTimeoutInfo.cleared_callbacks.insert(cancel_index);
    }
}

void _setIntervalRecur(v8Runtime * runtime, std::shared_ptr<v8::Persistent<v8::Context>> context, std::shared_ptr<v8::Persistent<v8::Function>> function, int delay_in_milliseconds, int index){
    auto local_context = context->Get(runtime->isolate);
    v8::Context::Scope scope(local_context);
    std::lock_guard<std::mutex> l(setIntervalInfo.m);
    auto canceled = setIntervalInfo.cleared_callbacks.find(index);
    if(canceled != setIntervalInfo.cleared_callbacks.end()){
        setIntervalInfo.cleared_callbacks.erase(canceled);
    } else {
        runtime->post_task_delayed([=]{
            _setIntervalRecur(runtime, context, function, delay_in_milliseconds, index);
        }, delay_in_milliseconds);
        function->Get(runtime->isolate)->Call(local_context, local_context->Global(), 0, nullptr);
    }
}

void v8SetInterval(const v8::FunctionCallbackInfo<v8::Value> & values){
    assert(values.Length() == 2 && values[0]->IsFunction() && values[1]->IsInt32() && "takes a function and a number");

    auto runtime = (v8Runtime*) values.Data().As<v8::External>()->Value();
    auto isolate = values.GetIsolate();
    auto context = isolate->GetCurrentContext();
    auto persistent_context = std::make_shared<v8::Persistent<v8::Context>>(isolate, context);

    int delay = values[1]->ToInt32(context).ToLocalChecked()->Value();
    auto callback = std::make_shared<v8::Persistent<v8::Function>>(values.GetIsolate(), values[0].As<v8::Function>());
    std::lock_guard<std::mutex> l(setIntervalInfo.m);
    int index = setIntervalInfo.next_index++;
    runtime->post_task_delayed(
            [=]{
                _setIntervalRecur(runtime, persistent_context, callback, delay, index);
            },
            delay);
    values.GetReturnValue().Set(index);
}

void v8ClearInterval(const v8::FunctionCallbackInfo<v8::Value> & values){
    assert(values.Length() == 1 && values[0]->IsInt32() && "takes the setInterval identifying number");

    auto isolate = values.GetIsolate();
    auto context = isolate->GetCurrentContext();

    int cancel_index = values[0]->ToInt32(context).ToLocalChecked()->Value();
    std::lock_guard<std::mutex> l(setIntervalInfo.m);
    if(cancel_index < setIntervalInfo.next_index){
        setIntervalInfo.cleared_callbacks.insert(cancel_index);
    }
}

void v8UTF8Decode(const v8::FunctionCallbackInfo<v8::Value> & values) {
    assert(values.Length() == 1 && values[0]->IsArrayBuffer() && "expects one array buffer");

    auto isolate = values.GetIsolate();

    auto arr = values[0].As<v8::ArrayBuffer>();

    values.GetReturnValue().Set(
        v8::String::NewFromUtf8(isolate, static_cast<const char *>(arr->GetContents().Data()), v8::NewStringType::kNormal, arr->GetContents().ByteLength()).ToLocalChecked()
    );
}

void v8PerfNow(const v8::FunctionCallbackInfo<v8::Value> & values){
    assert(values.Length() == 0 && "expects 0 arguments");

    values.GetReturnValue().Set(double(std::chrono::high_resolution_clock::now().time_since_epoch().count())/1000000);
}

void v8AttachWs(const v8::FunctionCallbackInfo<v8::Value> & values){
    assert(values.Length() == 4 && values[0]->IsObject() && values[1]->IsString() && values[2]->IsInt32() && values[3]->IsString() && "expects [ws object, address, port, subaddress");

    auto runtime = (v8Runtime*)values.Data().As<v8::External>()->Value();
    auto isolate = values.GetIsolate();
    auto context = isolate->GetCurrentContext();

    auto persistent = std::make_shared<v8::Persistent<v8::Object>>(isolate, values[0].As<v8::Object>());
    auto addr_utf8 = v8::String::Utf8Value(isolate, values[1].As<v8::String>());
    auto addr = std::string(*addr_utf8, *addr_utf8 + addr_utf8.length());

    auto port = values[2].As<v8::Int32>()->Value();

    auto sub_utf8 = v8::String::Utf8Value(isolate, values[3].As<v8::String>());
    auto sub = std::string(*sub_utf8, *sub_utf8 + sub_utf8.length());


    std::thread([=] {
        try {
            Websocket ws{addr, port, sub};
            runtime->post_task([=] {
                auto ctx = runtime->context()->Get(isolate);
                v8::Context::Scope scope(ctx);
                auto obj = persistent->Get(isolate);
                auto onopen = obj.As<v8::Object>()->Get(ctx, v8::String::NewFromUtf8Literal(isolate,
                                                                                            "onopen")).ToLocalChecked().As<v8::Function>();
                onopen->Call(ctx, ctx->Global(), 0, nullptr);
            });
            try {
                //TODO properly destroy websockets when context reset
                //this hack just disconnects after reload has been called;
                for(int i = 0; i < 2; i++){
                    auto msg = ws.read_blocking();
                    runtime->post_task([=] {
                        auto ctx = runtime->context()->Get(isolate);
                        v8::Context::Scope scope(ctx);
                        auto obj = persistent->Get(isolate);
                        auto onmessage = obj.As<v8::Object>()->Get(ctx, v8::String::NewFromUtf8Literal(
                                isolate, "__onmessage")).ToLocalChecked().As<v8::Function>();
                        auto val = msg_to_value(ctx, msg);
                        onmessage->Call(ctx, ctx->Global(), 1, &val);
                    });
                }
            } catch (std::exception &e) {
                runtime->post_task([=] {
                    auto ctx = runtime->context()->Get(isolate);
                    v8::Context::Scope scope(ctx);
                    auto obj = persistent->Get(isolate);
                    auto onclose = obj.As<v8::Object>()->Get(ctx,
                                                             v8::String::NewFromUtf8Literal(isolate,
                                                                                            "onclose")).ToLocalChecked().As<v8::Function>();
                    onclose->Call(ctx, ctx->Global(), 0, nullptr);
                });
            }
        } catch (std::exception & e){
            runtime->post_task([=]{
                auto ctx = runtime->context()->Get(isolate);
                v8::Context::Scope scope(ctx);
                isolate->ThrowException(v8::Exception::Error(v8::String::NewFromUtf8Literal(isolate, "failed to open websocket")));
            });
        }
    }).detach();
}
//
//void v8Load(const v8::FunctionCallbackInfo<v8::Value> & values){
//    assert(values.Length() == 2 && values[0]->IsString() && values[1]->IsInt32() && "takes an address and a port");
//    auto runtime = (v8Runtime*)values.Data().As<v8::External>()->Value();
//    auto isolate = values.GetIsolate();
//    auto utf_addr = v8::String::Utf8Value(isolate, values[0].As<v8::String>());
//    std::string addr{*utf_addr, *utf_addr + utf_addr.length()};
//    int port = values[1].As<v8::Int32>()->Value();
//    runtime->post_task([=]{
//        LiveServer::load_http(runtime, addr, port);
//    });
//}


void set_global_func(v8::Local<v8::Context> context, const char * name, v8::FunctionCallback function, void * data, bool has_side_effect = true){
    context->Global()->Set(context,
            v8::String::NewFromUtf8(context->GetIsolate(), name).ToLocalChecked(),
            v8::Function::New(context, function, v8::External::New(context->GetIsolate(), data),0, v8::ConstructorBehavior::kAllow, has_side_effect ? v8::SideEffectType::kHasSideEffect : v8::SideEffectType::kHasNoSideEffect).ToLocalChecked()).Check();
}

void builtins::set_context_builtins(v8Runtime *runtime) {
    auto data = runtime;
    auto local_context = runtime->context()->Get(runtime->isolate);
    set_global_func(local_context, "cppLog", v8Log, data);
    set_global_func(local_context, "cppError", v8Error, data);
    set_global_func(local_context, "cppWarn", v8Warn, data);
    set_global_func(local_context, "cppInfo", v8Info, data);
    set_global_func(local_context, "cppSetTimeout", v8SetTimeout, data);
    set_global_func(local_context, "cppClearTimeout", v8ClearTimeout, data);
    set_global_func(local_context, "cppSetInterval", v8SetInterval, data);
    set_global_func(local_context, "cppClearInterval", v8ClearInterval, data);
    set_global_func(local_context, "cppUTF8Decode", v8UTF8Decode, data);
    set_global_func(local_context, "cppPerfNow", v8PerfNow, data);
    set_global_func(local_context, "cppAttachWs", v8AttachWs, data);
    runtime->add_script(glue_code, "__embedded__glue.js");
//    set_global_func(local_context, "cppLoad", v8Load, data);
}