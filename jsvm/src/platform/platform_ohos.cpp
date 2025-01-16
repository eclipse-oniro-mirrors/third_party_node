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

#include "platform.h"

// v8 header
#include "v8.h"

// OHOS API header
#ifdef ENABLE_HILOG
#include "hilog/log.h"
#endif
#include <string>
#include <sys/prctl.h>
#include <unordered_set>

#include "hitrace_meter.h"
#include "init_param.h"
#include "jsvm_log.h"
#include "unistd.h"
#ifdef ENABLE_HISYSEVENT
#include "hisysevent.h"
#endif

extern "C" void ReportData(uint32_t resType,
                           int64_t value,
                           const std::unordered_map<std::string, std::string>& mapPayLoad);
static bool isJitMode = true;
namespace ResourceSchedule {
namespace ResType {
enum : uint32_t { RES_TYPE_REPORT_KEY_THREAD = 39 };

enum ReportChangeStatus : int64_t { CREATE = 0, REMOVE = 1 };

enum ThreadRole : int64_t {
    USER_INTERACT = 0,
    NORMAL_DISPLAY = 1,
    IMPORTANT_DISPLAY = 2,
    NORMAL_AUDIO = 3,
    IMPORTANT_AUDIO = 4,
    IMAGE_DECODE = 5
};
} // namespace ResType
} // namespace ResourceSchedule

