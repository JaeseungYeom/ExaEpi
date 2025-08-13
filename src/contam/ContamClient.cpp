/*! @file ContamClient.cpp
    \brief Implementation of RPC client for Contam simulation service 
*/

#include <grpcpp/grpcpp.h>

#include <iostream>
#include <fstream>
#include <memory>
#include <string>
#include <filesystem>
#include <thread>
#include <sstream>

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"

#ifdef BAZEL_BUILD
#include "examples/protos/contam.grpc.pb.h"
#else
#include "contam.grpc.pb.h"
#endif

#include "ContamClient.H"

ABSL_FLAG(std::string, ctm_target, "localhost:50051", "Server address");
ABSL_FLAG(std::string, ctm_prj_file, "", "Contam project filename");
ABSL_FLAG(std::string, ctm_stdout_file, "", "Contam stdout filename");
ABSL_FLAG(std::string, ctm_stderr_file, "", "Contam stderr filename");

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

/*! \brief Add a string to a filename before extension */
std::string addStrToFilenameBeforeExt(const std::string& filename, const std::string& str) {
  std::filesystem::path p(filename);

  // Get the filename without the extension
  std::string fnameWOExt = p.stem().string();

  // Get the extension (including the dot)
  std::string ext = p.extension().string();

  // Construct the new filename
  std::string newFilename = fnameWOExt + str + ext;

  // Create a new path with the modified filename
  std::filesystem::path newPath = p.parent_path() / newFilename;

  return newPath.string();
}

std::string loadContamProject()
{
  std::string prj_fname = absl::GetFlag(FLAGS_ctm_prj_file);
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

size_t writeFile(
  const std::string& fname,
  const std::string& content,
  std::ios_base::openmode mode = std::ios_base::out)
{
  if (!checkFilename(fname)) {
    std::cerr << "Invalid filename: " + fname << std::endl;
    return static_cast<size_t>(0ul);
  }
  std::ofstream ofs(fname, mode);
  ofs << content;
  ofs.close();
  return content.size();
}


struct ContamState {
  int rank;
  std::string stdout_file;
  std::string stderr_file;

  ContamState() : rank(-1) {}

  void SetState(int r) {
    rank = r;
    const std::string rank_str = std::to_string(r);

    stdout_file = absl::GetFlag(FLAGS_ctm_stdout_file);
    stdout_file = checkFilename(stdout_file)?
                  addStrToFilenameBeforeExt(stdout_file, rank_str) : "";

    stderr_file = absl::GetFlag(FLAGS_ctm_stderr_file);
    stderr_file = checkFilename(stderr_file)?
                  addStrToFilenameBeforeExt(stderr_file, rank_str) : "";
  }
};

static thread_local ContamState ctmState;
static void ContaStateInit () __attribute__ ((constructor));
static void ContaStateFini () __attribute__ ((destructor));

void ContaStateInit () {
  ctmState.rank = -1;
  ctmState.stdout_file.clear();
  ctmState.stderr_file.clear();
}

void ContaStateFini () {
  ctmState.rank = -1;
  ctmState.stdout_file.clear();
  ctmState.stderr_file.clear();
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


    std::ostringstream ss;
    ss << std::this_thread::get_id();
    const std::string tid_str = ss.str();

    if (Contam::ctmState.stdout_file.empty()) {
      std::cout << reply.stdout() << std::endl;
    } else {
      std::string stdout_file
        = addStrToFilenameBeforeExt(stdout_file, tid_str);
      writeFile(stdout_file, reply.stdout(), std::ios_base::app);
    }

    if (Contam::ctmState.stderr_file.empty()) {
      std::cerr << reply.stderr() << std::endl;
    } else {
      std::string stderr_file
        = addStrToFilenameBeforeExt(stderr_file, tid_str);
      writeFile(stderr_file, reply.stderr(), std::ios_base::app);
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

void pickContamArgs(int argc, char** argv, int rank)
{
  absl::ParseCommandLine(argc, argv);
  Contam::ctmState.SetState(rank);
}

ContamResponse contamClient() {
  std::string target_str = absl::GetFlag(FLAGS_ctm_target);

  // Instantiate the client. RPCs are created out of a channel, that models
  // a connection to an endpoint.
  // The channel here isn't authenticated (thus InsecureChannelCredentials())
  ContamServerClient contam_connector(
      grpc::CreateChannel(target_str, grpc::InsecureChannelCredentials()));

  std::string prj_file = loadContamProject();
  const auto reply = contam_connector.RunContam(prj_file);

  std::cout << "Contam run successful: " << reply.status << std::endl;

  return reply;
}

} // end of namespace
