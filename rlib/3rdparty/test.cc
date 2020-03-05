#include "prettyprint.hpp"
#include <rlib/stdio.hpp>
#include <list>
using namespace rlib::prettyprint;

int main() {
    std::list<int> ls {1,3,2};
    std::cout << ls;
}
