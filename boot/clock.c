/* Derived from linux-2.6.x/arch/arm/mach-mxc91231/clock.c */

/*
 *  A module for creating rebootless custom image boot support.
 *
 *  Copyright (C) 2010 XVilka <xvilka at gmail.com>
 *
 *  Inspired by 2ndboot by dimich: http://hg.ezxdev.org/2ndboot/
 * 
 *  This file is part of 2ndboot-ng.
 *
 *  2ndboot-ng is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  Foobar is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *   along with 2ndboot-ng.  If not, see <http://www.gnu.org/licenses/>.
 * 
*/

#include "types.h"
#include "common.h"
#include "string.h"
#include "division.h"
#include "clock_priv.h"

#ifdef __PLAT_TI_OMAP3430__
#include "plat/omap3430.h"
#endif

#ifdef __PLAT_FREESCALE_IMX31__
#include "plat/mxc91231.h"
#endif

#define SEXT(nbits,val)         ((((int)(val)) << (32-(nbits))) >> (32-(nbits)))

#define MXC_PLL_REF_CLK                         26000000
static unsigned long CRM_SMALL_DIV[] = { 2, 3, 4, 5, 6, 8, 10, 12 };
static unsigned long pll_base[] = {
  (unsigned long)(PLL0_BASE_ADDR),
  (unsigned long)(PLL1_BASE_ADDR),
  (unsigned long)(PLL2_BASE_ADDR),
  (unsigned long)(PLL3_BASE_ADDR),
};

static void clock_refs(unsigned long *ap_unc_pat_ref, unsigned long *ap_ref_x2, unsigned long *ap_ref){
  unsigned long ap_pat_ref_div_2, ap_isel, acs, ads;
  volatile unsigned long acsr, ascsr, adcr, acder2;

  acsr = read32(MXC_CRMAP_ACSR);
  ascsr = read32(MXC_CRMAP_ASCSR);
  adcr = read32(MXC_CRMAP_ADCR);
  acder2 = read32(MXC_CRMAP_ACDER2);

  ap_isel = ascsr & MXC_CRMAP_ASCSR_APISEL;
  ap_pat_ref_div_2 =
    ((ascsr >> MXC_CRMAP_ASCSR_AP_PATDIV2_OFFSET) & 0x1) + 1;

  *ap_ref_x2 =  MXC_PLL_REF_CLK;
  ads = ((acsr >> MXC_CRMAP_ACSR_ADS_OFFSET_PASS2) & 0x1);
  acs = acsr & MXC_CRMAP_ACSR_ACS;

  if (acs == 0) {
    if (ads == 0) {
      *ap_ref = MXC_PLL_REF_CLK / ap_pat_ref_div_2;
    } else {
      *ap_ref = MXC_PLL_REF_CLK * (ap_isel + 1);
    }
    *ap_unc_pat_ref = *ap_ref / 2;
  } else {
    *ap_ref = MXC_PLL_REF_CLK / ap_pat_ref_div_2;
    *ap_unc_pat_ref = *ap_ref;
  }
}

static unsigned long clock_get_pll(int pll_num) {
  signed long mfi, mfn, mfd, pdf, ref_clk, pll_out;
  signed long val1;
  signed long long val2;
  volatile unsigned long dp_op, dp_mfd, dp_mfn, pll_hfsm;
  unsigned long pllbase, flags;

  if (pll_num > (sizeof(pll_base) / sizeof(pll_base[0]))) {
    return 0;
  }

  pllbase = pll_base[pll_num];

  pll_hfsm = read32(pllbase + MXC_PLL_DP_CTL) & MXC_PLL_DP_CTL_HFSM;
  if (pll_hfsm == 0) {
    dp_op = read32(pllbase + MXC_PLL_DP_OP);
    dp_mfd = read32(pllbase + MXC_PLL_DP_MFD);
    dp_mfn = read32(pllbase + MXC_PLL_DP_MFN);
  } else {
    dp_op = read32(pllbase + MXC_PLL_DP_HFS_OP);
    dp_mfd = read32(pllbase + MXC_PLL_DP_HFS_MFD);
    dp_mfn = read32(pllbase + MXC_PLL_DP_HFS_MFN);
  }
  pdf = (signed long)(dp_op & MXC_PLL_DP_OP_PDF_MASK);
  mfi = (signed long)((dp_op >> MXC_PLL_DP_OP_MFI_OFFSET) &
		      MXC_PLL_DP_OP_PDF_MASK);
  mfi = (mfi <= 5) ? 5 : mfi;
  mfd = (signed long)(dp_mfd & MXC_PLL_DP_MFD_MASK);
  mfn = (signed long)(dp_mfn & MXC_PLL_DP_MFN_MASK);
  mfn = (mfn <= 0x4000000) ? mfn : (mfn - 0x10000000);

  ref_clk = MXC_PLL_REF_CLK;

  mfn = SEXT(27,mfn);

  val1 = 2 * ref_clk * mfi;
  val2 = 2 * ref_clk * (signed long long)mfn;
  val2 = do_div(val2, mfd);
  pll_out = val1 + (signed long)val2;
  pll_out = do_div(pll_out, (pdf + 1));
  
  return pll_out;
}
                                                                                               
static unsigned long clock_get_main_divider(const char *str) {
  uint32_t acdr, acder2;
  unsigned long div = 0;

  acdr = read32(MXC_CRMAP_ACDR);
  acder2 = read32(MXC_CRMAP_ACDER2);

  if (strcmp(str, "cpu") == 0) {
    div = (acdr & MXC_CRMAP_ACDR_ARMDIV_MASK) >> MXC_CRMAP_ACDR_ARMDIV_OFFSET;
    div = (div > 7) ? 1 : (CRM_SMALL_DIV[div]);
  } else if (strcmp(str, "ipg") == 0) {
    div = acdr & MXC_CRMAP_ACDR_IPDIV_MASK;
    div = (div == 0) ? 16 : div;
  }
  return div;
}

unsigned long clock_get(const char *str) {
  unsigned long ap_unc_pat_ref, ap_ref_x2, ap_ref_clk;
  unsigned long ap_clk_pre_dfs;
  unsigned long clock_pll, clock_main_divider;
  uint32_t acsr;

  clock_refs(&ap_unc_pat_ref, &ap_ref_x2, &ap_ref_clk);
  acsr = read32(MXC_CRMAP_ACSR);

  if ((acsr & MXC_CRMAP_ACSR_ACS) == 0) {
    ap_clk_pre_dfs =  ap_ref_clk;
  } else {
    uint32_t apsel;

    apsel = (acsr & MXC_CRMAP_ASCSR_APSEL_MASK) >> MXC_CRMAP_ASCSR_APSEL_OFFSET;
    clock_pll = clock_get_pll(apsel);
    clock_main_divider = clock_get_main_divider("cpu");
    ap_clk_pre_dfs = do_div(clock_pll, clock_main_divider);
  }

  if (strcmp(str, "ipg") == 0) {
	clock_main_divider = clock_get_main_divider("ipg");
    return do_div(ap_clk_pre_dfs, clock_main_divider);
  }
  return 0;
}
