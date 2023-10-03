#include "Bench.h"
#include <chrono>
#include <iostream>

namespace hax {
	
	Bench::Bench(const char* label, size_t runs): _label{ label }, _runs{ runs }, _startTime{}, _endTime{}, _counter{} {
		this->_durations = new double[this->_runs];
	}


	Bench::~Bench() {
		delete[] this->_durations;
	}


	void Bench::start() {
		this->_startTime = std::chrono::duration<double>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
	}


	void Bench::end() {
		this->_endTime = std::chrono::duration<double>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
		this->_durations[_counter] = this->_endTime - this->_startTime;
		this->_counter++;
	}


	void Bench::printAvg() {
		
		if (this->_counter >= this->_runs) {
			double avg = 0.;

			for (size_t i = 0; i < this->_counter; i++) {
				avg += this->_durations[i];
			}

			avg /= this->_counter;
			std::cout << "Average time " << this->_label << ": " << avg << std::endl;
			
			this->_counter = 0;
		}

	}

}