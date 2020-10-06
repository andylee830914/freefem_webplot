#include "httplib.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <csignal>
#include <filesystem>
#include <zlib.h>
#ifndef WITH_NO_INIT
#include "ff++.hpp"
#include "AFunction.hpp"
#include "AFunction_ext.hpp"
#endif
using namespace std;
using namespace std::__fs::filesystem;
using namespace Fem2D;
using namespace httplib;

Server svr;

namespace{
    const char DEFAULT_FETYPE[] = "P1";
    const char DEFAULT_CMM[] = "";
    const char DEFAULT_HOST[] = "localhost";
    const double DEFAULT_PORT = 1234;
    const char BASE_DIR[] =  "/Users/andylee/Work/cpp_web/static";
    int plotcount = 0;
}

std::string get_string(Stack stack, Expression e, const char *const DEFAULT)
{
    const size_t length = 128;
    char *const carg = new char[length];

    if (!e)
        strcpy(carg, DEFAULT);
    else
        strncpy(carg, GetAny<string *>((*e)(stack))->c_str(), length);

    return std::string(carg);
}

void signalHandler(int signum)
{
    cout << "Interrupt signal (" << signum << ") received.\n";
    std::ostringstream path;
    path << BASE_DIR << "/cache/";
    for (const auto &entry : directory_iterator(path.str()))
    {
        remove(entry.path());
    }
    svr.stop();
    exit(signum);
}

class SERVER_Op : public E_F0
{
  public:
    static const int n_name_param = 2;
    static basicAC_F0::name_and_type name_param[];
    Expression nargs[n_name_param];

    // int arg(int i, Stack stack, int defvalue) const { return nargs[i] ? GetAny<double>((*nargs[i])(stack)) : defvalue; }
    double arg(int i, Stack stack, double defvalue) const { return nargs[i] ? GetAny<double>((*nargs[i])(stack)) : defvalue; }
    long arg(int i, Stack stack, long defvalue) const { return nargs[i] ? GetAny<long>((*nargs[i])(stack)) : defvalue; }
    KN<double> *arg(int i, Stack stack, KN<double> *defvalue) const { return nargs[i] ? GetAny<KN<double> *>((*nargs[i])(stack)) : defvalue; }
    bool arg(int i, Stack stack, bool defvalue) const { return nargs[i] ? GetAny<bool>((*nargs[i])(stack)) : defvalue; }

  public:
    SERVER_Op(const basicAC_F0 &args)
    {
        args.SetNameParam(n_name_param, name_param, nargs);
    }

    AnyType operator()(Stack stack) const;
};

class WEBPLOT_Op : public E_F0mps
{
  public:
    Expression eTh, ef, empi;
    static const int n_name_param = 2;
    static basicAC_F0::name_and_type name_param[];
    Expression nargs[n_name_param];

    double arg(int i, Stack stack, double defvalue) const { return nargs[i] ? GetAny<double>((*nargs[i])(stack)) : defvalue; }
    long arg(int i, Stack stack, long defvalue) const { return nargs[i] ? GetAny<long>((*nargs[i])(stack)) : defvalue; }
    KN<double> *arg(int i, Stack stack, KN<double> *defvalue) const { return nargs[i] ? GetAny<KN<double> *>((*nargs[i])(stack)) : defvalue; }
    bool arg(int i, Stack stack, bool defvalue) const { return nargs[i] ? GetAny<bool>((*nargs[i])(stack)) : defvalue; }

  public:
    WEBPLOT_Op(const basicAC_F0 &args, Expression th)
    : eTh(th), ef(0)
    {
        args.SetNameParam(n_name_param, name_param, nargs);
    }

    WEBPLOT_Op(const basicAC_F0 &args, Expression f, Expression th)
    : eTh(th), ef(f)
    {
        args.SetNameParam(n_name_param, name_param, nargs);
    }

    AnyType operator()(Stack stack) const;
};

class WEBMPIPLOT_Op : public E_F0mps
{
  public:
    Expression eTh, ef, empirank, empisize;
    static const int n_name_param = 2;
    static basicAC_F0::name_and_type name_param[];
    Expression nargs[n_name_param];

