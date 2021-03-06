/*--------------------------------------------------------------------*/
/* DENRS.C         Rushikesh Sane                                     */
/*                                                                    */
/* Density-based random swap algorithm.                               */
/*                                                                    */
/* Based on the original random swap implementation (CBRS) by Marko   */
/* Tuononen and Pasi Fränti. This modified version calculates         */
/* weights for each centroid based on densities of the clusters.      */
/* The distances from centroids to vectors are then calculated as     */
/* weighted distances.                                                */
/*                                                                    */
/* ChangeLog:                                                         */
/*                                                                    */
/* 0.12:  16.3.18 RS: Clean up unused parts of the code.              */
/* 0.11:  11.3.18 RS: Density calculation based on dens(i)=n/md(i).   */
/* 0.10:  10.3.18 RS: Removed weight trial-and-error.                 */
/* 0.09:  20.2.18 RS: TSE-based weight calculation.                   */
/* 0.08: 10.12.17 RS: Print updated weights during clustering.        */
/* 0.07:  20.9.17 RS: Try to calculate weights during clustering.     */
/* 0.06:   3.7.17 RS: Use fixed weights and print centroid weights.   */
/* 0.05:  31.5.17 RS: Alternative algorithm based on mean distance.   */
/* 0.04:  21.5.17 RS: Minor cleanup, renamed functions and variables. */
/* 0.03:   7.5.17 RS: Fix to MSE calculation to use centroid weights. */
/* 0.02:  10.4.17 RS: Nearby points calculation based on radius.      */
/* 0.01:   9.3.17 RS: Initial version based on RS.C.                  */
/*--------------------------------------------------------------------*/


#define ProgName       "DENRS"
#define VersionNumber  "Version 0.12"
#define LastUpdated    "15.3.2018"  /* JP */

/* converts ObjectiveFunction values to MSE values */
#define CALC_MSE(val) (double) (val) / (TotalFreq(pTS) * VectorSize(pTS))
//#define CALC_MSE(val) (double) (val) / TotalFreq(pTS)

#define AUTOMATIC_MAX_ITER  50000
#define AUTOMATIC_MIN_SPEED 1e-5
#define min(a,b) ((a) < (b) ? (a) : (b))

/*-------------------------------------------------------------------*/

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <float.h>
#include <string.h>

#include "cb.h"
#include "random.h"
#include "interfc.h"
#include "reporting.h"
#include "file.h"
#include "memctrl.h"

/* ========================== PROTOTYPES ============================= */

int PerformDenRS(TRAININGSET *pTS, CODEBOOK *pCB, PARTITIONING *pP, int iter,
    int kmIter, int deterministic, int quietLevel, int useInitialCB,
    int monitoring);
void InitializeSolution(PARTITIONING *pP, CODEBOOK *pCB, TRAININGSET *pTS,
    int clus);
void FreeSolution(PARTITIONING *pP, CODEBOOK *pCB);
YESNO StopCondition(double currError, double newError, int iter);
llong GenerateInitialSolution(PARTITIONING *pP, CODEBOOK *pCB,
    TRAININGSET *pTS, int useInitialCB, double *weight);
void SelectRandomRepresentatives(TRAININGSET *pTS, CODEBOOK *pCB);
int SelectRandomDataObject(CODEBOOK *pCB, TRAININGSET *pTS);
void RandomCodebook(TRAININGSET *pTS, CODEBOOK *pCB);
void RandomSwap(CODEBOOK *pCB, TRAININGSET *pTS, int *j, int deterministic, 
    int quietLevel);
void LocalRepartition(PARTITIONING *pP, CODEBOOK *pCB, TRAININGSET *pTS, 
    double *weight, int j, double time, int quietLevel);
void OptimalRepresentatives(PARTITIONING *pP, TRAININGSET *pTS,
    CODEBOOK *pCB, int *active, llong *cdist, int *activeCount);
int BinarySearch(int *arr, int size, int key);
void OptimalPartition(CODEBOOK *pCB, TRAININGSET *pTS, PARTITIONING *pP, int *active,
    llong *cdist, int activeCount, llong *distance, double *weight, int quietLevel);
void KMeans(PARTITIONING *pP, CODEBOOK *pCB, TRAININGSET *pTS, llong *distance,
    double *weight, int iter, int quietLevel, double time, double *tempweight, llong currError);
llong ObjectiveFunction(PARTITIONING *pP, CODEBOOK *pCB, TRAININGSET *pTS,
    double *weight);
void CalculateDistances(TRAININGSET *pTS, CODEBOOK *pCB, PARTITIONING *pP,
    llong *distance, double *weight);
int FindSecondNearestVector(BOOKNODE *node, CODEBOOK *pCB, int firstIndex,
    llong *secondError);
int SelectClusterToBeSwapped(TRAININGSET *pTS, CODEBOOK *pCB, 
    PARTITIONING *pP, llong *distance);
char* DenRSInfo(void);
double GenerateOptimalPartitioningWithWeight(TRAININGSET* TS, CODEBOOK* CB,
    PARTITIONING* P, ERRORFTYPE errorf, double *weight);
