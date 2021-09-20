#include "mongoose.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <csignal>
#include <filesystem>
#include <thread>
#include <zlib.h>
// #include <pthread.h>
#include <thread>
#ifndef WITH_NO_INIT
#include "ff++.hpp"
#include "AFunction.hpp"
#include "AFunction_ext.hpp"
#endif
using namespace std;
using namespace std::__fs::filesystem;
using namespace Fem2D;

void mg_http_reply_wcl(struct mg_connection *c, int code,const char *status, const char *headers,
                   const char *fmt, int len ...) {
  mg_printf(c, "HTTP/1.1 %d %s\r\n%sContent-Length: %d\r\n\r\n", code,
            status, headers == NULL ? "" : headers, len);
  mg_send(c, fmt, len);
}

namespace{
    struct mg_mgr mgr;
    // pthread_t myPthread;
    std::shared_ptr<std::thread> mythread;
    const char DEFAULT_FETYPE[] = "P1";
    const char DEFAULT_CMM[] = "";
    const char DEFAULT_HOST[] = "localhost";
    const double DEFAULT_PORT = 1234;
    const char BASE_DIR[] =  ".";
    static const char *s_root_dir = ".";
    int plotcount = 0;

}

void server_listen(){
    for (;;) mg_mgr_poll(&mgr, 50);
    // pthread_exit(NULL);
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
    path << BASE_DIR << "/static/cache/";
    for (const auto &entry : directory_iterator(path.str()))
    {
        remove(entry.path());
    }
    mg_mgr_free(&mgr);
    exit(signum);
}


static void cb(struct mg_connection *c, int ev, void *ev_data, void *fn_data) {
  if (ev == MG_EV_HTTP_MSG) {
    struct mg_http_message *hm = (struct mg_http_message *) ev_data;
    
    if (mg_http_match_uri(hm, "/api/#")) {
      // All URIs starting with /api/ must be authenticated
      mg_printf(c, "%s", "HTTP/1.1 403 Denied\r\nContent-Length: 0\r\n\r\n");
    } else if (mg_http_match_uri(hm, "/total_temp")) {
        stringstream json;
        json << "{ \"total\" : " << plotcount << " }" << endl;
        const char* jsonstr;
        jsonstr = json.str().c_str();
        mg_http_reply(c, 200, "Content-Type: application/json\r\n", jsonstr);
        
    } else if(mg_http_match_uri(hm, "/static/cache/#")){
        const char* filename;
        filename = hm->uri.ptr;
        std::string s(filename);
        std::string token = "." + s.substr(0, s.find("?"));
        // cout<< token <<endl;
        std::string bodystr;
        std::ifstream fs(token, std::ios_base::binary);
        fs.seekg(0, std::ios_base::end);
        auto size = fs.tellg();
        fs.seekg(0);
        bodystr.resize(static_cast<size_t>(size));
        fs.read(&bodystr[0], static_cast<std::streamsize>(size));
        std::string header = "Content-Type: application/json\r\nContent-Encoding: gzip\r\nCache-Control: no-cache\r\n";
        mg_http_reply_wcl(c, 200,"OK", header.c_str(), bodystr.data(),bodystr.size());
    }else{
        struct mg_http_serve_opts opts = {.root_dir = s_root_dir};
        mg_http_serve_dir(c, (struct mg_http_message *)ev_data, &opts);
        
      
      
    }
  }
}

class SERVER_Op : public E_F0
{
  public:
    static const int n_name_param = 3;
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

class SERVERSHOW_Op : public E_F0
{
  public:
    static const int n_name_param = 3;
    static basicAC_F0::name_and_type name_param[];
    Expression nargs[n_name_param];

