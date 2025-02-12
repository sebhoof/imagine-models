#include <cassert>

#include "../headers/hamunits.h"
#include "../headers/YMW.h"

double YMW16::at_position(const double &x, const double &y, const double &z) const  { 
  // YMW16 using a different Cartesian frame from our default one
  std::vector<double> gc_pos{y, -x, z};
  // cylindrical r
  double r_cyl{std::sqrt(gc_pos[0] * gc_pos[0] + gc_pos[1] * gc_pos[1])};
  // warp
  if (r_cyl >= r_warp) {
    double theta_warp{std::atan2(gc_pos[1], gc_pos[0])};
    gc_pos[2] -= t0_gamma_w *
                 (r_cyl - r_warp) * std::cos(theta_warp);
  }
  double vec_length = std::sqrt(std::pow(gc_pos[0], 2) + std::pow(gc_pos[1], 2)+ std::pow(gc_pos[2], 2)); 
  if (vec_length > 25 ) {
    return 0.;
  } else {
    double ne{0.};
    double ne_comp[8]{0.};
    double weight_localbubble{0.};
    double weight_gum{0.};
    double weight_loop{0.};
    // longitude, in deg
    const double ec_l{
        std::atan2(gc_pos[0], r0 - gc_pos[1]) / cgs::rad};
    // call structure functions
    // since in YMW16, Fermi Bubble is not actually contributing, we ignore FB
    // for thick disk
    ne_comp[1] = thick(gc_pos[2], r_cyl);
    ne_comp[2] = thin(gc_pos[2], r_cyl);
    ne_comp[3] = spiral(gc_pos[0], gc_pos[1], gc_pos[2], r_cyl);
    ne_comp[4] = galcen(gc_pos[0], gc_pos[1], gc_pos[2]);
    ne_comp[5] = gum(gc_pos[0], gc_pos[1], gc_pos[2]);
    // localbubble boundary
    const double localbubble_boundary{110. * 0.001};
    ne_comp[6] = localbubble(gc_pos[0], gc_pos[1], gc_pos[2], ec_l,
                             localbubble_boundary);
    ne_comp[7] = nps(gc_pos[0], gc_pos[1], gc_pos[2]);
    // adding up rules
    ne_comp[0] = ne_comp[1] + std::max(ne_comp[2], ne_comp[3]);
    // distance to local bubble
    const double rlb{std::sqrt(
        std::pow(((gc_pos[1] - 8.34 ) * 0.94 - 0.34 * gc_pos[2]), 2) +
        gc_pos[0] * gc_pos[0])};
    if (rlb < localbubble_boundary) { // inside local bubble
      ne_comp[0] = rlb * ne_comp[1] +
                   std::max(ne_comp[2], ne_comp[3]);
      if (ne_comp[6] > ne_comp[0]) {
        weight_localbubble = 1;
      }
    } else { // outside local bubble
      if (ne_comp[6] > ne_comp[0] and ne_comp[6] > ne_comp[5]) {
        weight_localbubble = 1;
      }
    }
    if (ne_comp[7] > ne_comp[0]) {
      weight_loop = 1;
    }
    if (ne_comp[5] > ne_comp[0]) {
      weight_gum = 1;
    }
    // final density
    ne =
        (1 - weight_localbubble) *
            ((1 - weight_gum) * ((1 - weight_loop) * (ne_comp[0] + ne_comp[4]) +
                                 weight_loop * ne_comp[7]) +
             weight_gum * ne_comp[5]) +
        (weight_localbubble) * (ne_comp[6]);
    assert(std::isfinite(ne));
    return ne;
  }
}

// thick disk
double YMW16::thick(const double &zz, const double &rr) const {
  if (zz > 10. * t1_h1)
    return 0.; // timesaving
  double gd{1.};
  if (rr > t1_bd) {
    gd = std::pow(
        1. / std::cosh((rr - t1_bd) / t1_ad),
        2);
  }
  return t1_n1 * gd *
         std::pow(1. / std::cosh(zz / t1_h1), 2);
}

// thin disk
double YMW16::thin(const double &zz, const double &rr) const {
  // z scaling, K_2*h0 in ref
  double h0{t2_k2 *
               (32 * 0.001 + 1.6e-3 * rr + (4.e-7 / 0.001) * rr * rr)};
  if (zz > 10. * h0)
    return 0.; // timesaving
  double gd{1.};
  if (rr > t1_bd) {
    gd = std::pow(
        1. / std::cosh((rr - t1_bd) / t1_ad),
        2);
  }
  return t2_n2 * gd *
         std::pow(1. / std::cosh((rr - t2_b2) /
                                 t2_a2),
                  2) *
         std::pow(1. / std::cosh(zz / h0), 2);
}

