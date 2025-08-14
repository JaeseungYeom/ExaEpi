/*! @file ContamClient.cpp
    \brief Implementation of RPC client for Contam simulation service 
*/

#include <grpcpp/grpcpp.h>

#include <iostream>
#include <fstream>
#include <sstream>
#include <memory>
#include <string>
#include <filesystem>
#include <thread>
#ifdef __linux__
#include <sys/syscall.h>
#endif

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
ABSL_FLAG(std::string, ctm_of_prefix, "", "Prefix to contam output filenames");
ABSL_FLAG(bool, ctm_ach, false, "Retrieve .ach output file from Contam server");
ABSL_FLAG(bool, ctm_cex, false, "Retrieve .cex output file from Contam server");
ABSL_FLAG(bool, ctm_csm, false, "Retrieve .csm output file from Contam server");
ABSL_FLAG(bool, ctm_log, false, "Retrieve .log output file from Contam server");
ABSL_FLAG(bool, ctm_rst, false, "Retrieve .rst output file from Contam server");
ABSL_FLAG(bool, ctm_sim, false, "Retrieve .sim output file from Contam server");
ABSL_FLAG(bool, ctm_xlog, false, "Retrieve .xlog output file from Contam server");

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

std::vector<char> loadFile(const std::string& fname)
{
  if (!checkFilename(fname)) {
    std::cerr << fname + " is not valid a filename!\n";
    return std::vector<char>{};
  }

  auto fsize = std::filesystem::file_size(fname);
  std::vector<char> content(fsize, 0u);
  std::ifstream ifs(fname, std::ios_base::in | std::ios_base::binary);
  if (!ifs) {
    std::cerr << "Cannot open " + fname + " for reading!\n";
    return std::vector<char>{};
  }
  ifs.read(&content[0], fsize);
  ifs.close();
  return content;
}

/*! Write a binary file */
size_t writeFile(
  const std::string& fname,
  const std::string& content,
  std::ios_base::openmode mode = std::ios_base::out)
{
  if (!checkFilename(fname)) {
    std::cerr << "Invalid filename: " + fname << std::endl;
    return static_cast<size_t>(0ul);
  }
  std::ofstream ofs(fname, mode | std::ios::binary);
  if (!ofs) {
    std::cerr << "Failed to open " << fname << " for writing." << std::endl;
    return static_cast<size_t>(0ul);
  }
  ofs.write(content.data(), content.size());
  ofs.close();
  return content.size();
}

struct ContamState {
  int rank;
  bool of_ach;
  bool of_cex;
  bool of_csm;
  bool of_log;
  bool of_rst;
  bool of_sim;
  bool of_xlog;
  std::string uniq_str;
  std::string of_prefix;
  std::string stdout_file;
  std::string stderr_file;

  ContamState()
  : rank(-1), of_ach(false), of_cex(false), of_csm(false),
    of_log(false), of_rst(false), of_sim(false), of_xlog(false) {}

  void Clear() {
    rank = -1;
    uniq_str.clear();
    of_prefix.clear();
    stdout_file.clear();
    stderr_file.clear();
  }

  void InitState(int r) {
    rank = r;
    of_prefix = absl::GetFlag(FLAGS_ctm_of_prefix);
    const std::string rank_str = std::to_string(r);

#ifdef __linux__
    const std::string tid_str = std::to_string(syscall(SYS_gettid));
#else
    std::ostringstream ss;
    ss << std::this_thread::get_id();
    const std::string tid_str = ss.str();
#endif

    uniq_str = "." + rank_str + "-" + tid_str;

    stdout_file = absl::GetFlag(FLAGS_ctm_stdout_file);
    stdout_file = checkFilename(stdout_file)?
                  of_prefix + addStrToFilenameBeforeExt(stdout_file, uniq_str) : "";

    stderr_file = absl::GetFlag(FLAGS_ctm_stderr_file);
    stderr_file = checkFilename(stderr_file)?
                  of_prefix + addStrToFilenameBeforeExt(stderr_file, uniq_str) : "";

    of_ach = absl::GetFlag(FLAGS_ctm_ach);
    of_cex = absl::GetFlag(FLAGS_ctm_cex);
    of_csm = absl::GetFlag(FLAGS_ctm_csm);
    of_log = absl::GetFlag(FLAGS_ctm_log);
    of_rst = absl::GetFlag(FLAGS_ctm_rst);
    of_sim = absl::GetFlag(FLAGS_ctm_sim);
    of_xlog = absl::GetFlag(FLAGS_ctm_xlog);
  }

