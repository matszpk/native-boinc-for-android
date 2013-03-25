// Routines to add a value to a truncated value with gaussian distribution
// of rms=rms, with mean value v0.  
// Default is single bit truncation (rms=trunc=abs(val), v0=0, step=2*abs(val)) 
// multi-bit truncation later.

#include <cmath>
#include <complex>
#include <cassert>
#include <vector>


template <typename T, typename U>
T truncated_add(T val, U add, U v0=0, U rms=0, U trunc=0, U step=0) {
  T v=std::abs(val);   
  T s=(val>0)*2-1;     // sign of val
  U a=std::abs(add);
  if (!rms) rms=static_cast<U>(v);
  if (!trunc) trunc=static_cast<U>(v);
  if (!step) step=static_cast<U>(v)*2;  // 2*abs(val) for single bit
  if ((val*add)>0) {
    if ((v+a)>trunc) { 
      // T and U are the same sign and add to more than trunc, so return the
      // truncation value with appropriate sign.
      return s*trunc;  
    } else {
      // Otherwise return the sum
      return round(((val+add)-v0)/step)*step+v0;
    }
  }
  // if val was truncated, the only thing we know is that val>(trunc-step/2)
  U thresh=(trunc-v0-step/2)*s;
  // Now we want to determine the probability that abs(v0+add) is greater than
  // thresh, where v0 is the actual value of the what was measured as val.
  U sum=std::abs(thresh-add);
  // another way to do this would be to generate a gaussian dist random number
  // to be v0, but that gets difficult for larger numbers of bits.
  U prob=erfc(sum/rms);
  if (drand48()>prob) {
    return (trunc-step)*s;
  } else {
    return trunc*s;
  }
};

template <typename T, typename U>
std::complex<T> truncated_add(std::complex<T> val, std::complex<U> add, 
                         std::complex<U> v0=std::complex<U>(0,0), 
			 std::complex<U> rms=std::complex<U>(0,0), 
			 U trunc=0, U step=0) {
  std::complex<T> rv(truncated_add(val.real(),add.real(),v0.real(),rms.real(),trunc,step),
                truncated_add(val.imag(),add.imag(),v0.imag(),rms.imag(),trunc,step));
  return rv;
}

template <typename T, typename U>
void truncated_add(std::vector<T> &val, const std::vector<U> &add, U v0=0, U rms=0, U trunc=0, U step=0) {
  int i;
  assert(val.size() == add.size());
  for (i=0;i<val.size();i++) {
    val[i]=truncated_add(val[i],add[i],v0,rms,trunc,step);
  }
}

template <typename T, typename U>
void truncated_add(std::vector<std::complex<T> > &val, const std::vector<std::complex<U> > &add, std::complex<U> v0=0, std::complex<U> rms=0, U trunc=0, U step=0) {
  int i;
  assert(val.size() == add.size());
  for (i=0;i<val.size();i++) {
    val[i]=truncated_add(val[i],add[i],v0,rms,trunc,step);
  }
}