// spiral arms
double YMW16::spiral(const double &xx, const double &yy,
                     const double &zz, const double &rr) const {
  // structure scaling
  double scaling{1.};
  if (rr > t1_bd) {
    if ((rr - t1_bd) > 10. * t1_ad)
      return 0.;
    scaling = std::pow(
        1. / std::cosh((rr - t1_bd) / t1_ad),
        2);
  }
  // z scaling, K_a*h0 in ref
  const double h0{t3_ka * (32 * 0.001 + 1.6e-3 * rr +
                                               (4.e-7 / 0.001) * pow(rr, 2))};
  if (zz > 10. * h0)
    return 0.; // timesaving
  scaling *= std::pow(1. / std::cosh(zz / h0), 2);
  if ((rr - t3_b2s) > 10. * t3_aa)
    return 0.; // timesaving
  // 2nd raidus scaling
  scaling *= std::pow(
      1. / std::cosh((rr - t3_b2s) / t3_aa),
      2);
  double smin;
  double theta{std::atan2(yy, xx)};
  if (theta < 0)
    theta += 2 * cgs::pi;
  double ne3s{0.};
  // looping through arms
  for (int i = 0; i < 4; ++i) {
    // get distance to arm center
    if (i != 4) {
      double d_phi = theta - t3_phimin[i];
      if (d_phi < 0)
        d_phi += 2. * cgs::pi;
      double d =
          std::fabs(t3_rmin[i] *
                        std::exp(d_phi * t3_tpitch[i]) -
                    rr);
      double d_p = std::fabs(
          t3_rmin[i] *
              std::exp((d_phi + 2. * cgs::pi) * t3_tpitch[i]) -
          rr);
      smin = std::min(d, d_p) * t3_cpitch[i];
    } else if (i == 4 and theta >= t3_phimin[i] and
               theta < 2) { // Local arm
      smin = std::fabs(t3_rmin[i] *
                           std::exp((theta + 2 * cgs::pi -
                                     t3_phimin[i]) *
                                    t3_tpitch[i]) -
                       rr) *
             t3_cpitch[i];
    } else {
      continue;
    }
    if (smin > 10. * t3_warm[i])
      continue; // timesaving
    // accumulate density
    if (i != 2) {
      ne3s += t3_narm[i] * scaling *
              std::pow(1. / std::cosh(smin / t3_warm[i]), 2);
    } else if (rr > 6  and
               theta * cgs::rad > t3_thetacn) { // correction for Carina-Sagittarius
      const double ga =
          (1. - (t3_nsg) *
                    (std::exp(-std::pow(
                        (theta * cgs::rad - t3_thetasg) /
                            t3_wsg,
                        2)))) *
          (1. + t3_ncn) *
          std::pow(1. / std::cosh(smin / t3_warm[i]), 2);
      ne3s += t3_narm[i] * scaling * ga;
    } else {
      const double ga =
          (1. - (t3_nsg) *
                    (std::exp(-std::pow(
                        (theta * cgs::rad - t3_thetasg) /
                            t3_wsg,
                        2)))) *
          (1. + t3_ncn *
                    std::exp(-std::pow(
                        (theta * cgs::rad - t3_thetacn) /
                            t3_wcn,
                        2))) *
          std::pow(1. / std::cosh(smin / t3_warm[i]), 2);
      ne3s += t3_narm[i] * scaling * ga;
    }
  } // end of looping through arms
  return ne3s;
}

// galactic center
double YMW16::galcen(const double &xx, const double &yy,
                              const double &zz) const {
  // pos of center
  const double Xgc{50. * 0.001};
  const double Ygc{0.};
  const double Zgc{-7. * 0.001};
  const double R2gc{(xx - Xgc) * (xx - Xgc) + (yy - Ygc) * (yy - Ygc)};
  if (R2gc > 10. * t4_agc * t4_agc)
    return 0.; // timesaving
  const double Ar{
      std::exp(-R2gc / (t4_agc * t4_agc))};
  if (std::fabs(zz - Zgc) > 10. * t4_hgc)
    return 0.; // timesaving
  const double Az{
      std::pow(1. / std::cosh((zz - Zgc) / t4_hgc), 2)};
  return t4_ngc * Ar * Az;
}

