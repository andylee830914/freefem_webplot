#include "httplib.h"
#include <iostream>
#ifndef WITH_NO_INIT
#include "ff++.hpp"
#include "AFunction_ext.hpp"
#endif
using namespace std;

using namespace Fem2D;
int myserver(void)
{
    using namespace httplib;

    Server svr;

    svr.Get("/hi", [](const Request& req, Response& res) {
        res.set_content("Hello World!", "text/plain");
    });

    svr.Get(R"(/numbers/(\d+))", [&](const Request& req, Response& res) {
        auto numbers = req.matches[1];
        res.set_content(numbers, "text/plain");
    });

    svr.listen("localhost", 1234);
    return 0;
}


static void init(){
    Global.Add("myserver", "(", new OneOperator0<int>(myserver));
}

LOADFUNC(init);