  std::string to_string()
  {
    std::string str;
    str = " - rank: " + std::to_string(rank) + "\n"
        + " - of_ach: " + std::string(of_ach ? "true\n" : "false\n")
        + " - of_cex: " + std::string(of_cex ? "true\n" : "false\n")
        + " - of_csm: " + std::string(of_csm ? "true\n" : "false\n")
        + " - of_log: " + std::string(of_log ? "true\n" : "false\n")
        + " - of_rst: " + std::string(of_rst ? "true\n" : "false\n")
        + " - of_sim: " + std::string(of_sim ? "true\n" : "false\n")
        + " - of_xlog: " + std::string(of_xlog? "true\n" : "false\n")
        + " - uniq_str: " + uniq_str + "\n"
        + " - of_prefix: " + of_prefix + "\n"
        + " - stdout_file: " + stdout_file + "\n"
        + " - stderr_file: " + stderr_file + "\n";
    return str;
  }
};

static thread_local ContamState ctmState;
static void ContaStateInit () __attribute__ ((constructor));
static void ContaStateFini () __attribute__ ((destructor));

void ContaStateInit () {
  ctmState.Clear();
}

void ContaStateFini () {
  ctmState.Clear();
}

void pickContamArgs(int argc, char** argv)
{
  absl::ParseCommandLine(argc, argv);
}

void initContamClientState(int rank)
{
  ctmState.InitState(rank);
}


class ContamServerClient {
 public:
  ContamServerClient(std::shared_ptr<Channel> channel)
      : stub_(ContamServer::NewStub(channel)) {}

  /*! \brief Sends the client's request and presents the response received from the server.
   */
  ContamResponse RunContam(const std::vector<char>& prj) {
    // Message to send to the server.
    ContamRequest request;
    request.set_prj(prj.data(), prj.size());
    request.set_ach(ctmState.of_ach);
    request.set_cex(ctmState.of_cex);
    request.set_csm(ctmState.of_csm);
    request.set_log(ctmState.of_log);
    request.set_rst(ctmState.of_rst);
    request.set_sim(ctmState.of_sim);
    request.set_xlog(ctmState.of_xlog);
    //std::cout << ctmState.to_string() << std::endl;

    // Reponse received from the server
    ContamReply reply;

    // Context for the client. It could be used to convey extra information to
    // the server and/or tweak certain RPC behaviors.
    ClientContext context;

    // The actual RPC
    Status status = stub_->RunContam(&context, request, &reply);


    const auto& stdout_file = ctmState.stdout_file;
    const auto& stderr_file = ctmState.stderr_file;

    if (stdout_file.empty()) {
      std::cout << reply.stdout() << std::endl;
    } else {
      writeFile(stdout_file, reply.stdout(), std::ios_base::app);
    }

    if (stderr_file.empty()) {
      std::cerr << reply.stderr() << std::endl;
    } else {
      writeFile(stderr_file, reply.stderr(), std::ios_base::app);
    }

    const auto uniq_str = ctmState.of_prefix + "ctm" + ctmState.uniq_str;

    if (reply.ach().size() != 0ul) {
      writeFile(uniq_str + ".ach", reply.ach(), std::ios_base::app);
    }

    if (reply.cex().size() != 0ul) {
      writeFile(uniq_str + ".cex", reply.cex(), std::ios_base::app);
    }

    if (reply.csm().size() != 0ul) {
      writeFile(uniq_str + ".csm", reply.csm(), std::ios_base::app);
    }

    if (reply.log().size() != 0ul) {
      writeFile(uniq_str + ".log", reply.log(), std::ios_base::app);
    }

    if (reply.rst().size() != 0ul) {
      writeFile(uniq_str + ".rst", reply.rst(), std::ios_base::app);
    }

    if (reply.sim().size() != 0ul) {
      writeFile(uniq_str + ".sim", reply.sim(), std::ios_base::app);
    }

    if (reply.xlog().size() != 0ul) {
      writeFile(uniq_str + ".xlog", reply.xlog(), std::ios_base::app);
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

ContamResponse contamClient() {
  std::string target_str = absl::GetFlag(FLAGS_ctm_target);

  // Instantiate the client. RPCs are created out of a channel, that models
  // a connection to an endpoint.
  // The channel here isn't authenticated (thus InsecureChannelCredentials())
  ContamServerClient contam_connector(
      grpc::CreateChannel(target_str, grpc::InsecureChannelCredentials()));

  //std::string prj_file = loadContamProject();
  const auto prj_file = loadFile(absl::GetFlag(FLAGS_ctm_prj_file));
  const auto reply = contam_connector.RunContam(prj_file);

  std::cout << "Contam RPC reply status: " << reply.status << std::endl;

  return reply;
}

} // end of namespace
