/* Copyright (C) 2005-2011 M. T. Homer Reid
 *
 * This file is part of SCUFF-EM.
 *
 * SCUFF-EM is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * SCUFF-EM is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/*
 * MomentPFT.cc  -- libscuff class method for computing PFT from
 *               -- Cartesian multipole moments
 *
 * homer reid    -- 3/2016
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include <fenv.h>

#include <libhrutil.h>

#include "libscuff.h"
#include "libscuffInternals.h"

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif
#ifdef USE_OPENMP
#  include <omp.h>
#endif

#define II cdouble(0,1)

using namespace scuff;

namespace scuff { 

void CalcGC(double R[3], cdouble Omega,
            cdouble EpsR, cdouble MuR,
            cdouble GMuNu[3][3], cdouble CMuNu[3][3],
            cdouble GMuNuRho[3][3][3], cdouble CMuNuRho[3][3][3]);

inline cdouble VecHDot(cdouble *V1, cdouble *V2)
{ return conj(V1[0])*V2[0]+conj(V1[1])*V2[1]+conj(V1[2])*V2[2]; }


/***************************************************************/
/* contribution of surface #ns to its own PFT                  */
/***************************************************************/
void GetMomentPFTSelfTerm(int ns, cdouble Omega, HMatrix *PM, double PFT[NUMPFT])
{
  double k3   = real(Omega)*real(Omega)*real(Omega);
  double k4   = real(Omega)*k3;
  //double k5   = real(Omega)*k4;
  double PPF  = k4/(12.0*M_PI);
  double FPF  = -TENTHIRDS*k4*ZVAC/(12.0*M_PI);
  // double FPF2 = -TENTHIRDS*k5/(120.0*M_PI);
  double TPF  = -TENTHIRDS*k3*ZVAC/(6.0*M_PI);

  cdouble P[3], M[3];
  PM->GetEntries(ns, "0:2", P);
  PM->GetEntries(ns, "3:5", M);

  PFT[PFT_PABS]=0.0;
  PFT[PFT_PSCAT]=PPF*real( ZVAC*VecHDot(P,P) + VecHDot(M,M)/ZVAC );

  for(int Mu=0; Mu<3; Mu++)
   { 
     int MP1=(Mu+1)%3, MP2=(Mu+2)%3;

     PFT[PFT_XFORCE+Mu]
      = FPF*real( conj(M[MP1])*P[MP2] - conj(M[MP2])*P[MP1] );

     PFT[PFT_XTORQUE+Mu]
      = -1.0*TPF*(  (real(P[MP1])*imag(P[MP2]) - real(P[MP2])*imag(P[MP1]))
                   +(real(M[MP1])*imag(M[MP2]) - real(M[MP2])*imag(M[MP1]))
                 );
   };

}


