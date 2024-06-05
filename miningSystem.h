/**
 * @file miningSystem.h
 * @authors Miguel Lozano y Sergio Larriba
 * @brief Implements a mining system to run a proof of work.
 * @date 01-03-2023
*/

#ifndef MINING_SYSTEM_H
#define MINING_SYSTEM_H

/**
 * @brief Implements a mining system that uses various threads to run a proof of work.
 * 
 * @param target_ini First target of the proof of work.
 * @param rounds Number of times you have to find the solution of the pow.
 * @param n_threads Number of threads running to complete the pow.
 * @return EXIT_SUCCESS if there is no errors, otherwise EXIT_FAILLURE.
*/
int miningSystem(int target_ini, int rounds, int n_threads);

#endif