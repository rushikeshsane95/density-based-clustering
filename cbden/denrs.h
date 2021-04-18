#if ! defined(__DENRS_H)
#define __DENRS_H

int PerformDenRS(TRAININGSET *pTS, CODEBOOK *pCB, PARTITIONING *pP, int iter, 
    int kmIter, int deterministic, int quietLevel, 
    int useInitialCB, int monitoring);

void SelectRandomRepresentatives(TRAININGSET *pTS, CODEBOOK *pCB);

void CalculateDistances(TRAININGSET *pTS, CODEBOOK *pCB, PARTITIONING *pP,
    llong *distance);

void OptimalRepresentatives(PARTITIONING *pP, TRAININGSET *pTS, CODEBOOK *pCB, 
    int *active, llong *cdist, int *activeCount);

void OptimalPartition(CODEBOOK *pCB, TRAININGSET *pTS, PARTITIONING *pP,
    int *active, llong *cdist, int activeCount, llong *distance);

char* DenRSInfo(void);

#endif /* __DENRS_H */
