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
    explicit InspectorFrontend(Local<Context> context, LoggerCallback sendDebugMessage) {
        sendDebugMessage_ = sendDebugMessage;
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
        v8::HandleScope scope(isolate_);

        int length = string.length();
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
        Local<v8::String> v8Msg = maybeString.ToLocalChecked();
        int utfLen = v8Msg->Utf8Length(isolate_);
        char* utf = (char*) malloc( utfLen + 1);
        v8Msg->WriteUtf8(isolate_, utf);
        sendDebugMessage_(utf);
        free(utf);
    }

    Isolate* isolate_;
    Global<Context> context_;
    LoggerCallback sendDebugMessage_;
};

class XV8InspectorClient : public v8_inspector::V8InspectorClient {
public:
    XV8InspectorClient(
            Local<Context> context,
            bool connect,
            v8::Platform* platform,
            ReadDebugMessage readDebugMessage,
            LoggerCallback sendDebugMessage)
    :v8_inspector::V8InspectorClient()
    {
        if (!connect) return;
        readDebugMessage_ = readDebugMessage;
        platform_ = platform;
        isolate_ = context->GetIsolate();
        channel_.reset(new InspectorFrontend(context, sendDebugMessage));
        inspector_ = v8_inspector::V8Inspector::create(isolate_, this);
        session_ =
                inspector_->connect(1, channel_.get(), v8_inspector::StringView());
        context->SetAlignedPointerInEmbedderData(kInspectorClientIndex, this);
        inspector_->contextCreated(v8_inspector::V8ContextInfo(
                context, kContextGroupId, v8_inspector::StringView()));
        context_.Reset(isolate_, context);
    }

    void runMessageLoopOnPause(int context_group_id) override
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

    void quitMessageLoopOnPause() override {
        terminated_ = true;
    }

    void SendDebugMessage(XString msg) {
        Isolate* _isolate = isolate_;
        HandleScope handleScope(isolate_);
        Isolate::Scope scope (isolate_);
        Local<Context> context = context_.Get(_isolate);
        Context::Scope contextScope(context);
        Local<v8::String> message = V8_STRING(msg);
        String::Value buffer(isolate_, message);
        v8_inspector::StringView messageView(*buffer, buffer.length());
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
