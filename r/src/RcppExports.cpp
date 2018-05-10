// Generated by using Rcpp::compileAttributes() -> do not edit by hand
// Generator token: 10BE3573-1514-4C36-9D1C-5A225CD40393

#include <Rcpp.h>

using namespace Rcpp;

// cpp_tumopp
std::vector<std::string> cpp_tumopp(const std::vector<std::string>& args, size_t npair);
RcppExport SEXP _tumopp_cpp_tumopp(SEXP argsSEXP, SEXP npairSEXP) {
BEGIN_RCPP
    Rcpp::RObject rcpp_result_gen;
    Rcpp::RNGScope rcpp_rngScope_gen;
    Rcpp::traits::input_parameter< const std::vector<std::string>& >::type args(argsSEXP);
    Rcpp::traits::input_parameter< size_t >::type npair(npairSEXP);
    rcpp_result_gen = Rcpp::wrap(cpp_tumopp(args, npair));
    return rcpp_result_gen;
END_RCPP
}

static const R_CallMethodDef CallEntries[] = {
    {"_tumopp_cpp_tumopp", (DL_FUNC) &_tumopp_cpp_tumopp, 2},
    {NULL, NULL, 0}
};

RcppExport void R_init_tumopp(DllInfo *dll) {
    R_registerRoutines(dll, NULL, CallEntries, NULL, NULL);
    R_useDynamicSymbols(dll, FALSE);
}
