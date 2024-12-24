/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "jsvm_env.h"

#include "libplatform/libplatform.h"

void JSVM_Env__::RunAndClearInterrupts()
{
    while (messageQueue.size() > 0) {
        std::vector<Callback> messageQueueTmp {};
        {
            const std::lock_guard<std::mutex> lock(messageQueueMutex);
            messageQueueTmp.swap(messageQueue);
        }
        jsvm::DebugSealHandleScope sealHandleScope(isolate);

        for (auto& cb : messageQueueTmp) {
            cb(this);
        }
    }
}

JSVM_Env__::JSVM_Env__(v8::Isolate* isolate, int32_t apiVersion) : isolate(isolate), apiVersion(apiVersion)
{
    inspectorAgent = jsvm::InspectorAgent::New(this);
    ClearLastError(this);
}

void JSVM_Env__::DeleteMe()
{
    v8impl::RefTracker::FinalizeAll(&finalizerList);
    v8impl::RefTracker::FinalizeAll(&userReferenceList);

    {
        v8::Context::Scope context_scope(context());
        if (inspectorAgent->IsActive()) {
            inspectorAgent->WaitForDisconnect();
        }
        delete inspectorAgent;
    }

    // release lock
    if (locker) {
        delete locker;
        locker = nullptr;
    }

    delete this;
}

#ifndef ENABLE_INSPECTOR
#include "jsvm_log.h"
namespace {
/*
 * If inspector is not enabled, using fake jsvm inspect agent.
 * All Interface in fake agent log error.
 */
class FakeAgent final : public jsvm::InspectorAgent {
public:
    explicit FakeAgent(JSVM_Env env)
    {
        LogError();
    }
    ~FakeAgent() override = default;

public:
    bool Start(const std::string& path, const std::string& hostName, int port, int pid = -1) override
    {
        LogError();
        return false;
    }

    bool Start(const std::string& path, int pid) override
    {
        LogError();
        return false;
    }

    void Stop() override
    {
        LogError();
    }

    bool IsActive() override
    {
        LogError();
        return false;
    }

    void WaitForConnect() override
    {
        LogError();
    }

    void WaitForDisconnect() override
    {
        LogError();
    }

    void PauseOnNextJavascriptStatement(const std::string& reason) override
    {
        LogError();
    }

private:
    void LogError()
    {
        LOG(Error) << "JSVM Inspector is not enabled";
    }
};

} // namespace

jsvm::InspectorAgent* jsvm::InspectorAgent::New(JSVM_Env env)
{
    return new FakeAgent(env);
}
#endif