void LocalRepartitioningWithWeight(TRAININGSET* TS,CODEBOOK* CB, PARTITIONING* P,
    double* weight, int index, DISTANCETYPE disttype);
int FindNearestVectorWithWeight(BOOKNODE* v, CODEBOOK* CB, llong* error,
    int guess, DISTANCETYPE disttype, double* weight);
double GenerateOptimalPartitioningMeanErrorWithWeight(TRAININGSET* TS,
    CODEBOOK* CB, PARTITIONING* P, DISTANCETYPE  disttype, double* weight);
llong TotalDistance(TRAININGSET* TS, CODEBOOK* CB, PARTITIONING* P, int index);
double MeanDistance(TRAININGSET* TS, CODEBOOK* CB, PARTITIONING* P, int index);
void CalculateWeights(TRAININGSET* TS, CODEBOOK* CB, PARTITIONING* P, double *weight);
void CalculateNewWeights(TRAININGSET* TS, CODEBOOK* CB, PARTITIONING* P, double *tempweight);
void PrintCentroidWeights(CODEBOOK *CB, double *weight, double *tempweight);
void CheckOverflow(llong a, llong b);
int  CheckClusterFreqs(CODEBOOK *pCB, PARTITIONING *pP);
void InitializeWeights(CODEBOOK *pCB, double *weight);
void CopyWeights(double *weight, double *tempweight, int size);
void CopyFinalWeights(double *weight, double *tempweight, int size);
int CheckIsNan(double *weight, int size);


/* ========================== FUNCTIONS ============================== */

/* Gets training set pTS (and optionally initial codebook pCB or 
   partitioning pP) as a parameter, generates solution (codebook pCB + 
   partitioning pP) and returns 0 if clustering completed successfully. 
   N.B. Random number generator (in random.c) must be initialized! */

