#pragma once

#include <ugine/Color.h>
#include <ugine/Error.h>
#include <ugine/String.h>
#include <ugine/UUID.h>
#include <ugine/Ugine.h>
#include <ugine/Vector.h>

#include <algorithm>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <nlohmann/json.hpp>

namespace ugine {

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(ColorRGB, r, g, b);
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(ColorRGBA, r, g, b, a);

class ParseError : Error {
public:
    ParseError(const char* msg, const char* name, const char* file, int line, const char* function) noexcept
        : Error{ msg, name, file, line, function } {}
};
} // namespace ugine

template <class J, class T> void optional_to_json(J& j, const char* name, const std::optional<T>& value) {
    if (value) {
        j[name] = *value;
    }
}

template <class J, class T> void optional_from_json(const J& j, const char* name, std::optional<T>& value) {
    const auto it = j.find(name);
    if (it != j.end()) {
        value = it->template get<T>();
    } else {
        value = std::nullopt;
    }
}

template <typename> constexpr bool is_optional = false;
template <typename T> constexpr bool is_optional<std::optional<T>> = true;

template <typename T> void extended_to_json(const char* key, nlohmann::json& j, const T& value) {
    if constexpr (is_optional<T>)
        optional_to_json(j, key, value);
    else
        j[key] = value;
}
template <typename T> void extended_from_json(const char* key, const nlohmann::json& j, T& value) {
    if constexpr (is_optional<T>)
        optional_from_json(j, key, value);
    else
        j.at(key).get_to(value);
}

#define EXTEND_JSON_TO(v1) extended_to_json(#v1, nlohmann_json_j, nlohmann_json_t.v1);
#define EXTEND_JSON_FROM(v1) extended_from_json(#v1, nlohmann_json_j, nlohmann_json_t.v1);

#define NLOHMANN_JSONIFY_ALL_THINGS(Type, ...)                                                                                                                 \
    inline void to_json(nlohmann::json& nlohmann_json_j, const Type& nlohmann_json_t) {                                                                        \
        NLOHMANN_JSON_EXPAND(NLOHMANN_JSON_PASTE(EXTEND_JSON_TO, __VA_ARGS__))                                                                                 \
    }                                                                                                                                                          \
    inline void from_json(const nlohmann::json& nlohmann_json_j, Type& nlohmann_json_t) {                                                                      \
        NLOHMANN_JSON_EXPAND(NLOHMANN_JSON_PASTE(EXTEND_JSON_FROM, __VA_ARGS__))                                                                               \
    }

#define UGINE_SERIALIZABLE(T)                                                                                                                                  \
    namespace nlohmann {                                                                                                                                       \
        template <> struct adl_serializer<T> {                                                                                                                 \
            static void to_json(json& j, const T& v) { v.ToJson(j); }                                                                                          \
            static void from_json(const json& j, T& v) { v.FromJson(j); }                                                                                      \
        };                                                                                                                                                     \
    }

