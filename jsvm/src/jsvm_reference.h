#ifndef SRC_JSVM_REFERENCE_
#define SRC_JSVM_REFERENCE_
#include <stdint.h>

#include "jsvm_types.h"
#include "jsvm_util.h"

namespace v8impl {
class RefTracker;
using RefList = RefTracker;

class RefTracker {
public:
    RefTracker() : next(nullptr), prev(nullptr) {}

    virtual ~RefTracker() = default;

    static void FinalizeAll(RefList* list);

protected:
    virtual void Finalize();

    inline void Link(RefList* list);

    inline void Unlink()
    {
        if (prev != nullptr) {
            prev->next = next;
        }
        if (next != nullptr) {
            next->prev = prev;
        }
        prev = nullptr;
        next = nullptr;
    }

private:
    RefList* next;
    RefList* prev;
};

class UserReference final : public RefTracker {
public:
    static UserReference* New(JSVM_Env env, v8::Local<v8::Value> value, uint32_t initialRefcount);

    static UserReference* NewData(JSVM_Env env, v8::Local<v8::Data> data, uint32_t initialRefcount);

    ~UserReference() override;

    // Increase and decrease reference
    uint32_t Ref();
    uint32_t Unref();
    uint32_t RefCount();

    // Get v8::Local value
    v8::Local<v8::Value> Get();
    v8::Local<v8::Data> GetData();

    bool IsValue()
    {
        return isValue;
    }

protected:
    UserReference(JSVM_Env env, v8::Local<v8::Data> value, bool isValue, uint32_t initialRefcount);

    void Finalize() override;

private:
    void SetWeak();

private:
    v8impl::Persistent<v8::Data> persistent;
    bool isValue;
    JSVM_Env env;
    uint32_t refcount;
    bool canBeWeak;
};

class FinalizerTracker : public RefTracker {
protected:
    FinalizerTracker(JSVM_Env env, JSVM_Finalize cb, void* data, void* hint);

public:
    static FinalizerTracker* New(JSVM_Env env, JSVM_Finalize cb, void* data, void* hint);

    ~FinalizerTracker() override;

    void* GetData()
    {
        return data;
    }

protected:
    void ResetFinalizer();

    void CallFinalizer();

    void Finalize() override;

    void ResetEnv()
    {
        env = nullptr;
    }

private:
    JSVM_Env env;
    JSVM_Finalize cb;
    void* data;
    void* hint;
};

class RuntimeReference : public FinalizerTracker {
protected:
    RuntimeReference(JSVM_Env env, v8::Local<v8::Value> value, JSVM_Finalize cb, void* data, void* hint);

public:
    static RuntimeReference* New(JSVM_Env env, v8::Local<v8::Value> value, void* data);
    static RuntimeReference* New(JSVM_Env env, v8::Local<v8::Value> value, JSVM_Finalize cb, void* data, void* hint);
    static void DeleteReference(RuntimeReference* ref);

private:
    inline void SetWeak(bool needSecondPass);
    static void FirstPassCallback(const v8::WeakCallbackInfo<RuntimeReference>& data);
    static void SecondPassCallback(const v8::WeakCallbackInfo<RuntimeReference>& data);
    static void FirstPassCallbackWithoutFinalizer(const v8::WeakCallbackInfo<RuntimeReference>& data);

private:
    v8impl::Persistent<v8::Value> persistent;
};

class TrackedStringResource : public FinalizerTracker {
public:
    TrackedStringResource(JSVM_Env env, JSVM_Finalize finalizeCallback, void* data, void* finalizeHint)
        : FinalizerTracker(env, finalizeCallback, data, finalizeHint)
    {}

protected:
    // The only time Finalize() gets called before Dispose() is if the
    // environment is dying. Finalize() expects that the item will be unlinked,
    // so we do it here. V8 will still call Dispose() on us later, so we don't do
    // any deleting here. We just null out env_ to avoid passing a stale pointer
    // to the user's finalizer when V8 does finally call Dispose().
    void Finalize() override
    {
        Unlink();
        ResetEnv();
    }

    ~TrackedStringResource()
    {
        CallFinalizer();
    }
};

class ExternalOneByteStringResource : public v8::String::ExternalOneByteStringResource, TrackedStringResource {
public:
    ExternalOneByteStringResource(JSVM_Env env,
                                  char* string,
                                  const size_t length,
                                  JSVM_Finalize finalizeCallback,
                                  void* finalizeHint)
        : TrackedStringResource(env, finalizeCallback, string, finalizeHint), string_(string), length_(length)
    {}

    const char* data() const override
    {
        return string_;
    }
    size_t length() const override
    {
        return length_;
    }

private:
    const char* string_;
    const size_t length_;
};

class ExternalStringResource : public v8::String::ExternalStringResource, TrackedStringResource {
public:
    ExternalStringResource(JSVM_Env env,
                           char16_t* string,
                           const size_t length,
                           JSVM_Finalize finalizeCallback,
                           void* finalizeHint)
        : TrackedStringResource(env, finalizeCallback, string, finalizeHint),
          string_(reinterpret_cast<uint16_t*>(string)), length_(length)
    {}

    const uint16_t* data() const override
    {
        return string_;
    }
    size_t length() const override
    {
        return length_;
    }

private:
    const uint16_t* string_;
    const size_t length_;
};

} // namespace v8impl

#endif