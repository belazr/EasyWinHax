#include "Bench.h"
#include <chrono>
#include <iostream>

namespace hax {
	
	Bench::Bench(const char* label, size_t runs): _label{ label }, _runs{ runs }, _startTime{}, _endTime{}, _measurements{} {
		this->_measurements.reserve(this->_runs);

		return;
	}


	void Bench::start() {
		this->_startTime = std::chrono::duration<double>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();

		return;
	}


	void Bench::end() {
		this->_endTime = std::chrono::duration<double>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
		this->_measurements.append(this->_endTime - this->_startTime);

		return;
	}


	void Bench::printAvg() {
		
		if (this->_measurements.size() >= this->_runs) {
			double avg = 0.;

			for (size_t i = 0u; i < this->_measurements.size(); i++) {
				avg += this->_measurements[i];
			}

			avg /= this->_measurements.size();
			std::cout << "Average time " << this->_label << ": " << avg << std::endl;
			this->_measurements.resize(0u);
		}

		return;
	}

}