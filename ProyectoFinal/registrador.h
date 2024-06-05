/**
 * @file registrador.h
 * @authors Miguel Lozano y Sergio Larriba
 * @brief Define a registrador that checks the result of mining operation.
 * @date 03-03-2023
*/

#ifndef REGISTRADOR_H
#define REGISTRADOR_H

/**
 * @brief Implements a registrador that checks the output obtained by the miner.
 * 
 * @param pipe1 Miner ---> registrador
 * @param pipe2 registrador ---> Miner
 * @return EXIT_SUCCESS if there is no errors, otherwise EXIT_FAILLURE.
*/
int registrador(int *pipe1, int id);

#endif