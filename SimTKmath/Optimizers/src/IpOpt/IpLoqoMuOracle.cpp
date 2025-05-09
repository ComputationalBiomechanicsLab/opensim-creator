// Copyright (C) 2004, 2006 International Business Machines and others.
// All Rights Reserved.
// This code is published under the Common Public License.
//
// $Id: IpLoqoMuOracle.cpp 759 2006-07-07 03:07:08Z andreasw $
//
// Authors:  Carl Laird, Andreas Waechter     IBM    2004-08-13

#include "IpLoqoMuOracle.hpp"

#ifdef HAVE_CMATH
# include <cmath>
#else
# ifdef HAVE_MATH_H
#  include <math.h>
# else
#  error "don't have header file for math"
# endif
#endif

#include <limits>
#include <cstdio>

// Keeps MS VC++ 8 quiet about sprintf, strcpy, etc.
#ifdef _MSC_VER
#pragma warning(disable:4996)
#endif



namespace SimTKIpopt
{

#ifdef IP_DEBUG
  static const Index dbg_verbosity = 0;
#endif

  LoqoMuOracle::LoqoMuOracle()
      :
      MuOracle()
  {}

  LoqoMuOracle::~LoqoMuOracle()
  {}

  bool LoqoMuOracle::InitializeImpl(const OptionsList& options,
                                    const std::string& prefix)
  {
    return true;
  }

  bool LoqoMuOracle::CalculateMu(Number mu_min, Number mu_max,
                                 Number& new_mu)
  {
    DBG_START_METH("LoqoMuOracle::CalculateMu",
                   dbg_verbosity);

    Number avrg_compl = IpCq().curr_avrg_compl();
    Jnlst().Printf(J_DETAILED, J_BARRIER_UPDATE,
                   "  Average complemantarity is %lf\n", avrg_compl);

    Number xi = IpCq().curr_centrality_measure();
    Jnlst().Printf(J_DETAILED, J_BARRIER_UPDATE,
                   "  Xi (distance from uniformity) is %lf\n", xi);

    //Number factor = 1.-tau_min_;   //This is the original values
    Number factor = Number(0.05);   //This is the value I used otherwise
    Number sigma = Number(0.1)*std::pow(Min(factor*(1-xi)/xi,Number(2)),Number(3));

    Number mu = sigma*avrg_compl;
    Jnlst().Printf(J_DETAILED, J_BARRIER_UPDATE,
                   "  Barrier parameter proposed by LOQO rule is %lf\n", mu);

    // DELETEME
    const int n = 40;
    char ssigma[n];
    snprintf(ssigma, n, " sigma=%8.2e", sigma);
    IpData().Append_info_string(ssigma);
    snprintf(ssigma, n, " xi=%8.2e ", IpCq().curr_centrality_measure());
    IpData().Append_info_string(ssigma);

    new_mu = Max(Min(mu_max, mu), mu_min);
    return true;
  }

} // namespace Ipopt