    // int arg(int i, Stack stack, int defvalue) const { return nargs[i] ? GetAny<double>((*nargs[i])(stack)) : defvalue; }
    double arg(int i, Stack stack, double defvalue) const { return nargs[i] ? GetAny<double>((*nargs[i])(stack)) : defvalue; }
    long arg(int i, Stack stack, long defvalue) const { return nargs[i] ? GetAny<long>((*nargs[i])(stack)) : defvalue; }
    KN<double> *arg(int i, Stack stack, KN<double> *defvalue) const { return nargs[i] ? GetAny<KN<double> *>((*nargs[i])(stack)) : defvalue; }
    bool arg(int i, Stack stack, bool defvalue) const { return nargs[i] ? GetAny<bool>((*nargs[i])(stack)) : defvalue; }

  public:
    SERVERSHOW_Op(const basicAC_F0 &args)
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

class WEB3PLOT_Op : public E_F0mps
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
    WEB3PLOT_Op(const basicAC_F0 &args, Expression th)
    : eTh(th), ef(0)
    {
        args.SetNameParam(n_name_param, name_param, nargs);
    }

    WEB3PLOT_Op(const basicAC_F0 &args, Expression f, Expression th)
    : eTh(th), ef(f)
    {
        args.SetNameParam(n_name_param, name_param, nargs);
    }

    AnyType operator()(Stack stack) const;
};

class WEB3MPIPLOT_Op : public E_F0mps
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

    WEB3MPIPLOT_Op(const basicAC_F0 &args, Expression th, Expression mpirank, Expression mpisize)
    : eTh(th), ef(0), empirank(mpirank), empisize(mpisize)
    {
        args.SetNameParam(n_name_param, name_param, nargs);
    }

    WEB3MPIPLOT_Op(const basicAC_F0 &args, Expression f, Expression th, Expression mpirank, Expression mpisize)
    : eTh(th), ef(f), empirank(mpirank), empisize(mpisize)
    {
        args.SetNameParam(n_name_param, name_param, nargs);
    }

    AnyType operator()(Stack stack) const;
};

class WEBSPLOT_Op : public E_F0mps
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
    WEBSPLOT_Op(const basicAC_F0 &args, Expression th)
    : eTh(th), ef(0)
    {
        args.SetNameParam(n_name_param, name_param, nargs);
    }

    WEBSPLOT_Op(const basicAC_F0 &args, Expression f, Expression th)
    : eTh(th), ef(f)
    {
        args.SetNameParam(n_name_param, name_param, nargs);
    }

    AnyType operator()(Stack stack) const;
};



AnyType SERVER_Op::operator()(Stack stack) const
{

    const std::string host = get_string(stack, nargs[0], DEFAULT_HOST);
    const std::string base = get_string(stack, nargs[2], BASE_DIR);
    const int port = static_cast<int>(arg(1,stack,DEFAULT_PORT));

    std::ostringstream static_path;
    static_path << base << "/static";

    
    mg_mgr_init(&mgr);
    mg_http_listen(&mgr, "http://localhost:8081", cb, NULL);
    // std::async();
    // auto a1 = std::async(server_listen);
    // pthread_create(&myPthread, NULL, server_listen, NULL);
    mythread = std::make_shared<std::thread>(server_listen);
    signal(SIGINT, signalHandler);

    return 0.0;
}

AnyType SERVERSHOW_Op::operator()(Stack stack) const
{

    // int a =0;
    // pthread_join(myPthread, (void **)&a);
    mythread->join();
    return 0.0;
}

basicAC_F0::name_and_type WEBSPLOT_Op::name_param[] =
    {
        // modify static const int n_name_param = ... in the above member
        {"cmm", &typeid(string *)},
        {"fetype", &typeid(string *)}
        //{  "logscale",  &typeid(bool)} // not implemented
};

basicAC_F0::name_and_type WEB3MPIPLOT_Op::name_param[] =
    {
        // modify static const int n_name_param = ... in the above member
        {"cmm", &typeid(string *)},
        {"fetype", &typeid(string *)}
        //{  "logscale",  &typeid(bool)} // not implemented
};