    double arg(int i, Stack stack, double defvalue) const { return nargs[i] ? GetAny<double>((*nargs[i])(stack)) : defvalue; }
    long arg(int i, Stack stack, long defvalue) const { return nargs[i] ? GetAny<long>((*nargs[i])(stack)) : defvalue; }
    KN<double> *arg(int i, Stack stack, KN<double> *defvalue) const { return nargs[i] ? GetAny<KN<double> *>((*nargs[i])(stack)) : defvalue; }
    bool arg(int i, Stack stack, bool defvalue) const { return nargs[i] ? GetAny<bool>((*nargs[i])(stack)) : defvalue; }

  public:

    WEBMPIPLOT_Op(const basicAC_F0 &args, Expression th, Expression mpirank, Expression mpisize)
    : eTh(th), ef(0), empirank(mpirank), empisize(mpisize)
    {
        args.SetNameParam(n_name_param, name_param, nargs);
    }

    WEBMPIPLOT_Op(const basicAC_F0 &args, Expression f, Expression th, Expression mpirank, Expression mpisize)
    : eTh(th), ef(f), empirank(mpirank), empisize(mpisize)
    {
        args.SetNameParam(n_name_param, name_param, nargs);
    }

    AnyType operator()(Stack stack) const;
};

AnyType SERVER_Op::operator()(Stack stack) const
{

    const std::string host = get_string(stack, nargs[0], DEFAULT_HOST);
    const int port = static_cast<int>(arg(1,stack,DEFAULT_PORT));

    svr.set_base_dir(BASE_DIR);

    svr.set_file_request_handler([&](const Request &req, Response &res) {
        if (req.path.find(".json.gz") != string::npos)
        {
            res.set_header("Content-Encoding", "gzip");
            res.set_header("Content-Type", "application/json");
            res.set_header("Cache-Control", "no-cache");
        }
        else
        {
            res.set_header("Cache-Control", "public");
        }
          
    });

    svr.Get("/", [](const Request &req, Response &res) {
        ifstream ifs("/Users/andylee/Work/cpp_web/index.html");
        std::string hp_html((std::istreambuf_iterator<char>(ifs)),
                            std::istreambuf_iterator<char>());
        res.set_content(hp_html, "text/html");
    });

    svr.Get("/total_temp", [](const Request &req, Response &res) {
        stringstream json;
        json << "{ \"total\" :" << plotcount << "}" << endl;
        string jsonstr;
        jsonstr = json.str();
        res.set_content(jsonstr, "application/json");
    });

    cout << endl;
    cout << "Starting server at http://"<< host <<":"<< port <<"/" << endl;
    cout << "Quit the server with CONTROL-C." << endl;
    signal(SIGINT, signalHandler);
    svr.listen(host.c_str(), port);

    return 0.0;
}

basicAC_F0::name_and_type WEBMPIPLOT_Op::name_param[] =
    {
        // modify static const int n_name_param = ... in the above member
        {"cmm", &typeid(string *)},
        {"fetype", &typeid(string *)}
        //{  "logscale",  &typeid(bool)} // not implemented
};


basicAC_F0::name_and_type WEBPLOT_Op::name_param[] =
    {
        // modify static const int n_name_param = ... in the above member
        {"cmm", &typeid(string *)},
        {"fetype", &typeid(string *)}
        //{  "logscale",  &typeid(bool)} // not implemented
};

basicAC_F0::name_and_type SERVER_Op::name_param[] =
    {
        // modify static const int n_name_param = ... in the above member
        {"host", &typeid(string *)},
        {"port", &typeid(double)}
        //{  "logscale",  &typeid(bool)} // not implemented
};


