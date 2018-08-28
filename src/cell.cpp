/*! @file cell.cpp
    @brief Implementation of Cell class
*/
#include "cell.hpp"

#include <wtl/iostr.hpp>
#include <wtl/random.hpp>
#include <sfmt.hpp>
#include <boost/program_options.hpp>

#include <type_traits>

namespace tumopp {

double Cell::GAMMA_SHAPE_ = 1.0;
double Cell::PROB_SYMMETRIC_DIVISION_ = 1.0;
unsigned Cell::MAX_PROLIFERATION_CAPACITY_ = 10u;

//! Parameters of Cell class
/*! @ingroup params

    Command line option | Symbol              | Variable                  |
    ------------------- | ------------------- | ------------------------- |
    `-k,--shape`        | \f$k\f$             | Cell::GAMMA_SHAPE_
    `-p,--symmetric`    | \f$p_s\f$           | Cell::PROB_SYMMETRIC_DIVISION_
    `-r,--prolif`       | \f$\omega_{\max}\f$ | Cell::MAX_PROLIFERATION_CAPACITY_
*/
boost::program_options::options_description Cell::opt_description() {
    namespace po = boost::program_options;
    po::options_description desc{"Cell"};
    auto po_value = [](auto* var) {return po::value(var)->default_value(*var);};
    desc.add_options()
        ("shape,k", po_value(&GAMMA_SHAPE_))
        ("symmetric,p", po_value(&PROB_SYMMETRIC_DIVISION_))
        ("prolif,r", po_value(&MAX_PROLIFERATION_CAPACITY_))
    ;
    return desc;
}

template <class URBG>
inline bool bernoulli(double p, URBG& engine) {
    // consume less URBG when p is set to 0 or 1.
    return p >= 1.0 || (p > 0.0 && wtl::generate_canonical(engine) < p);
}

class bernoulli_distribution {
  public:
    bernoulli_distribution(double p) noexcept: p_(p) {}
    template <class URBG>
    bool operator()(URBG& engine) const {
        return p_ >= 1.0 || (p_ > 0.0 && wtl::generate_canonical(engine) < p_);
    }
    void param(double p) {p_ = p;}
  private:
    double p_;
};

namespace {
    std::normal_distribution<double> GAUSS_BIRTH(0.0, 0.0);
    std::normal_distribution<double> GAUSS_DEATH(0.0, 0.0);
    std::normal_distribution<double> GAUSS_MIGRA(0.0, 0.0);
    bernoulli_distribution BERN_MUT_BIRTH(0.0);
    bernoulli_distribution BERN_MUT_DEATH(0.0);
    bernoulli_distribution BERN_MUT_MIGRA(0.0);
}

void Cell::init_distributions(const DriverParams& dp) {
    GAUSS_BIRTH.param(decltype(GAUSS_BIRTH)::param_type(dp.MEAN_BIRTH, dp.SD_BIRTH));
    GAUSS_DEATH.param(decltype(GAUSS_DEATH)::param_type(dp.MEAN_DEATH, dp.SD_DEATH));
    GAUSS_MIGRA.param(decltype(GAUSS_MIGRA)::param_type(dp.MEAN_MIGRA, dp.SD_MIGRA));
    BERN_MUT_BIRTH.param(dp.RATE_BIRTH);
    BERN_MUT_DEATH.param(dp.RATE_DEATH);
    BERN_MUT_MIGRA.param(dp.RATE_MIGRA);
}

static_assert(std::is_nothrow_copy_constructible<Cell>{}, "");
static_assert(std::is_nothrow_move_constructible<Cell>{}, "");

Cell::Cell(const Cell& other) noexcept:
    coord_(other.coord_),
    event_rates_(other.event_rates_),
    type_(other.type_),
    proliferation_capacity_(other.proliferation_capacity_),
    id_(other.id_),
    ancestor_(other.ancestor_),
    time_of_birth_(other.time_of_birth_) {
    if (type_ == CellType::stem) {
        if (!bernoulli(PROB_SYMMETRIC_DIVISION_, wtl::sfmt64())) {
            type_ = CellType::nonstem;
        }
    }
}

std::string Cell::mutate() {
    auto oss = wtl::make_oss();
    if (BERN_MUT_BIRTH(wtl::sfmt64())) {
        event_rates_ = std::make_shared<EventRates>(*event_rates_);
        double s = GAUSS_BIRTH(wtl::sfmt64());
        oss << id_ << "\tbirth\t" << s << "\n";
        event_rates_->birth_rate *= (s += 1.0);
    }
    if (BERN_MUT_DEATH(wtl::sfmt64())) {
        event_rates_ = std::make_shared<EventRates>(*event_rates_);
        double s = GAUSS_DEATH(wtl::sfmt64());
        oss << id_ << "\tdeath\t" << s << "\n";
        event_rates_->death_rate *= (s += 1.0);
        event_rates_->death_prob *= (s += 1.0);
    }
    if (BERN_MUT_MIGRA(wtl::sfmt64())) {
        event_rates_ = std::make_shared<EventRates>(*event_rates_);
        double s = GAUSS_MIGRA(wtl::sfmt64());
        oss << id_ << "\tmigra\t" << s << "\n";
        event_rates_->migra_rate *= (s += 1.0);
    }
    return oss.str();
}

std::string Cell::force_mutate() {
    event_rates_ = std::make_shared<EventRates>(*event_rates_);
    const double s_birth = GAUSS_BIRTH(wtl::sfmt64());
    const double s_death = GAUSS_DEATH(wtl::sfmt64());
    const double s_migra = GAUSS_MIGRA(wtl::sfmt64());
    event_rates_->birth_rate *= (1.0 + s_birth);
    event_rates_->death_rate *= (1.0 + s_death);
    event_rates_->death_prob *= (1.0 + s_death);
    event_rates_->migra_rate *= (1.0 + s_migra);
    auto oss = wtl::make_oss();
    oss << id_ << "\tbirth\t" << s_birth << "\n"
        << id_ << "\tdeath\t" << s_death << "\n"
        << id_ << "\tmigra\t" << s_migra << "\n";
    return oss.str();
}

double Cell::delta_time(const double positional_value) {
    double t_birth = std::numeric_limits<double>::infinity();
    double t_death = std::numeric_limits<double>::infinity();
    double t_migra = std::numeric_limits<double>::infinity();
    if (proliferation_capacity_ > 0) {
        double mu = 1.0;
        mu /= birth_rate();
        mu /= positional_value;
        mu -= elapsed_;
        double theta = std::max(mu / GAMMA_SHAPE_, 0.0);
        std::gamma_distribution<double> gamma(GAMMA_SHAPE_, theta);
        t_birth = gamma(wtl::sfmt64());
    }
    if (death_rate() > 0.0) {
        std::exponential_distribution<double> exponential(death_rate());
        t_death = exponential(wtl::sfmt64());
    }
    if (migra_rate() > 0.0) {
        std::exponential_distribution<double> exponential(migra_rate());
        t_migra = exponential(wtl::sfmt64());
    }

    if (t_birth < t_death && t_birth < t_migra) {
        next_event_ = bernoulli(death_prob(), wtl::sfmt64())
                      ? Event::death : Event::birth;
        elapsed_ = 0.0;
        return t_birth;
    } else if (t_death < t_migra) {
        next_event_ = Event::death;
        return t_death;
    } else {
        next_event_ = Event::migration;
        elapsed_ += t_migra;
        return t_migra;
    }
}

void Cell::set_cycle_dependent_death(const double p) {
    //TODO: reduce redundant copy for susceptible cells
    event_rates_ = std::make_shared<EventRates>(*event_rates_);
    event_rates_->death_prob = p;
    next_event_ = bernoulli(p, wtl::sfmt64()) ? Event::death : Event::birth;
}

std::unordered_set<unsigned> Cell::traceback() const {
    std::unordered_set<unsigned> genealogy;
    genealogy.emplace(id_);
    for (std::shared_ptr<Cell> p = ancestor_; p; p = p->ancestor_) {
        genealogy.emplace(p->id_);
    }
    return genealogy;
}

std::vector<unsigned> Cell::has_mutations_of(const std::vector<unsigned>& mutants) const {
    const auto genealogy = traceback();
    std::vector<unsigned> genotype;
    genotype.reserve(mutants.size());
    for (const auto mut: mutants) {
        if (genealogy.find(mut) != genealogy.end()) {
            genotype.push_back(1u);
        } else {
            genotype.push_back(0u);
        }
    }
    return genotype;
}

size_t Cell::branch_length(const Cell& other) const {
    if (id_ == other.id_) return 0u;
    size_t length = 2u;
    unsigned mrca = 1u;
    const auto genealogy = traceback();
    for (std::shared_ptr<Cell> p = other.ancestor_; p; p = p->ancestor_) {
        if (genealogy.find(p->id_) != genealogy.end()) {
            mrca = p->id_;
            break;
        }
        ++length;
    }
    for (std::shared_ptr<Cell> p = ancestor_; p->id_ > mrca; p = p->ancestor_) {
        ++length;
    }
    return length;
}

std::string Cell::header() {
    std::ostringstream oss;
    oss << "x\ty\tz\t"
        << "id\tancestor\t"
        << "birth\tdeath\t"
        << "beta\tdelta\talpha\trho\t"
        << "type\tomega";
    return oss.str();
}

std::ostream& Cell::write(std::ostream& ost) const {
    int z = (coord_.size() > 2U) ? coord_[2] : 0;
    return ost
        << coord_[0] << "\t" << coord_[1] << "\t" << z << "\t"
        << id_ << "\t"
        << (ancestor_ ? ancestor_->id_ : 0u) << "\t"
        << time_of_birth_ << "\t" << time_of_death_ << "\t"
        << birth_rate() << "\t"
        << death_rate() << "\t"
        << death_prob() << "\t"
        << migra_rate() << "\t"
        << static_cast<unsigned>(type_) << "\t"
        << static_cast<unsigned>(proliferation_capacity_);
}

std::string Cell::str() const {
    auto oss = wtl::make_oss();
    write(oss);
    return oss.str();
}

//! Stream operator for debug print
std::ostream& operator<< (std::ostream& ost, const Cell& x) {
    return x.write(ost);
}

} // namespace tumopp
