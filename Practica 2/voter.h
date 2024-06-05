
#ifndef VOTER_H
#define VOTER_H 

/**
 * @brief Implements the voters of the system.
 * 
 * @param n_procs Number of processes.
 * @param semCandidate Semaphore to choose a candidate.
 * @param semVoting Semaphore to vote one by one.
 * @param semFinish Semaphore to indicate the end of voting.
 */
void voter(int n_procs, sem_t *semCandidate, sem_t *semVoting, sem_t *semFinish);

#endif 