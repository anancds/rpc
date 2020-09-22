#include <gflags/gflags.h>
#include <butil/logging.h>
#include <brpc/server.h>
#include <brpc/restful.h>
#include "build/echo.pb.h"

using helloworld::Greeter;
using helloworld::HelloReply;
using helloworld::HelloRequest;

DEFINE_int32(port, 50051, "TCP Port of this server");
DEFINE_int32(idle_timeout_s, -1, "Connection will be closed if there is no "
                                 "read/write operations during the last `idle_timeout_s'");
DEFINE_int32(logoff_ms, 2000, "Maximum duration of server's LOGOFF state "
                              "(waiting for client to close connection before server stops)");
DEFINE_bool(gzip, false, "compress body using gzip");

class GreeterImpl : public helloworld::Greeter {
 public:
  GreeterImpl() {};
  virtual ~GreeterImpl() {};
  void SayHello(google::protobuf::RpcController* cntl_base,
                const helloworld::HelloRequest* req,
                helloworld::HelloReply* res,
                google::protobuf::Closure* done) {
    brpc::ClosureGuard done_guard(done);
    brpc::Controller* cntl = static_cast<brpc::Controller*>(cntl_base);
    if (FLAGS_gzip) {
      cntl->set_response_compress_type(brpc::COMPRESS_TYPE_GZIP);
    }
    res->set_message("Hello " + req->name());
  }
};

int main(int argc, char* argv[]) {
  // Parse gflags. We recommend you to use gflags as well.
  GFLAGS_NS::ParseCommandLineFlags(&argc, &argv, true);

  // Generally you only need one Server.
  brpc::Server server;

  GreeterImpl http_svc;

  // Add services into server. Notice the second parameter, because the
  // service is put on stack, we don't want server to delete it, otherwise
  // use brpc::SERVER_OWNS_SERVICE.
  if (server.AddService(&http_svc,
                        brpc::SERVER_DOESNT_OWN_SERVICE) != 0) {
    LOG(ERROR) << "Fail to add http_svc";
    return -1;
  }

  // Start the server.
  brpc::ServerOptions options;
  options.idle_timeout_sec = FLAGS_idle_timeout_s;
  if (server.Start(FLAGS_port, &options) != 0) {
    LOG(ERROR) << "Fail to start HttpServer";
    return -1;
  }

  // Wait until Ctrl-C is pressed, then Stop() and Join() the server.
  server.RunUntilAskedToQuit();
  return 0;
}