AnyType WEBPLOT_Op::operator()(Stack stack) const
{
    const std::string cmm = get_string(stack, nargs[0], DEFAULT_CMM);
    const std::string fetype = get_string(stack, nargs[1], DEFAULT_FETYPE);
    plotcount = plotcount+1;
    const Mesh *const pTh = GetAny<const Mesh *const>((*eTh)(stack));
    ffassert(pTh);
    const Fem2D::Mesh &Th(*pTh);
    const int nVertices = Th.nv;
    const int nTriangles = Th.nt;

    R2 Pmin, Pmax;
    Th.BoundingBox(Pmin, Pmax);

    const double &x0 = Pmin.x;
    const double &y0 = Pmin.y;

    const double &x1 = Pmax.x;
    const double &y1 = Pmax.y;


    const double unset = -1e300;
    int mi,Mi;
    double myfmin = -unset;
    double myfmax = unset;

    KN<double> f_FE(Th.nv, unset);

    if (fetype != "P1")
    {
        std::cout << "plotPDF() : Unknown fetype : " << fetype << std::endl;
        std::cout << "plotPDF() : Interpolated as P1 (piecewise-linear)" << std::endl;
    }

    std::ostringstream mesh_name;
    mesh_name << BASE_DIR << "/cache/mesh" << plotcount << ".json.gz";
    gzFile gz_mesh_file;
    gz_mesh_file = gzopen(mesh_name.str().c_str(), "wb");
    std::stringstream mesh_json;
    mesh_json << "{ \"mesh\" :" << endl;
    mesh_json << std::setiosflags(std::ios::scientific) << std::setprecision(16) << "  [" << endl;
    for (int it = 0; it < Th.nt; it++)
    {
        for (int iv = 0; iv < 3; iv++)
        {

            int i = Th(it, iv);

            if (iv == 0){
                mesh_json << "[ ";
            }else{
                mesh_json << "  ";
            }
            MeshPointStack(stack)->setP(pTh, it, iv);
            // std::cout << it << std::endl;
            double temp;
            if(ef)
            {
                temp = GetAny<double>((*ef)(stack)); // Expression ef is atype<double>()
            }
            else
            {
                temp = 0;
            }
            
            if (myfmin >= temp) 
            {
                mi = i;
                myfmin = temp;
            }
            
            if (myfmax <= temp) 
            {
                Mi = i;
                myfmax = temp;
            }
            mesh_json << "{\"index\":" << i << ",\"x\":" << Th(i).x << ",\"y\":" << Th(i).y << ",\"u\":" << temp << "}";

            if (iv != 2)
            {
                mesh_json << "," << endl;
            }
            else
            {
                mesh_json << "]" << endl;
            }

        }
        if (it != Th.nt - 1)
        {
            mesh_json << ",";
        }
        mesh_json << endl;
    }
    mesh_json << "  ]" << endl;
    mesh_json << "}" << endl;
    // cout << myfmin << "," << myfmax << endl;
    unsigned long int file_mesh_size = sizeof(char) * mesh_json.str().size();
    // gzwrite(gz_mesh_file, (void *)&file_mesh_size, sizeof(file_mesh_size));
    gzwrite(gz_mesh_file, (void *)(mesh_json.str().data()), file_mesh_size);
    gzclose(gz_mesh_file);

    std::ostringstream vertex_name;
    vertex_name << BASE_DIR << "/cache/vertex" << plotcount << ".json.gz";
    gzFile gz_vertex_file;
    gz_vertex_file = gzopen(vertex_name.str().c_str(), "wb");
    std::stringstream vertex_json;

    vertex_json << std::setiosflags(std::ios::scientific) << std::setprecision(16);
    vertex_json << "{" << endl;
    vertex_json << "  \"cmm\" : \"" << cmm << "\"," << endl;
    vertex_json << "  \"minmax\": [{\"id\":" << mi << ",\"u\":" << myfmin << "}," << endl;
    vertex_json << "             {\"id\":" << Mi << ",\"u\":" << myfmax << "}]," << endl;

    vertex_json << "  \"position\": [" << endl;
    for (int i = 0; i < Th.nv; i++)
    {
        vertex_json << "    {\"x\":" << Th(i).x << ",\"y\":" << Th(i).y << "}";
        if (i != Th.nv - 1)
        {
            vertex_json << ",";
        }
        vertex_json << endl;
    }
    vertex_json << "  ]" << endl;
    vertex_json << "}" << endl;
    unsigned long int file_vertex_size = sizeof(char) * vertex_json.str().size();
    // gzwrite(gz_vertex_file, (void *)&file_vertex_size, sizeof(file_vertex_size));
    gzwrite(gz_vertex_file, (void *)(vertex_json.str().data()), file_vertex_size);
    gzclose(gz_vertex_file);

    std::ostringstream edge_name;
    edge_name << BASE_DIR << "/cache/edge" << plotcount << ".json.gz";
    gzFile gz_edge_file;
    gz_edge_file = gzopen(edge_name.str().c_str(), "wb");
    std::stringstream edge_json;
    edge_json << "{ \"edge\" :" << endl;
    edge_json << std::setiosflags(std::ios::scientific) << std::setprecision(16) << "  [" << endl;
    for (int i = 0; i < Th.neb; i++)
    {
        const int &v0 = Th(Th.bedges[i][0]);
        const int &v1 = Th(Th.bedges[i][1]);
        edge_json << "    { \"label\": " << Th.be(i) << "," << endl;
        edge_json << "      \"vertices\":[{\"x\":" << Th(v0).x << ",\"y\":" << Th(v0).y << "}," << endl;
        edge_json << "                  {\"x\":" << Th(v1).x << ",\"y\":" << Th(v1).y << "}]" << endl;
        edge_json << "    }";
        if (i != Th.neb - 1)
        {
            edge_json << ",";
        }
        edge_json << endl;
    }
    edge_json << "  ]" << endl;
    edge_json << "}" << endl;

    unsigned long int file_edge_size = sizeof(char) * edge_json.str().size();
    // gzwrite(gz_edge_file, (void *)&file_edge_size, sizeof(file_edge_size));
    gzwrite(gz_edge_file, (void *)(edge_json.str().data()), file_edge_size);
    gzclose(gz_edge_file);

    if (plotcount == 1)
    {
        std::ostringstream basic_name;
        basic_name << BASE_DIR << "/cache/basic" << plotcount << ".json.gz";
        gzFile gz_basic_file;
        gz_basic_file = gzopen(basic_name.str().c_str(), "wb");
        std::stringstream basic_json;

        basic_json << std::setiosflags(std::ios::scientific) << std::setprecision(16);
        basic_json << "{" << endl;
        basic_json << " \"bounds\":[[" << x0 << "," << y0 << "]," << endl;
        basic_json << "           [" << x1 << "," << y1 << "]]" << endl;
        basic_json << "}" << endl;
        unsigned long int file_basic_size = sizeof(char) * basic_json.str().size();
        // gzwrite(gz_basic_file, (void *)&file_basic_size, sizeof(file_basic_size));
        gzwrite(gz_basic_file, (void *)(basic_json.str().data()), file_basic_size);
        gzclose(gz_basic_file);
    }


    return true;
}

