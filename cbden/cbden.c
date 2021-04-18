/*-------------------------------------------------------------------*/
/* CBDEN.C         Rushikesh Sane.                                   */
/*                                                                   */
/* Density-based modification of the Random Swap (RS) algorithm.     */
/* Based on the original CBRS.C implementation by Pasi Fränti.       */
/*                                                                   */
/* ChangeLog:                                                        */
/*                                                                   */
/* 0.06: 16.3.18  RS: Cleaned up unused parts of the code.           */
/* 0.05:  3.7.17  RS: Removed radius parameter.                      */
/* 0.04: 10.4.17  RS: Renamed to CBDEN.                              */
/* 0.03:  7.5.17  RS: Cosmetic changes to parameter names.           */
/* 0.02: 10.4.17  RS: Added radius parameter.                        */
/* 0.01: 22.2.17  RS: Initial version based on CBRS.C.               */
/*-------------------------------------------------------------------*/

#define ProgName        "CBDEN"
#define VersionNumber   "Version 0.06"  /* JP */
#define LastUpdated     "15.3.2018"
#define FACTFILE        "cbden.fac"

/* ------------------------------------------------------------------- */

#include "parametr.c"
#include "cb.h"
#include "file.h"
#include "interfc.h"
#include "memctrl.h"
#include "random.h"
#include "reporting.h"
#include "denrs.h"


/* ======================== PRINT ROUTINES =========================== */


void PrintInfo(void)
{
  PrintMessage("%s\t%s\t%s\n\n"
        "Density-based random swap algorithm.\n"
        "Use: %s [%coption] <dataset> [initial cb/pa] <codebook>\n"
        "For example: %s birch3 initial tmp\n\n  Options:\n",
        ProgName, VersionNumber, LastUpdated, ProgName, OPTION_SYMBOL,
        ProgName);
  PrintOptions();
  PrintMessage("\n");
}


/* ------------------------------------------------------------------ */


static char* PrintInitialData(char *TSName, char *InName,
             char *OutCBName, char* OutPAName, int useInitial)
{
  char* str;
  str = DenRSInfo();

  if (Value(QuietLevel) >= 2)
    {
    PrintMessage("\n%s\n\n", str); 
    PrintMessage("Dataset                   = %s \n", TSName);

    if (useInitial)
      {
      PrintMessage("Initial ");
      if (useInitial == 1)  PrintMessage("codebook");
      else                  PrintMessage("partitioning");
      PrintMessage("          = %s \n", InName);
      if(Value(MonitorProgress))
        {
        PrintMessage("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! \n");
        PrintMessage("!!! Progress Monitor mode selected:       !!! \n");
        PrintMessage("!!! Initial codebook is used as REFERENCE !!! \n");
        PrintMessage("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! \n");
        }
      }

    PrintMessage("Codebook                  = %s\n", OutCBName);

    if (*OutPAName)
      {
      PrintMessage("Partition file            = %s\n", OutPAName);
      }

    if (Value(Iterations)==0)    
       {
       PrintMessage("Number of iterations      = AUTOMATIC\n");
       }
 
    PrintSelectedOptions();
    PrintMessage("\n");
  }
  return str;
}


/* ===========================  MAIN  ================================ */


int main(int argc, char* argv[])
{
  TRAININGSET   TS;
  CODEBOOK      CB;
  PARTITIONING  P;
  char          TSName[MAXFILENAME] = {'\0'};
  char          InName[MAXFILENAME] = {'\0'};
  char          OutCBName[MAXFILENAME] = {'\0'};
  char          OutPAName[MAXFILENAME] = {'\0'};
  int           useInitial = 0; 
  char*         genMethod;
  ParameterInfo paraminfo[3] = { { TSName,  FormatNameTS, 0, INFILE },
                                 { InName,  FormatNameCB, 1, INFILE },
                                 { OutCBName,  FormatNameCB, 0, OUTFILE } };

  ParseParameters(argc, argv, 3, paraminfo);
  initrandom(Value(RandomSeed));
  
  if (Value(SavePartition)) 
    {
    PickFileName(OutCBName, OutPAName);
    CheckFileName(OutPAName, FormatNamePA);
    }
  
  TS = CheckParameters(TSName, OutCBName, OutPAName, InName, 
       Value(Clusters), Value(OverWrite));
  
  useInitial = ReadInitialCBorPA(InName, Value(Clusters), &TS, &CB, &P);
  
  genMethod = PrintInitialData(TSName, InName, OutCBName, 
              OutPAName, useInitial);
    
  if (PerformDenRS(&TS, &CB, &P, Value(Iterations), 
      Value(KMeansIterations), Value(Deterministic), 
      Value(QuietLevel), useInitial, Value(MonitorProgress)))
    {
    ErrorMessage("ERROR: Clustering failed!\n");
    FreeCodebook(&TS);
    FreeCodebook(&CB);
    FreePartitioning(&P);
    free(genMethod);
    ExitProcessing(FATAL_ERROR);
    }

  AddGenerationMethod(&CB, genMethod); 
  WriteCodebook(OutCBName, &CB, Value(OverWrite));
  
  if (Value(SavePartition))
    {
    WritePartitioning(OutPAName, &P, &TS, Value(OverWrite));
    }
  
  FreeCodebook(&TS);
  FreeCodebook(&CB);
  FreePartitioning(&P);
  free(genMethod);
 
  return EVERYTHING_OK;
} 


/* ----------------------------------------------------------------- */
