#include <algorithm>
#include <exception>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <vector>
#include <memory>

namespace dummy
{
    struct wtf
    {
        std::vector<std::unique_ptr<std::string>> values;
    };
}