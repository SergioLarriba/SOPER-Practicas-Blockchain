/**
 * @file monitor.h
 * @authors Miguel Lozano y Sergio Larriba
 * @brief Define a monitor that checks the result of mining operation.
 * @date 03-03-2023
*/

#ifndef MONITOR_H
#define MONITOR_H

/**
 * @brief Implements a monitor that checks the output obtained by the miner.
 * 
 * @param pipe1 Miner ---> Monitor
 * @param pipe2 Monitor ---> Miner
 * @return EXIT_SUCCESS if there is no errors, otherwise EXIT_FAILLURE.
*/
int monitor(int *pipe1, int *pipe2, long int target);

#endif