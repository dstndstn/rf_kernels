#ifndef _RF_KERNELS_INTENSITY_CLIPPER_INTERNALS_HPP
#define _RF_KERNELS_INTENSITY_CLIPPER_INTERNALS_HPP

#include "upsample_internals.hpp"
#include "downsample_internals.hpp"
#include "mean_rms_internals.hpp"
#include "intensity_clipper.hpp"

namespace rf_kernels {
#if 0
}; // pacify emacs c-mode
#endif

template<typename T, int S> using simd_t = simd_helpers::simd_t<T,S>;


template<typename T, int S, int DfX, int DtX, bool TwoPass, typename std::enable_if<((DfX>1)||(DtX>1)),int>::type = 0>
inline void kernel_intensity_clipper_taxis(const intensity_clipper *ic, const T *in_i, T *in_w, int stride)
{
    // For upsampler.  Note std::min() can't be used in a constexpr!
    constexpr int Dfu = (DfX < 8) ? DfX : 8;
    constexpr bool Hflag = true;

    const int Df = ic->Df;
    const int Dt = ic->Dt;
    const int nfreq_ds = ic->nfreq_ds;
    const int nt_ds = ic->nt_ds;
    const int niter = ic->niter;
    const simd_t<T,S> sigma(ic->sigma);
    const simd_t<T,S> iter_sigma(ic->iter_sigma);

    float *tmp_i = ic->tmp_i;
    float *tmp_w = ic->tmp_w;
    
    _wrms_buf_linear<T,S> buf(tmp_i, tmp_w, nt_ds);
    _wi_downsampler_1d<T, S, DfX, DtX> ds1(Df, Dt);
    _weight_upsampler_0d<T, S, Dfu, DtX> us0(Dt);

    for (int ifreq_ds = 0; ifreq_ds < nfreq_ds; ifreq_ds++) {
	_wrms_first_pass<T,S,Hflag,TwoPass> fp;
	
	ds1.downsample_1d(fp, nt_ds, stride,
			  in_i + ifreq_ds * Df * stride,
			  in_w + ifreq_ds * Df * stride,
			  tmp_i, tmp_w);

	simd_t<T,S> mean, var;
	fp.finalize(buf, mean, var);

	// Note iter_sigma here (not sigma)
	_wrms_iterate<Hflag> (buf, mean, var, niter-1, iter_sigma);
	
	// Note sigma here (not iter_sigma)
	simd_t<T,S> thresh = sigma * var.sqrt();

	// Mask rows (note that we mask "Dfu" rows at a time, where Dfu = min(Df,8).
	for (int ifreq_u = ifreq_ds*Df; ifreq_u < (ifreq_ds+1)*Df; ifreq_u += Dfu) {
	    T *wrow = in_w + ifreq_u * stride;

	    for (int it = 0; it < nt_ds; it += S) {
		simd_t<T,S> mask = buf.get_mask(mean, thresh, it);
		us0.put_mask(wrow + it*Dt, stride, mask);
	    }
	}
    }
}


template<typename T, int S, int DfX, int DtX, bool TwoPass>
inline void kernel_intensity_clipper_faxis(const intensity_clipper *ic, const T *in_i, T *in_w, int stride)
{
    constexpr bool Hflag = false;
	
    const int Df = ic->Df;
    const int Dt = ic->Dt;
    const int nfreq_ds = ic->nfreq_ds;
    const int nt_ds = ic->nt_ds;
    const int niter = ic->niter;
    const simd_t<T,S> sigma(ic->sigma);
    const simd_t<T,S> iter_sigma(ic->iter_sigma);

    float *tmp_i = ic->tmp_i;
    float *tmp_w = ic->tmp_w;
    
    _wrms_buf_linear<T,S> buf(tmp_i, tmp_w, nfreq_ds * S);
    _wi_downsampler_1f<T,S,DfX,DtX> ds1(Df, Dt);
    _weight_upsampler_0f<T,S,DfX,DtX> us0(Df, Dt);

    for (int it_ds = 0; it_ds < nt_ds; it_ds += S) {
	_wrms_first_pass<T,S,Hflag,TwoPass> fp;
	
	ds1.downsample_1f(fp, nfreq_ds, stride,
			  in_i + it_ds * Dt,
			  in_w + it_ds * Dt,
			  tmp_i, tmp_w);

	simd_t<T,S> mean, var;
	fp.finalize(buf, mean, var);

	// Note iter_sigma here (not sigma)
	_wrms_iterate<Hflag> (buf, mean, var, niter-1, iter_sigma);
	
	// Note sigma here (not iter_sigma)
	simd_t<T,S> thresh = sigma * var.sqrt();

	for (int ifreq_ds = 0; ifreq_ds < nfreq_ds; ifreq_ds++) {
	    simd_t<T,S> mask = buf.get_mask(mean, thresh, ifreq_ds*S);
	    us0.put_mask(in_w + ifreq_ds * Df * stride + it_ds * Dt, stride, mask);
	}
    }	
}