AnyType WEBMPIPLOT_Op::operator()(Stack stack) const
{
    int mpi_rank = GetAny<long>((*empirank)(stack));
    int mpi_size = GetAny<long>((*empisize)(stack));
    if (mpi_rank == 1){
        plotcount = plotcount+1;
    }

    const std::string cmm = get_string(stack, nargs[0], DEFAULT_CMM);
    const std::string fetype = get_string(stack, nargs[1], DEFAULT_FETYPE);
    const Mesh *const pTh = GetAny<const Mesh *const>((*eTh)(stack));
    ffassert(pTh);
    const Fem2D::Mesh &Th(*pTh);
    const int nVertices = Th.nv;
    const int nTriangles = Th.nt;

    R2 Pmin, Pmax;
    Th.BoundingBox(Pmin, Pmax);

    const double &x0 = Pmin.x;
    const double &y0 = Pmin.y;

    const double &x1 = Pmax.x;
    const double &y1 = Pmax.y;


    const double unset = -1e300;
    int mi,Mi;
    double myfmin = -unset;
    double myfmax = unset;

    KN<double> f_FE(Th.nv, unset);

    if (fetype != "P1")
    {
        std::cout << "plotPDF() : Unknown fetype : " << fetype << std::endl;
        std::cout << "plotPDF() : Interpolated as P1 (piecewise-linear)" << std::endl;
    }

    std::ostringstream mesh_name;
    mesh_name << BASE_DIR << "/cache/mesh" << plotcount << "_" << mpi_rank << ".json.gz";
    gzFile gz_mesh_file;
    gz_mesh_file = gzopen(mesh_name.str().c_str(), "wb");
    std::stringstream mesh_json;
    mesh_json << "{ \"mesh\" :" << endl;
    mesh_json << std::setiosflags(std::ios::scientific) << std::setprecision(16) << "  [" << endl;
    for (int it = 0; it < Th.nt; it++)
    {
        for (int iv = 0; iv < 3; iv++)
        {

            int i = Th(it, iv);

            if (iv == 0){
                mesh_json << "[ ";
            }else{
                mesh_json << "  ";
            }
            MeshPointStack(stack)->setP(pTh, it, iv);
            // std::cout << it << std::endl;
            double temp;
            if(ef)
            {
                temp = GetAny<double>((*ef)(stack)); // Expression ef is atype<double>()
            }
            else
            {
                temp = 0;
            }
            
            if (myfmin >= temp) 
            {
                mi = i;
                myfmin = temp;
            }
            
            if (myfmax <= temp) 
            {
                Mi = i;
                myfmax = temp;
            }
            mesh_json << "{\"index\":" << i << ",\"x\":" << Th(i).x << ",\"y\":" << Th(i).y << ",\"u\":" << temp << "}";

            if (iv != 2)
            {
                mesh_json << "," << endl;
            }
            else
            {
                mesh_json << "]" << endl;
            }

        }
        if (it != Th.nt - 1)
        {
            mesh_json << ",";
        }
        mesh_json << endl;
    }
    mesh_json << "  ]" << endl;
    mesh_json << "}" << endl;
    // cout << myfmin << "," << myfmax << endl;
    unsigned long int file_mesh_size = sizeof(char) * mesh_json.str().size();
    // gzwrite(gz_mesh_file, (void *)&file_mesh_size, sizeof(file_mesh_size));
    gzwrite(gz_mesh_file, (void *)(mesh_json.str().data()), file_mesh_size);
    gzclose(gz_mesh_file);

    std::ostringstream vertex_name;
    vertex_name << BASE_DIR << "/cache/vertex" << plotcount << "_" << mpi_rank << ".json.gz";
    gzFile gz_vertex_file;
    gz_vertex_file = gzopen(vertex_name.str().c_str(), "wb");
    std::stringstream vertex_json;

    vertex_json << std::setiosflags(std::ios::scientific) << std::setprecision(16);
    vertex_json << "{" << endl;
    vertex_json << "  \"cmm\" : \"" << cmm << "\"," << endl;
    vertex_json << "  \"minmax\": [{\"id\":" << mi << ",\"u\":" << myfmin << "}," << endl;
    vertex_json << "             {\"id\":" << Mi << ",\"u\":" << myfmax << "}]," << endl;

    vertex_json << "  \"position\": [" << endl;
    for (int i = 0; i < Th.nv; i++)
    {
        vertex_json << "    {\"x\":" << Th(i).x << ",\"y\":" << Th(i).y << "}";
        if (i != Th.nv - 1)
        {
            vertex_json << ",";
        }
        vertex_json << endl;
    }
    vertex_json << "  ]" << endl;
    vertex_json << "}" << endl;
    unsigned long int file_vertex_size = sizeof(char) * vertex_json.str().size();
    // gzwrite(gz_vertex_file, (void *)&file_vertex_size, sizeof(file_vertex_size));
    gzwrite(gz_vertex_file, (void *)(vertex_json.str().data()), file_vertex_size);
    gzclose(gz_vertex_file);

    std::ostringstream edge_name;
    edge_name << BASE_DIR << "/cache/edge" << plotcount << "_" << mpi_rank << ".json.gz";
    gzFile gz_edge_file;
    gz_edge_file = gzopen(edge_name.str().c_str(), "wb");
    std::stringstream edge_json;
    edge_json << "{ \"edge\" :" << endl;
    edge_json << std::setiosflags(std::ios::scientific) << std::setprecision(16) << "  [" << endl;
    for (int i = 0; i < Th.neb; i++)
    {
        const int &v0 = Th(Th.bedges[i][0]);
        const int &v1 = Th(Th.bedges[i][1]);
        edge_json << "    { \"label\": " << Th.be(i) << "," << endl;
        edge_json << "      \"vertices\":[{\"x\":" << Th(v0).x << ",\"y\":" << Th(v0).y << "}," << endl;
        edge_json << "                  {\"x\":" << Th(v1).x << ",\"y\":" << Th(v1).y << "}]" << endl;
        edge_json << "    }";
        if (i != Th.neb - 1)
        {
            edge_json << ",";
        }
        edge_json << endl;
    }
    edge_json << "  ]" << endl;
    edge_json << "}" << endl;

    unsigned long int file_edge_size = sizeof(char) * edge_json.str().size();
    // gzwrite(gz_edge_file, (void *)&file_edge_size, sizeof(file_edge_size));
    gzwrite(gz_edge_file, (void *)(edge_json.str().data()), file_edge_size);
    gzclose(gz_edge_file);

    // if (plotcount == 1)
    {
        std::ostringstream basic_name;
        basic_name << BASE_DIR << "/cache/basic" << plotcount << "_" << mpi_rank << ".json.gz";
        gzFile gz_basic_file;
        gz_basic_file = gzopen(basic_name.str().c_str(), "wb");
        std::stringstream basic_json;

        basic_json << std::setiosflags(std::ios::scientific) << std::setprecision(16);
        basic_json << "{" << endl;
        basic_json << " \"rank\": "<< mpi_rank <<"," << endl;
        basic_json << " \"bounds\":[[" << x0 << "," << y0 << "]," << endl;
        basic_json << "           [" << x1 << "," << y1 << "]]" << endl;
        basic_json << "}" << endl;
        unsigned long int file_basic_size = sizeof(char) * basic_json.str().size();
        // gzwrite(gz_basic_file, (void *)&file_basic_size, sizeof(file_basic_size));
        gzwrite(gz_basic_file, (void *)(basic_json.str().data()), file_basic_size);
        gzclose(gz_basic_file);
    }

    if (mpi_rank == 1)
    {
        std::ostringstream basic_name;
        basic_name << BASE_DIR << "/cache/basic" << plotcount << ".json.gz";
        gzFile gz_basic_file;
        gz_basic_file = gzopen(basic_name.str().c_str(), "wb");
        std::stringstream basic_json;

        basic_json << std::setiosflags(std::ios::scientific) << std::setprecision(16);
        basic_json << "{" << endl;
        basic_json << " \"mpi\": \"True\"," << endl;
        basic_json << " \"size\":"<< mpi_size << endl;
        basic_json << "}" << endl;
        unsigned long int file_basic_size = sizeof(char) * basic_json.str().size();
        // gzwrite(gz_basic_file, (void *)&file_basic_size, sizeof(file_basic_size));
        gzwrite(gz_basic_file, (void *)(basic_json.str().data()), file_basic_size);
        gzclose(gz_basic_file);
    }


    return true;
}



