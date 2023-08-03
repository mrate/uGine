#pragma once

#include <ugine/String.h>

#include <algorithm>
#include <unordered_map>

namespace ugine {

struct CVar {
    enum class Type {
        Float,
        Int,
        Bool,
    };

    union Value {
        f32 f;
        i32 i;
        bool b;
    };

    StringID id;
    const char* name;
    /*String*/ const char* description;
    /*String*/ const char* category;
    Type type;

    Value value;
    Value minValue;
    Value maxValue;

    void SetFloat(f32 value) { Set<f32>(value); }
    void SetInt(i32 value) { Set<i32>(value); }
    void SetBool(bool value) { Set<bool>(value); }

    i32 GetInt() const { return Get<i32>(); }
    f32 GetFloat() const { return Get<f32>(); }
    bool GetBool() const { return Get<bool>(); }

    template <typename T> void Set(T v) {
        switch (type) {
        case Type::Float: value.f = std::max<f32>(minValue.f, std::min<f32>(maxValue.f, f32(v))); break;
        case Type::Int: value.i = std::max<i32>(minValue.i, std::min<i32>(maxValue.i, i32(v))); break;
        case Type::Bool: value.b = bool(v); break;
        default: UGINE_ASSERT("Invalid CVar type"); break;
        }
    }

    template <typename T> T Get() const {
        switch (type) {
        case Type::Float: return T(value.f);
        case Type::Int: return T(value.i);
        case Type::Bool: return T(value.b);
        default: UGINE_ASSERT("Invalid CVar type"); return T{};
        }
    }
};

class CVars {
public:
    template <typename T>
    static CVar& Register(const char* name, const char* description, const char* category, CVar::Type type, T initValue, T minVal = T{}, T maxVal = T{}) {
        const auto hashed{ StringID{ name } };
        auto& value = vars_
                          .insert(std::make_pair(hashed,
                              CVar{
                                  .id = hashed,
                                  .name = name,
                                  .description = description,
                                  .category = category,
                                  .type = type,
                              }))
                          .first->second;
        switch (type) {
        case CVar::Type::Int:
            value.minValue.i = i32(minVal);
            value.maxValue.i = i32(maxVal);
            break;
        case CVar::Type::Float:
            value.minValue.f = f32(minVal);
            value.maxValue.f = f32(maxVal);
            break;
        }
        value.Set(initValue);

        return value;
    }
    static CVar& Get(StringID hash);

    static std::unordered_map<StringID, CVar>& All() { return vars_; }

private:
    inline static std::unordered_map<StringID, CVar> vars_;
};

} // namespace ugine