int PerformDenRS(TRAININGSET *pTS, CODEBOOK *pCB, PARTITIONING *pP, int iter, 
int kmIter, int deterministic, int quietLevel, int useInitial, int monitoring)
{
  PARTITIONING  Pnew;
  CODEBOOK      CBnew, CBref;
  int           i, j, better;
  int           ci=0, ciPrev=0, ciZero=0, ciMax=0, PrevSuccess=0;
  int           CIHistogram[111];
  llong         currError, newError;
  llong         distance[BookSize(pTS)];
  double        weight[BookSize(pCB)], tempweight[BookSize(pCB)];
  double        c, error;
  int           stop=NO, automatic=((iter==0) ? YES : NO);
  int 			nullcluster=0;
  

  printf("\nInitial infor: TotalFreq(pTS) = %d  VectorSize(pTS) = %d\n",TotalFreq(pTS),VectorSize(pTS));
  /* Error checking for invalid parameters */ 
  if ((iter < 0) || (kmIter < 0) || (BookSize(pTS) < BookSize(pCB)))
    {
    return 1;  // Error: clustering failed
    }

  InitializeWeights(pCB, weight);
  printf("\nUseful information: TotalFreq = %d, VectorSize = %d, TotalFreq(pTS) * VectorSize(pTS) = %d \n",TotalFreq(pTS),VectorSize(pTS),TotalFreq(pTS) * VectorSize(pTS));
  /* Progress monitor uses input codebook as reference */
  if (monitoring)
    {
    CreateNewCodebook(&CBref, BookSize(pCB), pTS);
    CopyCodebook(pCB, &CBref);
    for( ci=0; ci<=100; ci++ ) CIHistogram[ci]=0;
    useInitial *= 100;  /* Special code: 0->0, 1->100, 2->200 */
    }
  InitializeSolution(&Pnew, &CBnew, pTS, BookSize(pCB));
  SetClock(&c);
  currError = GenerateInitialSolution(pP, pCB, pTS, useInitial, weight);
  error = CALC_MSE(currError);
  printf("\nTotal MSE: %lld",currError);
  printf("\n================Initialization Ends========================\n");
  if(useInitial) ciPrev = CentroidIndex(&CBref, pCB);
  else           ciPrev = 100;
  
  /* use automatic iteration count */
  if (automatic)  iter = AUTOMATIC_MAX_ITER;

  PrintHeader(quietLevel);
  PrintIterationRS(quietLevel, 0, error, 0, GetClock(c), 1);

  printf("Initial Centroids and weights");
  PrintCentroidWeights(pCB, weight, tempweight);

  /* Deterministic variant initialization */
  if (deterministic)
    {
    CalculateDistances(pTS, pCB, pP, distance, weight);
    j = SelectClusterToBeSwapped(pTS, pCB, pP, distance);
    }
  
  /* - - - - -  Random Swap iterations - - - - - */

  for (i=1; (i<=iter) && (!stop); i++)
    {
    better = NO;

    /* generate new solution */
	CopyWeights(weight, tempweight, BookSize(pCB));
	
    CopyCodebook(pCB, &CBnew);
    CopyPartitioning(pP, &Pnew);
    
    RandomSwap(&CBnew, pTS, &j, deterministic, quietLevel);
    /*printf("*******ONE MORE*******");
    PrintCentroidWeights(&CBnew, weight, tempweight);
    printf("**********************");*/
    /* tuning new solution */
    LocalRepartition(&Pnew, &CBnew, pTS, tempweight, j, c, quietLevel);
    
	
    KMeans(&Pnew, &CBnew, pTS, distance, weight, kmIter, quietLevel, c, tempweight, currError);
	CalculateNewWeights(pTS, &CBnew, &Pnew, tempweight);
	
	printf("\nRS Iteration number: %d\n",i);
    newError = ObjectiveFunction(&Pnew, &CBnew, pTS, tempweight);
	printf("\nNew SSE: %lld",newError);
	printf("\nOld SSE: %lld",currError);
    error    = CALC_MSE(newError);
    nullcluster = CheckClusterFreqs(&CBnew, &Pnew);
	
    /* Found better solution */
    if (newError < currError && !nullcluster && !CheckIsNan(tempweight,BookSize(pCB)))
      {
      /* Monitoring outputs CI-value: relative to Prev or Reference */
      if(monitoring)  
         {
         if(useInitial) ci = CentroidIndex(&CBnew, &CBref);
         else           ci = CentroidIndex(&CBnew, pCB);
         /* CI decreases: update Success histogram */
         if( (ci>=0) && (ci<ciPrev) && (ci<100) )
           {
           /* printf("XXXX Prev=%d  Curr=%d  CI=%d  CIPrev=%i  Iter=%d \n", PrevSuccess, i, ci, ciPrev, (i-PrevSuccess)); */
           CIHistogram[ci] += (i-PrevSuccess);
           if(ci>ciMax)  ciMax = ci;
           if(ci==0)     ciZero = i;
           PrevSuccess = i;
           }
         /* CI increases: report warning message */
         if( (ci>ciPrev) && (quietLevel) ) printf("!!! CI increased %i to %i at iteration %d\n", ciPrev, ci, i);
         /* Remember to update CI value */
         ciPrev = ci;
         /* If monitoring, then stop criterion is CI=0 */
         if(automatic) stop=(ci==0); 
         }

      /* Check stopping criterion */
      else if(automatic)
        {
        stop = StopCondition(currError, newError, i);
        }

      
      CopyCodebook(&CBnew, pCB);
      CopyPartitioning(&Pnew, pP);
      
      currError = newError;
      better = YES;

      //CalculateWeights(pTS, &CBnew, &Pnew, weight);
	  CopyFinalWeights(weight, tempweight, BookSize(pTS));
      if (deterministic) /* Alterantive ro Random. But why here?  */
        {
        j = SelectClusterToBeSwapped(pTS, pCB, pP, distance);
        }
		
	  //Code to generate partition files after every accepted iteration
	  printf("Accepted Centroids for iteration %d\n",i);
	  PrintCentroidWeights(&CBnew, weight, tempweight);
	  printf("New error: %lld\n",newError);
	  /*char OutPAName[20] = "Acceptedtemp_pa\0";
	  char filenumber[10];
	  sprintf(filenumber, "%d", i);
	  strcat(OutPAName, filenumber);
	  WritePartitioning(OutPAName, &Pnew, pTS, 0);*/
		
      }
	
    nullcluster = 0;
    /*printf("Centroids for iteration %d\n",i);
    PrintCentroidWeights(&CBnew, weight, tempweight);*/
	/*Printing distance array*/
	/*printf("\nDistance Array is\n");
	int d;
	for(d=0; d<BookSize(pTS); d++)
	{
	  printf("%ld\n",distance[d]);
	}*/
    /*char OutPAName[20] = "temp_pa\0";
    char filenumber[10];
    sprintf(filenumber, "%d", i);
    strcat(OutPAName, filenumber);
    WritePartitioning(OutPAName, &Pnew, pTS, 0);*/

    PrintIterationRS(quietLevel, i, error, ci, GetClock(c), better);
	printf("\n================RS Iteration %d Ends========================\n",i);
    }

  /* - - - - -  Random Swap iterations - - - - - */
  error = CALC_MSE(currError);  
  PrintFooterRS(quietLevel, i-1, error, GetClock(c));

  if(monitoring && quietLevel)  
     {
     PrintMessage("Total: %-7d   Swaps: ", ciZero);
     for( ci=0; ci<=ciMax; ci++ )
       {
       PrintMessage("%3d  ", CIHistogram[ci]);
       }
     PrintMessage("\n", ciZero);
     }

  FreeSolution(&Pnew, &CBnew);
  return 0;
}  


/*-------------------------------------------------------------------*/


void InitializeSolution(PARTITIONING *pP, CODEBOOK *pCB, TRAININGSET *pTS, 
int clus)
{
  CreateNewCodebook(pCB, clus, pTS);
  CreateNewPartitioning(pP, pTS, clus);
} 


/*-------------------------------------------------------------------*/

int CheckIsNan(double *weight, int size)
{
	int i,isnan = 0;
	for(i=0; i<size; i++)
	{
		if(isnan(weight[i]))
		{
			isnan = 1;
			break;
		}
	}
	return isnan;
}

/*-------------------------------------------------------------------*/

