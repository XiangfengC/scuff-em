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
 * GTransformation.cc -- a very simple mechanism for handling geometric 
 *                    -- transformations (displacements and rotations)
 *                        
 * homer reid         -- 11/2011
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <libhrutil.h>

#include "GTransformation.h"
#include "libscuff.h" // for Vec* functions

#define MAXSTR 1000
#define MAXTOK 50

namespace scuff {

/***************************************************************/
// Identity transformation:
void GTransformation::Reset()
{ 
  DX[0] = DX[1] = DX[2] = 
    M[0][1] = M[0][2] = M[1][0] = M[1][2] = M[2][0] = M[2][1] = 0.0;
  M[0][0] = M[1][1] = M[2][2] = 1.0;
}

/***************************************************************/
/***************************************************************/
// Constructors:

GTransformation::GTransformation() { Reset(); }

GTransformation::GTransformation(const double dx[3]) {
  Reset();
  Displace(dx);
}

GTransformation::GTransformation(const double ZHat[3], double ThetaDegrees) {
  Reset();
  Rotate(ZHat, ThetaDegrees);
}

GTransformation::GTransformation(const char *TransformString, char **ErrMsg) {
  Reset();
  Parse(TransformString, ErrMsg);
}

GTransformation::GTransformation(char **Tokens, int NumTokens, char **ErrMsg,
				 int *TokensConsumed) {
  Reset();
  Parse(Tokens, NumTokens, ErrMsg, TokensConsumed);
}

GTransformation::GTransformation(const GTransformation &G) {
  VecCopy(G.DX, DX);
  memcpy(&M[0][0], &G.M[0][0], sizeof(double)*9);
}

GTransformation::GTransformation(const  GTransformation *G) {
  VecCopy(G->DX, DX);
  memcpy(&M[0][0], &G->M[0][0], sizeof(double)*9);
}

void GTransformation::operator=(const GTransformation &G) {
  VecCopy(G.DX, DX);
  memcpy(&M[0][0], &G.M[0][0], sizeof(double)*9);
}

/***************************************************************/
/* a transformation that displaces through vector Scale*DX     */
/***************************************************************/
void GTransformation::Displace(const double dx[3], const double Scale) {
  VecPlusEquals(DX, Scale, dx);
}

/***************************************************************/
/* a transformation that rotates through Theta degrees         */
/* (DEGREES, NOT RADIANS) about an axis that passes through    */
/* the origin and the point with coordinates ZHat[0..2]        */
/***************************************************************/

// rotate a 3-vector by Theta (given its cos & sin) around the UNIT vector U
static void RotateVec(const double U[3], double cosTheta, double sinTheta, 
		      double V[3]) {
  double Vpar[3], Vperp[3]; // components of V parallel/perpendicular to U
  double Vcross[3]; // U x V

  VecScale(VecCopy(U, Vpar), VecDot(U, V)); // compute Vpar
  VecSub(V, Vpar, Vperp); // compute Vperp
  VecCross(U, V, Vcross); // compute Vcross = U x V
  VecAdd(Vpar, VecAdd(VecScale(Vperp,cosTheta), VecScale(Vcross,sinTheta), V),
	 V); // V = Vpar + Vperp*cos + Vcross*sin
}

// same as RotateVec, but operates on V[0],V[stride],V[2*stride]
static void RotateVec(const double U[3], double cosTheta, double sinTheta,
                      double *V, int stride) {
  double Vtmp[3];
  Vtmp[0] = V[0]; Vtmp[1] = V[stride]; Vtmp[2] = V[2*stride]; 
  RotateVec(U, cosTheta, sinTheta, Vtmp);
  V[0] = Vtmp[0]; V[stride] = Vtmp[1]; V[2*stride] = Vtmp[2]; 
}

void GTransformation::Rotate(const double ZHat[3], double ThetaDegrees) {
  double U[3], cosTheta, sinTheta;
  VecNormalize(VecCopy(ZHat, U));

  ThetaDegrees *= (3.14159265358979323846 / 180.0); // convert to radians
  cosTheta = cos(ThetaDegrees); sinTheta = sin(ThetaDegrees);

  // apply rotation to DX:
  RotateVec(U, cosTheta, sinTheta, DX);

  // apply rotation to each column of M
  RotateVec(U, cosTheta, sinTheta, &M[0][0], 3);
  RotateVec(U, cosTheta, sinTheta, &M[0][1], 3);
  RotateVec(U, cosTheta, sinTheta, &M[0][2], 3);
}

/***************************************************************/
/* a transformation described by a character string.           */
/*                                                             */
/* the string should be of the form                            */
/*                                                             */
/*  DISPLACED xx yy zz                                         */
/*                                                             */
/* or                                                          */
/*                                                             */
/*  ROTATED tt ABOUT xx yy zz                                  */
/*                                                             */
/* in this case, the caller should look at the value returned  */
/* for the parameter ErrMsg; if ErrMsg==0 on return, the       */
/* string was successfully parsed and interpreted as a         */
/* GTransformation, and otherwise ErrMsg points to an error    */
/* message.                                                    */
/*                                                             */
/* Returns true on success, false on failure.                  */
/***************************************************************/

bool GTransformation::Parse(const char *TransformString, char **ErrMsg) {
  char Line[MAXSTR];
  char *Tokens[MAXTOK];
  int NumTokens;
  
  strncpy(Line,TransformString,MAXSTR);
  NumTokens=Tokenize(Line, Tokens, MAXTOK);
  return Parse(Tokens, NumTokens, ErrMsg, NULL);
}

bool GTransformation::Parse(char **Tokens, int NumTokens, 
			    char **ErrMsg, int *TokensConsumed)
{
 
  double Theta, ZHat[3], dx[3];

  if (NumTokens==0) {
    if (ErrMsg) *ErrMsg = strdupEC("no tranformation specified");
    if (TokensConsumed) *TokensConsumed=0;
    return false;
  }
  else if ( !StrCaseCmp(Tokens[0],"DISP") 
	    || !StrCaseCmp(Tokens[0],"DISPLACED") )
   { 
     /*--------------------------------------------------------------*/
     /*-- DISPLACED xx yy zz ----------------------------------------*/
     /*--------------------------------------------------------------*/
     if ( NumTokens<4 )
      { if (ErrMsg) *ErrMsg=vstrdup("too few values specified for %s ",Tokens[0]);
        return false;
      };

     if (    1!=sscanf(Tokens[1],"%le",dx+0)
          || 1!=sscanf(Tokens[2],"%le",dx+1)
          || 1!=sscanf(Tokens[3],"%le",dx+2)
        )
      { if (ErrMsg) *ErrMsg=vstrdup("bad value specified for %s",Tokens[0]);
        return false;
      };

     if (ErrMsg) *ErrMsg=0;
     if (TokensConsumed) *TokensConsumed=4;
     Displace(dx);
   }
  else if ( !StrCaseCmp(Tokens[0],"ROT") || !StrCaseCmp(Tokens[0],"ROTATED") )
   { 
     /*--------------------------------------------------------------*/
     /*-- ROTATED tt ABOUT xx yy zz ---------------------------------*/
     /*--------------------------------------------------------------*/
     if ( NumTokens<6 )
      { if (ErrMsg) *ErrMsg=vstrdup("too few values specified for %s ",Tokens[0]);
        return false;
      };

     if ( StrCaseCmp(Tokens[2],"ABOUT") )
      {  if (ErrMsg) *ErrMsg=vstrdup("invalid syntax for %s statement",Tokens[0]);
         return false;
      };

     if (    1!=sscanf(Tokens[1],"%le",&Theta)
          || 1!=sscanf(Tokens[3],"%le",ZHat+0)
          || 1!=sscanf(Tokens[4],"%le",ZHat+1)
          || 1!=sscanf(Tokens[5],"%le",ZHat+2)
        )
      { if (ErrMsg) *ErrMsg=vstrdup("bad value specified for %s",Tokens[0]);
        return false;
      };

     if (ErrMsg) *ErrMsg=0;
     if (TokensConsumed) *TokensConsumed=6;
     Rotate(ZHat, Theta);
   }
  else
   { 
     if (ErrMsg) *ErrMsg=vstrdup("unknown keyword %s",Tokens[0]); 
     if (TokensConsumed) *TokensConsumed=0;
     return false;
   }
  return true;
} 

/***************************************************************/
// Inversion: if Apply(X) computes Y = M*X+DX, then
//            inverse = UnApply(Y) computes X = M'*(Y-DX) = M'*Y - M'*DX

GTransformation GTransformation::Inverse() const {
  GTransformation C;

  for (int i = 0; i < 3; ++i) {
    C.DX[i] = 0.0;
    for (int j = 0; j < 3; ++j) {
      C.DX[i] -= M[j][i] * DX[j]; // -M' * DX
      C.M[i][j] = M[j][i]; // M'
    }
  }

  return C;
}

void GTransformation::Invert() {
  *this = Inverse();
}

/***************************************************************/
/* Composition: G1 + G2 or G1 - G2 form the composition of G1  */
/* with G2 or inv(G2).  The composition G1 + G2 is equivalent  */
/* to FIRST performing G2 and THEN performing G1 (= this).     */
/***************************************************************/

void GTransformation::Transform(const GTransformation *G) { // G + this
  // new Apply(X) = G.Apply(this.Apply(X)) = G.Apply(M*X + DX)
  //              = (G.M*M)*X + G.M*DX + G.DX

  G->Apply(DX); // new DX = G.M*DX + G.DX

  // new M = G.M * M
  double newM[3][3];
  for (int i = 0; i < 3; ++i)
    for (int j = 0; j < 3; ++j) {
      newM[i][j] = 0;
      for (int k = 0; k < 3; ++k)
	newM[i][j] += G->M[i][k] * M[k][j];
    }
  memcpy(&M[0][0], &newM[0][0], sizeof(double)*9);
}

GTransformation GTransformation::operator+(const GTransformation &G2) const {
  GTransformation C(G2);
  C.Transform(this);
  return C;
}

GTransformation GTransformation::operator-(const GTransformation &G2) const {
  GTransformation C(-G2);
  C.Transform(this);
  return C;
}

/***************************************************************/
/* Apply the transformation (out-of-place) to a list of NX points  */
/* X[3*nx+0, 3*nx+1, 3*nx+2] = cartesian coords of point #nx,  */
/* storing the result in XP.                                   */
/* NOTE: we allow XP == X (i.e., can operate in-place).        */
/***************************************************************/
void GTransformation::Apply(const double *X, double *XP, int NX) const {
  for (int nx = 0; nx < NX; ++nx) {
    double x0 = X[3*nx], x1 = X[3*nx+1], x2 = X[3*nx+2];
    XP[3*nx+0] = M[0][0]*x0 + M[0][1]*x1 + M[0][2]*x2 + DX[0];
    XP[3*nx+1] = M[1][0]*x0 + M[1][1]*x1 + M[1][2]*x2 + DX[1];
    XP[3*nx+2] = M[2][0]*x0 + M[2][1]*x1 + M[2][2]*x2 + DX[2];
  }
}

// same, but apply the inverse transformation:
void GTransformation::UnApply(const double *X, double *XP, int NX) const {
  for (int nx = 0; nx < NX; ++nx) {
    double x0 = X[3*nx]-DX[0], x1 = X[3*nx+1]-DX[1], x2 = X[3*nx+2]-DX[2];
    XP[3*nx+0] = M[0][0]*x0 + M[1][0]*x1 + M[2][0]*x2;
    XP[3*nx+1] = M[0][1]*x0 + M[1][1]*x1 + M[2][1]*x2;
    XP[3*nx+2] = M[0][2]*x0 + M[1][2]*x1 + M[2][2]*x2;
  }
}

void GTransformation::ApplyRotation(const double X[3], double XP[3]) const {
  double x0 = X[0], x1 = X[1], x2 = X[2];
  XP[0] = M[0][0]*x0 + M[0][1]*x1 + M[0][2]*x2;
  XP[1] = M[1][0]*x0 + M[1][1]*x1 + M[1][2]*x2;
  XP[2] = M[2][0]*x0 + M[2][1]*x1 + M[2][2]*x2;
}

void GTransformation::UnApplyRotation(const double X[3], double XP[3]) const {
  double x0 = X[0], x1 = X[1], x2 = X[2];
  XP[0] = M[0][0]*x0 + M[1][0]*x1 + M[2][0]*x2;
  XP[1] = M[0][1]*x0 + M[1][1]*x1 + M[2][1]*x2;
  XP[2] = M[0][2]*x0 + M[1][2]*x1 + M[2][2]*x2;
}

// same, but apply the inverse transformation:
/***************************************************************/
/***************************************************************/
/***************************************************************/

/********************************************************************/
/* this is a helper function for the ReadTransFile() routine below  */
/* that attempts to parse the shorthand syntax for specifying       */
/* transformations in a .trans file (i.e. a single line beginning   */
/* with the token TRANS).                                           */
/* on entry we have already verified that Tokens[0]==TRANS so that  */
/* does not need to be checked.                                     */
/********************************************************************/
GTComplex *ParseTRANSLine(sVec Tokens, char **ErrMsg)
{
  /***************************************************************/
  /* initialize a bare GTComplex *********************************/
  /***************************************************************/
  GTComplex *GTC = CreateGTComplex(Tokens[1]);
  GTransformation *CurrentGT=0;

  /***************************************************************/
  /* parse sections of the token string one at at time.          */
  /* each section is treated as though it were on a separate     */
  /* line of a TRANSFORMATION...ENDTRANSFORMATION section.       */
  /***************************************************************/
  unsigned NumTokens=Tokens.size(), nt=2; // start parsing the rest of the line at token #2
  while( nt < NumTokens )
   { 
     if ( !StrCaseCmp(Tokens[nt], "OBJECT") || !StrCaseCmp(Tokens[nt], "SURFACE") )
      { 
        if ( nt+1 == NumTokens )
         { *ErrMsg = vstrdup("empty %s statement",Tokens[nt]);
           return 0;
         }

        CurrentGT = new GTransformation();
        GTC->GTs.push_back(CurrentGT);
        GTC->SurfaceLabels.push_back(strdupEC(Tokens[nt+1]));
        nt+=2;
      }
     else if (    !StrCaseCmp(Tokens[nt], "DISP") || !StrCaseCmp(Tokens[nt], "DISPLACED")
               || !StrCaseCmp(Tokens[nt], "ROT")  || !StrCaseCmp(Tokens[nt], "ROTATED")
             )
      { 
        // allow transformations with no specified surface
        if ( CurrentGT==0 )
         { CurrentGT = new GTransformation();
           GTC->GTs.push_back(CurrentGT);
           GTC->SurfaceLabels.push_back(0);
         }

        int TokensConsumed;
	CurrentGT->Parse( &(Tokens[0])+nt, NumTokens-nt, ErrMsg, &TokensConsumed);
        if (*ErrMsg) return 0;
        nt += TokensConsumed;
      }
     else
      { *ErrMsg = vstrdup("unknown keyword %s %i",Tokens[nt],nt);
        return 0;
      }
   }

  *ErrMsg=0;
  return GTC;
}

/********************************************************************/
/* this is a helper function for the ReadTransFile() routine        */
/* below; it attempts to parse a TRANSFORMATION...ENDTRANSFORMATION */
/* section in a scuff-EM .trans file. (the file read pointer is     */
/* assumed to point to the line following TRANSFORMATION ...)       */
/* if successful, 0 is returned and *pGTC points on return to a     */
/* newly allocated GTComplex for the specified transformation.      */
/* if unsuccessful, 0 is returned and *ErrMsg is set.               */
/********************************************************************/
GTComplex *ParseTRANSFORMATIONSection(char *Tag, FILE *f, int *pLineNum, char **ErrMsg)
{
  /***************************************************************/
  /* initialize a bare GTComplex *********************************/
  /***************************************************************/
  GTComplex *GTC = CreateGTComplex(Tag);
  GTransformation *CurrentGT=0;

  /***************************************************************/
  /* parse each line in the TRANSFORMATION...ENDTRANSFORMATION   */
  /* section.                                                    */
  /***************************************************************/
  char Line[MAXSTR];
  while(fgets(Line, MAXSTR, f))
   { 
     // read line, break it up into tokens, skip blank lines and comments
     (*pLineNum)++;
     sVec Tokens=Tokenize(Line);
     int NumTokens=Tokens.size();
     if ( NumTokens==0 || Tokens[0][0]=='#' )
      continue; 

     // switch off to handle the various tokens that may be encountered
     // in a TRANSFORMATION...ENDTRANSFORMATION section
     if ( !StrCaseCmp(Tokens[0],"OBJECT") || !StrCaseCmp(Tokens[0],"SURFACE") )
      { 
        /*--------------------------------------------------------------*/
        /*-- OBJECT MyObject or SURFACE MySurface -----------------------*/
        /*--------------------------------------------------------------*/
        if (NumTokens!=2) 
         { *ErrMsg=strdupEC("OBJECT/SURFACE keyword requires one argument");
            return 0;
         }

        // save the label of the affected surface and initialize the corresponding 
        // GTransformation to the identity (subsequent DISPLACEMENTs / ROTATIONs will
        // augment this GTransformation)
        CurrentGT = new GTransformation();
        GTC->GTs.push_back(CurrentGT);
        GTC->SurfaceLabels.push_back(strdupEC(Tokens[1]));
      }
     else if ( !StrCaseCmp(Tokens[0],"ENDTRANSFORMATION") )
      { *ErrMsg=0;
        return GTC;
      }
     else 
      { /*--------------------------------------------------------------*/
        /*-- try to process the line as DISPLACED ... or ROTATED ... ---*/
        /*--------------------------------------------------------------*/
        // allow transformations with no specified surface
        if ( CurrentGT==0 )
         { CurrentGT = new GTransformation();
           GTC->GTs.push_back(CurrentGT);
           GTC->SurfaceLabels.push_back(0);
         }

        int TokensConsumed;
	CurrentGT->Parse( &(Tokens[0]), NumTokens, ErrMsg, &TokensConsumed);
        if (*ErrMsg) return 0;
        if (TokensConsumed!=NumTokens)
         { *ErrMsg = strdupEC("junk at end of line");
           return 0;
         }
      }

   } // while(fgets(Line, MAXSTR, f))

  // if we made it here, the file ended before the TRANSFORMATION
  // section was properly terminated
  *ErrMsg = strdupEC("unexpected end of file");
  return 0;
}

/***************************************************************/
/***************************************************************/
/***************************************************************/
GTComplex *CreateGTComplex(const char *Tag)
{
  GTComplex *GTC=(GTComplex *)mallocEC(sizeof(GTComplex));
  GTC->Tag=strdupEC(Tag ? Tag : "DEFAULT");
  return GTC;
}

/***************************************************************/
/* this routine reads a scuff-EM transformation (.trans) file  */
/* and returns a list of GTComplex structures. If FileName==0, */
/* the list contains a single GTComplex (identity transform).  */
/***************************************************************/
GTCList ReadTransFile(const char *FileName)
{
  GTCList GTCs;

  /***************************************************************/
  /***************************************************************/
  /***************************************************************/
  if (FileName==0)
   { 
     GTCs.push_back( CreateGTComplex() );
     Log("Using a single (empty) geometrical transformation with label %s.",GTCs[0]->Tag);
     return GTCs;
   }

  FILE *f=fopen(FileName,"r");
  if (f==0)
   ErrExit("could not open file %s",FileName);

  int LineNum=0;
  char Line[MAXSTR];
  while(fgets(Line, MAXSTR, f))
   { 
     // read line, break it up into tokens, skip blank lines and comments
     LineNum++;
     sVec Tokens=Tokenize(Line);
     int NumTokens = Tokens.size();
     if ( NumTokens==0 || Tokens[0][0]=='#' )
      continue;

     // separately handle the two possible ways to specify complices:
     //  (a) TRANSFORMATION ... ENDTRANSFORMATION sections
     //  (b) single-line transformations (TRANS ... )
     char *ErrMsg=0;
     GTComplex *GTC=0;
     if ( !StrCaseCmp(Tokens[0], "TRANSFORMATION") )
      GTC=ParseTRANSFORMATIONSection(Tokens[1], f, &LineNum, &ErrMsg);
     else if ( !StrCaseCmp(Tokens[0], "TRANS") )
      GTC=ParseTRANSLine(Tokens, &ErrMsg);
     else
      ErrMsg=vstrdup("unknown token %s",Tokens[0]);

     if (ErrMsg)
      ErrExit("%s:%i: %s",FileName,LineNum,ErrMsg);

     // if that was successful, add the new GTComplex to the list
     GTCs.push_back(GTC);
   } // while(fgets(Line, MAXSTR, f))

  fclose(f);
  Log("Read %i geometrical transformations from file %s.",GTCs.size(),FileName); 
  return GTCs;
}

/***************************************************************/
/***************************************************************/
/***************************************************************/
void DestroyGTCList(GTCList GTCs)
{
  for(unsigned nt=0; nt<GTCs.size(); nt++)
   { GTComplex *GTC=GTCs[nt];
     free(GTC->Tag);
     for(unsigned ns=0; ns<GTC->GTs.size(); ns++)
      { if (GTC->SurfaceLabels[ns]) free(GTC->SurfaceLabels[ns]);
        delete GTC->GTs[ns];
      }
   }
}

} // namespace scuff