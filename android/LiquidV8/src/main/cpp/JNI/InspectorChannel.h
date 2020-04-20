//
// Created by ackav on 13-04-2020.
//

#ifndef ANDROID_INSPECTORCHANNEL_H
#define ANDROID_INSPECTORCHANNEL_H

#include <android/log.h>

#include "common.h"
#include "v8-inspector.h"
#include "V8Context.h"

const int kInspectorClientIndex = v8::Context::kDebugIdIndex + 1;

class V8OutputInspectorMessageTask: public v8::Task {
private:
    V8Context* context;
    std::unique_ptr<v8_inspector::StringBuffer> message;
public:

    V8OutputInspectorMessageTask(
            V8Context* c,
            std::unique_ptr<v8_inspector::StringBuffer> &msg) {
        context = c;
        message.swap(msg);
    }

    static void Post(Isolate* isolate, std::unique_ptr<v8_inspector::StringBuffer> &buffer) {
        V8Context* context = V8Context::From(isolate);
        std::unique_ptr<Task> t1 =
                std::make_unique<V8OutputInspectorMessageTask>(context, buffer);
        context
           ->GetPlatform()
           ->GetForegroundTaskRunner(isolate)
           ->PostTask(std::move(t1));
    }

    void Run() override {
        context->OutputInspectorMessage(message);
    }
};

class InspectorFrontend final : public v8_inspector::V8Inspector::Channel {
public:
    explicit InspectorFrontend(V8Context* vc, SendDebugMessage sendDebugMessage) {
        sendDebugMessage_ = sendDebugMessage;
        isolate_ = vc->GetIsolate();
    }
    ~InspectorFrontend() override = default;

private:
    void sendResponse(
            int callId,
            std::unique_ptr<v8_inspector::StringBuffer> message) override {
//        if (message.get() != nullptr)
//            V8OutputInspectorMessageTask::Post(isolate_, message);
         Send(callId, message->string());
    }
    void sendNotification(
            std::unique_ptr<v8_inspector::StringBuffer> message) override {
        Send(0, message->string());
//        if (message.get() != nullptr)
//            V8OutputInspectorMessageTask::Post(isolate_, message);
    }
    void flushProtocolNotifications() override {}

    void Send(int callId, const v8_inspector::StringView& string) {
//        sendDebugMessage_(
//                string.length(),
//                (void*) (string.is8Bit() ? string.characters8() : nullptr),
//                (void*) (!string.is8Bit() ? string.characters16() : nullptr)
//                );

        int length = static_cast<int>(string.length());
        __android_log_print(ANDROID_LOG_INFO, "V8", "Send %d %d", callId, length);

        // __android_log_print(ANDROID_LOG_INFO, "V8", "Send %s ", string.characters8());
        if (string.is8Bit()) {
            __android_log_print(ANDROID_LOG_INFO, "V8", "Send %s ", string.characters8());
            sendDebugMessage_((char*)string.characters8());
            return;
        } else {
            __android_log_print(ANDROID_LOG_INFO, "V8", "Allocating Array for %d bytes", length);
            long* t = new long[length + 1];
            t[length] = 0;
            const uint16_t * s = string.characters16();
            for(int i = 0; i < length; i++) {
                t[i] = s[i];
            }
            __android_log_print(ANDROID_LOG_INFO, "V8", "Send %ls ", t);
        }

        Isolate* _isolate = isolate_;
        V8_HANDLE_SCOPE
        v8::TryCatch tryCatch(isolate_);
        v8::Local<v8::String> message;
        v8::MaybeLocal<v8::String> maybeString =
                (string.is8Bit()
                 ? v8::String::NewFromOneByte(
                                isolate_,
                                reinterpret_cast<const uint8_t*>(string.characters8()),
                                v8::NewStringType::kNormal, length)
                 : v8::String::NewFromTwoByte(
                                isolate_,
                                reinterpret_cast<const uint16_t*>(string.characters16()),
                                v8::NewStringType::kNormal, length));
        Local<v8::String> v8Msg;
        if (!maybeString.ToLocal(&v8Msg)) {
            __android_log_print(ANDROID_LOG_INFO, "V8", "Failed to create String");
            return;
        }
        char* utf = V8StringToXString(_isolate,  v8Msg);
        sendDebugMessage_(utf);
        free(utf);
    }

