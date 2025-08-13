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

/*! \brief Parse and consume Contam related arguments */
void pickContamArgs(int argc, char** argv);

/*! \brief Run Comtam client */
ContamResponse contamClient();

}
#endif
