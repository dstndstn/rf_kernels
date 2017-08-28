#include <sstream>
#include "rf_kernels/core.hpp"

using namespace std;

namespace rf_kernels {
#if 0
};  // pacify emacs c-mode!
#endif


ostream &operator<<(ostream &os, axis_type axis)
{
    os << axis_type_to_string(axis);
    return os;
}


string axis_type_to_string(axis_type axis)
{
    if (axis == AXIS_FREQ)
	return "AXIS_FREQ";
    else if (axis == AXIS_TIME)
	return "AXIS_TIME";
    else if (axis == AXIS_NONE)
	return "AXIS_NONE";

    throw runtime_error("rf_kernels: internal error: bad argument to axis_type_to_string()");
}


}  // namespace rf_kernels