void FreeSolution(PARTITIONING *pP, CODEBOOK *pCB)
{
  FreeCodebook(pCB);
  FreePartitioning(pP);
} 


/*-------------------------------------------------------------------*/

            
YESNO  StopCondition(double currError, double newError, int iter)
{
  static double   currImpr=DBL_MAX, prevImpr=DBL_MAX;
  static int      prevIter=1;

  currImpr  = (double)(currError - newError) / (double)currError;
  currImpr /= (double) (iter - prevIter);
  if (AUTOMATIC_MIN_SPEED < currImpr + prevImpr)
     {
     prevImpr = currImpr;
     prevIter = iter;
     return(NO);
     }
  else  /* too slow speed, better to stop.. */
     {
     return(YES);
     }
}


/*-------------------------------------------------------------------*/


llong GenerateInitialSolution(PARTITIONING *pP, CODEBOOK *pCB, 
TRAININGSET *pTS, int useInitial, double *weight)
{
  if (useInitial == 1)
  {
    GenerateOptimalPartitioningWithWeight(pTS, pCB, pP, MSE, weight);
  } 
  else if (useInitial == 2) 
  {
    GenerateOptimalCodebookGeneral(pTS, pCB, pP, MSE);
  } 
  else
  {
    SelectRandomRepresentatives(pTS, pCB);
    GenerateOptimalPartitioningWithWeight(pTS, pCB, pP, MSE, weight);
  }
  
  return ObjectiveFunction(pP, pCB, pTS, weight);
}


/*-------------------------------------------------------------------*/
// Copied from Ismo's GenerateRandomCodebook() from cb_util.c
/*-------------------------------------------------------------------*/


void SelectRandomRepresentatives(TRAININGSET *pTS, CODEBOOK *pCB)
{
  
  int k, n, x, Unique;

  for (k = 0; k < BookSize(pCB); k++) 
    {
    do 
      {
      Unique = 1;
      x = irand(0, BookSize(pTS) - 1);
      for (n = 0; (n < k) && Unique; n++) 
         Unique = !EqualVectors(Vector(pTS, x), Vector(pCB, n), VectorSize(pCB));
      } 
    while (!Unique);

    CopyVector(Vector(pTS, x), Vector(pCB, k), VectorSize(pCB));
    VectorFreq(pCB, k) = 0;
    }

}


/*-------------------------------------------------------------------*/
/* Pasi's solution 20.9.2016                                         */
/* (1) Shuffle training set. (2) Select first k vectors.             */
/* Someone else please test this...                                  */
/*-------------------------------------------------------------------*/


void RandomCodebook(TRAININGSET *pTS, CODEBOOK *pCB)
{
  int i;

  ShuffleTS(pTS);
  for(i=0; i<BookSize(pCB); i++) 
     {
     CopyNode( &Node(pTS,i), &Node(pCB,i), VectorSize(pCB));
     }       
}


/*-------------------------------------------------------------------*/


int SelectRandomDataObject(CODEBOOK *pCB, TRAININGSET *pTS)
{
  int i, j, count = 0;
  int ok;

  do 
    {
    count++;

    /* random number generator must be initialized! */
    j = IRZ(BookSize(pTS));

    /* eliminate duplicates */
    ok = 1;
    for (i = 0; i < BookSize(pCB); i++) 
      {
      if (EqualVectors(Vector(pCB, i), Vector(pTS, j), VectorSize(pTS)))
        {
        ok = 0;
        }
      }
  } 
  while (!ok && (count <= BookSize(pTS)));   /* fixed 25.01.2005 */

  return j;
}


/*-------------------------------------------------------------------*/
/* random number generator must be initialized! */


void RandomSwap(CODEBOOK *pCB, TRAININGSET *pTS, int *j, int deterministic, 
                int quietLevel)
{
  int i;

  if (!deterministic)
    {
    *j = IRZ(BookSize(pCB));
    }

  i = SelectRandomDataObject(pCB, pTS);

  CopyVector(Vector(pTS, i), Vector(pCB, *j), VectorSize(pTS));
  if (quietLevel >= 5)  PrintMessage("Random Swap done: x=%i  c=%i \n", i, *j);
}



/*-------------------------------------------------------------------*/


void LocalRepartition(PARTITIONING *pP, CODEBOOK *pCB, TRAININGSET *pTS,
double *weight, int j, double time, int quietLevel)
{
  if (quietLevel >= 5)  PrintMessage("Local repartition of vector %i \n", j);

  /* object rejection; maps points from a cluster to their nearest cluster */
  LocalRepartitioningWithWeight(pTS, pCB, pP, weight, j, EUCLIDEANSQ);

  /* object attraction; moves vectors from their old partitions to
     a the cluster j if its centroid is closer */
  RepartitionDueToNewVectorGeneral(pTS, pCB, pP, j, EUCLIDEANSQ);

  if (quietLevel >= 3)  PrintMessage("RepartitionTime= %f   ", GetClock(time));
} 


/*-------------------------------------------------------------------*/