namespace nlohmann {

template <> struct adl_serializer<glm::vec4> {
    static void to_json(json& j, const glm::vec4& v) {
        j["x"] = v.x;
        j["y"] = v.y;
        j["z"] = v.z;
        j["w"] = v.w;
    }
    static void from_json(const json& j, glm::vec4& v) {
        if (j.is_array()) {
            for (size_t i = 0; i < std::min<size_t>(4, j.size()); ++i) {
                v[static_cast<int>(i)] = j[i];
            }
            for (size_t i = j.size(); i < 4; ++i) {
                v[static_cast<int>(i)] = 0.0f;
            }
        } else if (j.is_object()) {
            v.x = j.value<f32>("x", 0.0f);
            v.y = j.value<f32>("y", 0.0f);
            v.z = j.value<f32>("z", 0.0f);
            v.w = j.value<f32>("w", 0.0f);
        } else if (j.is_number()) {
            v = glm::vec4{ static_cast<f32>(j) };
        } else {
            UGINE_THROW(ugine::ParseError, "Invalid vector value.");
        }
    }
};

template <> struct adl_serializer<glm::fquat> {
    static void to_json(json& j, const glm::fquat& v) {
        j["x"] = v.x;
        j["y"] = v.y;
        j["z"] = v.z;
        j["w"] = v.w;
    }
    static void from_json(const json& j, glm::fquat& v) {
        if (j.is_array()) {
            for (size_t i = 0; i < std::min<size_t>(4, j.size()); ++i) {
                v[static_cast<int>(i)] = j[i];
            }

            for (size_t i = j.size(); i < 4; ++i) {
                v[static_cast<int>(i)] = 0.0f;
            }
        } else if (j.is_object()) {
            v.x = j.value<f32>("x", 0.0f);
            v.y = j.value<f32>("y", 0.0f);
            v.z = j.value<f32>("z", 0.0f);
            v.w = j.value<f32>("w", 0.0f);
        } else {
            UGINE_THROW(ugine::ParseError, "Invalid vector value.");
        }
    }
};

template <> struct adl_serializer<glm::vec3> {
    static void to_json(json& j, const glm::vec3& v) {
        j["x"] = v.x;
        j["y"] = v.y;
        j["z"] = v.z;
    }
    static void from_json(const json& j, glm::vec3& v) {
        if (j.is_array()) {
            for (size_t i = 0; i < std::min<size_t>(3, j.size()); ++i) {
                v[static_cast<int>(i)] = j[i];
            }
            for (size_t i = j.size(); i < 3; ++i) {
                v[static_cast<int>(i)] = 0.0f;
            }
        } else if (j.is_object()) {
            v.x = j.value<f32>("x", 0.0f);
            v.y = j.value<f32>("y", 0.0f);
            v.z = j.value<f32>("z", 0.0f);
        } else if (j.is_number()) {
            v = glm::vec3{ static_cast<f32>(j) };
        } else {
            UGINE_THROW(ugine::ParseError, "Invalid vector value.");
        }
    }
};

template <> struct adl_serializer<glm::vec2> {
    static void to_json(json& j, const glm::vec2& v) {
        j["x"] = v.x;
        j["y"] = v.y;
    }
    static void from_json(const json& j, glm::vec2& v) {
        if (j.is_array()) {
            for (size_t i = 0; i < std::min<size_t>(2, j.size()); ++i) {
                v[static_cast<int>(i)] = j[i];
            }
            for (size_t i = j.size(); i < 2; ++i) {
                v[static_cast<int>(i)] = 0.0f;
            }
        } else if (j.is_object()) {
            v.x = j.value<f32>("x", 0.0f);
            v.y = j.value<f32>("y", 0.0f);
        } else if (j.is_number()) {
            v = glm::vec2{ static_cast<f32>(j) };
        } else {
            UGINE_THROW(ugine::ParseError, "Invalid vector value.");
        }
    }
};

template <> struct adl_serializer<glm::ivec4> {
    static void to_json(json& j, const glm::ivec4& v) {
        j["x"] = v.x;
        j["y"] = v.y;
        j["z"] = v.z;
        j["w"] = v.w;
    }
    static void from_json(const json& j, glm::ivec4& v) {
        if (j.is_array()) {
            for (size_t i = 0; i < std::min<size_t>(4, j.size()); ++i) {
                v[static_cast<int>(i)] = j[i];
            }
            for (size_t i = j.size(); i < 4; ++i) {
                v[static_cast<int>(i)] = 0;
            }
        } else if (j.is_object()) {
            v.x = j.value<int>("x", 0);
            v.y = j.value<int>("y", 0);
            v.z = j.value<int>("z", 0);
            v.w = j.value<int>("w", 0);
        } else if (j.is_number()) {
            v = glm::ivec4{ static_cast<int>(j) };
        } else {
            UGINE_THROW(ugine::ParseError, "Invalid vector value.");
        }
    }
};

template <> struct adl_serializer<glm::ivec3> {
    static void to_json(json& j, const glm::ivec3& v) {
        j["x"] = v.x;
        j["y"] = v.y;
        j["z"] = v.z;
    }
    static void from_json(const json& j, glm::ivec3& v) {
        if (j.is_array()) {
            for (size_t i = 0; i < std::min<size_t>(3, j.size()); ++i) {
                v[static_cast<int>(i)] = j[i];
            }
            for (size_t i = j.size(); i < 3; ++i) {
                v[static_cast<int>(i)] = 0;
            }
        } else if (j.is_object()) {
            v.x = j.value<int>("x", 0);
            v.y = j.value<int>("y", 0);
            v.z = j.value<int>("z", 0);
        } else if (j.is_number()) {
            v = glm::ivec3{ static_cast<int>(j) };
        } else {
            UGINE_THROW(ugine::ParseError, "Invalid vector value.");
        }
    }
};

template <> struct adl_serializer<glm::ivec2> {
    static void to_json(json& j, const glm::ivec2& v) {
        j["x"] = v.x;
        j["y"] = v.y;
    }
    static void from_json(const json& j, glm::ivec2& v) {
        if (j.is_array()) {
            for (size_t i = 0; i < std::min<size_t>(2, j.size()); ++i) {
                v[static_cast<int>(i)] = j[i];
            }
            for (size_t i = j.size(); i < 2; ++i) {
                v[static_cast<int>(i)] = 0;
            }
        } else if (j.is_object()) {
            v.x = j.value<int>("x", 0);
            v.y = j.value<int>("y", 0);
        } else if (j.is_number()) {
            v = glm::ivec2{ static_cast<int>(j) };
        } else {
            UGINE_THROW(ugine::ParseError, "Invalid vector value.");
        }
    }
};

template <> struct adl_serializer<glm::mat4> {
    static void to_json(json& j, const glm::mat4& v) { UGINE_ASSERT(false); }
    static void from_json(const json& j, glm::mat4& v) {
        if (j.is_array()) {
            int index{};
            for (int row{}; row < 4; ++row) {
                for (int col{}; col < 4; ++col) {
                    v[row][col] = index < j.size() ? f32(j[index]) : 0.0f;
                    ++index;
                }
            }
        } else {
            UGINE_THROW(ugine::ParseError, "Invalid matrix4 value.");
        }
    }
};

template <> struct adl_serializer<glm::mat3> {
    static void to_json(json& j, const glm::mat3& v) { UGINE_ASSERT(false); }
    static void from_json(const json& j, glm::mat3& v) {
        if (j.is_array()) {
            int index{};
            for (int row{}; row < 3; ++row) {
                for (int col{}; col < 3; ++col) {
                    v[row][col] = index < j.size() ? f32(j[index]) : 0.0f;
                    ++index;
                }
            }
        } else {
            UGINE_THROW(ugine::ParseError, "Invalid matrix3 value.");
        }
    }
};

template <typename T> struct adl_serializer<ugine::Vector<T>> {
    static void to_json(json& j, const ugine::Vector<T>& v) {
        for (const auto& t : v) {
            j.push_back(t);
        }
    }

    static void from_json(const json& j, ugine::Vector<T>& v) {
        v.Clear();

        for (auto t : j) {
            v.PushBack(t.get<T>());
        }
    }
};

template <> struct adl_serializer<ugine::UUID> {
    static void to_json(json& j, const ugine::UUID& uuid) { j = std::string{ uuid.ToString().Data() }; }
    static void from_json(const json& j, ugine::UUID& uuid) {
        if (j.is_array() && j.size() == 4) {
            for (int i{}; i < 4; ++i) {
                uuid.v[i] = j[i];
            }
        } else {
            uuid = ugine::UUID::FromString(j.get<std::string>().c_str());
        }
    }
};

template <> struct adl_serializer<ugine::String> {
    static void to_json(json& j, const ugine::String& s) { j = std::string{ s.Data() }; }
    static void from_json(const json& j, ugine::String& s) { s = ugine::String{ j.get<std::string>().c_str() }; }
};

} // namespace nlohmann
