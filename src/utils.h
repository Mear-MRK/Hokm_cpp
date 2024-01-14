/*
 *  utils.h
 *
 *  Created on: May 20, 2023
 *      Author: mear
 */

#ifndef UTILS_HPP_
#define UTILS_HPP_

#include <iostream>
#include <cstring>
#include <cstdlib>

#ifdef DEBUG
#define LOG(x) std::cout << x << std::endl
#else
#define LOG(x)
#endif

template <typename T>
void print_arr(const T *arr, size_t size, bool hex = false, size_t max = 128)
{
	max = (max == 0) ? size : max;
	std::cout << std::endl
			  << "[";
	if (hex)
		std::cout << std::hex;
	for (size_t i = 0; i < max && i < size; i++)
	{
		if (hex)
		{
			int long long mask = ~0ull >> 8 * (sizeof(int long long) - sizeof(T));
			int long long value = (*(int long long *)(arr + i)) // is this gonna work?
								  & mask;
			std::cout << " " << value;
		}
		else
			std::cout << " " << arr[i];
	}
	if (hex)
		std::cout << std::dec;
	std::cout << " ]" << std::endl;
}

template <typename T>
void inline move(T *arr, size_t end, size_t begin, int64_t step = 1)
{
	memmove(arr + begin + step, arr + begin, (end - begin) * sizeof(T));
}

// Gives insertion index i of val such that arr[i-1] <= val < arr[i] or end
template <typename T>
size_t inline bisect(const T *arr, size_t end, T val)
{
	if (end == 0 || val < arr[0])
		return 0;
	else if (val >= arr[end - 1])
		return end;

	size_t i_min = 0;
	size_t i_max = end;
	size_t i_mid = end / 2;
	while (i_mid != i_min)
	{
		if (val < arr[i_mid])
			i_max = i_mid;
		else
			i_min = i_mid;
		i_mid = (i_max + i_min) / 2;
	}
	return i_max;
}

template <typename T>
bool inline sorted_is_in(const T *arr, size_t end, T a)
{
	size_t i = bisect<T>(arr, end, a);
	if (i > 0 && arr[i - 1] == a)
		return true;
	return false;
}

template <typename T>
void inline sorted_insert(T *arr, size_t end, T a)
{
	size_t i = bisect<T>(arr, end, a);
	move<T>(arr, end, i);
	arr[i] = a;
}

template <typename T>
bool sorted_unique_insert(T *arr, size_t end, T a)
{
	size_t i = bisect<T>(arr, end, a);
	if (i > 0 && arr[i - 1] == a)
		return false;
	move<T>(arr, end, i);
	arr[i] = a;
	return true;
}

template <typename T>
void inline swap(T &a, T &b)
{
	T tmp = a;
	a = b;
	b = tmp;
}

template <typename T>
void inline sort3(T &a, T &b, T &c)
{
	if (a > b)
		swap(a, b);
	if (b > c)
		swap(b, c);
	if (a > b)
		swap(a, b);
}

template <typename T>
static inline size_t quicksort_partition(T *arr, size_t size, T pivot)
{
	size_t i = 1;
	size_t j = size - 2;
	while (i <= j)
		if (arr[i] > pivot)
		{
			swap(arr[i], arr[j]);
			j--;
		}
		else
		{
			i++;
		}
	return i;
}

template <typename T>
T *quicksort(T *arr, size_t size)
{
	switch (size)
	{
	case 0:
	case 1:
		return arr;
	case 2:
		if (arr[0] > arr[1])
			swap(arr[0], arr[1]);
		return arr;
	case 3:
		sort3(arr[0], arr[1], arr[2]);
		return arr;
	}
	sort3(arr[0], arr[size / 2], arr[size - 1]);
	size_t i = quicksort_partition<T>(arr, size, arr[size / 2]);
	quicksort(arr, i);
	quicksort(arr + i, size - i);

	return arr;
}

template <typename T>
T *rnd_arr(T *arr, size_t size, T (*rnd)(void))
{
	for (size_t i = 0; i < size; i++)
		arr[i] = rnd();

	return arr;
}

template <typename T>
T *selection_sort(T *arr, size_t size)
{
	if (size < 2)
		return arr;
	for (size_t i = 0; i < size - 1; i++)
		for (size_t j = i + 1; j < size; j++)
			if (arr[j] < arr[i])
				swap<T>(arr[i], arr[j]);

	return arr;
}

template <typename T>
T *shuffle(T *arr, int size, int (*rnd)(void))
{
	if (size < 2)
		return arr;
	for (int i = 0; i < size - 2; i++)
	{
		int ri = i + rnd() % (size - i - 1);
		if (i != ri)
			swap<T>(arr[i], arr[ri]);
	}
	return arr;
}

template <typename T>
int compare(const T *arr_l, const T *arr_r, size_t size)
{
	for (size_t i = 0; i < size; i++)
		if (arr_l[i] > arr_r[i])
			return 1;
		else if (arr_l[i] < arr_r[i])
			return -1;
	return 0;
}

template <typename T>
T *inc_sort(const T *arr, T *sorted_arr, size_t size)
{
	if (size < 1 || (arr < sorted_arr + size && sorted_arr < arr + size))
		return nullptr;
	sorted_arr[0] = arr[0];
	for (size_t i = 1; i < size; i++)
	{
		sorted_insert<T>(sorted_arr, i, arr[i]);
	}
	return sorted_arr;
}

#endif /* UTILS_HPP_ */