void LocalRepartitioningWithWeight(TRAININGSET* TS,CODEBOOK* CB, PARTITIONING* P,
double* weight, int index, DISTANCETYPE disttype)
{
  int   i, j;
  llong error;
  int   new;

  i = FirstVector(P, index);
  while (!EndOfPartition(i))
    {
    new = FindNearestVectorWithWeight(&Node(TS,i), CB, &error, Map(P,i), disttype, weight);
    j   = i;
    i   = NextVector(P,i);

    if (new != index)
      {
      ChangePartition(TS, P, new, j);
      }
    }
}


/*-------------------------------------------------------------------*/


double GenerateOptimalPartitioningWithWeight(TRAININGSET* TS, CODEBOOK* CB,
PARTITIONING* P, ERRORFTYPE errorf, double* weight)
{
  switch (errorf)
    {
    case MSE:
      {
      return GenerateOptimalPartitioningMeanErrorWithWeight(TS, CB, P, EUCLIDEANSQ, weight);
      }
    default:
      {
      /* Only MSE supported for now. */
      ErrorMessage("ERROR: Unknown error function.\n");
      ExitProcessing(-1);
      }
    }
  return(0);
}


/*-------------------------------------------------------------------*/
/* Generates an optimal partitioning from a codebook.                */
/* Takes weights of the centroids into account. Returns mean error.  */
/*-------------------------------------------------------------------*/

double GenerateOptimalPartitioningMeanErrorWithWeight(TRAININGSET* TS,
CODEBOOK* CB, PARTITIONING* P, DISTANCETYPE  disttype, double* weight)
{
  llong  error;
  llong  totalerror = 0;
  int    i;
  int    nearest;

  /* Find mapping from training vector to code vector */
  for(i = 0; i < BookSize(TS); i++)
    {
    nearest = FindNearestVectorWithWeight(&Node(TS,i),
              CB, &error, Map(P,i), disttype, weight);
    if(nearest != Map(P, i))
      {
      ChangePartition(TS, P, nearest, i);
      }
    totalerror += error * VectorFreq(TS, i);
    }

  return (double) totalerror / (double) (TotalFreq(TS) * VectorSize(TS));
}


/*-------------------------------------------------------------------*/


int FindNearestVectorWithWeight(BOOKNODE* v, CODEBOOK* CB, llong* error,
int guess, DISTANCETYPE disttype, double* weight)
{
  int   i;
  int   MinIndex = guess;
  llong e;

  *error = weight[guess] * sqrt(VectorDistance(Vector(CB, guess),
                                          v->vector,
                                          VectorSize(CB),
                                          MAXLLONG,
                                          disttype));

  for(i = 0; i < BookSize(CB); i++)
    {
    e = weight[i] * sqrt(VectorDistance(Vector(CB, i),
                                   v->vector,
                                   VectorSize(CB),
                                   MAXLLONG,
                                   disttype));
    if( e < *error )
      {
      *error   = e;
      MinIndex = i;
      if( e == 0 )
        {
        return( MinIndex );
        }
      }
    }

  return( MinIndex );
}


/*----------------------------------------------------------------------*/
/* Calculates total distance of the vectors in the given cluster.       */
/*----------------------------------------------------------------------*/

llong TotalDistance(TRAININGSET* TS, CODEBOOK* CB, PARTITIONING* P, int index)
{
  llong totaldistance = 0;
  int i = FirstVector(P, index);
  while (!EndOfPartition(i))
    {
    llong distance = sqrt(VectorDistance(Vector(TS, i), Vector(CB, index),
                                    VectorSize(CB), MAXLLONG, EUCLIDEANSQ));
    CheckOverflow(totaldistance, distance);
    totaldistance += distance;
    i = NextVector(P, i);
    }
  return totaldistance;
}


/*----------------------------------------------------------------------*/
/* Calculates the mean distance of the vectors in the given cluster.    */
/*----------------------------------------------------------------------*/

double MeanDistance(TRAININGSET* TS, CODEBOOK* CB, PARTITIONING* P, int index)
{
  return TotalDistance(TS, CB, P, index) / (double) CCFreq(P, index);
}


/*----------------------------------------------------------------------*/
/* Calculates the density of the given cluster.                         */
/*----------------------------------------------------------------------*/

double CalculateDensity(TRAININGSET* TS, CODEBOOK* CB, PARTITIONING* P, int index)
{
  /*
   * Alternatively:
   * return ((double) CCFreq(P, index)) / TotalDistance(TS, CB, P, index);
   */
  if(CCFreq(P, index) == 1 || CCFreq(P, index) == 0)
  {
	return 0.001;
  }
  return ((double) CCFreq(P, index)) / MeanDistance(TS, CB, P, index);
}

/*----------------------------------------------------------------------*/
/* Calculates temporary centroid weights with the specified method.               */
/*----------------------------------------------------------------------*/

void CalculateNewWeights(TRAININGSET* TS, CODEBOOK* CB, PARTITIONING* P, double *tempweight)
{
  int i;
  double density[BookSize(CB)];
  double totaldensity = 0.0;

  /* Calculate densities */
  for (i = 0; i < BookSize(CB); i++)
    {
    density[i] = CalculateDensity(TS, CB, P, i);

    totaldensity += density[i];
    }

  /* Calculate weights based on densities */
  for (i = 0; i < BookSize(CB); i++)
    {
    tempweight[i] = density[i] / totaldensity;
    }
}

