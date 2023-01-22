#include "FileLoader.h"
#include <stdio.h>

namespace hax {
	
	FileLoader::FileLoader(const char* path):
		_path(path), _pBytes(nullptr), _size(0), _errno(0)
	{
		FILE* pFile = nullptr;
		this->_errno = fopen_s(&pFile,this->_path, "rb");
		
		if (pFile && !this->_errno) {
			this->_errno = fseek(pFile, 0, SEEK_END);
			this->_size = ftell(pFile);
			this->_pBytes = new BYTE[this->_size]{};
			this->_errno = fclose(pFile);
		}

	}


	FileLoader::~FileLoader() {
		
		if (this->_pBytes) {
			delete[] this->_pBytes;
		}

	}


	bool FileLoader::readBytes() {
		FILE* pFile = nullptr;
		this->_errno = fopen_s(&pFile, this->_path, "rb");
		
		if (this->_errno) return false;

		const size_t read = fread_s(this->_pBytes, this->_size, sizeof(BYTE), this->_size, pFile);
		this->_errno = fclose(pFile);
		
		if (this->_errno || read != _size) {
			
			return false;
		}

		return true;
	}


	BYTE* FileLoader::getBytes() const {

		return this->_pBytes;
	}


	errno_t FileLoader::getErrno() const {
		
		return this->_errno;
	}


	const char* FileLoader::getPath() const {
		
		return this->_path;
	}


	size_t FileLoader::getSize() const {
		
		return this->_size;
	}

}