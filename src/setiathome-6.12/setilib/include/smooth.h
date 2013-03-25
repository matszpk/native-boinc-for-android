template <typename T> 
double total(const std::vector<T> &x) {
  double rv=0;
  typename std::vector<T>::const_iterator i(x.begin());
  for ( ; i!=x.end() ; i++ ) rv+=*i;
  return rv;
}

template <typename T> 
complex<double> total(const std::vector<complex<T> > &x) {
  complex<double> rv=0;
  typename std::vector<complex<T> >::const_iterator i(x.begin());
  for ( ; i!=x.end() ; i++ ) rv+=*i;
  return rv;
}

template <typename T> 
double avg(const std::vector<T> &x) {
  return total(x)/x.size();
}

template <typename T> 
complex<double> avg(const std::vector<complex<T> > &x) {
  return total(x)/(double)x.size();
}

template <typename T>
std::vector<T> smooth(const std::vector<T> &x, int len) {
  len>>=1;
  T a=avg(x);
  T val=a*double(len);
  int j;
  std::vector<T> rv(x.size());
  if (len>=(x.size()/2)) {
    for (j=0 ; j<x.size() ; j++ ) {
      rv[j]=a;
    }
  } else if (len>0) {
    for (j=0 ; j<x.size() ; j++ ) {
      if ((j-len)<0) {
        val-=a;
      } else {
        val-=x[j-len];
      } 
      if ((j+len/2)>=x.size()) {
        val+=a;
      } else {
        val+=x[j];
      }
      rv[j]=val/(double)len;
    }
  } else {
    rv=x;
  }
  return rv;
}

template <typename T>
std::vector<complex<T> > smooth(const std::vector<complex<T> > &x, int len) {
  len>>=1;
  complex<T> a=avg(x);
  complex<T> val=a*(T)(len);
  int j;
  std::vector<complex<T> > rv(x.size());
  if (len>=(x.size()/2)) {
    for (j=0 ; j<x.size() ; j++ ) {
      rv[j]=a;
    }
  } else if (len>0) {
    for (j=0 ; j<x.size() ; j++ ) {
      if ((j-len)<0) {
        val-=a;
      } else {
        val-=x[j-len];
      } 
      if ((j+len/2)>=x.size()) {
        val+=a;
      } else {
        val+=x[j];
      }
      rv[j]=val/(T)len;
    }
  } else {
    rv=x;
  }
  return rv;
}