/*-------------------------------------------------------------------*/


/*----------------------------------------------------------------------*/
/* Calculates centroid weights with the specified method.               */
/*----------------------------------------------------------------------*/

void CalculateWeights(TRAININGSET* TS, CODEBOOK* CB, PARTITIONING* P, double *weight)
{
  int i;
  double density[BookSize(CB)];
  double totaldensity = 0.0;

  /* Calculate densities */
  for (i = 0; i < BookSize(CB); i++)
    {
    density[i] = CalculateDensity(TS, CB, P, i);

    totaldensity += density[i];
    }

  /* Calculate weights based on densities */
  for (i = 0; i < BookSize(CB); i++)
    {
    weight[i] = density[i] / totaldensity;
    }
	
   

  /* To test with fixed weights, you can set the weights here. */

  /* Fixed weights for data set 2 (3 clusters) */
  /*
  weight[0] = 0.108982427363303;
  weight[1] = 0.6741309708665223;
  weight[2] = 0.21688660177017471;
  */

  /* Fixed weights for data set 3 (6 clusters) */
  /*
  weight[0] = 0.02106790516316829;
  weight[1] = 0.41617120230566335;
  weight[2] = 0.1406377044364807;
  weight[3] = 0.20898574056675104;
  weight[4] = 0.1409327584628596;
  weight[5] = 0.07220468906507695;
  */
}

/*-------------------------------------------------------------------*/
// AKTIVITEETIN PÄIVITTÄMINEN TULEE TÄNNE

/* generates optimal codebook with respect to a given partitioning */
void OptimalRepresentatives(PARTITIONING *pP, TRAININGSET *pTS, CODEBOOK *pCB, 
int *active, llong *cdist, int *activeCount)
{
  int i, j;
  VECTORTYPE v;

  j = 0;
  v = CreateEmptyVector(VectorSize(pCB));

  for(i = 0; i < BookSize(pCB); i++)
    {  
    if (CCFreq(pP, i) > 0)
      {
      CopyVector(Vector(pCB, i), v, VectorSize(pCB));
      /* calculate mean values for centroid */
      PartitionCentroid(pP, i, &Node(pCB, i));
      /* if centroid changed, cluster is active */
      if (CompareVectors(Vector(pCB, i), v, VectorSize(pCB)) != 0)
        {
        active[j] = i;
        j++;
        }
      }
    else
      {
      VectorFreq(pCB, i) = 0;
      }
    }

  FreeVector(v);
  (*activeCount) = j;
}  


/*-------------------------------------------------------------------*/
/* arr must be sorted ascending order! */


int BinarySearch(int *arr, int size, int key)
{
  int top, bottom, middle;

  top = 0;
  bottom = size - 1;
  middle = (top + bottom) / 2;

  do 
    {
    if (arr[middle] < key)     top    = middle + 1;
    else                       bottom = middle;
    middle = (top + bottom) / 2;
    } 
  while (top < bottom);

  if (arr[middle] == key)    return middle;
  else                       return -1;
}


/*-------------------------------------------------------------------*/
/* generates optimal partitioning with respect to a given codebook */
// AKTIIVINEN-PASIIVINEN VEKTORI MUUTOS


void OptimalPartition(CODEBOOK *pCB, TRAININGSET *pTS, PARTITIONING *pP, 
int *active, llong *cdist, int activeCount, llong *distance, double *weight, int quietLevel)
{
  int i, j, k;
  int nearest;
  llong error, dist;
  CODEBOOK CBact;
  
  if (quietLevel >= 5)  PrintMessage("\n Optimal Partition starts. ActiveCount=%i..\n", activeCount);

  /* all vectors are static; there is nothing to do! */
  if (activeCount < 1) return;

  /* creating subcodebook (active clusters) */
  if (quietLevel >= 5)  PrintMessage("Creating subcodebook...");
  CreateNewCodebook(&CBact, activeCount, pTS);
  for (i = 0; i < activeCount; i++) 
    {
    CopyVector(Vector(pCB, active[i]), Vector(&CBact, i), VectorSize(pCB));
    }
  if (quietLevel >= 5)  PrintMessage("Done.\n");
  
  if (quietLevel >= 5)  PrintMessage("Looping ... ");
  for(i = 0; i < BookSize(pTS); i++)
     {
     if (quietLevel >= 5)  PrintMessage(" %i ", i);
     j     = Map(pP, i);
     k     = BinarySearch(active, activeCount, j);
     dist  = weight[j] * sqrt(VectorDistance(Vector(pTS, i), Vector(pCB, j), VectorSize(pTS), MAXLLONG, EUCLIDEANSQ)); 
     
     // static vector - search subcodebook
     if (k < 0)  
       {
       nearest = FindNearestVectorWithWeight(&Node(pTS,i), &CBact, &error, 0, EUCLIDEANSQ, weight);
       nearest = (error < dist) ? active[nearest] : j;
       }
     // active vector, centroid moved closer - search subcodebook
     else if (dist < distance[i])  
       {
       nearest = FindNearestVectorWithWeight(&Node(pTS,i), &CBact, &error, k, EUCLIDEANSQ, weight);
       nearest = active[nearest];
       } 
     // active vector, centroid moved farther - FULL search
     else  
       {
       nearest = FindNearestVectorWithWeight(&Node(pTS,i), pCB, &error, j, EUCLIDEANSQ, weight);
       }
     
     if (nearest != j)  
       {
       /* closer cluster was found */
       ChangePartition(pTS, pP, nearest, i);
       distance[i] = error;
       } 
     else 
       {
       distance[i] = dist;
       }
    }

  FreeCodebook(&CBact);
  
  if (quietLevel >= 5)  PrintMessage("Optimal Partition ended.\n");
}
/*-------------------------------------------------------------------*/