basicAC_F0::name_and_type WEB3PLOT_Op::name_param[] =
    {
        // modify static const int n_name_param = ... in the above member
        {"cmm", &typeid(string *)},
        {"fetype", &typeid(string *)}
        //{  "logscale",  &typeid(bool)} // not implemented
};


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
        {"port", &typeid(double)},
        {"basedir", &typeid(string *)}
        //{  "logscale",  &typeid(bool)} // not implemented
};
basicAC_F0::name_and_type SERVERSHOW_Op::name_param[] =
    {
        // modify static const int n_name_param = ... in the above member
        {"host", &typeid(string *)},
        {"port", &typeid(double)},
        {"basedir", &typeid(string *)}
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
    mesh_name << BASE_DIR << "/static/cache/mesh" << plotcount << ".json.gz";
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
    vertex_name << BASE_DIR << "/static/cache/vertex" << plotcount << ".json.gz";
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
    edge_name << BASE_DIR << "/static/cache/edge" << plotcount << ".json.gz";
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
        basic_name << BASE_DIR << "/static/cache/basic" << plotcount << ".json.gz";
        gzFile gz_basic_file;
        gz_basic_file = gzopen(basic_name.str().c_str(), "wb");
        std::stringstream basic_json;

        basic_json << std::setiosflags(std::ios::scientific) << std::setprecision(16);
        basic_json << "{" << endl;
        basic_json << " \"type\": \"Mesh\"," << endl;
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
    mesh_name << BASE_DIR << "/static/cache/mesh" << plotcount << "_" << mpi_rank << ".json.gz";
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
    vertex_name << BASE_DIR << "/static/cache/vertex" << plotcount << "_" << mpi_rank << ".json.gz";
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
    edge_name << BASE_DIR << "/static/cache/edge" << plotcount << "_" << mpi_rank << ".json.gz";
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
        basic_name << BASE_DIR << "/static/cache/basic" << plotcount << "_" << mpi_rank << ".json.gz";
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
        basic_name << BASE_DIR << "/static/cache/basic" << plotcount << ".json.gz";
        gzFile gz_basic_file;
        gz_basic_file = gzopen(basic_name.str().c_str(), "wb");
        std::stringstream basic_json;

        basic_json << std::setiosflags(std::ios::scientific) << std::setprecision(16);
        basic_json << "{" << endl;
        basic_json << " \"type\": \"Mesh\"," << endl;
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

AnyType WEB3PLOT_Op::operator()(Stack stack) const
{
    const std::string cmm = get_string(stack, nargs[0], DEFAULT_CMM);
    const std::string fetype = get_string(stack, nargs[1], DEFAULT_FETYPE);
    plotcount = plotcount+1;
    const Mesh3 *const pTh = GetAny<const Mesh3 *const>((*eTh)(stack));
    ffassert(pTh);
    const Mesh3 &Th = *pTh;
    const int nVertices = Th.nv;
    const int nTriangles = Th.nt;

    R3 Pmin, Pmax;
    Th.BoundingBox(Pmin, Pmax);

    const double &x0 = Pmin.x;
    const double &y0 = Pmin.y;
    const double &z0 = Pmin.z;

    const double &x1 = Pmax.x;
    const double &y1 = Pmax.y;
    const double &z1 = Pmax.z;


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
    mesh_name << BASE_DIR << "/static/cache/mesh" << plotcount << ".json.gz";
    gzFile gz_mesh_file;
    gz_mesh_file = gzopen(mesh_name.str().c_str(), "wb");
    std::stringstream mesh_json;
    mesh_json << "{ \"mesh\" :" << endl;
    mesh_json << std::setiosflags(std::ios::scientific) << std::setprecision(16) << "  [" << endl;
    for (int it = 0; it < Th.nt; it++)
    {
        for (int iv = 0; iv < 4; iv++)
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
            mesh_json << "{\"index\":" << i << ",\"x\":" << Th(i).x << ",\"y\":" << Th(i).y << ",\"z\":" << Th(i).z << ",\"u\":" << temp << "}";

            if (iv != 3)
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

    unsigned long int file_mesh_size = sizeof(char) * mesh_json.str().size();
    // gzwrite(gz_mesh_file, (void *)&file_mesh_size, sizeof(file_mesh_size));
    gzwrite(gz_mesh_file, (void *)(mesh_json.str().data()), file_mesh_size);
    gzclose(gz_mesh_file);

    std::ostringstream vertex_name;
    vertex_name << BASE_DIR << "/static/cache/vertex" << plotcount << ".json.gz";
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
        vertex_json << "    {\"x\":" << Th(i).x << ",\"y\":" << Th(i).y <<",\"z\":" << Th(i).z <<  "}";
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
    edge_name << BASE_DIR << "/static/cache/edge" << plotcount << ".json.gz";
    gzFile gz_edge_file;
    gz_edge_file = gzopen(edge_name.str().c_str(), "wb");
    std::stringstream edge_json;
    edge_json << "{ \"edge\" :" << endl;
    edge_json << std::setiosflags(std::ios::scientific) << std::setprecision(16) << "  [" << endl;
    for (int i = 0; i < Th.nbe; i++)
    {
        const Triangle3 &vi(Th.be(i));
        const int &v0 = Th(vi[0]);
        const int &v1 = Th(vi[1]);
        const int &v2 = Th(vi[2]);
        edge_json << "    { \"label\": " << Th.be(i) << "," << endl;
        edge_json << "      \"vertices\":[{\"x\":" << Th(v0).x << ",\"y\":" << Th(v0).y <<",\"z\":" << Th(v0).z <<  "}," << endl;
        edge_json << "                  {\"x\":" << Th(v1).x << ",\"y\":" << Th(v1).y <<",\"z\":" << Th(v1).z <<  "}," << endl;
        edge_json << "                  {\"x\":" << Th(v2).x << ",\"y\":" << Th(v2).y <<",\"z\":" << Th(v2).z <<  "}]" << endl;
        edge_json << "    }";
        if (i != Th.nbe - 1)
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
        basic_name << BASE_DIR << "/static/cache/basic" << plotcount << ".json.gz";
        gzFile gz_basic_file;
        gz_basic_file = gzopen(basic_name.str().c_str(), "wb");
        std::stringstream basic_json;

        basic_json << std::setiosflags(std::ios::scientific) << std::setprecision(16);
        basic_json << "{" << endl;
        basic_json << " \"type\": \"Mesh3\"," << endl;
        basic_json << " \"bounds\":[[" << x0 << "," << y0<< "," << z0 << "]," << endl;
        basic_json << "           [" << x1 << "," << y1<< "," << z1 << "]]" << endl;
        basic_json << "}" << endl;
        unsigned long int file_basic_size = sizeof(char) * basic_json.str().size();
        // gzwrite(gz_basic_file, (void *)&file_basic_size, sizeof(file_basic_size));
        gzwrite(gz_basic_file, (void *)(basic_json.str().data()), file_basic_size);
        gzclose(gz_basic_file);
    }


    return true;
}

AnyType WEB3MPIPLOT_Op::operator()(Stack stack) const
{
    int mpi_rank = GetAny<long>((*empirank)(stack));
    int mpi_size = GetAny<long>((*empisize)(stack));
    if (mpi_rank == 1){
        plotcount = plotcount+1;
    }

    const std::string cmm = get_string(stack, nargs[0], DEFAULT_CMM);
    const std::string fetype = get_string(stack, nargs[1], DEFAULT_FETYPE);
    const Mesh3 *const pTh = GetAny<const Mesh3 *const>((*eTh)(stack));
    ffassert(pTh);
    const Mesh3 &Th = *pTh;
    const int nVertices = Th.nv;
    const int nTriangles = Th.nt;

    R3 Pmin, Pmax;
    Th.BoundingBox(Pmin, Pmax);

    const double &x0 = Pmin.x;
    const double &y0 = Pmin.y;
    const double &z0 = Pmin.z;

    const double &x1 = Pmax.x;
    const double &y1 = Pmax.y;
    const double &z1 = Pmax.z;


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
    mesh_name << BASE_DIR << "/static/cache/mesh" << plotcount << "_" << mpi_rank << ".json.gz";
    gzFile gz_mesh_file;
    gz_mesh_file = gzopen(mesh_name.str().c_str(), "wb");
    std::stringstream mesh_json;
    mesh_json << "{ \"mesh\" :" << endl;
    mesh_json << std::setiosflags(std::ios::scientific) << std::setprecision(16) << "  [" << endl;
    for (int it = 0; it < Th.nt; it++)
    {
        for (int iv = 0; iv < 4; iv++)
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
            mesh_json << "{\"index\":" << i << ",\"x\":" << Th(i).x << ",\"y\":" << Th(i).y << ",\"z\":" << Th(i).z << ",\"u\":" << temp << "}";


            if (iv != 3)
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
    vertex_name << BASE_DIR << "/static/cache/vertex" << plotcount << "_" << mpi_rank << ".json.gz";
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
        vertex_json << "    {\"x\":" << Th(i).x << ",\"y\":" << Th(i).y <<",\"z\":" << Th(i).z <<  "}";
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
    edge_name << BASE_DIR << "/static/cache/edge" << plotcount << "_" << mpi_rank << ".json.gz";
    gzFile gz_edge_file;
    gz_edge_file = gzopen(edge_name.str().c_str(), "wb");
    std::stringstream edge_json;
    edge_json << "{ \"edge\" :" << endl;
    edge_json << std::setiosflags(std::ios::scientific) << std::setprecision(16) << "  [" << endl;
    for (int i = 0; i < Th.nbe; i++)
    {
        const Triangle3 &vi(Th.be(i));
        const int &v0 = Th(vi[0]);
        const int &v1 = Th(vi[1]);
        const int &v2 = Th(vi[2]);
        edge_json << "    { \"label\": " << Th.be(i) << "," << endl;
        edge_json << "      \"vertices\":[{\"x\":" << Th(v0).x << ",\"y\":" << Th(v0).y <<",\"z\":" << Th(v0).z <<  "}," << endl;
        edge_json << "                  {\"x\":" << Th(v1).x << ",\"y\":" << Th(v1).y <<",\"z\":" << Th(v1).z <<  "}," << endl;
        edge_json << "                  {\"x\":" << Th(v2).x << ",\"y\":" << Th(v2).y <<",\"z\":" << Th(v2).z <<  "}]" << endl;
        edge_json << "    }";
        if (i != Th.nbe - 1)
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
        basic_name << BASE_DIR << "/static/cache/basic" << plotcount << "_" << mpi_rank << ".json.gz";
        gzFile gz_basic_file;
        gz_basic_file = gzopen(basic_name.str().c_str(), "wb");
        std::stringstream basic_json;

        basic_json << std::setiosflags(std::ios::scientific) << std::setprecision(16);
        basic_json << "{" << endl;
        basic_json << " \"type\": \"Mesh3\"," << endl;
        basic_json << " \"rank\": "<< mpi_rank <<"," << endl;
        basic_json << " \"bounds\":[[" << x0 << "," << y0<< "," << z0 << "]," << endl;
        basic_json << "           [" << x1 << "," << y1<< "," << z1 << "]]" << endl;
        basic_json << "}" << endl;
        unsigned long int file_basic_size = sizeof(char) * basic_json.str().size();
        // gzwrite(gz_basic_file, (void *)&file_basic_size, sizeof(file_basic_size));
        gzwrite(gz_basic_file, (void *)(basic_json.str().data()), file_basic_size);
        gzclose(gz_basic_file);
    }

    if (mpi_rank == 1)
    {
        std::ostringstream basic_name;
        basic_name << BASE_DIR << "/static/cache/basic" << plotcount << ".json.gz";
        gzFile gz_basic_file;
        gz_basic_file = gzopen(basic_name.str().c_str(), "wb");
        std::stringstream basic_json;

        basic_json << std::setiosflags(std::ios::scientific) << std::setprecision(16);
        basic_json << "{" << endl;
        basic_json << " \"type\": \"Mesh3\"," << endl;
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

AnyType WEBSPLOT_Op::operator()(Stack stack) const
{
    const std::string cmm = get_string(stack, nargs[0], DEFAULT_CMM);
    const std::string fetype = get_string(stack, nargs[1], DEFAULT_FETYPE);
    plotcount = plotcount+1;
    const MeshS *const pTh = GetAny<const MeshS *const>((*eTh)(stack));
    ffassert(pTh);
    const MeshS &Th = *pTh;
    const int nVertices = Th.nv;
    const int nTriangles = Th.nt;

    R3 Pmin, Pmax;
    Th.BoundingBox(Pmin, Pmax);

    const double &x0 = Pmin.x;
    const double &y0 = Pmin.y;
    const double &z0 = Pmin.z;

    const double &x1 = Pmax.x;
    const double &y1 = Pmax.y;
    const double &z1 = Pmax.z;


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
    // std::cout << Th.nt << std::endl;

    std::ostringstream mesh_name;
    mesh_name << BASE_DIR << "/static/cache/mesh" << plotcount << ".json.gz";
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
            mesh_json << "{\"index\":" << i << ",\"x\":" << Th(i).x << ",\"y\":" << Th(i).y << ",\"z\":" << Th(i).z << ",\"u\":" << temp << "}";

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

    unsigned long int file_mesh_size = sizeof(char) * mesh_json.str().size();
    // gzwrite(gz_mesh_file, (void *)&file_mesh_size, sizeof(file_mesh_size));
    gzwrite(gz_mesh_file, (void *)(mesh_json.str().data()), file_mesh_size);
    gzclose(gz_mesh_file);

    std::ostringstream vertex_name;
    vertex_name << BASE_DIR << "/static/cache/vertex" << plotcount << ".json.gz";
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
        vertex_json << "    {\"x\":" << Th(i).x << ",\"y\":" << Th(i).y <<",\"z\":" << Th(i).z <<  "}";
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
    edge_name << BASE_DIR << "/static/cache/edge" << plotcount << ".json.gz";
    gzFile gz_edge_file;
    gz_edge_file = gzopen(edge_name.str().c_str(), "wb");
    std::stringstream edge_json;
    edge_json << "{ \"edge\" :" << endl;
    edge_json << std::setiosflags(std::ios::scientific) << std::setprecision(16) << "  [" << endl;
    for (int i = 0; i < Th.nbe; i++)
    {
        const BoundaryEdgeS &vi(Th.be(i));
        const int &v0 = Th(vi[0]);
        const int &v1 = Th(vi[1]);
        edge_json << "    { \"label\": " << Th.be(i) << "," << endl;
        edge_json << "      \"vertices\":[{\"x\":" << Th(v0).x << ",\"y\":" << Th(v0).y <<",\"z\":" << Th(v0).z <<  "}," << endl;
        edge_json << "                  {\"x\":" << Th(v1).x << ",\"y\":" << Th(v1).y <<",\"z\":" << Th(v1).z <<  "}]" << endl;
        edge_json << "    }";
        if (i != Th.nbe - 1)
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
        basic_name << BASE_DIR << "/static/cache/basic" << plotcount << ".json.gz";
        gzFile gz_basic_file;
        gz_basic_file = gzopen(basic_name.str().c_str(), "wb");
        std::stringstream basic_json;

        basic_json << std::setiosflags(std::ios::scientific) << std::setprecision(16);
        basic_json << "{" << endl;
        basic_json << " \"type\": \"MeshS\"," << endl;
        basic_json << " \"bounds\":[[" << x0 << "," << y0<< "," << z0 << "]," << endl;
        basic_json << "           [" << x1 << "," << y1<< "," << z1 << "]]" << endl;
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
    WEBPLOT(int,int) : OneOperator(atype<long>(),   atype<const Mesh3 *>()), argc(3) {}
    WEBPLOT(int,int,int) : OneOperator(atype<long>(), atype<double>(),  atype<const Mesh3 *>()), argc(4) {}
    WEBPLOT(int,int,int,int) : OneOperator(atype<long>(),   atype<const MeshS *>()), argc(5) {}
    WEBPLOT(int,int,int,int,int) : OneOperator(atype<long>(), atype<double>(),  atype<const MeshS *>()), argc(6) {}



    E_F0 *code(const basicAC_F0 &args) const
    {
        if (argc == 1)
            return new WEBPLOT_Op(args, t[0]->CastTo(args[0]));
        else if (argc == 2)
            return new WEBPLOT_Op(args, t[0]->CastTo(args[0]), t[1]->CastTo(args[1]));
        else if (argc == 3)
            return new WEB3PLOT_Op(args, t[0]->CastTo(args[0]));
        else if (argc == 4)
            return new WEB3PLOT_Op(args, t[0]->CastTo(args[0]), t[1]->CastTo(args[1]));
        else if (argc == 5)
            return new WEBSPLOT_Op(args, t[0]->CastTo(args[0]));
        else if (argc == 6)
            return new WEBSPLOT_Op(args, t[0]->CastTo(args[0]), t[1]->CastTo(args[1]));
        else
            ffassert(0);
    }
};

class WEBMPIPLOT : public OneOperator
{
    const int argc;

  public:
    WEBMPIPLOT()     : OneOperator(atype<long>(),                  atype<const Mesh *>(), atype<long>(), atype<long>()), argc(1) {}
    WEBMPIPLOT(int) : OneOperator(atype<long>(), atype<double>(), atype<const Mesh *>(), atype<long>(), atype<long>()), argc(2) {}
    WEBMPIPLOT(int,int)     : OneOperator(atype<long>(),                  atype<const Mesh3 *>(), atype<long>(), atype<long>()), argc(3) {}
    WEBMPIPLOT(int,int,int) : OneOperator(atype<long>(), atype<double>(), atype<const Mesh3 *>(), atype<long>(), atype<long>()), argc(4) {}

    E_F0 *code(const basicAC_F0 &args) const
    {
        if (argc == 1)
            return new WEBMPIPLOT_Op(args, t[0]->CastTo(args[0]), t[1]->CastTo(args[1]), t[2]->CastTo(args[2]));
        else if (argc == 2)
            return new WEBMPIPLOT_Op(args, t[0]->CastTo(args[0]), t[1]->CastTo(args[1]), t[2]->CastTo(args[2]), t[3]->CastTo(args[3]));
        else if (argc == 3)
            return new WEB3MPIPLOT_Op(args, t[0]->CastTo(args[0]), t[1]->CastTo(args[1]), t[2]->CastTo(args[2]));
        else if (argc == 4)
            return new WEB3MPIPLOT_Op(args, t[0]->CastTo(args[0]), t[1]->CastTo(args[1]), t[2]->CastTo(args[2]), t[3]->CastTo(args[3]));
        else
            ffassert(0);
    }
};

class SERVER : public OneOperator
{
    const int argc;

  public:
    SERVER()    : OneOperator(atype<double>()), argc(0) {}
    SERVER(int)    : OneOperator(atype<double>()), argc(1) {}

    E_F0 *code(const basicAC_F0 &args) const
    {
        if (argc == 0)
            return new SERVER_Op(args);
        else if (argc == 1)
            return new SERVERSHOW_Op(args);
        else
            ffassert(0);
    }
};


static void init(){
    Global.Add("server", "(", new SERVER());
    Global.Add("show", "(", new SERVER(0));
    Global.Add("webplot", "(", new WEBPLOT());
    Global.Add("webplot", "(", new WEBPLOT(0));
    Global.Add("webplotMPI", "(", new WEBMPIPLOT());
    Global.Add("webplotMPI", "(", new WEBMPIPLOT(0));
    Global.Add("webplot", "(", new WEBPLOT(0,0));
    Global.Add("webplot", "(", new WEBPLOT(0,0,0));
    Global.Add("webplotMPI", "(", new WEBMPIPLOT(0,0));
    Global.Add("webplotMPI", "(", new WEBMPIPLOT(0,0,0));
    Global.Add("webplot", "(", new WEBPLOT(0,0,0,0));
    Global.Add("webplot", "(", new WEBPLOT(0,0,0,0,0));

}

LOADFUNC(init);
