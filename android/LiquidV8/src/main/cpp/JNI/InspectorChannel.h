//
// Created by ackav on 13-04-2020.
//

#ifndef ANDROID_INSPECTORCHANNEL_H
#define ANDROID_INSPECTORCHANNEL_H

#include "common.h"
#include "v8-inspector.h"

const int kInspectorClientIndex = v8::Context::kDebugIdIndex + 1;

class InspectorFrontend final : public v8_inspector::V8Inspector::Channel {
public:
    explicit InspectorFrontend(Local<Context> context) {
        isolate_ = context->GetIsolate();
        context_.Reset(isolate_, context);
    }
    ~InspectorFrontend() override = default;

private:
    void sendResponse(
            int callId,
            std::unique_ptr<v8_inspector::StringBuffer> message) override {
        Send(message->string());
    }
    void sendNotification(
            std::unique_ptr<v8_inspector::StringBuffer> message) override {
        Send(message->string());
    }
    void flushProtocolNotifications() override {}

    void Send(const v8_inspector::StringView& string) {
        v8::Isolate::AllowJavascriptExecutionScope allow_script(isolate_);
        v8::HandleScope handle_scope(isolate_);
        int length = static_cast<int>(string.length());
        // DCHECK_LT(length, v8::String::kMaxLength);
        Local<v8::String> message =
                (string.is8Bit()
                 ? v8::String::NewFromOneByte(
                                isolate_,
                                reinterpret_cast<const uint8_t*>(string.characters8()),
                                v8::NewStringType::kNormal, length)
                 : v8::String::NewFromTwoByte(
                                isolate_,
                                reinterpret_cast<const uint16_t*>(string.characters16()),
                                v8::NewStringType::kNormal, length))
                        .ToLocalChecked();
        Local<v8::String> callback_name =
                v8::String::NewFromUtf8(isolate_, "receive", v8::NewStringType::kNormal)
                        .ToLocalChecked();
        Local<Context> context = context_.Get(isolate_);
        Local<Value> callback =
                context->Global()->Get(context, callback_name).ToLocalChecked();
        if (callback->IsFunction()) {
            v8::TryCatch try_catch(isolate_);
            Local<Value> args[] = {message};
            Local<v8::Function>::Cast(callback)->Call(context, v8::Undefined(isolate_), 1,
                                                      args).ToLocalChecked();
#ifdef DEBUG
            if (try_catch.HasCaught()) {
                Local<v8::Object> exception = Local<v8::Object>::Cast(try_catch.Exception());
                Local<v8::String> key = v8::String::NewFromUtf8(isolate_, "message",
                                                            v8::NewStringType::kNormal)
                        .ToLocalChecked();
                Local<v8::String> expected =
                        v8::String::NewFromUtf8(isolate_,
                                                "Maximum call stack size exceeded",
                                                v8::NewStringType::kNormal)
                                .ToLocalChecked();
                Local<Value> value = exception->Get(context, key).ToLocalChecked();
                value->StrictEquals(expected);
            }
#endif
        }
    }

    Isolate* isolate_;
    Global<Context> context_;
};

class XV8InspectorClient : public v8_inspector::V8InspectorClient {
public:
    XV8InspectorClient(
            Local<Context> context,
            bool connect,
            v8::Platform* platform,
            ReadDebugMessage readDebugMessage)
    :v8_inspector::V8InspectorClient()
    {
        if (!connect) return;
        readDebugMessage_ = readDebugMessage;
        platform_ = platform;
        isolate_ = context->GetIsolate();
        channel_.reset(new InspectorFrontend(context));
        inspector_ = v8_inspector::V8Inspector::create(isolate_, this);
        session_ =
                inspector_->connect(1, channel_.get(), v8_inspector::StringView());
        context->SetAlignedPointerInEmbedderData(kInspectorClientIndex, this);
        inspector_->contextCreated(v8_inspector::V8ContextInfo(
                context, kContextGroupId, v8_inspector::StringView()));

        Local<Value> function =
                FunctionTemplate::New(isolate_, SendInspectorMessage)
                        ->GetFunction(context)
                        .ToLocalChecked();
        Local<v8::String> function_name =
                String::NewFromUtf8(isolate_, "send", NewStringType::kNormal)
                        .ToLocalChecked();
        context->Global()->Set(context, function_name, function).ToChecked();

        context_.Reset(isolate_, context);
    }

    void runMessageLoopOnPause(int context_group_id)
    {
        if (running_nested_loop_) {
            return;
        }



        terminated_ = false;
        running_nested_loop_ = true;
        while (!terminated_) {
            char* dm = readDebugMessage_();
            Local<v8::String> message =
                    v8::String::NewFromUtf8(isolate_, dm, NewStringType::kNormal)
                    .ToLocalChecked();
            free(dm);
            v8::String::Value buffer(isolate_, message);
            v8_inspector::StringView message_view(*buffer, buffer.length());

            session_->dispatchProtocolMessage(message_view);

            while (v8::platform::PumpMessageLoop(platform_, isolate_)) {}
        }

        terminated_ = false;
        running_nested_loop_ = false;
    }

    void quitMessageLoopOnPause() {
        terminated_ = true;
    }


private:
    static v8_inspector::V8InspectorSession* GetSession(Local<Context> context) {
        XV8InspectorClient* inspector_client = static_cast<XV8InspectorClient*>(
                context->GetAlignedPointerFromEmbedderData(kInspectorClientIndex));
        return inspector_client->session_.get();
    }

    Local<Context> ensureDefaultContextInGroup(int group_id) override {
        // DCHECK(isolate_);
        // DCHECK_EQ(kContextGroupId, group_id);
        return context_.Get(isolate_);
    }

    static void SendInspectorMessage(
            const v8::FunctionCallbackInfo<v8::Value>& args) {
        Isolate* isolate = args.GetIsolate();
        v8::HandleScope handle_scope(isolate);
        Local<Context> context = isolate->GetCurrentContext();
        args.GetReturnValue().Set(v8::Undefined(isolate));
        Local<v8::String> message = args[0]->ToString(context).ToLocalChecked();
        v8_inspector::V8InspectorSession* session =
                XV8InspectorClient::GetSession(context);
        int length = message->Length();
        std::unique_ptr<uint16_t[]> buffer(new uint16_t[length]);
        message->Write(isolate, buffer.get(), 0, length);
        v8_inspector::StringView message_view(buffer.get(), length);
        {
            v8::SealHandleScope seal_handle_scope(isolate);
            session->dispatchProtocolMessage(message_view);
        }
        args.GetReturnValue().Set(v8::True(isolate));
    }

    static const int kContextGroupId = 1;

    std::unique_ptr<v8_inspector::V8Inspector> inspector_;
    std::unique_ptr<v8_inspector::V8InspectorSession> session_;
    std::unique_ptr<v8_inspector::V8Inspector::Channel> channel_;
    Global<Context> context_;
    Isolate* isolate_;
    v8::Platform* platform_;
    bool running_nested_loop_;
    bool terminated_;
    ReadDebugMessage readDebugMessage_;
};

#endif //ANDROID_INSPECTORCHANNEL_H
