/*! @file cell.hpp
    @brief Interface of Cell class
*/
#pragma once
#ifndef CELL_HPP_
#define CELL_HPP_

#include <cstdint>
#include <vector>
#include <valarray>
#include <unordered_set>
#include <string>
#include <memory>

namespace boost {namespace program_options {class options_description;}}

namespace tumopp {

//! C1 cell type
enum class CellType: uint_fast8_t {
   stem,
   nonstem,
};

//! event types
enum class Event: uint_fast8_t {
   birth,
   death,
   migration,
};

/*! @brief Cancer cell
*/
class Cell {
  public:
    //! Default constructor
    Cell() = default;
    //! Constructor for first cells
    Cell(const std::valarray<int>& v, unsigned i=0) noexcept: coord_(v), id_(i) {}
    //! Copy constructor
    Cell(const Cell& other) noexcept;
    //! Destructor
    ~Cell() noexcept = default;
    //! Copy assignment operator
    Cell& operator=(const Cell&) = delete;
    //! Move constructor
    Cell(Cell&& other) = default;
    //! Move assignment operator
    Cell& operator=(Cell&&) = default;

    //! driver mutation
    std::string mutate();
    //! driver mutation on all traits
    std::string force_mutate();

    //! Set death rate to simulate Moran-like model
    void increase_death_rate() noexcept;

    //! Calc dt and set #next_event_
    double delta_time(double positional_value);

    //! Check mutant ancestors
    std::vector<unsigned> has_mutations_of(const std::vector<unsigned>&) const;

    //! Branch length (# of divisions) between two cells
    size_t branch_length(const Cell&) const;

    //! setter of #coord_
    void set_coord(const std::valarray<int>& v) noexcept {coord_ = v;}
    //! setter of #time_of_birth_; reset other properties
    void set_time_of_birth(double t, unsigned i, const std::shared_ptr<Cell>& ancestor) noexcept {
        time_of_birth_ = t;
        id_ = i;
        ancestor_ = ancestor;
        if (type_ == CellType::nonstem) {--proliferation_capacity_;}
    }
    //! setter of #time_of_death_
    void set_time_of_death(double t) noexcept {time_of_death_ = t;}

    //! getter of #birth_rate_
    double birth_rate() const noexcept {return birth_rate_;}
    //! getter of #death_rate_
    double death_rate() const noexcept {return death_rate_;}
    //! getter of #migra_rate_
    double migra_rate() const noexcept {return migra_rate_;}
    //! getter of #next_event_
    Event next_event() const noexcept {return next_event_;}
    //! getter of #coord_
    const std::valarray<int>& coord() const noexcept {return coord_;}
    //! getter of #MUTATION_RATE_
    static double MUTATION_RATE() noexcept {return MUTATION_RATE_;}
    //! getter of #HAS_AT_LEAST_1_MUTATION_PER_DIVISION_
    static bool HAS_AT_LEAST_1_MUTATION_PER_DIVISION() noexcept {return HAS_AT_LEAST_1_MUTATION_PER_DIVISION_;}

    //! TSV header
    static std::string header();
    //! TSV
    std::ostream& write(std::ostream& ost) const;
    //! TSV
    std::string str() const;
    friend std::ostream& operator<< (std::ostream&, const Cell&);

    //! Initialize probability distributions for events
    static void init_distributions();

    static boost::program_options::options_description opt_description();

  private:
    //! Accumulate ancestral #id_
    std::unordered_set<unsigned> traceback() const;
    /////1/////////2/////////3/////////4/////////5/////////6/////////7/////////
    // Data member

    //! \f$\beta_0\f$
    static double BIRTH_RATE_;
    //! \f$\delta_0\f$
    static double DEATH_RATE_;
    //! \f$\alpha_0\f$, death probability on cell division attempt
    static double DEATH_PROB_;
    //! \f$\rho_0\f$
    static double MIGRATION_RATE_;
    //! \f$k\f$
    static double GAMMA_SHAPE_;
    //! \f$p_s\f$
    static double PROB_SYMMETRIC_DIVISION_;
    //! \f$\omega_\text{max}\f$
    static unsigned MAX_PROLIFERATION_CAPACITY_;

    //! \f$\mu_\beta\f$
    static double DRIVER_RATE_BIRTH_;
    //! \f$\mu_\delta\f$
    static double DRIVER_RATE_DEATH_;
    //! \f$\mu_\rho\f$
    static double DRIVER_RATE_MIGRA_;
    //! \f$\bar s_\beta\f$
    static double DRIVER_MEAN_BIRTH_;
    //! \f$\bar s_\delta\f$
    static double DRIVER_MEAN_DEATH_;
    //! \f$\bar s_\rho\f$
    static double DRIVER_MEAN_MIGRA_;
    //! \f$\sigma_\beta\f$
    static double DRIVER_SD_BIRTH_;
    //! \f$\sigma_\delta\f$
    static double DRIVER_SD_DEATH_;
    //! \f$\sigma_\rho\f$
    static double DRIVER_SD_MIGRA_;

    //! \f$\mu\f$: neutral mutation rate per cell division
    static double MUTATION_RATE_;
    //! for testing tree inference from ms-like output
    static bool HAS_AT_LEAST_1_MUTATION_PER_DIVISION_;

    //! Position in a tumor
    std::valarray<int> coord_;

    //! \f$\beta\f$
    double birth_rate_ = BIRTH_RATE_;
    //! \f$\delta\f$
    double death_rate_ = DEATH_RATE_;
    //! \f$\alpha\f$
    double death_prob_ = DEATH_PROB_;
    //! \f$\rho\f$
    double migra_rate_ = MIGRATION_RATE_;

    //! C1 cell type
    CellType type_ = CellType::stem;
    //! \f$\omega\f$
    uint_fast8_t proliferation_capacity_ = static_cast<uint_fast8_t>(MAX_PROLIFERATION_CAPACITY_);

    //! next event: birth, death, or migration
    Event next_event_ = Event::birth;
    //! elapsed time by migration
    double elapsed_ = 0.0;

    //! ID
    unsigned id_;
    //! ancestor's ID
    std::shared_ptr<Cell> ancestor_;
    //! time of birth
    double time_of_birth_ = 0.0;
    //! time of death
    double time_of_death_ = 0.0;
};

} // namespace tumopp

#endif /* CELL_HPP_ */
