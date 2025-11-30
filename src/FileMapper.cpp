#include "FileMapper.h"
#include <Windows.h>

namespace blue {
	
	FileMapper::FileMapper(const char* path) : _path{ path }, _data{}, _size{} {
		const size_t length = strlen(path);
		
		this->_path = new char[length + 1];
		strcpy_s(const_cast<char*>(this->_path), length + 1, path);

		return;
	}


	FileMapper::~FileMapper() {
		this->unmap();

		if (this->_path) {
			delete[] this->_path;
		}

		return;
	}


	DWORD FileMapper::map(bool image) {
		const HANDLE hFile = CreateFileA(_path, GENERIC_READ | GENERIC_EXECUTE, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);

		if (hFile == INVALID_HANDLE_VALUE) {
			
			return GetLastError();
		}

		LARGE_INTEGER size{};
		
		if (!GetFileSizeEx(hFile, &size)) {

			return GetLastError();
		}

		this->_size = static_cast<size_t>(size.QuadPart);
		const DWORD protect = image ? PAGE_READONLY | SEC_IMAGE : PAGE_READONLY;
		const HANDLE hMapping = CreateFileMappingA(hFile, nullptr, protect, 0ul, 0ul, nullptr);

		if (!hMapping) {
			const DWORD error = GetLastError();
			CloseHandle(hFile);

			return error;
		}

		CloseHandle(hFile);

		this->_data = reinterpret_cast<BYTE*>(MapViewOfFile(hMapping, FILE_MAP_READ, 0ul, 0ul, 0));

		if (!this->_data) {
			const DWORD error = GetLastError();
			CloseHandle(hMapping);

			return error;
		}

		CloseHandle(hMapping);

		return ERROR_SUCCESS;
	}


	void FileMapper::unmap() {
		
		if (this->_data) {
			UnmapViewOfFile(this->_data);
			this->_data = nullptr;
		}

		return;
	}


	const BYTE* FileMapper::data() const {

		return this->_data;
	}


	const char* FileMapper::path() const {
		
		return this->_path;
	}


	size_t FileMapper::size() const {
		
		return this->_size;
	}

}