void CopyWeights(double *weight, double *tempweight, int size)
{
	int i;
	for(i = 0; i < size; i++)
	{
		tempweight[i] = weight[i];
	}
}


/*-------------------------------------------------------------------*/

void CopyFinalWeights(double *weight, double *tempweight, int size)
{
	int i;
	for(i = 0; i < size; i++)
	{
		weight[i] = tempweight[i];
	}
}


/*-------------------------------------------------------------------*/
/* fast K-means implementation (uses activity detection method) */


void KMeans(PARTITIONING *pP, CODEBOOK *pCB, TRAININGSET *pTS, llong *distance, 
double *weight, int iter, int quietLevel, double time, double *tempweight, llong currError) 
{

  double starttime = GetClock(time);
  int     i, activeCount;
  int     active[BookSize(pCB)];
  llong   cdist[BookSize(pCB)];
  llong newError = currError;

  CalculateDistances(pTS, pCB, pP, distance, weight);
  
  CopyWeights(weight, tempweight, BookSize(pCB));

  double inittime = GetClock(time) - starttime;
  /* performs iter K-means iterations */
  for (i = 0; i < iter; i++)
    {
    /* OptimalRepresentatives-operation should be before 
       OptimalPartition-operation, because we have previously tuned 
       partition with LocalRepartition-operation */ 
	currError = newError;
    OptimalRepresentatives(pP, pTS, pCB, active, cdist, &activeCount);
	OptimalPartition(pCB, pTS, pP, active, cdist, activeCount, distance, tempweight, quietLevel);
    

    if (quietLevel >= 3)  
      {
      PrintIterationActivity(GetClock(time), i, activeCount, BookSize(pCB), quietLevel);
      }
	  
	CalculateNewWeights(pTS, pCB, pP, tempweight);
	/*printf("Centroids in Kmeans iteration %d",i);
	PrintCentroidWeights(pCB, weight, tempweight);
	printf("=================");*/
    //Code to display distances of points from centroids
	  /*int y;
	  printf("\nDistances are:\n");
	  for(y=0; y<BookSize(pTS);y++)
	  {
		  printf("%d\n",distance[y]);
	  }
	  printf("\nDistances printing done.\n");*/
		
	}
	//OptimalRepresentatives(pP, pTS, pCB, active, cdist, &activeCount);

  if ((quietLevel >= 4) && iter > 0) 
     {
     PrintIterationKMSummary(GetClock(time)-starttime, inittime);
     }
}


/*-------------------------------------------------------------------*/


/*-------------------------------------------------------------------*/


llong ObjectiveFunction(PARTITIONING *pP, CODEBOOK *pCB, TRAININGSET *pTS, double *weight)
{
  llong sum = 0;
  //llong current;
//  llong error = 0;
  int i, j;

  /* sum of weighted squared distances of the data object
     to their cluster representatives */
  for (i = 0; i < BookSize(pTS); i++) 
    {
    j = Map(pP, i);
	
	
	/*error = weight[j] * sqrt(VectorDistance(Vector(pTS, i), Vector(pCB, j), VectorSize(pTS),
           MAXLLONG, EUCLIDEANSQ) * VectorFreq(pTS, i));
	
	sum = sum + (error*error);*/
	//printf("\np1 = ");
	//PrintVector(Vector(pTS,i), (pTS)->BlockSizeX, (pTS)->BlockSizeY);
	//printf("  p2 = ");
	//PrintVector(Vector(pCB,j), (pCB)->BlockSizeX, (pCB)->BlockSizeY);
	//current = (VectorDistance(Vector(pTS, i), Vector(pCB, j), VectorSize(pTS), MAXLLONG, EUCLIDEANSQ)*weight[j]);
    //printf("weight = %.3f, sqaured eu dist = %lld, weighted sqaured eu dist = %lld", weight[j], VectorDistance(Vector(pTS, i), Vector(pCB, j), VectorSize(pTS), MAXLLONG, EUCLIDEANSQ), current);	
    sum += weight[j] * VectorDistance(Vector(pTS, i), Vector(pCB, j), VectorSize(pTS),
           MAXLLONG, EUCLIDEANSQ); //* VectorFreq(pTS, i);
	//printf("  sum = %lld ", sum);
    }

  //double temperror = CALC_MSE(sum);
  //printf("\nMSE = %f",temperror);
  return sum;
}


