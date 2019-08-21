//////////////////////////////////////////////////////////////////////////////////////
// This file is distributed under the University of Illinois/NCSA Open Source License.
// See LICENSE file in top directory for details.
//
// Copyright (c) 2019 QMCPACK developers.
//
// File developed by: Peter Doak, doakpw@ornl.gov, Oak Ridge National Laboratory
//
// File created by: Peter Doak, doakpw@ornl.gov, Oak Ridge National Laboratory
//////////////////////////////////////////////////////////////////////////////////////

#include "Message/catch_mpi_main.hpp"

#include "Message/Communicate.h"
#include "QMCDrivers/VMC/VMCDriverInput.h"
#include "QMCDrivers/VMC/VMCBatched.h"
#include "QMCDrivers/tests/ValidQMCInputSections.h"
#include "QMCApp/tests/MinimalParticlePool.h"
#include "QMCApp/tests/MinimalWaveFunctionPool.h"
#include "QMCApp/tests/MinimalHamiltonianPool.h"

namespace qmcplusplus
{
TEST_CASE("VMCBatched::calc_default_local_walkers", "[drivers]")
{
  using namespace testing;
  Communicate* comm;
  OHMMS::Controller->initialize(0, NULL);
  comm = OHMMS::Controller;

  Libxml2Document doc;
  doc.parseFromString(valid_vmc_input_sections[valid_vmc_input_vmc_batch_index]);
  xmlNodePtr node = doc.getRoot();
  QMCDriverInput qmcdriver_input(3);
  qmcdriver_input.readXML(node);

  MinimalParticlePool mpp;
  ParticleSetPool particle_pool = mpp(comm);
  MinimalWaveFunctionPool wfp(comm);
  WaveFunctionPool wavefunction_pool = wfp(particle_pool);
  MinimalHamiltonianPool mhp(comm);
  HamiltonianPool hamiltonian_pool = mhp(particle_pool, wavefunction_pool);

  int num_ranks  = 4;
  int num_crowds = 8;
  MCPopulation population(num_ranks);

  auto testWRTWalkersPerRank = [&](int walkers_per_rank) {
    QMCDriverInput qmcdriver_copy(qmcdriver_input);
    VMCDriverInput vmcdriver_input(walkers_per_rank, "yes");
    VMCBatched vmc_batched(std::move(qmcdriver_copy), std::move(vmcdriver_input), population,
                           *(wavefunction_pool.getPrimary()), *(hamiltonian_pool.getPrimary()), wavefunction_pool,
                           comm);
    VMCBatched::IndexType local_walkers       = vmc_batched.calc_default_local_walkers();
    QMCDriverNew::IndexType walkers_per_crowd = vmc_batched.get_walkers_per_crowd();

    if (walkers_per_rank < num_crowds)
    {
      REQUIRE(walkers_per_crowd == 1);
      REQUIRE(local_walkers == num_crowds);
      REQUIRE(population.get_num_local_walkers() == num_crowds);
      REQUIRE(population.get_num_global_walkers() == num_crowds * num_ranks);
    }
    else if (walkers_per_rank % num_crowds)
    {
      REQUIRE(walkers_per_crowd == walkers_per_rank / num_crowds + 1);
      REQUIRE(local_walkers == walkers_per_crowd * num_crowds);
      REQUIRE(population.get_num_local_walkers() == walkers_per_crowd * num_crowds);
      REQUIRE(population.get_num_global_walkers() == walkers_per_crowd * num_crowds * num_ranks);
    }
    else
    {
      REQUIRE(local_walkers == walkers_per_rank);
      REQUIRE(population.get_num_local_walkers() == walkers_per_rank);
      REQUIRE(population.get_num_global_walkers() == walkers_per_rank * num_ranks);
    }
  };
  testWRTWalkersPerRank(7);
  testWRTWalkersPerRank(31);
  testWRTWalkersPerRank(32);
  testWRTWalkersPerRank(33);
}

} // namespace qmcplusplus
