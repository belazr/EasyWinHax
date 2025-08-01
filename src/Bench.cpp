#include "Bench.h"
#include <stdio.h>
#include <Windows.h>

namespace hax {
	
	Bench::Bench(const char* label, size_t runs) : _label{ label }, _runs{ runs }, _freq{}, _beginTime {}, _endTime{}, _measurements{} {
		this->_measurements.reserve(this->_runs);

		LARGE_INTEGER freq{};
		QueryPerformanceFrequency(&freq);
		this->_freq = freq.QuadPart;

		return;
	}


	void Bench::begin() {
		LARGE_INTEGER count{};
		QueryPerformanceCounter(&count);
		this->_beginTime = count.QuadPart;

		return;
	}


	void Bench::end() {
		LARGE_INTEGER count{};
		QueryPerformanceCounter(&count);
		this->_endTime = count.QuadPart;
		this->_measurements.append(this->_endTime - this->_beginTime);

		return;
	}


	void Bench::printAvg() {
		
		if (this->_measurements.size() < this->_runs) return;

		long long sum = 0ll;

		for (size_t i = 0u; i < this->_measurements.size(); i++) {
			sum += this->_measurements[i];
		}

		const double avg = static_cast<double>(sum) / this->_measurements.size();
		const double avgSec = avg / this->_freq;

		printf("Average time %s: %.7fs\n", this->_label, avgSec);

		this->_measurements.resize(0u);

		return;
	}

}