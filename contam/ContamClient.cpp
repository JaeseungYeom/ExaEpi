/*! @file ContamClient.cpp
    \brief Implementation of RPC client for Contam simulation service 
*/

#include <grpcpp/grpcpp.h>

#include <iostream>
#include <fstream>
#include <memory>
#include <string>
#include <filesystem>

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"

#ifdef BAZEL_BUILD
#include "examples/protos/contam.grpc.pb.h"
#else
#include "contam.grpc.pb.h"
#endif

#include "ContamClient.H"

ABSL_FLAG(std::string, target, "localhost:50051", "Server address");
ABSL_FLAG(std::string, ctm_prj_filename, "", "Contam project filename");
ABSL_FLAG(std::string, ctm_stdout_filename, "", "Contam stdout filename");
ABSL_FLAG(std::string, ctm_stderr_filename, "", "Contam stderr filename");

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using contam::ContamServer;
using contam::ContamReply;
using contam::ContamRequest;


namespace Contam {

bool checkFilename(const std::string& filename)
{
  std::filesystem::path p(filename);
  if (p.filename().empty() || p.filename() == "." || p.filename() == "..") {
    return false;
  }
  return true;
}

std::string loadContamProject()
{
  std::string prj_fname = absl::GetFlag(FLAGS_ctm_prj_filename);
  if (!checkFilename(prj_fname)) {
    return "";
  }

  auto fsize = std::filesystem::file_size(prj_fname);
  std::string prj_file(fsize, '\0');
  std::ifstream ifs(prj_fname);
  ifs.read(&prj_file[0], fsize);
  ifs.close();
  return prj_file;
}

size_t writeFile(const std::string& fname, const std::string& content)
{
  if (!checkFilename(fname)) {
    std::cerr << "Invalid filename: " + fname << std::endl;
    return static_cast<size_t>(0ul);
  }
  std::ofstream ofs(fname);
  ofs << content;
  ofs.close();
  return content.size();
}

class ContamServerClient {
 public:
  ContamServerClient(std::shared_ptr<Channel> channel)
      : stub_(ContamServer::NewStub(channel)) {}

  /*! \brief Sends the client's request and presents the response received from the server.
   */
  ContamResponse RunContam(const std::string& prj) {
    // Message to send to the server.
    ContamRequest request;
    request.set_prj(prj);

    // Reponse received from the server
    ContamReply reply;

    // Context for the client. It could be used to convey extra information to
    // the server and/or tweak certain RPC behaviors.
    ClientContext context;

    // The actual RPC
    Status status = stub_->RunContam(&context, request, &reply);

    const std::string stdout_fname = absl::GetFlag(FLAGS_ctm_stdout_filename);
    const std::string stderr_fname = absl::GetFlag(FLAGS_ctm_stderr_filename);

    if (stdout_fname.empty()) {
      std::cout << reply.stdout();
    } else {
      writeFile(stdout_fname, reply.stdout());
    }
    if (stderr_fname.empty()) {
      std::cerr << reply.stderr();
    } else {
      writeFile(stderr_fname, reply.stderr());
    }

    ContamResponse response;
    if (status.ok()) {
      response.status = true;
    } else {
      response.status = false;
      response.err_str = std::to_string(status.error_code()) + ": " + status.error_message();
    }
    return response;
  }

 private:
  std::unique_ptr<ContamServer::Stub> stub_;
};

void pickContamArgs(int argc, char** argv)
{
  // Instantiate the client. It requires a channel, out of which the actual RPCs
  // are created. This channel models a connection to an endpoint specified by
  // the argument "--target=" which is the only expected argument.
  absl::ParseCommandLine(argc, argv);
}

ContamResponse contamClient() {
  std::string target_str = absl::GetFlag(FLAGS_target);

  // We indicate that the channel isn't authenticated (use of
  // InsecureChannelCredentials()).
  ContamServerClient contam_connector(
      grpc::CreateChannel(target_str, grpc::InsecureChannelCredentials()));

  std::string prj_file = loadContamProject();
  const auto reply = contam_connector.RunContam(prj_file);

  std::cout << "Contam run successful: " << reply.status << std::endl;

  return reply;
}

} // end of namespace
