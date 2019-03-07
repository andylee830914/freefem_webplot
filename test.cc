#include "httplib.h"
#include <iostream>
#ifndef WITH_NO_INIT
#include "ff++.hpp"
#include "AFunction.hpp"
#endif
using namespace std;

// using namespace Fem2D;
using namespace httplib;
Server svr;

double myserver(Stack stack)
{

    cout << "server started" << endl;
    svr.listen("localhost", 1234);

    return 0.0;
}

double myresponse(double a)
{

    // // to get FreeFem++  data
    // MeshPoint &mp = *MeshPointStack(stack); // the struct to get x,y, normal , value
    // double x = mp.P.x;                      // get the current x value
    // double y = mp.P.y;                      // get the current y value

    // cout << "x = " << x << " y=" << y << " " << sin(x)*cos(y) <<  endl;
    svr.Get("/hi", [](const Request &req, Response &res) {
        
        res.set_content("<h1>Hello World!</h1>", "text/html");
    });

    svr.Get(R"(/hello/(\d+))", [a](const Request &req, Response &res) {
        // auto numbers = req.matches[1];
        res.set_content(to_string(a), "text/plain");
    });

    return 0.0;
}

template <class R>
class OneOperator0s : public OneOperator
{
    // the class to defined a evaluated a new function
    // It  must devive from  E_F0 if it is mesh independent
    // or from E_F0mps if it is mesh dependent
    class E_F0_F : public E_F0mps
    {
      public:
        typedef R (*func)(Stack stack);
        func f; // the pointeur to the fnction myfunction
        E_F0_F(func ff) : f(ff) {}

        // the operator evaluation in freefem++
        AnyType operator()(Stack stack) const { return SetAny<R>(f(stack)); }
    };

    typedef R (*func)(Stack);
    func f;

  public:
    // the function which build the freefem++ byte code
    E_F0 *code(const basicAC_F0 &) const { return new E_F0_F(f); }

    // the constructor to say ff is a function without parameter
    // and returning a R
    OneOperator0s(func ff) : OneOperator(map_type[typeid(R).name()]), f(ff) {}
};

static void init(){
    Global.Add("myserver", "(", new OneOperator0s<double>(myserver));
    Global.Add("myresponse", "(", new OneOperator1<double>(myresponse));
}

LOADFUNC(init);
