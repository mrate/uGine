#include <fstream>
#include <iostream>
#include <regex>
#include <string>

class Parser {
public:
    explicit Parser(std::ostream& out)
        : out_{ out } {}

    void Parse(std::string_view line) {
        switch (state_) {
        case State::None: ParseNone(line); break;
        case State::InStruct: ParseStructMember(line); break;
        case State::InEnum: ParseEnumValue(line); break;
        }
    }

    void Print() {}

private:
    enum class State {
        None,
        InStruct,
        InEnum,
    };

    void ParseNone(std::string_view line) {
        // Just enums for now.

        //{
        //    std::regex RE_Struct{ "^\\s*struct ([a-zA-Z_][a-zA-Z0-9_]*) \\{" };

        //    auto it{ std::regex_iterator(line.begin(), line.end(), RE_Struct) };
        //    if (it != std::regex_iterator<std::string_view::iterator>{}) {
        //        auto match{ *it };

        //        state_ = State::InStruct;
        //        identifier_ = match[1];
        //        return;
        //    }
        //}

        {
            std::regex RE_Enum{ "^\\s*enum class ([a-zA-Z_][a-zA-Z0-9_]*)\\s+(:.*)?\\{" };

            auto it{ std::regex_iterator(line.begin(), line.end(), RE_Enum) };
            if (it != std::regex_iterator<std::string_view::iterator>{}) {
                auto match{ *it };

                state_ = State::InEnum;
                identifier_ = match[1];
                return;
            }
        }
    }

    void ParseStructMember(std::string_view line) {
        {
            std::regex RE_StructEnd{ "^\\s*\\};" };

            auto it{ std::regex_iterator(line.begin(), line.end(), RE_StructEnd) };
            if (it != std::regex_iterator<std::string_view::iterator>{}) {
                Finalize();
                return;
            }
        }

        {
            std::regex RE_StructMember{ "^\\s*[a-zA-Z_]([a-zA-Z0-9_<>,\\s]*)\\s+([a-zA-Z_][a-zA-Z0-9_]+)\\s*(\\{[^\\}]*\\})?;" };

            auto it{ std::regex_iterator(line.begin(), line.end(), RE_StructMember) };
            if (it != std::regex_iterator<std::string_view::iterator>{}) {
                auto match{ *it };
                AddMember(match[2]);
            }
        }
    }

    void ParseEnumValue(std::string_view line) {
        {
            std::regex RE_End{ "\\};" };

            auto it{ std::regex_iterator(line.begin(), line.end(), RE_End) };
            if (it != std::regex_iterator<std::string_view::iterator>{}) {
                Finalize();
                return;
            }
        }

        {
            std::regex RE_EnumMember{ "\\s*([a-zA-Z_][a-zA-Z0-9_]+)\\s*(=.*)?," };

            auto it{ std::regex_iterator(line.begin(), line.end(), RE_EnumMember) };
            if (it != std::regex_iterator<std::string_view::iterator>{}) {
                auto match{ *it };
                AddMember(match[1]);
            }
        }
    }

    void Finalize() {
        if (state_ == State::InStruct) {
            out_ << "NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(" << identifier_;
            for (const auto& member : members_) {
                out_ << ", " << member;
            }
            out_ << ");\n";
        } else if (state_ == State::InEnum) {
            out_ << "NLOHMANN_JSON_SERIALIZE_ENUM(" << identifier_ << ", {\n";
            for (const auto& member : members_) {
                out_ << "\t{" << identifier_ << "::" << member << ", \"" << member << "\"},\n";
            }
            out_ << "});\n";
        }

        members_.clear();
        state_ = State::None;
    }

    void AddMember(const std::string& member) { members_.push_back(member); }

    std::ostream& out_;
    State state_{ State::None };
    std::string identifier_;
    std::vector<std::string> members_;
};

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " header_file [output_file]\n";
        return -1;
    }

    std::ifstream in{ argv[1] };
    std::ostream* out{ &std::cout };

    std::ofstream fout;
    if (argc > 2) {
        fout = std::ofstream{ argv[2] };
    }

    Parser parser{ *out };

    std::string line;
    for (; !in.eof(); std::getline(in, line)) {
        parser.Parse(line);
    }
    parser.Parse(line);

    //parser.Print();
}