#pragma once

#include <iostream>
#include <sstream>
#include <cstring>
#include <cstdlib>
#include <cassert>
#include <cmath>

#ifdef DEBUG
#define LOG(x) std::cout << x << std::endl
#else
#define LOG(x)
#endif

template <typename T>
std::string arr_toString(const T *arr, size_t size, bool hex = false, size_t max = 128)
{
	max = (max == 0) ? size : max;
	std::ostringstream oss;
	oss << "[";
	if (hex)
		std::cout << std::hex;
	for (size_t i = 0; i < max && i < size; i++)
	{
		if (hex)
		{
			int long long mask = ~0ull >> 8 * (sizeof(int long long) - sizeof(T));
			int long long value = (*(int long long *)(arr + i)) // is this gonna work?
								  & mask;
			oss << " " << value;
		}
		else
			oss << " " << arr[i];
	}
	if (hex)
		oss << std::dec;
	oss << " ]";
	return oss.str();
}

template <typename T>
void print_arr(const T *arr, size_t size, bool hex = false, size_t max = 128)
{
	std::cout << arr_toString(arr, size, hex, max);
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

template <typename T>
bool inline static isFinite(T v)
{
	return !std::isnan(v) && !std::isinf(v);
}

template <int N_eq, int N_var, typename T>
int leastSquaresSolver(const T A[N_eq][N_var], const T b[N_eq], T x[N_var])
{
	T AT[N_var][N_eq];
	T ATA[N_var][N_var];
	T ATb[N_var];
	static constexpr bool isFloat = std::is_same_v<T, float>;
	static constexpr T eps = isFloat ? 1e-6F : 1e-12;

	int stat = 0;

	// Transpose of A
	for (int i = 0; i < N_eq; i++)
	{
		for (int j = 0; j < N_var; j++)
		{
			AT[j][i] = A[i][j];
		}
	}

	// ATA = AT * A
	for (int i = 0; i < N_var; i++)
	{
		for (int j = 0; j < N_var; j++)
		{
			ATA[i][j] = 0;
			for (int k = 0; k < N_eq; k++)
			{
				ATA[i][j] += AT[i][k] * A[k][j];
			}
		}
	}

	// ATb = AT * b
	for (int i = 0; i < N_var; i++)
	{
		ATb[i] = 0;
		for (int j = 0; j < N_eq; j++)
		{
			ATb[i] += AT[i][j] * b[j];
		}
	}

	// Solve ATA * x = ATb using Gaussian elimination with partial pivoting
	for (int k = 0; k < N_var; k++)
	{
		// Partial pivoting
		int maxRow = k;
		T maxVal = std::abs(ATA[k][k]);
		for (int i = k + 1; i < N_var; i++)
		{
			if (std::abs(ATA[i][k]) > maxVal)
			{
				maxVal = std::abs(ATA[i][k]);
				maxRow = i;
			}
		}
		if (maxRow != k)
		{
			// Swap rows k and maxRow in ATA
			for (int j = 0; j < N_var; j++)
			{
				std::swap(ATA[k][j], ATA[maxRow][j]);
			}
			// Swap elements in ATb
			std::swap(ATb[k], ATb[maxRow]);
		}

		for (int i = k + 1; i < N_var; i++)
		{
			T factor = (ATA[k][k] == 0 && ATA[i][k] == 0) ? 1 : ATA[i][k] / ATA[k][k];
			for (int j = k; j < N_var; j++)
			{
				ATA[i][j] -= (0 == ATA[k][j]) ? 0 : factor * ATA[k][j];
			}
			ATb[i] -= (0 == ATb[k])? 0 : factor * ATb[k];
		}
	}

	// Back substitution
	for (int i = N_var - 1; i >= 0; i--)
	{
		x[i] = ATb[i];
		for (int j = i + 1; j < N_var; j++)
		{
			x[i] -= ATA[i][j] * x[j];
		}
		x[i] = (ATA[i][i] == 0 && x[i] == 0) ? 1 : x[i] / ATA[i][i];
		if (ATA[i][i] == 0)
			stat |= 1;
	}

	// Ax - b
	for (int i = 0; i < N_eq; i++)
	{
		T sum = 0;
		for (int j = 0; j < N_var; j++)
		{
			sum += A[i][j] * x[j];
		}
		if (std::abs(sum - b[i]) > eps)
		{
			stat |= 2;
			break;
		}
	}
	return stat;
}

template <typename T>
void mask_arr(T *arr, size_t size, bool *mask)
{
	assert(arr);
	assert(mask);

	for (size_t i = 0; i < size; i++)
		arr[i] = mask[i] ? arr[i] : 0;
}
