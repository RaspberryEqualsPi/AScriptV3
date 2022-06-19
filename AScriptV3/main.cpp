#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <fstream>
#include <map>
#include <sstream>
#include <filesystem>
enum class ASCTypes {
    string = 0,
    integer = 1,
    nil = 2,
    errored = 3
};
enum class LineType {
    funccall = 0,
    unknown = 1,
    set = 2,
    ascgoto = 3,
    ifgoto = 4,
};
struct ASCType {
    std::string name;
    ASCTypes type;
    ASCType(std::string name1, ASCTypes type1) {
        name = name1;
        type = type1;
    }
    ASCType() { ; }
};
struct ASCArg {
    uintptr_t first;
    ASCType second;
};
typedef void(*ASCFunc)(std::vector<ASCArg>, void*);
namespace ASC {
    std::string ASCToString(ASCArg arg) {
        return *(std::string*)arg.first; // pointer to string
    }
    int ASCToInteger(ASCArg arg) {
        return (int)arg.first; // not a pointer
    }
    void ASCCleanString(ASCArg arg) {
        delete (std::string*)arg.first;
    }
    class ASCContext {
    public:
        void regfunc(std::string name, ASCFunc ptr) {
            
            funcreg.push_back(std::pair<std::string, ASCFunc>(name, ptr));
        }
        void run() {
            std::string unused;
            std::vector<std::pair<int, int>> lines;
            int counter = 0;
            for (int i = 0; i < code.size(); i++) {
                if (code[i] == ';') {
                    lines.push_back(std::pair<int, int>(i - counter, i));
                    counter = 0;
                }
                else {
                    counter++;
                }
            }
            while (cline < lines.size()){
                std::string line = code.substr(lines[cline].first, lines[cline].second);
                runLine(line);
                cline++;
                //std::cout << "cline: " << cline << "\n";
            }
        }
        std::map<std::string, std::string>& getVars() {
            return vars;
        }
        ASCContext(std::string filename) {
            std::fstream code1;
            code1.open(filename, std::fstream::in);
            std::stringstream buffer;
            buffer << code1.rdbuf();
            code = buffer.str();
            //std::cout << code << "\n";
        }
    private:
        int lines = 0;
        int cline = 0;
        std::string code;
        std::vector<std::pair<std::string, ASCFunc>> funcreg;
        std::map<std::string, std::string> vars;
        void runLine(std::string line){
            //std::cout << "\nline: " << line << "\n";
            while (line[0] == '\n' || line[0] == '\r') {
                line.erase(line.begin());
            }
            line = line.substr(0, line.find(';'));// + (line.find(';') < line.size() - 1));
            LineType lt = getLineType(line);
            if (lt == LineType::funccall) {
                //std::cout << "funccall\n";
                std::vector<ASCArg> args = lexArguments(line.substr(line.find('(') + 1, line.size() - line.find('(')).substr(0, line.find(')'))); // gets whats inside of the parenthesis
                for (int i = 0; i < funcreg.size(); i++) {
                    if (funcreg[i].first == line.substr(0, line.find('('))) {
                        funcreg[i].second(args, (void*)this);
                        //std::cout << "function found\n";
                    }
                }
            }
            if (lt == LineType::set) {
                //std::cout << "line thing" << line << "end\n";
                std::string after = line.substr(4, line.size() - 4);
                std::string result = after.substr(after.find(' ') + 1, after.size() - after.find(' ') - 1);
                while (result.substr(result.size() - 1, 1) == ";") {
                    result = result.substr(0, result.size() - 1);
                }
                if (vars.count(after.substr(0, after.find(' '))) < 1) {
                    vars.insert({ after.substr(0, after.find(' ')),  result});
                }
                else {
                    vars[after.substr(0, after.find(' '))] = result;
                }
                //std::cout << "result one: " << result << "end\n";
            }
            if (lt == LineType::ascgoto) {
                cline = stoi(line.substr(5, line.size() - 5)) - 2;
                //std::cout << "did goto" << cline << "\n";
            }
            if (lt == LineType::ifgoto) {
                std::string after = line.substr(7, line.size() - 7);
                std::string condition = after.substr(0, after.find(' '));
                std::string go = after.substr(after.find(' ') + 1, after.size() - after.find(' ') - 1);
                //std::cout << "condition: " << condition << ", size: " << condition.size() << "\n";
                //std::cout << "line: " << go << ", size: " << go.size() << "\n";
                if (vars.count(condition) > 0) {
                    if (stoi(vars[condition]) > 0) {
                        cline = stoi(go) - 2;
                        //std::cout << "condition is " << vars["condition"] << " with size " << vars["condition"].size() << "\n";
                        //std::cout << "hello world: " << stoi(vars[condition]) << "\n";
                        //std::cout << "set line to " << cline << "\n";
                    }
                }
                else {
                    fprintf(stderr, "Error on line %i: Undefined variable \"%s\" used in ifgoto statement", cline, condition.c_str());
                }
            }
        }
        LineType getLineType(std::string& line) {
            bool isFunc = false;
            bool isSet = false;
            bool isGoto = false;
            bool isIfGoto = false;
            try {
                for (int i = 0; i < funcreg.size(); i++) {
                    std::string funcname = funcreg[i].first + "(";
                    if (line.size() >= funcname.size()) {
                        if (line.substr(0, funcname.size()) == funcname) {
                            isFunc = true;
                        }
                    }
                }
                if (line.size() >= 4) {
                    if (line.substr(0, 4) == "set ") {
                        isSet = true;
                        //std::cout << "sets have been implemented\n";
                    }
                }
                if (line.size() >= 7) {
                    if (line.substr(0, 7) == "ifgoto ") {
                        isIfGoto = true;
                    }
                }
            }
            catch (std::out_of_range) {
                fprintf(stderr, "There seems to be an unidentified error in your code");
            }
            if (line.substr(0, 5) == "goto ")
                isGoto = true;
            if (isFunc)
                return LineType::funccall;
            if (isSet)
                return LineType::set;
            if (isGoto)
                return LineType::ascgoto;
            if (isIfGoto)
                return LineType::ifgoto;
            fprintf(stderr, "Unknown action type, behavior may be undefined\n");
            return LineType::unknown;
        }
        bool replace(std::string& str, const std::string& from, const std::string& to) {
            size_t start_pos = str.find(from);
            if (start_pos == std::string::npos)
                return false;
            str.replace(start_pos, from.length(), to);
            return true;
        }
        std::vector<ASCArg> lexArguments(std::string list) {
            //std::cout << "list: " << list << "\n";
            std::vector<ASCArg> result;
            bool escaped = false;
            bool insstr = false;
            ASCTypes ctype = ASCTypes::nil;
            bool newtype = true;
            bool isvar = true;
            ASCArg curarg;
            bool sdec = false;
            bool negative = false;
            curarg.second = ASCType("nil", ASCTypes::nil);
            for (int i = 0; i < list.size(); i++) {
                if (!(newtype && list[i] == ' ')) {
                    if (sdec) {
                        i--;
                        sdec = false;
                        //std::cout << "list[i] = " << list[i] << "\n";
                    }
                    if ((isdigit(list[i]) || list[i] == '-') && newtype) {
                        if (list[i] == '-') {
                            negative = true;
                        }
                        ctype = ASCTypes::integer;
                        curarg.second.name = "integer";
                        curarg.second.type = ctype;
                        curarg.first = 0;
                        newtype = false;
                    }
                    if (ctype == ASCTypes::integer && !(list[i] == ',' || list[i] == ')')) {
                        if (!(isdigit(list[i]) || list[i] == '-')) {
                            ctype = ASCTypes::errored;
                            curarg.second.name = "errored";
                            curarg.second.type = ctype;
                            fprintf(stderr, "Non-digit value mixed into integer");
                        }
                        else if (isdigit(list[i])) {
                            curarg.first *= 10;
                            curarg.first += list[i] - '0';
                        }
                    }
                    if (list[i] == '"' && !escaped) { // gotta make sure this isn't the ending quote
                        insstr = !insstr;
                        if (insstr) {
                            ctype = ASCTypes::string;
                            curarg.second.name = "string";
                            curarg.second.type = ctype;
                            curarg.first = (uintptr_t)(new std::string());
                            //std::cout << "quotation mark identified\n";
                            newtype = false;
                        }
                    }
                    if (ctype == ASCTypes::string) {
                        bool special = false;
                        if (!(list[i] == '"' && !escaped) && !(list[i] == '\\' && !escaped) && !(list[i] == ',' && !insstr || list[i] == ')' && !insstr)) { // the mother of all checks
                            std::string appendee = std::string(&(list[i]), 1);
                            if (appendee == "\\")
                                special = true;
                            *((std::string*)(curarg.first)) += appendee;
                            //std::cout << "appendee: " << appendee << "\n";
                        }
                        escaped = false;
                        if (list[i] == '\\' && !escaped && !special) {
                            escaped = true;
                        }
                    }
                    if (newtype && isvar && i < list.find(')')) { // this must mean it's unidentified, do a variable check
                        std::string after = list.substr(i, list.size() - i);
                        std::string result;
                        if (after.find(',') != std::string::npos)
                            result = after.substr(0, after.find(','));
                        else if (after.find(')') != std::string::npos)
                            result = after.substr(0, after.find(')'));
                        while (result[0] == ' ') {
                            result = result.substr(1, result.size() - 1);
                            //std::cout << "result now: " << result << "\n";
                        }
                        //std::cout << "size: " << result.size() << ", result: " << result << "\n";
                        if (vars.count(result) > 0) {
                            //std::cout << "replacement: " << list.substr(0, i - 1) + vars[result] + list.substr(list.find(result) + result.size()) << "\n";
                            //std::cout << "old list: " << list << " end\n";
                            //std::cout << "result: " << result << " end\n";
                            //std::cout << "vars[result]: " << vars[result] << " end\n";
                            replace(list, result, vars[result]);
                            //std::cout << "list: " << list << "\n";
                            //std::cout << "new list: " << list << " end\n";
                            sdec = true;
                        }
                        isvar = false;
                    }
                    if (list[i] == ',' && !insstr || list[i] == ')' && !insstr) {
                        if (i + 1 < list.size())
                            i++;
                        if (ctype == ASCTypes::integer && negative) {
                            curarg.first *= -1;
                        }
                        result.push_back(curarg);
                        //std::cout << "string: " << ASCToString(curarg) << "\n";
                        curarg = ASCArg();
                        curarg.second = ASCType("nil", ASCTypes::nil);
                        ctype = ASCTypes::nil;
                        insstr = false;
                        newtype = true;
                        isvar = true;
                        negative = false;
                    }
                }
            }
            return result;
        }
    };
}
void ASCPrintf(std::vector<ASCArg> args, void* con) {
    if (args.size() < 1) {
        fprintf(stderr, "1 or more arguments needed for printf, %i passed\n", (int)args.size());
        return;
    }
    for (int i = 0; i < args.size(); i++) {
        bool identified = false;
        if (args[i].second.name == "string"){
            std::cout << ASC::ASCToString(args[i]);
            ASC::ASCCleanString(args[i]); // make sure to always clean up after yourself when handling strings
            identified = true;
        }
        if (args[i].second.name == "integer") {
            std::cout << ASC::ASCToInteger(args[i]);
            identified = true;
        }
        if (!identified)
            std::cout << "\nType \"" << args[i].second.name << "\" not supported by printf, ignored.\n";
    }
    std::cout << "\n";
}
void increment(std::vector<ASCArg> args, void* conptr) {
    ASC::ASCContext* con = (ASC::ASCContext*)conptr;
    if (args.size() != 2) {
        fprintf(stderr, "2 arguments required for increment, %i passed", (int)args.size());
        return;
    }
    if (args[0].second.type != ASCTypes::string) {
        fprintf(stderr, "String expected for argument 1, %s passed", args[0].second.name.c_str());
    }
    if (args[1].second.type != ASCTypes::integer) {
        fprintf(stderr, "Integer expected for argument 2, %s passed", args[0].second.name.c_str());
    }
    con->getVars()[ASC::ASCToString(args[0])] = std::to_string(stoi(con->getVars()[ASC::ASCToString(args[0])]) + ASC::ASCToInteger(args[1]));
    //std::cout << "incremented: " << con->getVars()[ASC::ASCToString(args[0])] << "\n";
    ASC::ASCCleanString(args[0]);
}

int main(int argc, char** argv) {
    if (argc <= 1) {
        fprintf(stderr, "Usage: %s (filename)", argv[0]);
        return 0;
    }
    if (!std::filesystem::exists(argv[1])) {
        fprintf(stderr, "File \"%s\" not found\nUsage: %s (filename)", argv[1], argv[0]);
        return 0;
    }
    ASC::ASCContext asc(argv[1]);
    asc.regfunc("printf", ASCPrintf);
    asc.regfunc("increment", increment);
    asc.run();
}