#include <unordered_map>

#include "rf_kernels/core.hpp"
#include "rf_kernels/internals.hpp"
#include "rf_kernels/upsample.hpp"
#include "rf_kernels/upsample_internals.hpp"

using namespace std;

namespace rf_kernels {
#if 0
}; // pacify emacs c-mode
#endif


// -------------------------------------------------------------------------------------------------


// Inner namespace for the kernel table
// Must have a different name for each kernel, otherwise gcc is happy but clang has trouble!
namespace upsampling_kernel_table {
#if 0
}; // pacify emacs c-mode
#endif


// Usage: kernel(wp, nfreq_in, nt_in, dst, dstride, src, sstride, w_cutoff)
using kernel_t = void (*)(const weight_upsampler *, int, int, float *, int, const float *, int, float);

static unordered_map<array<int,2>, kernel_t> kernel_table;   // (Df,Dt) -> kernel


inline void _no_kernel(int Df, int Dt)
{
    stringstream ss;
    ss << "rf_kernels::weighted_upsampler: (Df,Dt)=(" << Df << "," << Dt << ") is not supported";
    throw runtime_error(ss.str());
}


inline kernel_t get_kernel(int Df, int Dt)
{
    if (Df > 8) {
	if (_unlikely(Df % 8 != 0))
	    _no_kernel(Df,Dt);
	Df = 8;
    }

    if (Dt > 8) {
	if (_unlikely(Dt % 8 != 0))
	    _no_kernel(Df,Dt);
	Dt = 16;
    }

    auto p = kernel_table.find({{Df,Dt}});
    
    if (_unlikely(p == kernel_table.end()))
	_no_kernel(Df, Dt);

    return p->second;
}


template<int Df, int Dt, typename enable_if<(Dt==0),int>::type=0>
inline void _populate1() { }

template<int Df, int Dt, typename enable_if<(Dt>0),int>::type=0>
inline void _populate1()
{
    _populate1<Df,(Dt/2)> ();
    kernel_table[{{Df,Dt}}] = kernel_upsample_weights<float,8,Df,Dt>;
}


template<int Df, int Dt, typename enable_if<(Df==0),int>::type=0>
inline void _populate2() { }

// Called for (Df,Dt)=(*,8).
template<int Df, int Dt, typename enable_if<(Df>0),int>::type=0>
inline void _populate2()
{
    _populate2<(Df/2),Dt> ();
    _populate1<Df,Dt> ();
}


struct _initializer {
    _initializer() { _populate2<8,16>(); }
} _init;


}  // namespace upsampling_kernel_table


// -------------------------------------------------------------------------------------------------


weight_upsampler::weight_upsampler(int Df_, int Dt_) :
    Df(Df_),
    Dt(Dt_),
    _f(upsampling_kernel_table::get_kernel(Df_,Dt_))
{ }


void weight_upsampler::upsample(int nfreq_in, int nt_in, float *out, int ostride, const float *in, int istride, float w_cutoff)
{
    if (_unlikely(nfreq_in <= 0))
	throw runtime_error("rf_kernels::weighted_upsampler: expected nfreq_in > 0");
    if (_unlikely(nt_in <= 0))
	throw runtime_error("rf_kernels::weighted_upsampler: expected nt_in > 0");
    if (_unlikely(nt_in % 8))
	throw runtime_error("rf_kernels::weighted_upsampler: expected nt_in divisible by 8");
    if (_unlikely(!out || !in))
	throw runtime_error("rf_kernels::weighted_upsampler: null pointer passed to upsample()");
    if (_unlikely(abs(ostride) < Dt*nt_in))
	throw runtime_error("rf_kernels::weighted_upsampler: ostride is too small");
    if (_unlikely(abs(istride) < nt_in))
	throw runtime_error("rf_kernels::weighted_upsampler: istride is too small");
    if (_unlikely(w_cutoff < 0.0))
	throw runtime_error("rf_kernels::weighted_upsampler: expected w_cutoff >= 0");

    this->_f(this, nfreq_in, nt_in, out, ostride, in, istride, w_cutoff);
}



}   // namespace rf_kernels
