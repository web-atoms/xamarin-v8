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

class InspectorFrontend final : public v8_inspector::V8Inspector::Channel {
public:
    explicit InspectorFrontend(V8Context* c, SendDebugMessage sendDebugMessage) {
        sendDebugMessage_ = sendDebugMessage;
        isolate_ = c->GetIsolate();
        vc = c;
    }
    ~InspectorFrontend() override = default;

private:
    void sendResponse(
            int callId,
            std::unique_ptr<v8_inspector::StringBuffer> message) override {
         Send(callId, message->string());
    }
    void sendNotification(
            std::unique_ptr<v8_inspector::StringBuffer> message) override {
        Send(0, message->string());
    }
    void flushProtocolNotifications() override {}

    inline Local<Context> GetContext() {
        return vc->GetContext();
    }

    void Send(int callId, const v8_inspector::StringView& string) {
        if (string.is8Bit()) {
            sendDebugMessage_(string.length(), string.characters8(), nullptr);
        } else {
            sendDebugMessage_(string.length(), nullptr, string.characters16());
        }
    }

    Isolate* isolate_;
    V8Context* vc;
    SendDebugMessage sendDebugMessage_;
};

class XV8InspectorClient : public v8_inspector::V8InspectorClient {
public:
    XV8InspectorClient(
            V8Context* vc,
            bool connect,
            v8::Platform* platform,
            ClrEnv env)
    :v8_inspector::V8InspectorClient()
    {
        if (!connect) return;
        readDebugMessage_ = env->readDebugMessage;
        breakPauseOn_ = env->breakPauseOn;
        platform_ = platform;
        isolate_ = vc->GetIsolate();
        channel_.reset(new InspectorFrontend(vc, env->sendDebugMessage));
        inspector_ = v8_inspector::V8Inspector::create(isolate_, this);
        session_ =
                inspector_->connect(1, channel_.get(), v8_inspector::StringView());
        Local<Context> context = vc->GetContext();
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
        breakPauseOn_(true);
        running_nested_loop_ = true;
        while (!terminated_) {
            auto dm = readDebugMessage_();
            v8_inspector::StringView message_view(dm.Value, dm.Length);
            session_->dispatchProtocolMessage(message_view);
            while (v8::platform::PumpMessageLoop(platform_, isolate_)) {}
        }

        terminated_ = false;
        running_nested_loop_ = false;
    }

    void quitMessageLoopOnPause() override {
        terminated_ = true;
        breakPauseOn_(false);
    }

    inline void SendDebugMessage(v8_inspector::StringView &messageView) {
        session_->dispatchProtocolMessage(messageView);
    }


private:

    Local<Context> ensureDefaultContextInGroup(int group_id) override {
        // DCHECK(isolate_);
        // DCHECK_EQ(kContextGroupId, group_id);
        return context_.Get(isolate_);
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
    BreakPauseOn  breakPauseOn_;
};

#endif //ANDROID_INSPECTORCHANNEL_H