/***************************************************************/
/***************************************************************/
/***************************************************************/
void GetMomentPFTContribution(RWGGeometry *G, int nsa, int nsb,
                              cdouble Omega, HMatrix *PM, HMatrix *PMResolved,
                              double PFT[NUMPFT])
{
   cdouble Pa[3], Ma[3], Pb[3], Mb[3];
   PM->GetEntries(nsa, "0:2", Pa);
   PM->GetEntries(nsa, "3:5", Ma);
   PM->GetEntries(nsb, "0:2", Pb);
   PM->GetEntries(nsb, "3:5", Mb);

  cdouble PKa[3], MNa[3], PKb[3], MNb[3];
  for(int Mu=0; Mu<3; Mu++)
   { PKa[Mu] =  PMResolved->GetEntry(nsa,Mu + 0*3);
     MNa[Mu] =  PMResolved->GetEntry(nsa,Mu + 1*3);
     PKb[Mu] =  PMResolved->GetEntry(nsb,Mu + 0*3);
     MNb[Mu] =  PMResolved->GetEntry(nsb,Mu + 1*3);
   };

   cdouble Gij[3][3], Cij[3][3], dGijk[3][3][3], dCijk[3][3][3];
   double R[3];
   VecSub(G->Surfaces[nsa]->Origin, G->Surfaces[nsb]->Origin, R);
   cdouble EpsR=1.0, MuR=1.0;
   CalcGC(R, Omega, EpsR, MuR, Gij, Cij, dGijk, dCijk);

   double k=real(Omega), k2=k*k, k3=k2*k;
   memset(PFT, 0, NUMPFT*sizeof(double));
   for(int i=0; i<3; i++)
    for(int j=0; j<3; j++)
     { 
       cdouble PPpMM=conj(PKa[i])*PKb[j] + conj(MNa[i])*MNb[j];
       cdouble PMmMP=conj(PKa[i])*MNb[j] - conj(MNa[i])*PKb[j];

       PFT[PFT_PSCAT]
        += 0.5*real( II*k3*ZVAC*( PPpMM*Gij[i][j] + PMmMP*Cij[i][j]));

       PFT[PFT_XFORCE]
        -= 0.5*TENTHIRDS*imag( II*k2*ZVAC*( PPpMM*dGijk[i][j][0] + PMmMP*dCijk[i][j][0]));

       PFT[PFT_YFORCE] 
        -= 0.5*TENTHIRDS*imag( II*k2*ZVAC*( PPpMM*dGijk[i][j][1] + PMmMP*dCijk[i][j][1]));

       PFT[PFT_ZFORCE]
        -= 0.5*TENTHIRDS*imag( II*k2*ZVAC*( PPpMM*dGijk[i][j][2] + PMmMP*dCijk[i][j][2]));

     };

   for(int Mu=0; Mu<3; Mu++)
    { int Nu=(Mu+1)%3, Rho=(Mu+2)%3;
      for(int Sigma=0; Sigma<3; Sigma++)
       { 
         cdouble PPpMM, PMmMP;

         PPpMM=conj(PKa[Nu])*PKb[Sigma] + conj(MNa[Nu])*MNb[Sigma];
         PMmMP=conj(PKa[Nu])*MNb[Sigma] - conj(MNa[Nu])*PKb[Sigma];

         PFT[PFT_XTORQUE + Mu]
          -= 0.5*TENTHIRDS*imag( II*k2*ZVAC*PPpMM*Gij[Rho][Sigma]
                                +II*k2*ZVAC*PMmMP*Cij[Rho][Sigma]);

         PPpMM=conj(PKa[Rho])*PKb[Sigma] + conj(MNa[Rho])*MNb[Sigma];
         PMmMP=conj(PKa[Rho])*MNb[Sigma] - conj(MNa[Rho])*PKb[Sigma];

         PFT[PFT_XTORQUE + Mu]
          += 0.5*TENTHIRDS*imag( II*k2*ZVAC*PPpMM*Gij[Nu][Sigma]
                                +II*k2*ZVAC*PMmMP*Cij[Nu][Sigma]);

       };
    };

}

/***************************************************************/
/***************************************************************/
/***************************************************************/
void GetExtinctionMomentPFT(RWGGeometry *G, int ns,
                            IncField *IF, cdouble Omega,
                            HMatrix *PM, HMatrix *PMResolved,
                            double PFT[NUMPFT])
{
  // get incident fields and derivatives at body origin
  cdouble EH[6], dEH[3][6];
  IF->GetFields(G->Surfaces[ns]->Origin, EH);
  IF->GetFieldGradients(G->Surfaces[ns]->Origin, dEH);
  cdouble *E, *H, *dE[3], *dH[3];
   E    =  EH    + 0*3;  H    =  EH    + 1*3;
  dE[0] = dEH[0] + 0*3; dH[0] = dEH[0] + 1*3;
  dE[1] = dEH[1] + 0*3; dH[1] = dEH[1] + 1*3;
  dE[2] = dEH[2] + 0*3; dH[2] = dEH[2] + 1*3;

  // note: the magnetic-moment contributions to PFT
  // are generally expressed in terms of the B-field instead
  // of the H-field, i.e. energy = M \cdot B, etc.
  // since (in our units with c=1) the B-field is ZVAC* the H-field,
  // we need to put in an extra factor of ZVAC, which we 
  // conveniently here put into the magnetic moment M.
  cdouble P[3], M[3];
  for(int Mu=0; Mu<3; Mu++)
   { P[Mu] = conj(PM->GetEntry(ns,Mu + 0*3));
     M[Mu] = ZVAC*conj(PM->GetEntry(ns,Mu + 1*3));
   };

  cdouble PK[3], PN[3], MK[3], MN[3];
  for(int Mu=0; Mu<3; Mu++)
   { PK[Mu] =       conj(PMResolved->GetEntry(ns,Mu + 0*3));
     MN[Mu] =  ZVAC*conj(PMResolved->GetEntry(ns,Mu + 1*3));
     MK[Mu] =       conj(PMResolved->GetEntry(ns,Mu + 2*3));
     PN[Mu] = -ZVAC*conj(PMResolved->GetEntry(ns,Mu + 3*3));
   };
     
  memset(PFT, 0, NUMPFT*sizeof(double));
  for(int i=0; i<3; i++)
   { 
 //    PFT[PFT_PABS]    -= 0.5*imag(Omega*( P[i]*E[i] + M[i]*H[i] ));
//     PFT[PFT_XFORCE]  += 0.5*TENTHIRDS*real( P[i]*dE[0][i] + M[i]*dH[0][i]);
//     PFT[PFT_YFORCE]  += 0.5*TENTHIRDS*real( P[i]*dE[1][i] + M[i]*dH[1][i]);
//     PFT[PFT_ZFORCE]  += 0.5*TENTHIRDS*real( P[i]*dE[2][i] + M[i]*dH[2][i]);
//     PFT[PFT_XTORQUE] += 0.5*TENTHIRDS*real( P[1]*E[2]-P[2]*E[1] + M[1]*H[2]-M[2]*H[1]);
//     PFT[PFT_YTORQUE] += 0.5*TENTHIRDS*real( P[2]*E[0]-P[0]*E[2] + M[2]*H[0]-M[0]*H[2]);
//     PFT[PFT_ZTORQUE] += 0.5*TENTHIRDS*real( P[0]*E[1]-P[1]*E[0] + M[0]*H[1]-M[1]*H[0]);

PFT[PFT_PABS]   -= 0.5*real(II*Omega*(PK[i]*E[i]));
PFT[PFT_PSCAT]  -= 0.5*real(II*Omega*(MN[i]*H[i]));

PFT[PFT_YFORCE]   += 0.5*TENTHIRDS*imag( II*PK[i]*dE[2][i] );
PFT[PFT_ZFORCE]   += 0.5*TENTHIRDS*imag( II*MN[i]*dH[2][i] );

   };

}

