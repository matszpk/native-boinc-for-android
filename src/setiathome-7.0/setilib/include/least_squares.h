using std::complex;

template <typename T>
std::vector<complex<double> > least_squares(const std::vector<complex<T> > x,
        const std::vector<complex<T> > y) {
    size_t n=std::min(x.size(),y.size());
    size_t i;
    complex<double> xsum,xsum2,ysum,ysum2,xysum;
    for (i=0; i<n; i++) {
        complex<double> x0(x[i].real(),x[i].imag());
        complex<double> y0(y[i].real(),y[i].imag());
        // sum of data points
        xsum+=x0;
        ysum+=y0;
        // sum of squares
        xsum2+=complex<double>(x0.real()*x0.real(),x0.imag()*x0.imag());
        ysum2+=complex<double>(y0.real()*y0.real(),y0.imag()*y0.imag());
        // cross term
        xysum+=complex<double>(x0.real()*y0.real(),x0.imag()*y0.imag());
    }
    complex<double> ss_xx(xsum2-complex<double>(
            n*xsum.real()*xsum.real(),
            n*xsum.imag()*xsum.imag()
            ));
    complex<double> ss_yy(ysum2-complex<double>(
            n*ysum.real()*ysum.real(),
            n*ysum.imag()*ysum.imag()
            ));
    complex<double> ss_xy(xysum-complex<double>(
            n*xsum.real()*ysum.real(),
            n*xsum.imag()*ysum.imag()
            ));

    complex<double> m(ss_xy.real()/ss_xx.real(),ss_xy.imag()/ss_xx.imag());
    complex<double> b(ysum-complex<double>(m.real()*xsum.real(),m.imag()*xsum.imag()));
    std::vector<complex<double> > rv;
    rv.push_back(b);
    rv.push_back(m);
    return rv;
}

template <typename T>
std::vector<double> least_squares(const std::vector<T> x, const std::vector<T> y) {
    size_t n=std::min(x.size(),y.size());
    size_t i;
    double xsum,xsum2,ysum,ysum2,xysum;
    for (i=0; i<n; i++) {
        // sum of data points
        xsum+=static_cast<double>(x[i]);
        ysum+=static_cast<double>(y[i]);
        // sum of squares
        xsum2+=static_cast<double>(x[i])*x[i];
        ysum2+=static_cast<double>(y[i])*y[i];
        // cross term
        xysum+=static_cast<double>(x[i])*y[i];
    }
    double ss_xx=xsum2-n*xsum*xsum;
    double ss_yy=ysum2-n*ysum*ysum;
    double ss_xy=xysum-n*xsum*ysum;

    double m=ss_xy/ss_xx;
    double b=ysum-m*xsum;
    std::vector<double> rv;
    rv.push_back(b);
    rv.push_back(m);
    return rv;
}


