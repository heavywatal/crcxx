// [[Rcpp::plugins(cpp14)]]
#include <Rcpp.h>
#include <tumopp/simulation.hpp>

//' Run C++ simulation
//' @return conf and population as strings
//' @rdname tumopp
// [[Rcpp::export]]
Rcpp::CharacterVector cpp_tumopp(const std::vector<std::string>& args, unsigned int npair=0, unsigned int nsam=0) {
    try {
        tumopp::Simulation simulation(args);
        simulation.run();
        return Rcpp::CharacterVector::create(
            Rcpp::Named("config", simulation.config_string()),
            Rcpp::Named("specimens", simulation.specimens()),
            Rcpp::Named("drivers", simulation.drivers()),
            Rcpp::Named("distances", simulation.pairwise_distance(npair)),
            Rcpp::Named("ms", simulation.ms())
        );
    } catch (const std::runtime_error& e) {
        Rcpp::Rcerr << e.what() << "\n";
    }
    return {};
}