/***************************************************************/
/***************************************************************/
/***************************************************************/
HMatrix *GetMomentPFTMatrix(RWGGeometry *G, cdouble Omega, IncField *IF,
                            HVector *KNVector, HMatrix *DRMatrix,
                            HMatrix *PFTMatrix, bool Itemize)
{ 
  (void) DRMatrix;

  /***************************************************************/
  /***************************************************************/
  /***************************************************************/
  int NS = G->NumSurfaces;
  if (    (PFTMatrix==0)
       || (PFTMatrix->NR != NS)
       || (PFTMatrix->NC != NUMPFT)
     )
   ErrExit("invalid PFTMatrix in MomentPFT");

  /***************************************************************/
  /* ScatteredPFT[ns] = contributions of surface #ns to          */
  /*                    scattered PFT                            */
  /***************************************************************/
  static int NSSave=0;
  static HMatrix **ScatteredPFT=0, *ExtinctionPFT=0;
  static HMatrix *PM, *PMResolved;
  if (NSSave!=NS)
   { if (ScatteredPFT)
      { for(int ns=0; ns<NSSave; ns++)
         if (ScatteredPFT[ns]) 
          delete ScatteredPFT[ns];
        free(ScatteredPFT);
       if (ExtinctionPFT)
        delete ExtinctionPFT;
       if (PM)
        delete PM;
      };
     NSSave=NS;
     ScatteredPFT=(HMatrix **)mallocEC(NS*sizeof(HMatrix));
     for(int ns=0; ns<NS; ns++)
      ScatteredPFT[ns]=new HMatrix(NS, NUMPFT);
     ExtinctionPFT=new HMatrix(NS, NUMPFT);
     PM=new HMatrix(NS, 6, LHM_COMPLEX);
     PMResolved=new HMatrix(NS, 12, LHM_COMPLEX);
   };

  /*--------------------------------------------------------------*/
  /*--------------------------------------------------------------*/
  /*--------------------------------------------------------------*/
  G->GetDipoleMoments(Omega, KNVector, PM, PMResolved);

  /*--------------------------------------------------------------*/
  /*- loop over all surfaces to moment PFT on that surface        */
  /*--------------------------------------------------------------*/
  for(int ns=0; ns<NS; ns++)
   ScatteredPFT[ns]->Zero();

  bool UseSymmetry=true;
  char *s=getenv("SCUFF_MOMENTPFT_SYMMETRY");
  if (s && s[0]=='0')
   { UseSymmetry=false;
     Log("Not using symmetry in Moment PFT calculation.");
   };

  for(int ns=0; ns<NS; ns++)
   ScatteredPFT[ns]->Zero();

  for(int nsa=0; nsa<NS; nsa++)
   for(int nsb=(UseSymmetry ? nsa : 0); nsb<NS; nsb++)
    { 
      double PFT[NUMPFT];

      if (nsa==nsb)
       GetMomentPFTSelfTerm(nsa, Omega, PM, PFT);
      else
       GetMomentPFTContribution(G, nsa, nsb, Omega, PM, PMResolved, PFT);

      for(int nq=0; nq<NUMPFT; nq++)
       ScatteredPFT[nsb]->AddEntry(nsa, nq, PFT[nq]);

      if (UseSymmetry && nsa!=nsb)
       { ScatteredPFT[nsa]->AddEntry(nsb, PFT_PSCAT, PFT[PFT_PSCAT]);
         for(int nq=PFT_XFORCE; nq<NUMPFT; nq++)
          ScatteredPFT[nsa]->AddEntry(nsb, nq, -1.0*PFT[nq]);
       };
    };

  /***************************************************************/
  /* get incident-field contributions ****************************/
  /***************************************************************/
  ExtinctionPFT->Zero();
  if (IF)
   { G->UpdateIncFields(IF, Omega);
     for(int ns=0; ns<NS; ns++)
      { double PFT[NUMPFT];
        GetExtinctionMomentPFT(G, ns, IF, Omega, PM, PMResolved, PFT);
        ExtinctionPFT->SetEntriesD(ns,":",PFT);
      };
   };
   
  /***************************************************************/
  /* sum scattered contributions from all surfaces plus          */
  /* extinction contributions to get total PFT.                  */
  /* note that the formula for total PFT is                      */
  /* Q^{full} = Q^{extinction} - Q^{scattering}                  */
  /* so the scattering contributions enter with a minus sign     */
  /* (except for the scattered power).                           */
  /***************************************************************/
  PFTMatrix->Zero();
  for(int nsa=0; nsa<NS; nsa++)
   for(int nq=0; nq<NUMPFT; nq++)
    { 
      PFTMatrix->AddEntry(nsa,nq,ExtinctionPFT->GetEntry(nsa,nq));

      for(int nsb=0; nsb<NS; nsb++)
       { 
         if (nq==PFT_PABS)
          PFTMatrix->AddEntry(nsa,nq,-1.0*ScatteredPFT[nsb]->GetEntry(nsa,PFT_PSCAT));
         else if (nq==PFT_PSCAT)
          PFTMatrix->AddEntry(nsa,nq,+1.0*ScatteredPFT[nsb]->GetEntry(nsa,PFT_PSCAT));
         else // force or torque 
          PFTMatrix->AddEntry(nsa,nq,-1.0*ScatteredPFT[nsb]->GetEntry(nsa,nq));
       };
    };
  
  /***************************************************************/
  /***************************************************************/
  /***************************************************************/
  char *ss=getenv("SCUFF_ITEMIZE_PFT");
  if (ss && ss[0]=='1')
   Itemize=true;
  if (Itemize)
   { 
     static bool WrotePreamble=false;
     for(int nsa=0; nsa<NS; nsa++)
      { FILE *f=vfopen("%s.%s.MomentPFT","a",
                        GetFileBase(G->GeoFileName),
                        G->Surfaces[nsa]->Label);
        if (!f) continue;
        if (!WrotePreamble)
         { fprintf(f,"# Moment contributions to surface %s\n",G->Surfaces[nsa]->Label);
           fprintf(f,"# columns: \n");
           fprintf(f,"# 1 frequency \n");
           fprintf(f,"# 2 destination surface label \n");
           fprintf(f,"# 03-10 PAbs, PScat, Fxyz, Txyz (total)\n");
           fprintf(f,"# 11-21 PAbs, PScat, Fxyz, Txyz, 0 0 0 (extinction)\n");
           int nc=22;
           for(int nsb=0; nsb<NS; nsb++, nc+=NUMPFT)
            fprintf(f,"# %i-%i PAbs, PScat, Fxyz, Txyz, 0 0 0 (surface %s)\n",nc,nc+NUMPFT-1,G->Surfaces[nsb]->Label);
         };
        fprintf(f,"%e %s ",real(Omega),G->Surfaces[nsa]->Label);
        for(int nq=0; nq<NUMPFT; nq++)
         fprintf(f,"%e ",PFTMatrix->GetEntryD(nsa,nq));
        for(int nq=0; nq<NUMPFT; nq++)
         fprintf(f,"%e ",ExtinctionPFT->GetEntryD(nsa,nq));
        for(int nsb=0; nsb<NS; nsb++)
         for(int nq=0; nq<NUMPFT; nq++)
          fprintf(f,"%e ",ScatteredPFT[nsb]->GetEntryD(nsa,nq));
        fprintf(f,"\n");
        fclose(f);
      };
     WrotePreamble=true;
   };

  return PFTMatrix;
}
  

} // namespace scuff