    Isolate* isolate_;
    SendDebugMessage sendDebugMessage_;
};

class XV8InspectorClient : public v8_inspector::V8InspectorClient {
public:
    XV8InspectorClient(
            V8Context* vc,
            bool connect,
            v8::Platform* platform,
            ReadDebugMessage readDebugMessage,
            SendDebugMessage sendDebugMessage)
    :v8_inspector::V8InspectorClient()
    {
        if (!connect) return;
        readDebugMessage_ = readDebugMessage;
        platform_ = platform;
        isolate_ = vc->GetIsolate();
        channel_.reset(new InspectorFrontend(vc, sendDebugMessage));
        inspector_ = v8_inspector::V8Inspector::create(isolate_, this);
        session_ =
                inspector_->connect(1, channel_.get(), v8_inspector::StringView());
        Local<Context> context = vc->GetContext();
        context->SetAlignedPointerInEmbedderData(kInspectorClientIndex, this);
        inspector_->contextCreated(v8_inspector::V8ContextInfo(
                context, kContextGroupId, v8_inspector::StringView()));
        context_.Reset(isolate_, context);
    }

//    void runMessageLoopOnPause(int context_group_id) override
//    {
//        if (running_nested_loop_) {
//            return;
//        }
//
//
//
//        terminated_ = false;
//        running_nested_loop_ = true;
//        while (!terminated_) {
//            char* dm = readDebugMessage_();
//            Local<v8::String> message =
//                    v8::String::NewFromUtf8(isolate_, dm, NewStringType::kNormal)
//                    .ToLocalChecked();
//            free(dm);
//            v8::String::Value buffer(isolate_, message);
//            v8_inspector::StringView message_view(*buffer, buffer.length());
//            session_->dispatchProtocolMessage(message_view);
//
//            while (v8::platform::PumpMessageLoop(platform_, isolate_)) {}
//        }
//
//        terminated_ = false;
//        running_nested_loop_ = false;
//    }
//
//    void quitMessageLoopOnPause() override {
//        terminated_ = true;
//    }

    inline void SendDebugMessage(v8_inspector::StringView &messageView) {
        session_->dispatchProtocolMessage(messageView);
    }


private:

    Local<Context> ensureDefaultContextInGroup(int group_id) override {
        // DCHECK(isolate_);
        // DCHECK_EQ(kContextGroupId, group_id);
        return context_.Get(isolate_);
    }

//    static void SendInspectorMessage(
//            const v8::FunctionCallbackInfo<v8::Value>& args) {
//        Isolate* isolate = args.GetIsolate();
//        v8::HandleScope handle_scope(isolate);
//        Local<Context> context = isolate->GetCurrentContext();
//        Context::Scope scope(context);
//        args.GetReturnValue().Set(v8::Undefined(isolate));
//        Local<v8::String> message = args[0]->ToString(context).ToLocalChecked();
//        v8_inspector::V8InspectorSession* session =
//                XV8InspectorClient::GetSession(context);
//        int length = message->Length();
//        std::unique_ptr<uint16_t[]> buffer(new uint16_t[length]);
//        message->Write(isolate, buffer.get(), 0, length);
//        v8_inspector::StringView message_view(buffer.get(), length);
//        {
//            v8::SealHandleScope seal_handle_scope(isolate);
//            session->dispatchProtocolMessage(message_view);
//        }
//        args.GetReturnValue().Set(v8::True(isolate));
//    }

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