// gum nebula
double YMW16::gum(const double &xx, const double &yy,
  const double &zz) const {
  if (yy < 0 or xx > 0)
    return 0.; // timesaving
  // center of Gum Nebula
  const double lc{264. * cgs::rad};
  const double bc{-4. * cgs::rad};
  const double dc{450. * 0.001};
  const double xc{dc * std::cos(bc) * std::sin(lc)};
  const double yc{r0 - dc * std::cos(bc) * std::cos(lc)};
  const double zc{dc * std::sin(bc)};
  // theta is limited in I quadrant
  const double theta{
      std::atan2(std::fabs(zz - zc),
                 std::sqrt((xx - xc) * (xx - xc) + (yy - yc) * (yy - yc)))};
  const double tantheta = std::tan(theta);
  // zp is positive
  double zp{
      (t5_agn * t5_kgn) /
      std::sqrt(1. + t5_kgn * t5_kgn /
                         (tantheta * tantheta))};
  // xyp is positive
  const double xyp{zp / tantheta};
  // alpha is positive
  const double xy_dist = {
      std::sqrt(t5_agn * t5_agn - xyp * xyp) *
      double(t5_agn > xyp)};
  const double alpha{std::atan2(t5_kgn * xyp, xy_dist) +
                        theta}; // add theta, timesaving
  const double R2{(xx - xc) * (xx - xc) + (yy - yc) * (yy - yc) +
                     (zz - zc) * (zz - zc)};
  const double r2{zp * zp + xyp * xyp};
  const double D2min{(R2 + r2 - 2. * std::sqrt(R2 * r2)) * std::sin(alpha) *
                        std::sin(alpha)};
  if (D2min > 10. * t5_wgn * t5_wgn)
    return 0.;
  return t5_ngn *
         std::exp(-D2min / (t5_wgn * t5_wgn));
}

// local bubble
double YMW16::localbubble(const double &xx, const double &yy,
                                   const double &zz, const double &ll,
                                   const double &Rlb) const {
  if (yy < 0)
    return 0.; // timesaving
  double nel{0.};
  // r_LB in ref
  const double rLB{
      std::sqrt(std::pow(((yy - 8.34 ) * 0.94 - 0.34 * zz), 2) +
                std::pow(xx, 2))};
  // l-l_LB1 in ref
  const double dl1{
      std::min(std::fabs(ll + 360. - t6_thetalb1),
               std::fabs(t6_thetalb1 - (ll)))};
  if (dl1 < 10. * t6_detlb1 or
      (rLB - Rlb) < 10. * t6_wlb1 or
      zz < 10. * t6_hlb1) // timesaving
    nel += t6_nlb1 *
           std::pow(1. / std::cosh(dl1 / t6_detlb1), 2) *
           std::pow(1. / std::cosh((rLB - Rlb) / t6_wlb1), 2) *
           std::pow(1. / std::cosh(zz / t6_hlb1), 2);
  // l-l_LB2 in ref
  const double dl2{
      std::min(std::fabs(ll + 360 - t6_thetalb2),
               std::fabs(t6_thetalb2 - (ll)))};
  if (dl2 < 10. * t6_detlb2 or
      (rLB - Rlb) < 10. * t6_wlb2 or
      zz < 10. * t6_hlb2) // timesaving
    nel += t6_nlb2 *
           std::pow(1. / std::cosh(dl2 / t6_detlb2), 2) *
           std::pow(1. / std::cosh((rLB - Rlb) / t6_wlb2), 2) *
           std::pow(1. / std::cosh(zz / t6_hlb2), 2);
  return nel;
}

// north polar spur
double YMW16::nps(const double &xx, const double &yy,
                           const double &zz) const {
  if (yy < 0)
    return 0.; // timesaving
  const double theta_LI{(t7_thetali) * cgs::rad};
  const double x_c{-10.156 * 0.001};
  const double y_c{8106.207 * 0.001};
  const double z_c{10.467 * 0.001};
  // r_LI in ref
  const double rLI{std::sqrt((xx - x_c) * (xx - x_c) +
                                (yy - y_c) * (yy - y_c) +
                                (zz - z_c) * (zz - z_c))};
  const double theta{std::acos(((xx - x_c) * (std::cos(theta_LI)) +
                                   (zz - z_c) * (std::sin(theta_LI))) /
                                  rLI) /
                        cgs::rad};
  if (theta > 10. * t7_detthetali or
      (rLI - t7_rli) > 10. * t7_wli)
    return 0.; // timesaving
  return (t7_nli) *
         std::exp(-std::pow(
             (rLI - t7_rli) / t7_wli, 2)) *
         std::exp(-std::pow(theta / t7_detthetali, 2));
}