class WEBPLOT : public OneOperator
{
    const int argc;

  public:
    WEBPLOT()    : OneOperator(atype<long>(),                  atype<const Mesh *>()), argc(1) {}
    WEBPLOT(int) : OneOperator(atype<long>(), atype<double>(), atype<const Mesh *>()), argc(2) {}
    WEBPLOT(int,int)     : OneOperator(atype<long>(),                  atype<const Mesh *>(), atype<long>(), atype<long>()), argc(3) {}
    WEBPLOT(int,int,int) : OneOperator(atype<long>(), atype<double>(), atype<const Mesh *>(), atype<long>(), atype<long>()), argc(4) {}

    E_F0 *code(const basicAC_F0 &args) const
    {
        if (argc == 1)
            return new WEBPLOT_Op(args, t[0]->CastTo(args[0]));
        else if (argc == 2)
            return new WEBPLOT_Op(args, t[0]->CastTo(args[0]), t[1]->CastTo(args[1]));
        else if (argc == 3)
            return new WEBMPIPLOT_Op(args, t[0]->CastTo(args[0]), t[1]->CastTo(args[1]), t[2]->CastTo(args[2]));
        else if (argc == 4)
            return new WEBMPIPLOT_Op(args, t[0]->CastTo(args[0]), t[1]->CastTo(args[1]), t[2]->CastTo(args[2]), t[3]->CastTo(args[3]));
        else
            ffassert(0);
    }
};

class SERVER : public OneOperator
{
    const int argc;

  public:
    SERVER()    : OneOperator(atype<double>()), argc(0) {}

    E_F0 *code(const basicAC_F0 &args) const
    {
        if (argc == 0)
            return new SERVER_Op(args);
        else
            ffassert(0);
    }
};


static void init(){
    Global.Add("server", "(", new SERVER());
    Global.Add("webplot", "(", new WEBPLOT());
    Global.Add("webplot", "(", new WEBPLOT(0));
    Global.Add("webplotMPI", "(", new WEBPLOT(0,0));
    Global.Add("webplotMPI", "(", new WEBPLOT(0,0,0));
}

LOADFUNC(init);
