#ifndef SYC_UTILS_H_
#define SYC_UTILS_H_

#include "common.h"

namespace syc {

/// Indent each line of `text` by adding `indent` to the start.
std::string indent_str(const std::string &text, const std::string &indent);

}

#endif 