/* -------------------------------------------------------------------- */
/* Calculates data objects current distances to their cluster centroids */
/* -------------------------------------------------------------------- */


void CalculateDistances(TRAININGSET *pTS, CODEBOOK *pCB, PARTITIONING *pP,
                        llong *distance, double *weight)
{
  int i, j;

  for (i = 0; i < BookSize(pTS); i++) 
    {
    j = Map(pP, i);
    distance[i] = weight[j] * sqrt(VectorDistance(Vector(pTS, i), Vector(pCB, j),
                  VectorSize(pTS), MAXLLONG, EUCLIDEANSQ));
    }
}


/*-------------------------------------------------------------------*/


int FindSecondNearestVector(BOOKNODE *node, CODEBOOK *pCB, 
                            int firstIndex, llong *secondError)
{
  int   i;
  int   secondIndex;
  llong e;

  secondIndex = -1;
  *secondError = MAXLLONG;

  for(i = 0; i < BookSize(pCB); i++)
    {
    e = sqrt(VectorDistance(Vector(pCB,i), node->vector, VectorSize(pCB), 
        *secondError, EUCLIDEANSQ));

      if ((e < *secondError) && (i != firstIndex))
    {
      *secondError = e;
      secondIndex  = i;
      }
    }
  return secondIndex;
}


/*-------------------------------------------------------------------*/
/* selects deterministicly, which cluster centroid to swap. one that 
   increases objective function (MSE) least, if removed, is selected. */

int SelectClusterToBeSwapped(TRAININGSET *pTS, CODEBOOK *pCB, 
                             PARTITIONING *pP, llong *distance)
{
  int i, j, min;
  llong error;
  llong priError[BookSize(pCB)];  /* current error; data objects are in 
                                     their primary (closest) cluster) */
  llong secError[BookSize(pCB)];  /* error after partition is removed and 
                                     data objects are repartitioned; data 
                                     objects are in their secondary 
                                     (second closest) cluster */

  /* initializing */
  for (i = 0; i < BookSize(pCB); i++) 
    {
    priError[i] = 0;
    secError[i] = 0;
    }

  /* calculating primary and secondary cluster errors */
  for (i = 0; i < BookSize(pTS); i++) 
    {
    j = Map(pP, i);
    FindSecondNearestVector(&Node(pTS,i), pCB, j, &error);
    priError[j] += distance[i] * VectorFreq(pTS, i);
    secError[j] += error * VectorFreq(pTS, i);    
    }

  /* finding cluster that increases objective function least */
  min = -1;
  error = MAXLLONG;
  for (j = 0; j < BookSize(pCB); j++) 
    {
    if ((secError[j] - priError[j]) < error)
      {
      min = j;
      error = secError[j] - priError[j];
      }    
    }

  return min;
}


/*-------------------------------------------------------------------*/


char* DenRSInfo(void)
{
  char* p;
  int len;
  
  len = strlen(ProgName)+strlen(VersionNumber)+strlen(LastUpdated)+4;  
  p   = (char*) malloc(len*sizeof(char));
  
  if (!p) 
    {
    ErrorMessage("ERROR: Allocating memory failed!\n");
    ExitProcessing(FATAL_ERROR);
    }
 
  sprintf(p, "%s\t%s\t%s", ProgName, VersionNumber, LastUpdated);
 
  return p;
}


/*-------------------------------------------------------------------*/


void PrintCentroidWeights(CODEBOOK *CB, double *weight, double *tempweight)
{
  int i;
  for(i = 0; i < BookSize(CB); i++)
    {
    PrintMessage("c[%d] = ", i);
    PrintVector(Vector(CB, i), CB->BlockSizeX, CB->BlockSizeY);
    PrintMessage("\tw[%d] = %.3f", i, weight[i]);
	PrintMessage("\ttempw[%d] = %.3f\n", i, tempweight[i]);
    }
}


/*-------------------------------------------------------------------*/


void CheckOverflow(llong a, llong b)
{
  /* Make sure a + b doesn't overflow */
  if (a > MAXLLONG - b)
    {
    PrintMessage("Overflow: %lld + %lld > %lld!\n", a, b, MAXLLONG);
    exit(-1);
    }
}


/*-------------------------------------------------------------------*/


int CheckClusterFreqs(CODEBOOK *pCB, PARTITIONING *pP)
{
  int i;
  int nullcluster = 0;
  for (i = 0; i < BookSize(pCB); i++)
    {
    if (CCFreq(pP, i) == 0 || CCFreq(pP, i) == 1)
      {
      /*
       * Alert if the number of vectors in the cluster is zero.
       * Most likely some other centroid with a low weight attracted
       * all vectors that previously belonged to this partition.
       */
      PrintMessage("WARNING: Number of vectors in cluster %d became zero!\n", i);
	  nullcluster = 1;
      }
    }
	return nullcluster;
}


/*-------------------------------------------------------------------*/


void InitializeWeights(CODEBOOK *pCB, double *weight)
{
  int i;
  double initialWeight = 1.0 / BookSize(pCB);
  for (i = 0; i < BookSize(pCB); i++)
    {
    weight[i] = initialWeight;
    }
}


/*-------------------------------------------------------------------*/
