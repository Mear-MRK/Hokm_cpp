/*
 * test_helper.cpp
 *
 *  Created on: May 20, 2023
 *      Author: mear
 */
#include <iostream>
#include <cstdlib>
#include <ctime>

#include "utils.h"

int rnd()
{
	return rand() % 30 - 15;
}

void utils_test()
{
	// 0  1  2  3  4  5   6   7   8
	int arr[] = {1, 1, 3, 3, 6, 7, 10, 12, 12};
	std::cout << "Arr: ";
	print_arr(arr, 9);
	std::cout << "  0 1 2 3 4 5 6  7  8" << std::endl;
	std::cout << "Bisect (v:ind): " << std::endl;
	for (int i = 0; i < 15; i++)
		std::cout << i << ":" << bisect(arr, 9, i) << " ";
	std::cout << std::endl;

	int b_arr[13];

	srand(time(NULL));
	rnd_arr(b_arr, 13, rnd);
	std::cout << "Rand Arr: ";
	print_arr(b_arr, 13);
	quicksort(b_arr, 13);
	std::cout << "Quick sorted arr: ";
	print_arr(b_arr, 13);

	const int Neq = 2;
	const int Nvar = 2;
	float x[Nvar];

	float A1[Neq][Nvar] = {{1, 1}, {1, -1}};
	float b1[Neq] = {-1, 3};
	int stat = leastSquaresSolver<Neq, Nvar, float>(A1, b1, x);
	std::cout << "Least squares solver result; issue: " << stat << " x: ";
	print_arr(x, Nvar);

	float A2[Neq][Nvar] = {{1, 1}, {1, 1}};
	float b2[Neq] = {-1, 3};
	stat = leastSquaresSolver<Neq, Nvar, float>(A2, b2, x);
	std::cout << "Least squares solver result; issue: " << stat << " x: ";
	print_arr(x, Nvar);

	float A3[Neq][Nvar] = {{1, 1}, {1, 1}};
	float b3[Neq] = {-1, -1};
	stat = leastSquaresSolver<Neq, Nvar, float>(A3, b3, x);
	std::cout << "Least squares solver result; issue: " << stat << " x: ";
	print_arr(x, Nvar);

	const int Ne = 2;
	const int Nv = 3;
	float A[Ne][Nv] = {{1, 1, 0}, {1, -1, 0}};
	float b[Ne] = {-1, 3};
	float x3[Nv];
	stat = leastSquaresSolver<Ne, Nv, float>(A, b, x3);
	std::cout << "Least squares solver result; issue: " << stat << " x: ";
	print_arr(x3, Nv);
}