namespace platform {
void OS::Abort()
{
    std::abort();
}

uint64_t OS::GetUid()
{
    return static_cast<uint64_t>(getuid());
}

uint64_t OS::GetPid()
{
    return static_cast<uint64_t>(getprocpid());
}

uint64_t OS::GetTid()
{
    return static_cast<uint64_t>(getproctid());
}

#ifdef LOG_DOMAIN
#undef LOG_DOMAIN
#endif
#ifdef LOG_TAG
#undef LOG_TAG
#endif

// TODO: set log domain
#define LOG_DOMAIN 0xD003900
#define LOG_TAG "JSVM"

void OS::PrintString(LogLevel level, const char* string)
{
#ifdef ENABLE_HILOG
    // convert platform defined LogLevel to hilog LogLevel
    static constexpr ::LogLevel convertArray[] = { ::LogLevel::LOG_DEBUG, ::LogLevel::LOG_INFO, ::LogLevel::LOG_WARN,
                                                   ::LogLevel::LOG_ERROR, ::LogLevel::LOG_FATAL };
    static_assert(sizeof(convertArray) / sizeof(::LogLevel) == static_cast<uint64_t>(OS::LogLevel::LOG_FATAL) + 1);

    // TODO: use LOG_APP or other LogType
    HiLogPrint(LOG_APP, convertArray[static_cast<uint64_t>(level)], LOG_DOMAIN, LOG_TAG, "%{public}s", string);
#else
    printf("%s", string);
#endif
}

void OS::Print(LogLevel level, const char* format, ...)
{
    constexpr size_t MAX_STRING_SIZE = 1024;
    char string[MAX_STRING_SIZE];
    va_list arguments;
    va_start(arguments, format);
    vsnprintf(string, MAX_STRING_SIZE, format, arguments);
    va_end(arguments);

    PrintString(level, string);
}

RunJsTrace::RunJsTrace(bool RunJs) : RunJs(RunJs)
{
    if (RunJs) {
        StartTrace(HITRACE_TAG_APP, "PureJS");
    } else {
        FinishTrace(HITRACE_TAG_APP);
    }
}

RunJsTrace::RunJsTrace(const char* name) : RunJs(true)
{
    StartTrace(HITRACE_TAG_APP, name);
}

RunJsTrace::~RunJsTrace()
{
    if (RunJs) {
        FinishTrace(HITRACE_TAG_APP);
    } else {
        StartTrace(HITRACE_TAG_APP, "PureJS");
    }
}

namespace ohos {
#define JITFORT_QUERY_ENCAPS 'E'
#define HM_PR_SET_JITFORT 0x6a6974

const std::string ENABLE_JIT_CONF_PATH = "/etc/jsvm/app_jit_enable_list.conf";
bool ProcessBundleName(std::string& bundleName);

void ReadEnableList(const std::string& jitConfigPath, std::unordered_set<std::string>& enableSet)
{
    std::ifstream file(jitConfigPath);
    if (file.is_open()) {
        std::string line;
        while (std::getline(file, line)) {
            if (!line.empty()) {
                enableSet.insert(line);
            }
        }
        file.close();
    } else {
        LOG(Error) << "Failed to open file: " << jitConfigPath << std::endl;
    }
}

bool InJitMode() {
    return isJitMode;
}

inline bool InAppEnableList(const std::string& bundleName, std::unordered_set<std::string>& enableSet)
{
    return (enableSet.count(bundleName) != 0);
}

inline bool HasJitfortACL()
{
    return (prctl(HM_PR_SET_JITFORT, JITFORT_QUERY_ENCAPS, 0) == 0);
}

void ReportKeyThread(ThreadRole role)
{
    uint64_t uid = OS::GetUid();
    uint64_t tid = OS::GetTid();
    uint64_t pid = OS::GetPid();
    std::unordered_map<std::string, std::string> payLoad = { { "uid", std::to_string(uid) },
                                                             { "pid", std::to_string(pid) },
                                                             { "tid", std::to_string(tid) },
                                                             { "role", std::to_string(role) } };
    ReportData(ResourceSchedule::ResType::RES_TYPE_REPORT_KEY_THREAD,
               ResourceSchedule::ResType::ReportChangeStatus::CREATE, payLoad);
}

inline bool ReadSystemXpmState()
{
    constexpr size_t ARG_BUFF_SIZE = 32;
    char buffer[ARG_BUFF_SIZE] = { 0 };
    uint32_t buffSize = sizeof(buffer);

    if (SystemGetParameter("ohos.boot.advsecmode.state", buffer, &buffSize) == 0 && strcmp(buffer, "0") != 0) {
        return true;
    }
    return false;
}

void SetSecurityMode()
{
    constexpr size_t SEC_ARG_CNT = 2;
    std::string bundleName;
    if (!ProcessBundleName(bundleName)) {
        LOG(Error) << "Failed to get bundleName" << std::endl;
        bundleName = "INVALID_BUNDLE_NAME";
    }
    std::unordered_set<std::string> enableList {};
    ReadEnableList(ENABLE_JIT_CONF_PATH, enableList);

    if (ReadSystemXpmState() || (!InAppEnableList(bundleName, enableList) && !HasJitfortACL())) {
        isJitMode = false;
        int secArgc = SEC_ARG_CNT;
        constexpr bool removeFlag = false;
        const char* secArgv[SEC_ARG_CNT] = { "jsvm", "--jitless" };
        v8::V8::SetFlagsFromCommandLine(&secArgc, const_cast<char**>(reinterpret_cast<const char**>(secArgv)),
                                        removeFlag);
    }
}

constexpr int MAX_FILE_LENGTH = 32 * 1024 * 1024;

bool LoadStringFromFile(const std::string& filePath, std::string& content)
{
    std::ifstream file(filePath.c_str());
    if (!file.is_open()) {
        return false;
    }

    file.seekg(0, std::ios::end);
    const long fileLength = file.tellg();
    if (fileLength > MAX_FILE_LENGTH) {
        return false;
    }

    content.clear();
    file.seekg(0, std::ios::beg);
    std::copy(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>(), std::back_inserter(content));
    return true;
}

bool ProcessBundleName(std::string& bundleName)
{
    int pid = getprocpid();
    std::string filePath = "/proc/" + std::to_string(pid) + "/cmdline";
    if (!LoadStringFromFile(filePath, bundleName)) {
        return false;
    }
    if (bundleName.empty()) {
        return false;
    }
    auto pos = bundleName.find(":");
    if (pos != std::string::npos) {
        bundleName = bundleName.substr(0, pos);
    }
    bundleName = bundleName.substr(0, strlen(bundleName.c_str()));
    return true;
}

void WriteHisysevent()
{
#ifdef ENABLE_HISYSEVENT
    std::string bundleName;
    if (!ProcessBundleName(bundleName)) {
        bundleName = "INVALID_BUNDLE_NAME";
    }
    HiSysEventWrite(OHOS::HiviewDFX::HiSysEvent::Domain::JSVM_RUNTIME, "APP_STATS",
                    OHOS::HiviewDFX::HiSysEvent::EventType::STATISTIC, "BUNDLE_NAME", bundleName);
#endif
}
} // namespace ohos

} // namespace platform