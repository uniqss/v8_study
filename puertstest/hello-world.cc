// Copyright 2015 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#define V8_COMPRESS_POINTERS
#define V8_31BIT_SMIS_ON_64BIT_ARCH

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <map>
#include <string>
#include <limits>

#include "Binding.hpp"
#include "CppObjectMapper.h"

#include "libplatform/libplatform.h"
#include "v8.h"


static void log(const v8::FunctionCallbackInfo<v8::Value>& info) {
    v8::Isolate* isolate = info.GetIsolate();
    v8::Local<v8::Context> context = isolate->GetCurrentContext();

    std::string msg = *(v8::String::Utf8Value(isolate, info[0]->ToString(context).ToLocalChecked()));
    std::cout << msg << std::endl;
}

class TestClass {
   public:
    TestClass(int p) {
        std::cout << "TestClass(" << p << ")" << std::endl;
        X = p;
    }

    static void Print(std::string msg) { std::cout << msg << std::endl; }

    int Add(int a, int b) {
        std::cout << "Add(" << a << "," << b << ")" << std::endl;
        return a + b;
    }

    int X;
};

UsingCppType(TestClass);


int main(int argc, char* argv[]) {
    std::cout << "main 1" << std::endl;
    // Initialize V8.
    v8::V8::InitializeICUDefaultLocation(argv[0]);
    v8::V8::InitializeExternalStartupData(argv[0]);

    std::unique_ptr<v8::Platform> platform = v8::platform::NewDefaultPlatform();
    v8::V8::InitializePlatform(platform.get());
    v8::V8::Initialize();
    std::cout << "main 2" << std::endl;

    // Create a new Isolate and make it the current one.
    v8::Isolate::CreateParams create_params;
    create_params.array_buffer_allocator = v8::ArrayBuffer::Allocator::NewDefaultAllocator();
    v8::Isolate* isolate = v8::Isolate::New(create_params);
    std::cout << "main 3" << std::endl;

    puerts::FCppObjectMapper cppObjectMapper;
    {
        v8::Isolate::Scope isolate_scope(isolate);

        // Create a stack-allocated handle scope.
        v8::HandleScope handle_scope(isolate);

        // Create a new context.
        v8::Local<v8::Context> context = v8::Context::New(isolate);

        // Enter the context for compiling and running the hello world script.
        v8::Context::Scope context_scope(context);


        v8::Local<v8::Object> Global = context->Global();

        Global
            ->Set(context, v8::String::NewFromUtf8(isolate, "log").ToLocalChecked(), v8::FunctionTemplate::New(isolate, log)->GetFunction(context).ToLocalChecked())
            .Check();


        {
            const char* csource = R"(
                log('hello world');
              )";

            // Create a string containing the JavaScript source code.
            v8::Local<v8::String> source = v8::String::NewFromUtf8(isolate, csource).ToLocalChecked();

            // Compile the source code.
            v8::Local<v8::Script> script = v8::Script::Compile(context, source).ToLocalChecked();

            // Run the script
            auto _unused = script->Run(context);
        }


        std::cout << "main 4" << std::endl;

        cppObjectMapper.Initialize(isolate, context);
        isolate->SetData(MAPPER_ISOLATE_DATA_POS, static_cast<puerts::ICppObjectMapper*>(&cppObjectMapper));
        std::cout << "main 5" << std::endl;

        Global
            ->Set(context, v8::String::NewFromUtf8(isolate, "loadCppType", v8::NewStringType::kNormal).ToLocalChecked(),
                  v8::FunctionTemplate::New(
                      isolate,
                      [](const v8::FunctionCallbackInfo<v8::Value>& Info) {
                          auto pom = static_cast<puerts::FCppObjectMapper*>((v8::Local<v8::External>::Cast(Info.Data()))->Value());
                          pom->LoadCppType(Info);
                      },
                      v8::External::New(isolate, &cppObjectMapper))
                      ->GetFunction(context)
                      .ToLocalChecked())
            .Check();

        // Global
        //     ->Set(context, v8::String::NewFromUtf8(isolate, "loadCppType").ToLocalChecked(),
        //           v8::FunctionTemplate::New(
        //               isolate,
        //               [](const v8::FunctionCallbackInfo<v8::Value>& info) {
        //                   auto pom = static_cast<puerts::FCppObjectMapper*>((v8::Local<v8::External>::Cast(info.Data()))->Value());
        //                   pom->LoadCppType(info);
        //               },
        //               v8::External::New(isolate, &cppObjectMapper))
        //               ->GetFunction(context)
        //               .ToLocalChecked())
        //     .Check();
        std::cout << "main 6" << std::endl;


        //注册
        puerts::DefineClass<TestClass>()
            .Constructor<int>()
            .Function("Print", MakeFunction(&TestClass::Print))
            .Property("X", MakeProperty(&TestClass::X))
            // .Property("X", &(::puerts::PropertyWrapper<decltype(&TestClass::X), &TestClass::X>::getter),
            //           &(::puerts::PropertyWrapper<decltype(&TestClass::X), &TestClass::X>::setter),
            //           ::puerts::PropertyWrapper<decltype(&TestClass::X), &TestClass::X>::info())
            .Method("Add", MakeFunction(&TestClass::Add))
            .Register();
        std::cout << "main 7" << std::endl;

        {
            const char* csource = R"(
                log('function enter');
                const TestClass = loadCppType('TestClass');
                log('loadCppType ok.');
                TestClass.Print('hello world');
                let obj = new TestClass(123);
                
                TestClass.Print(obj.X);
                obj.X = 99;
                TestClass.Print(obj.X);
                
                TestClass.Print('ret = ' + obj.Add(1, 3));
              )";

            // Create a string containing the JavaScript source code.
            v8::Local<v8::String> source = v8::String::NewFromUtf8(isolate, csource).ToLocalChecked();
            std::cout << "main 8" << std::endl;

            // Compile the source code.
            v8::Local<v8::Script> script = v8::Script::Compile(context, source).ToLocalChecked();
            std::cout << "main 9" << std::endl;

            // Run the script
            auto ret = script->Run(context);
            if (ret.IsEmpty()) {
                std::cout << "ret IsEmpty" << std::endl;
            } else {
                std::cout << "ret not empty" << std::endl;
            }
            std::cout << "main 10" << std::endl;
        }

        cppObjectMapper.UnInitialize(isolate);
        std::cout << "main 11" << std::endl;
    }

    // Dispose the isolate and tear down V8.
    isolate->Dispose();
    std::cout << "main 12" << std::endl;
    v8::V8::Dispose();
    std::cout << "main 13" << std::endl;
    v8::V8::ShutdownPlatform();
    delete create_params.array_buffer_allocator;
    return 0;
}