template<typename T, int S, int DfX, int DtX, bool TwoPass>
inline void kernel_intensity_clipper_naxis(const intensity_clipper *ic, const T *in_i, T *in_w, int stride)
{
    // For upsampler.  Note std::min() can't be used in a constexpr!
    constexpr int Dfu = (DfX < 8) ? DfX : 8;
    constexpr bool Hflag = true;

    const int Df = ic->Df;
    const int Dt = ic->Dt;
    const int nfreq_ds = ic->nfreq_ds;
    const int nt_ds = ic->nt_ds;
    const int niter = ic->niter;
    const simd_t<T,S> sigma(ic->sigma);
    const simd_t<T,S> iter_sigma(ic->iter_sigma);

    float *tmp_i = ic->tmp_i;
    float *tmp_w = ic->tmp_w;
    
    _wrms_buf_linear<T,S> buf(tmp_i, tmp_w, nfreq_ds * nt_ds);
    _wrms_first_pass<T,S,Hflag,TwoPass> fp;
    
    _wi_downsampler_1d<T, S, DfX, DtX> ds1(Df, Dt);
    _weight_upsampler_0d<T, S, Dfu, DtX> us0(Dt);

    for (int ifreq_ds = 0; ifreq_ds < nfreq_ds; ifreq_ds++) {
	ds1.downsample_1d(fp, nt_ds, stride,
			  in_i + ifreq_ds * Df * stride,
			  in_w + ifreq_ds * Df * stride,
			  tmp_i + ifreq_ds * nt_ds,
			  tmp_w + ifreq_ds * nt_ds);
    }

    simd_t<T,S> mean, var;
    fp.finalize(buf, mean, var);
    
    // Note iter_sigma here (not sigma)
    _wrms_iterate<Hflag> (buf, mean, var, niter-1, iter_sigma);
	
    // Note sigma here (not iter_sigma)
    simd_t<T,S> thresh = sigma * var.sqrt();
    
    // Mask rows (note that we mask "Dfu" rows at a time, where Dfu = min(Df,8).
    for (int ifreq_ds = 0; ifreq_ds < nfreq_ds; ifreq_ds++) {
	for (int ifreq_u = ifreq_ds*Df; ifreq_u < (ifreq_ds+1)*Df; ifreq_u += Dfu) {
	    T *wrow = in_w + ifreq_u * stride;
	    
	    for (int it = 0; it < nt_ds; it += S) {
		simd_t<T,S> mask = buf.get_mask(mean, thresh, ifreq_ds*nt_ds + it);
		us0.put_mask(wrow + it*Dt, stride, mask);
	    }
	}
    }
}


// -------------------------------------------------------------------------------------------------


template<typename T, int S, int DfX, int DtX, bool TwoPass, typename std::enable_if<((DfX==1)&&(DtX==1)),int>::type = 0>
inline void kernel_intensity_clipper_taxis(const intensity_clipper *ic, const T *in_i, T *in_w, int stride)
{
    constexpr bool Hflag = true;

    const int nfreq = ic->nfreq;
    const int nt = ic->nt_chunk;
    const int niter = ic->niter;
    const simd_t<T,S> sigma(ic->sigma);
    const simd_t<T,S> iter_sigma(ic->iter_sigma);
    
    for (int ifreq = 0; ifreq < nfreq; ifreq++) {
	_wrms_buf_linear<T,S> buf(in_i + ifreq*stride, in_w + ifreq*stride, nt);
	_wrms_first_pass<T,S,Hflag,TwoPass> fp;

	simd_t<T,S> mean, var;
	fp.run(buf, mean, var);

	// Note iter_sigma here (not sigma)
	_wrms_iterate<Hflag> (buf, mean, var, niter-1, iter_sigma);
	
	// Note sigma here (not iter_sigma)
	simd_t<T,S> thresh = sigma * var.sqrt();

	// Mask rows
	for (int it = 0; it < nt; it += S) {
	    simd_t<T,S> mask = buf.get_mask(mean, thresh, it);

	    T *p = in_w + ifreq*stride + it;
	    simd_t<T,S> w = simd_helpers::simd_load<T,S> (p);
	    simd_helpers::simd_store(p, w & mask);
	}
    }
}


}  // namespace rf_kernels

#endif // _RF_KERNELS_INTENSITY_CLIPPER_INTERNALS_HPP
