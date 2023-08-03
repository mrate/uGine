#include <gtest/gtest.h>

#include <ugine/Delegate.h>

using namespace ugine;

void func1(int param) {
    std::cout << "func1(" << param << ")\n";
}

struct Type {
    void MemberFunc(int param) { std::cout << "MemberFunc(" << param << "), member = " << member << "\n"; }

    int member;
};

void func2(const Type& param) {
    std::cout << "func2(Type)\n";
}

void passDelegate(Delegate<void(int)> del) {
    del(5);
}

TEST(DelegateTest, Basic) {
    {
        Delegate<void(int)> d{};
        d.Connect<&func1>();
        d(5);
    }

    {
        Delegate<void(const Type&)> d{};
        d.Connect<&func2>();
        d(Type{});
    }

    {
        Type t{ 42 };

        Delegate<void(int)> d{};
        d.Connect<&Type::MemberFunc>(t);
        d(5);
    }

    {
        int val{ 4 };

        Delegate<void(int)> d{};
        d.Connect([val](int param) { std::cout << "Capturing lambda(" << val << ", " << param << ")\n"; });
        d(5);
    }

    {
        int val{ 1 };

        passDelegate([val](int param) { std::cout << "Capturing lambda(" << val << ", " << param << ")\n"; });

        //Delegate<void(int)> d;
        //d.Connect([val](int param) { std::cout << "Capturing lambda(" << val << ", " << param << ")\n"; });
        //passDelegate(d);
    }

    //{
    //    int v1{ 1 };
    //    int v2{ 2 };
    //    int v3{ 3 };
    //    int v4{ 4 };

    //    passDelegate([v1, v2, v3, v4](int param) { std::cout << "Multi-capture lambda\n"; });
    //}
}