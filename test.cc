#include "httplib.h"
#include <iostream>
#include <sstream>
#ifndef WITH_NO_INIT
#include "ff++.hpp"
#include "AFunction.hpp"
#include "AFunction_ext.hpp"
#endif
using namespace std;

// using namespace Fem2D;
using namespace httplib;
Server svr;
double myserver(Stack stack)
{



    svr.set_base_dir("./static");
    svr.Get("/", [](const Request &req, Response &res) {
        ifstream ifs("index.html");
        std::string hp_html((std::istreambuf_iterator<char>(ifs)),
                            std::istreambuf_iterator<char>());
        res.set_content(hp_html, "text/html");
    });
    cout << "server started" << endl;
    svr.listen("localhost", 1234);

    return 0.0;
}

bool myresponse(    Stack stack,
                    KN<double> *const &pu,
                    Fem2D::Mesh const * const &pTh)
{
    const Fem2D::Mesh &Th(*pTh);
    KN<double> &u(*pu);

    // ajax API part
    svr.Get("/hi", [](const Request &req, Response &res) {
        res.set_content("<svg height=\"250\" width=\"500\"><polygon points=\"220,10 300,210 170,250 123,234\" style=\"fill:lime;stroke:purple;stroke-width:1\" />Sorry, your browser does not support inline SVG.</svg>", "text/html");
    });

    svr.Get("/mesh", [&](const Request &req, Response &res) {
        // generate element(triangle) JSON
        stringstream json;
        json << "[" << endl;
        for (int i = 0; i < Th.nt; i++)
        {
            const int &v0 = Th(i, 0);
            const int &v1 = Th(i, 1);
            const int &v2 = Th(i, 2);
            json << "  [ {\"index\":" <<v0<< ",\"x\":" << Th(v0).x << ",\"y\":" << Th(v0).y << ",\"u\":" << u[v0] << "}," << endl;
            json << "    {\"index\":" <<v1<< ",\"x\":" << Th(v1).x << ",\"y\":" << Th(v1).y << ",\"u\":" << u[v1] << "}," << endl;
            json << "    {\"index\":" <<v2<< ",\"x\":" << Th(v2).x << ",\"y\":" << Th(v2).y << ",\"u\":" << u[v2] << "}]";
            if (i != Th.nt - 1)
            {
                json << ",";
            }
            json << endl;
        }
        json << "]" << endl;
        string jsonstr;
        jsonstr = json.str();
        res.set_content(jsonstr, "application/json");
    });

    svr.Get("/vertex", [&](const Request &req, Response &res) {
        // generate vertices JSON
        stringstream json;
        json << "[" << endl;
        for (int i = 0; i < Th.nv; i++)
        {
            json << "  {\"x\":" << Th(i).x << ",\"y\":" << Th(i).y << ",\"u\":" << u[i] << "}";
            if (i != Th.nv - 1)
            {
                json << ",";
            }
            json << endl;
        }
        json << "]" << endl;
        string jsonstr;
        jsonstr = json.str();
        res.set_content(jsonstr, "application/json");
    });

    svr.Get("/edge", [&](const Request &req, Response &res) {
        // generate edge JSON
        stringstream json;
        json << "[" << endl;
        for (int i = 0; i < Th.neb; i++)
        {
            const int &v0 = Th(Th.bedges[i][0]);
            const int &v1 = Th(Th.bedges[i][1]);
            json << "  { \"label\": "<< Th.be(i) <<"," << endl;
            json << "    \"vertices\":[{\"x\":" << Th(v0).x << ",\"y\":" << Th(v0).y << ",\"u\":" << u[v0] << "}," << endl;
            json << "                {\"x\":" << Th(v1).x << ",\"y\":" << Th(v1).y << ",\"u\":" << u[v1] << "}]" << endl;
            json << "  }";
            if (i != Th.neb - 1)
            {
                json << ",";
            }
            json << endl;
        }
        json << "]" << endl;
        string jsonstr;
        jsonstr = json.str();
        res.set_content(jsonstr, "application/json");
    });

    svr.Get(R"(/hello/(\d+))", [](const Request &req, Response &res) {
        auto numbers = req.matches[1];
        res.set_content(numbers, "text/plain");
    });

    return true;
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
    Global.Add("myresponse", "(", new OneOperator2s_<bool, KN<double> *const, Fem2D::Mesh const * const>(myresponse));
}

LOADFUNC(init);
