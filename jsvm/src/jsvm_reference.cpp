#include "jsvm_reference.h"

#include "js_native_api_v8.h"

namespace v8impl {
namespace {
// In JavaScript, weak references can be created for object types (Object,
// Function, and external Object) and for local symbols that are created with
// the `Symbol` function call. Global symbols created with the `Symbol.for`
// method cannot be weak references because they are never collected.
//
// Currently, V8 has no API to detect if a symbol is local or global.
// Until we have a V8 API for it, we consider that all symbols can be weak.
inline bool CanBeHeldWeakly(v8::Local<v8::Value> value)
{
    return value->IsObject() || value->IsSymbol();
}
} // namespace

// RefTracker
inline void RefTracker::Link(RefList* list)
{
    DCHECK(list != nullptr);
    prev = list;
    next = list->next;
    if (next != nullptr) {
        next->prev = this;
    }
    list->next = this;
}

void RefTracker::FinalizeAll(RefList* list)
{
    while (list->next != nullptr) {
        list->next->Finalize();
    }
}

void RefTracker::Finalize()
{
    UNREACHABLE("Finalize need to be realized");
}

// UserReference
UserReference* UserReference::New(JSVM_Env env, v8::Local<v8::Value> value, uint32_t initialRefcount)
{
    auto ref = new UserReference(env, value, true, initialRefcount);

    return ref;
}

UserReference* UserReference::NewData(JSVM_Env env, v8::Local<v8::Data> value, uint32_t initialRefcount)
{
    auto ref = new UserReference(env, value, false, initialRefcount);

    return ref;
}

UserReference::UserReference(JSVM_Env env, v8::Local<v8::Data> value, bool isValue, uint32_t initialRefcount)
    : persistent(env->isolate, value), isValue(isValue), env(env), refcount(initialRefcount), canBeWeak(isValue && CanBeHeldWeakly(value.As<v8::Value>()))
{
    if (refcount == 0) {
        SetWeak();
    }

    Link(&env->userReferenceList);
}

UserReference::~UserReference()
{
    persistent.Reset();
    Unlink();
}

void UserReference::Finalize()
{
    persistent.Reset();
    Unlink();
}

v8::Local<v8::Value> UserReference::Get()
{
    DCHECK(isValue);
    if (persistent.IsEmpty()) {
        return v8::Local<v8::Value>();
    } else {
        return  v8::Local<v8::Data>::New(env->isolate, persistent).As<v8::Value>();
    }
}

v8::Local<v8::Data> UserReference::GetData() {
  if (persistent.IsEmpty()) {
    return v8::Local<v8::Data>();
  } else {
    return v8::Local<v8::Data>::New(env->isolate, persistent);
  }
}

void UserReference::SetWeak()
{
    if (canBeWeak) {
        persistent.SetWeak();
    } else {
        persistent.Reset();
    }
}

uint32_t UserReference::Ref()
{
    // If persistent is cleared by GC, return 0 unconditionally.
    if (persistent.IsEmpty()) {
        return 0;
    }

    if (++refcount == 1) {
        // If persistent can not be weak, it will be cleared in SetWeak().
        DCHECK(canBeWeak);
        persistent.ClearWeak();
    }

    return refcount;
}

uint32_t UserReference::Unref()
{
    // If persistent is cleared by GC, return 0 unconditionally.
    if (persistent.IsEmpty() || refcount == 0) {
        return 0;
    }

    if (--refcount == 0) {
        SetWeak();
    }

    return refcount;
}

uint32_t UserReference::RefCount()
{
    return refcount;
}

// FinalizerTracker
FinalizerTracker* FinalizerTracker::New(JSVM_Env env, JSVM_Finalize cb, void* data, void* hint)
{
    return new FinalizerTracker(env, cb, data, hint);
}

FinalizerTracker::FinalizerTracker(JSVM_Env env, JSVM_Finalize cb, void* data, void* hint)
    : env(env), cb(cb), data(data), hint(hint)
{
    Link(&env->finalizerList);
}

FinalizerTracker::~FinalizerTracker()
{
    Unlink();
}

void FinalizerTracker::ResetFinalizer()
{
    cb = nullptr;
    data = nullptr;
    hint = nullptr;
}

void FinalizerTracker::CallFinalizer()
{
    if (!cb) {
        return;
    }

    JSVM_Finalize cbTemp = cb;
    void* dataTemp = data;
    void* hintTemp = hint;
    ResetFinalizer();

    if (!env) {
        cbTemp(env, dataTemp, hintTemp);
    } else {
        // env->CallFinalizer(cbTemp, dataTemp, hintTemp);
        env->CallIntoModule([&](JSVM_Env env) { cbTemp(env, dataTemp, hintTemp); });
    }
}

void FinalizerTracker::Finalize()
{
    CallFinalizer();
    delete this;
}

RuntimeReference::RuntimeReference(JSVM_Env env, v8::Local<v8::Value> value, JSVM_Finalize cb, void* data, void* hint)
    : FinalizerTracker(env, cb, data, hint), persistent(env->isolate, value)
{
    DCHECK(CanBeHeldWeakly(value));
}

RuntimeReference* RuntimeReference::New(JSVM_Env env, v8::Local<v8::Value> value, void* data)
{
    auto* ref = new RuntimeReference(env, value, nullptr, data, nullptr);
    // Delete self in first pass callback
    ref->SetWeak(false);

    return ref;
}

RuntimeReference*
RuntimeReference::New(JSVM_Env env, v8::Local<v8::Value> value, JSVM_Finalize cb, void* data, void* hint)
{
    auto* ref = new RuntimeReference(env, value, cb, data, hint);
    // Need second pass callback to call finalizer
    ref->SetWeak(cb != nullptr);

    return ref;
}

void RuntimeReference::DeleteReference(RuntimeReference* ref)
{
    // If reference is not added into first pass callbacks, delete this direct.
    if (ref->persistent.IsWeak()) {
        delete ref;
        return;
    }

    // If reference is added into first pass callbacks, reset finalizer function.
    ref->ResetFinalizer();
}

inline void RuntimeReference::SetWeak(bool needSecondPass)
{
    if (needSecondPass) {
        persistent.SetWeak(this, FirstPassCallback, v8::WeakCallbackType::kParameter);
    } else {
        persistent.SetWeak(this, FirstPassCallbackWithoutFinalizer, v8::WeakCallbackType::kParameter);
    }
}

void RuntimeReference::FirstPassCallback(const v8::WeakCallbackInfo<RuntimeReference>& data)
{
    RuntimeReference* reference = data.GetParameter();

    reference->persistent.Reset();
    data.SetSecondPassCallback(RuntimeReference::SecondPassCallback);
}

void RuntimeReference::SecondPassCallback(const v8::WeakCallbackInfo<RuntimeReference>& data)
{
    RuntimeReference* reference = data.GetParameter();

    reference->Finalize();
}

void RuntimeReference::FirstPassCallbackWithoutFinalizer(const v8::WeakCallbackInfo<RuntimeReference>& data)
{
    RuntimeReference* reference = data.GetParameter();

    reference->persistent.Reset();
    delete reference;
}

} // namespace v8impl