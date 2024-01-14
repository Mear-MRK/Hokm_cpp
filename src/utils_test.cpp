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


 int rnd(){
	return rand() % 30 - 15;
}

void utils_test() {
				//0  1  2  3  4  5   6   7   8
	int arr[] = { 1, 1, 3, 3, 6, 7, 10, 12, 12};
	std::cout << "Arr: ";
	print_arr(arr, 9);
	std::cout << "  0 1 2 3 4 5 6  7  8" << std::endl;
	std::cout << "Bisect (v:ind): " << std::endl;
	for(int i = 0; i < 15; i++)
		std::cout << i << ":" << bisect(arr, 9, i) << " ";
	std::cout << std::endl;

	int b[13];

	srand(time(NULL));
	rnd_arr(b, 13, rnd);
	std::cout << "Rand Arr: ";
	print_arr(b, 13);
	quicksort(b, 13);
	std::cout << "Quick sorted arr: ";
	print_arr(b, 13);


}


