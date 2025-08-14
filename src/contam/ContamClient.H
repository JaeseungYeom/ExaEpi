/*! @file ContamClient.H
 * \brief Contains the APIs to run Contam client
 */

#ifndef _CONTAM_CLIENT_H_
#define _CONTAM_CLIENT_H_

namespace Contam {

/*! \brief Contam simulation result returned */
struct ContamResponse {
  bool status;
  std::string err_str;
  ContamResponse() : status(false) {}
  ContamResponse(bool st, const std::string& str) : status(st), err_str(str) {}
};

/*! \brief Parse and consume Contam related arguments
 *
 * --ctm_target=<server_addrs:port>
 *  : address of the server to establish a connection with for RPC.
 *    The default is "localhost:50051".
 * --ctm_prj_file=<filename>
 *  : Contam project filename. The file will be read from the client and sent
 *    to the server as a payload of an RPC.
 * --ctm_stdout_file=<filename>
 *  : The stdout from the Contam simulation on server. It will be stored into
 *    a file on a client side. The MPI rank id will be added to the filename
 *    string for uniqueness.
 * --ctm_stderr_file=<filename>
 *  : The stderr from the Contam simulation on server. It will be stored into
 *    a file on a client side. The MPI rank id will be added to the filename
 *    string for uniqueness.
 * --ctm_of_prefix=<string>
 *  : The prefix string that will be added to output files from Contam.
 *    This could be a directory, in which case it should ends with '/' and
 *    the directory should exists.
 * --ctm_[ach|cex|csm|log|rst|sim|xlog]=[false|true]
 *  : Retrieve the output file with specific extension from Contam server.
 *    The default value is false.
 */
void pickContamArgs(int argc, char** argv);

/*! \brief Set MPI rank \p r to label per-rank output filename unique.
 * */
void initContamClientState(int r);

/*! \brief Run Comtam client */
ContamResponse contamClient();

}
#endif
