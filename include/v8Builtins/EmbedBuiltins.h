//
// Created by 16182 on 12/6/2020.
//

#ifndef V8DEBUGGER_EMBEDBUILTINS_H
#define V8DEBUGGER_EMBEDBUILTINS_H

#include"../../../../../../../libs/include/v8.h"
#include"../../../../../../../../../../../AppData/Local/Android/Sdk/ndk/21.1.6352462/toolchains/llvm/prebuilt/windows-x86_64/sysroot/usr/include/c++/v1/vector"
#include"../../../v8Core/include/v8Core/v8Runtime.h"

namespace builtins {
    void set_context_builtins(v8Runtime *runtime);
}

#endif //V8DEBUGGER_EMBEDBUILTINS_H
