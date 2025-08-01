#pragma once
#include "Vector.h"

// Class to benchmark the average execution time of multiple code executions and write it to standard out.
// Can be used to meassure the execution time of a function hook for gui apps with an allocated console.

namespace hax {
	
	class Bench {
	private:
		const char* const _label;
		const size_t _runs;
		long long _freq;
		long long _beginTime;
		long long _endTime;
		Vector<long long> _measurements;

	public:
		// Initializes members
		// 
		// Parameters:
		// 
		// [in] label:
		// Label for the output. Format: "Average time <label>: <average time in seconds>"
		// 
		// [in] runs:
		// How many times the code execution is meassured before the average is calculated and printed when calling printAvg.
		Bench(const char* label, size_t runs);
		
		// Begins the meassurement.
		void begin();
		
		// Ends the meassurement.
		void end();

		// Writes the output to std out after the code inbetween begin() and end() is executed the amount of times passed as runs to the constructor.
		void printAvg();
	};

}