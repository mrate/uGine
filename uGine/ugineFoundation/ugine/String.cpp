#include "String.h"

#include <ugine/Ugine.h>

namespace ugine {

String::~String() {
    if (capacity_) {
        allocator_->Free(dynamic_);
        capacity_ = 0;
    }
}

String::String(IAllocator& allocator)
    : allocator_{ allocator } {
    Clear();
}

String::String(const char* src, IAllocator& allocator)
    : allocator_{ allocator } {
    Init(src, strlen(src));
}

String::String(const String& other)
    : allocator_{ other.allocator_ }
    , size_{ other.size_ } {

    if (other.size_) {
        Reserve(size_ + 1);
        memcpy(GetStorage(), other.GetStorage(), size_ + 1);
    } else {
        Clear();
    }
}

String& String::operator=(const String& other) {
    size_ = other.size_;
    Reserve(other.capacity_);
    memcpy(GetStorage(), other.GetStorage(), size_ + 1);
    return *this;
}

String::String(String&& other) noexcept
    : allocator_{ other.allocator_ }
    , size_{ other.size_ }
    , capacity_{ other.capacity_ } {

    if (capacity_) {
        dynamic_ = std::exchange(other.dynamic_, nullptr);
        other.capacity_ = 0;
        other.Clear();
    } else {
        memcpy(static_, other.static_, size_ + 1);
    }
}

String& String::operator=(String&& other) {
    if (other.capacity_) {
        if (capacity_) {
            allocator_->Free(dynamic_);
        }
        capacity_ = std::exchange(other.capacity_, 0);
        dynamic_ = std::exchange(other.dynamic_, nullptr);
        size_ = other.size_;

        other.Clear();
    } else {
        memcpy(GetStorage(), other.GetStorage(), other.size_ + 1);
        size_ = other.size_;
    }

    return *this;
}

String& String::operator=(const char* str) {
    Assign(str, strlen(str));
    return *this;
}

String::String(const StringView& other, IAllocator& allocator)
    : allocator_{ allocator } {
    Init(other.Data(), other.Size());
}

String& String::operator=(const StringView& other) {
    Assign(other.Data(), other.Size());
    return *this;
}

String::String(Span<const char> src, IAllocator& allocator)
    : allocator_{ allocator } {
    Init(src.Data(), src.Size());
}

String& String::operator=(Span<const char> str) {
    Assign(str.Data(), str.Size());
    return *this;
}

void String::Init(const char* str, size_t size) {
    if (size > 0) {
        size_ = size;
        const auto cap{ size_ + (str[size - 1] != '\0' ? 1 : 0) };
        Reserve(cap);
        memcpy(GetStorage(), str, size);
        GetStorage()[cap - 1] = '\0';
    } else {
        Clear();
    }
}

void String::Assign(const char* str, size_t size) {
    if (size) {
        size_ = size;
        const auto cap{ size_ + (str[size - 1] != '\0' ? 1 : 0) };
        Reserve(cap);
        memcpy(GetStorage(), str, size);
        GetStorage()[cap - 1] = '\0';
    } else {
        Clear();
    }
}

void String::Reserve(size_t size) {
    if (size <= SMALL_BUFFER_OPTIMIZATION) {
        return;
    }

    if (capacity_ < size) {
        auto dynamic{ static_cast<char*>(allocator_->Realloc(capacity_ ? dynamic_ : nullptr, size)) };
        if (capacity_ == 0) {
            memcpy(dynamic, static_, size_);
        }
        dynamic_ = dynamic;
        capacity_ = size;
    }
}

void String::Append(const String& str) {
    if (str.size_) {
        Reserve(size_ + str.size_ + 1);
        memcpy(GetStorage() + size_, str.GetStorage(), str.size_ + 1);
        size_ += str.size_;
    }
}

void String::Append(const char* str) {
    const auto len{ strlen(str) };
    if (len) {
        Reserve(size_ + len + 1);
        memcpy(GetStorage() + size_, str, len + 1);
        size_ += len;
    }
}

void String::Append(const StringView& str) {
    if (!str.Empty()) {
        Reserve(size_ + str.Size() + 1);
        memcpy(GetStorage() + size_, str.Data(), str.Size() + 1);
        size_ += str.Size();
    }
}

//bool operator==(const String& a, const String& b) {
//    if (a.size_ == b.size_) {
//        auto as{ a.GetStorage() };
//        auto bs{ b.GetStorage() };
//        for (size_t i{}; i < a.size_; ++i, ++as, ++bs) {
//            if (*as != *bs) {
//                return false;
//            }
//        }
//        return true;
//    }
//    return false;
//}
//
//bool operator==(const String& a, const char* b) {
//    const auto size{ strlen(b) };
//    if (a.size_ == size) {
//        auto as{ a.GetStorage() };
//        for (size_t i{}; i < a.size_; ++i, ++as, ++b) {
//            if (*as != *b) {
//                return false;
//            }
//        }
//        return true;
//    }
//    return false;
//}

Span<const char> String::SubStr(size_t start, size_t len) const {
    if (start >= size_) {
        return Span<const char>{};
    } else {
        return Span<const char>{ GetStorage() + start, std::min(len, size_ - start) };
    }
}

bool operator==(StringView a, StringView b) {
    if (a.Size() == b.Size()) {
        auto as{ a.Data() };
        auto bs{ b.Data() };
        for (size_t i{}; i < a.Size(); ++i, ++as, ++bs) {
            if (*as != *bs) {
                return false;
            }
        }
        return true;
    }
    return false;
}

} // namespace ugine
