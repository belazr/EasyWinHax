#pragma once
#include "proc.h"
#include "mem.h"

namespace proc {

	namespace ex {

		DWORD getProcId(const char* procName) {
			PROCESSENTRY32 procEntry{};

			if (!getTlHelpProcEntry(procName, &procEntry)) return 0;

			return procEntry.th32ProcessID;
		}


		bool getTlHelpProcEntry(const char* procName, PROCESSENTRY32* pProcEntry) {
			const HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

			if (!hSnap || hSnap == INVALID_HANDLE_VALUE) return false;

			pProcEntry->dwSize = sizeof(PROCESSENTRY32);

			if (!Process32First(hSnap, pProcEntry)) return false;

			bool found = false;

			do {
				
				if (!_stricmp(pProcEntry->szExeFile, procName)) {
					found = true;
				}

			} while (!found && Process32Next(hSnap, pProcEntry));

			CloseHandle(hSnap);

			return found;
		}


		bool getTlHelpModEntry(HANDLE hProc, const char* modName, MODULEENTRY32* pModEntry) {
			const HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, GetProcessId(hProc));

			if (!hSnap || hSnap == INVALID_HANDLE_VALUE) return false;

			pModEntry->dwSize = sizeof(MODULEENTRY32);

			if (!Module32First(hSnap, pModEntry)) return false;

			bool found = false;

			do {

				if (!_stricmp(pModEntry->szModule, modName)) {
					found = true;
				}

			} while (!found && Module32Next(hSnap, pModEntry));

			CloseHandle(hSnap);

			return found;
		}

		
		static bool getDataDirFromPeHeaders(HANDLE hProc, const PeHeaders* pPeHeaders, IMAGE_DATA_DIRECTORY* pDataDir, char index);

		FARPROC getProcAddress(HANDLE hProc, HMODULE hMod, const char* funcName) {
			const BYTE* const pBase = reinterpret_cast<BYTE*>(hMod);
			PeHeaders peHeaders{};

			if (!getPeHeaders(hProc, hMod, &peHeaders)) return nullptr;

			IMAGE_DATA_DIRECTORY dirEntryExport{};

			if (!getDataDirFromPeHeaders(hProc, &peHeaders, &dirEntryExport, IMAGE_DIRECTORY_ENTRY_EXPORT)) return nullptr;
			
			IMAGE_EXPORT_DIRECTORY exportDir{};

			if (!ReadProcessMemory(hProc, pBase + dirEntryExport.VirtualAddress, &exportDir, sizeof(IMAGE_EXPORT_DIRECTORY), nullptr)) return nullptr;

			const DWORD numberOfFuncs = exportDir.NumberOfFunctions;
			DWORD* const exportFunctionTable = new DWORD[numberOfFuncs];
			const BYTE* const addressOfFunctions = pBase + exportDir.AddressOfFunctions;

			if (!ReadProcessMemory(hProc, addressOfFunctions, exportFunctionTable, numberOfFuncs * sizeof(DWORD), nullptr)) {
				delete[] exportFunctionTable;

				return nullptr;
			}

			const DWORD numberOfNames = exportDir.NumberOfNames;
			DWORD* const exportNameTable = new DWORD[numberOfNames];
			const BYTE* const addressOfNames = pBase + exportDir.AddressOfNames;

			if (!ReadProcessMemory(hProc, addressOfNames, exportNameTable, numberOfNames * sizeof(DWORD), nullptr)) {
				delete[] exportFunctionTable;
				delete[] exportNameTable;

				return nullptr;
			}

			WORD* const exportOrdinalTable = new WORD[numberOfNames];
			const BYTE* const addressOfNameOrdinals = pBase + exportDir.AddressOfNameOrdinals;

			if (!ReadProcessMemory(hProc, addressOfNameOrdinals, exportOrdinalTable, numberOfNames * sizeof(WORD), nullptr)) {
				delete[] exportFunctionTable;
				delete[] exportNameTable;
				delete[] exportOrdinalTable;
				
				return nullptr;
			}

			DWORD funcRva = 0;
			// export by ordinal if everything but the lowest word of name param is zero
			const bool byOrdinal = (reinterpret_cast<uintptr_t>(funcName) & 0xFFFFFFFFFFFF0000) == 0;

			if (byOrdinal) {
				const WORD index = static_cast<WORD>((reinterpret_cast<uintptr_t>(funcName) & 0xFFFF) - exportDir.Base);
				funcRva = exportFunctionTable[index];
			}
			else {

				for (DWORD i = 0; i < exportDir.NumberOfNames; i++) {
					char curFuncName[MAX_PATH]{};

					if (!mem::ex::copyRemoteString(hProc, curFuncName, pBase + exportNameTable[i], MAX_PATH)) continue;

					if (!_stricmp(funcName, curFuncName)) {
						// the function rva is in the export table indexed by the ordinal in the ordinal table at the same index as the name in the name table
						funcRva = exportFunctionTable[exportOrdinalTable[i]];

						break;
					}

				}

			}

			if (!funcRva) {
				delete[] exportFunctionTable;
				delete[] exportNameTable;
				delete[] exportOrdinalTable;

				return nullptr;
			}

			FARPROC procAddress = nullptr;
			const bool forwarded = funcRva >= dirEntryExport.VirtualAddress && funcRva <= dirEntryExport.VirtualAddress + dirEntryExport.Size;

			if (forwarded) {
				// forward has the format "module.function"
				// it is split a the dot and the ".dll" extension is appended to the module name
				char curForward[MAX_PATH]{};

				if (!mem::ex::copyRemoteString(hProc, curForward, pBase + funcRva, MAX_PATH)) {
					delete[] exportFunctionTable;
					delete[] exportNameTable;
					delete[] exportOrdinalTable;
					
					return nullptr;
				}

				char* forwardModName = nullptr;
				char* forwardFuncName = nullptr;

				forwardModName = strtok_s(curForward, ".", &forwardFuncName);

				char forwardModFileName[MAX_PATH]{};

				strcpy_s(forwardModFileName, forwardModName);
				strcat_s(forwardModFileName, ".dll");

				HMODULE hForwardMod = getModuleHandle(hProc, forwardModFileName);

				if (!hForwardMod) {
					delete[] exportFunctionTable;
					delete[] exportNameTable;
					delete[] exportOrdinalTable;

					return nullptr;
				}

				// check if exported by ordinal and looking for the forwarded funcion in the module it was forwarded to
				if (forwardFuncName[0] == '#') {
					char* forwardFuncOrdinal = reinterpret_cast<char*>(static_cast<uintptr_t>(atoi(forwardFuncName++)));
					procAddress = getProcAddress(hProc, hForwardMod, forwardFuncOrdinal);
				}
				else {
					procAddress = getProcAddress(hProc, hForwardMod, forwardFuncName);
				}

			}
			else {
				procAddress = reinterpret_cast<FARPROC>(pBase + funcRva);
			}

			delete[] exportFunctionTable;
			delete[] exportNameTable;
			delete[] exportOrdinalTable;

			return procAddress;
		}


		template <typename ITD, typename FLG>
		static BYTE* getIatEntryAddressFromImportDesc(HANDLE hProc, const char* funcName, const BYTE* pBase, const IMAGE_IMPORT_DESCRIPTOR* pImportDesc, FLG ordinalFlag);

		BYTE* getIatEntryAddress(HANDLE hProc, HMODULE hImportMod, const char* exportModName, const char* funcName) {
			PeHeaders peHeaders{};

			if (!getPeHeaders(hProc, hImportMod, &peHeaders)) return nullptr;

			IMAGE_DATA_DIRECTORY dirEntryImport{};

			if (!getDataDirFromPeHeaders(hProc, &peHeaders, &dirEntryImport, IMAGE_DIRECTORY_ENTRY_IMPORT)) return nullptr;

			const BYTE* const pBase = const_cast<BYTE* const>(reinterpret_cast<const BYTE* const>(peHeaders.pDosHeader));

			if (!pBase) return nullptr;

			bool found = false;
			const IMAGE_IMPORT_DESCRIPTOR* pImportDesc = reinterpret_cast<const IMAGE_IMPORT_DESCRIPTOR*>(pBase + dirEntryImport.VirtualAddress);
			IMAGE_IMPORT_DESCRIPTOR importDesc{};

			if (!ReadProcessMemory(hProc, pImportDesc, &importDesc, sizeof(importDesc), nullptr)) return nullptr;

			while (importDesc.Characteristics) {
				char curModName[MAX_PATH]{};

				if (!mem::ex::copyRemoteString(hProc, curModName, pBase + importDesc.Name, MAX_PATH)) break;

				if (!_stricmp(exportModName, curModName)) {
					// found the import descriptor of the module exporting the function
					found = true;

					break;
				}

				pImportDesc++;

				if (!ReadProcessMemory(hProc, pImportDesc, &importDesc, sizeof(importDesc), nullptr)) break;

			}

			if (!found) return nullptr;

			BYTE* pIatEntry = nullptr;

			// depending on the architecture the IAT entry address is saved in different thunk data structures
			if (peHeaders.pOptHeader64) {
				pIatEntry = getIatEntryAddressFromImportDesc<IMAGE_THUNK_DATA64>(hProc, funcName, pBase, &importDesc, IMAGE_ORDINAL_FLAG64);
			}
			else {
				pIatEntry = getIatEntryAddressFromImportDesc<IMAGE_THUNK_DATA32>(hProc, funcName, pBase, &importDesc, IMAGE_ORDINAL_FLAG32);
			}

			return pIatEntry;
		}


		bool getPeHeaders(HANDLE hProc, HMODULE hMod, PeHeaders* pPeHeaders) {

			if (!hProc || !hMod || !pPeHeaders) return false;
			
			BYTE* pBase = reinterpret_cast<BYTE*>(hMod);

			pPeHeaders->pDosHeader = reinterpret_cast<const IMAGE_DOS_HEADER*>(pBase);
			IMAGE_DOS_HEADER dosHeader{};

			if (!ReadProcessMemory(hProc, pPeHeaders->pDosHeader, &dosHeader, sizeof(dosHeader), nullptr)) return false;

			// make sure it is a a vaild pe header
			if (dosHeader.e_magic != IMAGE_DOS_SIGNATURE) return false;

			DWORD ntSig = 0;

			if (!ReadProcessMemory(hProc, pBase + dosHeader.e_lfanew, &ntSig, sizeof(ntSig), nullptr)) return false;

			// make sure it is a a vaild pe header
			if (ntSig != IMAGE_NT_SIGNATURE) return false;

			pPeHeaders->pFileHeader = reinterpret_cast<const IMAGE_FILE_HEADER*>(pBase + dosHeader.e_lfanew + sizeof(IMAGE_NT_SIGNATURE));
			IMAGE_FILE_HEADER fileHeader{};

			if (!ReadProcessMemory(hProc, pPeHeaders->pFileHeader, &fileHeader, sizeof(fileHeader), nullptr)) return false;

			// check if binary is x64
			if (fileHeader.SizeOfOptionalHeader == sizeof(IMAGE_OPTIONAL_HEADER64)) {
				pPeHeaders->pNtHeaders64 = reinterpret_cast<const IMAGE_NT_HEADERS64*>(pBase + dosHeader.e_lfanew);
				pPeHeaders->pOptHeader64 = reinterpret_cast<const IMAGE_OPTIONAL_HEADER64*>(reinterpret_cast<const BYTE*>(pPeHeaders->pFileHeader) + sizeof(IMAGE_FILE_HEADER));
				
				#ifdef _WIN64
				
				// if the caller and target architecture match we set the values of the general headers for easier usage
				pPeHeaders->pNtHeaders = pPeHeaders->pNtHeaders64;
				pPeHeaders->pOptHeader = pPeHeaders->pOptHeader64;
				
				#endif // _WIN64

			}
			else {
				pPeHeaders->pNtHeaders32 = reinterpret_cast<const IMAGE_NT_HEADERS32*>(pBase + dosHeader.e_lfanew);
				pPeHeaders->pOptHeader32 = reinterpret_cast<const IMAGE_OPTIONAL_HEADER32*>(reinterpret_cast<const BYTE*>(pPeHeaders->pFileHeader) + sizeof(IMAGE_FILE_HEADER));
				
				#ifndef _WIN64

				// if the caller and target architecture match we set the values of the general headers for easier usage
				pPeHeaders->pNtHeaders = pPeHeaders->pNtHeaders32;
				pPeHeaders->pOptHeader = pPeHeaders->pOptHeader32;
				
				#endif // _WIN64

			}

			return true;
		}


		HMODULE getModuleHandle(DWORD procId, const char* modName) {
			const HANDLE hProc = OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_LIMITED_INFORMATION, false, procId);
			
			return getModuleHandle(hProc, modName);
		}


		HMODULE getModuleHandle(HANDLE hProc, const char* modName) {
			uintptr_t modBase = 0;

			BOOL isWow64 = FALSE;
			IsWow64Process(hProc, &isWow64);

			if (isWow64) {
				// if the target is x86 (running in the WOW64 evironment on x64 Windows) search in the x86 loader data table
				const LDR_DATA_TABLE_ENTRY32* const pLdrEntry32 = getLdrDataTableEntry32Address(hProc, modName);

				if (!pLdrEntry32) return nullptr;

				LDR_DATA_TABLE_ENTRY32 ldrEntry32{};

				if (!ReadProcessMemory(hProc, pLdrEntry32, &ldrEntry32, sizeof(LDR_DATA_TABLE_ENTRY32), nullptr)) return nullptr;

				// static cast to convert from ULONG (4 bytes) to uintptr_t (4 bytes for x86, 8 bytes for x64)
				modBase = static_cast<uintptr_t>(ldrEntry32.DllBase);
			}
			else {

				// target is a native x64 binary so intertaction is only possible when compiled in x64
				#ifdef _WIN64

				// target is a native x64 binary so searching the x64 loader data table
				const LDR_DATA_TABLE_ENTRY64* const pLdrEntry64 = getLdrDataTableEntry64Address(hProc, modName);

				if (!pLdrEntry64) return nullptr;

				LDR_DATA_TABLE_ENTRY64 ldrEntry64{};
				
				if (!ReadProcessMemory(hProc, pLdrEntry64, &ldrEntry64, sizeof(LDR_DATA_TABLE_ENTRY64), nullptr)) return nullptr;

				// static cast to convert from ULONG (4 bytes) to uintptr_t (4 bytes for x86, 8 bytes for x64)
				modBase = static_cast<uintptr_t>(ldrEntry64.DllBase);

				#endif // _WIN64

			}

			return reinterpret_cast<HMODULE>(modBase);
		}


		// will not work compiled in x86 because there is no way to retrieve the x64 process environment block from x86
		#ifdef _WIN64

		LDR_DATA_TABLE_ENTRY64* getLdrDataTableEntry64Address(HANDLE hProc, const char* modName) {
			// x64 loader data table pointer is stored in x64 process envrionment block
			const PEB64* const pPeb64 = proc::ex::getPeb64Address(hProc);

			if (!pPeb64) return nullptr;

			PEB64 peb{};

			if (!ReadProcessMemory(hProc, pPeb64, &peb, sizeof(PEB64), nullptr)) return nullptr;

			PEB_LDR_DATA64* pPebLdrData64 = reinterpret_cast<PEB_LDR_DATA64*>(peb.Ldr);
			PEB_LDR_DATA64 pebLdrData64{};

			if (!ReadProcessMemory(hProc, pPebLdrData64, &pebLdrData64, sizeof(PEB_LDR_DATA64), nullptr)) return nullptr;

			wchar_t wModName[MAX_PATH]{};
			MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, modName, -1, wModName, MAX_PATH);

			LDR_DATA_TABLE_ENTRY64* pLdrTableEntry = nullptr;
			// static cast to convert from DWORD (4 bytes) to uintptr_t (4 bytes for x86, 8 bytes for x64)
			const LIST_ENTRY64* pNextEntry = reinterpret_cast<LIST_ENTRY64*>(static_cast<uintptr_t>(pebLdrData64.InMemoryOrderModuleList.Flink));

			// walk the linked list until back at the beginning
			while (pNextEntry != &pPebLdrData64->InMemoryOrderModuleList) {
				const LDR_DATA_TABLE_ENTRY64* const pCurLdrTableEntry = CONTAINING_RECORD(pNextEntry, LDR_DATA_TABLE_ENTRY64, InMemoryOrderLinks);

				LDR_DATA_TABLE_ENTRY64 curDataTableEntry{};

				if (!ReadProcessMemory(hProc, pCurLdrTableEntry, &curDataTableEntry, sizeof(LDR_DATA_TABLE_ENTRY64), nullptr)) break;

				if (!curDataTableEntry.BaseDllName.Buffer) break;

				const wchar_t* const wRemoteCurModName = reinterpret_cast<wchar_t*>(static_cast<uintptr_t>(curDataTableEntry.BaseDllName.Buffer));
				wchar_t wCurModName[MAX_PATH]{};

				if (!ReadProcessMemory(hProc, wRemoteCurModName, wCurModName, curDataTableEntry.BaseDllName.Length, nullptr)) break;

				if (!_wcsicmp(wModName, wCurModName)) {
					pLdrTableEntry = const_cast<LDR_DATA_TABLE_ENTRY64* const>(pCurLdrTableEntry);
					
					break;
				}

				if (!ReadProcessMemory(hProc, &pNextEntry->Flink, &pNextEntry, sizeof(ULONGLONG), nullptr)) break;

			}

			return pLdrTableEntry;
		}

		#endif // _WIN64


		LDR_DATA_TABLE_ENTRY32* getLdrDataTableEntry32Address(HANDLE hProc, const char* modName) {
			// x86 loader data table pointer is stored in x86 process envrionment block
			const PEB32* const pPeb32 = proc::ex::getPeb32Address(hProc);

			if (!pPeb32) return nullptr;

			PEB32 peb{};

			if (!ReadProcessMemory(hProc, pPeb32, &peb, sizeof(PEB32), nullptr)) return nullptr;

			PEB_LDR_DATA32* pPebLdrData32 = reinterpret_cast<PEB_LDR_DATA32*>(static_cast<uintptr_t>(peb.Ldr));
			PEB_LDR_DATA32 pebLdrData{};

			if (!ReadProcessMemory(hProc, pPebLdrData32, &pebLdrData, sizeof(PEB_LDR_DATA32), nullptr)) return nullptr;

			wchar_t wModName[MAX_PATH]{};
			MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, modName, -1, wModName, MAX_PATH);

			LDR_DATA_TABLE_ENTRY32* pLdrTableEntry = nullptr;
			// static cast to convert from DWORD (4 bytes) to uintptr_t (4 bytes for x86, 8 bytes for x64)
			const LIST_ENTRY32* pNextEntry = reinterpret_cast<LIST_ENTRY32*>(static_cast<uintptr_t>(pebLdrData.InMemoryOrderModuleList.Flink));

			// walk the linked list until back at the beginning
			while (pNextEntry != &pPebLdrData32->InMemoryOrderModuleList) {
				const LDR_DATA_TABLE_ENTRY32* const pCurLdrTableEntry = CONTAINING_RECORD(pNextEntry, LDR_DATA_TABLE_ENTRY32, InMemoryOrderLinks);
				LDR_DATA_TABLE_ENTRY32 curDataTableEntry{};

				if (!ReadProcessMemory(hProc, pCurLdrTableEntry, &curDataTableEntry, sizeof(LDR_DATA_TABLE_ENTRY32), nullptr)) break;

				if (!curDataTableEntry.BaseDllName.Buffer) break;

				const wchar_t* const wRemoteCurModName = reinterpret_cast<wchar_t*>(static_cast<uintptr_t>(curDataTableEntry.BaseDllName.Buffer));
				wchar_t wCurModName[MAX_PATH]{};

				if (!ReadProcessMemory(hProc, wRemoteCurModName, wCurModName, curDataTableEntry.BaseDllName.Length, nullptr)) break;

				if (!_wcsicmp(wModName, wCurModName)) {
					pLdrTableEntry = const_cast<LDR_DATA_TABLE_ENTRY32* const>(pCurLdrTableEntry);

					break;
				}

				if (!ReadProcessMemory(hProc, &pNextEntry->Flink, &pNextEntry, sizeof(DWORD), nullptr)) break;

			}

			return pLdrTableEntry;
		}


		#ifdef _WIN64

		PEB64* getPeb64Address(HANDLE hProc) {
			const HMODULE hNtdll = in::getModuleHandle("ntdll.dll");

			if (!hNtdll) return nullptr;

			const tNtQueryInformationProcess NtQueryInformationProcess = reinterpret_cast<tNtQueryInformationProcess>(in::getProcAddress(hNtdll, "NtQueryInformationProcess"));

			if (!NtQueryInformationProcess) return nullptr;

			PROCESS_BASIC_INFORMATION pbi{};
			NtQueryInformationProcess(hProc, ProcessBasicInformation, &pbi, sizeof(pbi), nullptr);

			return reinterpret_cast<PEB64*>(pbi.PebBaseAddress);
		}

		#endif // _WIN64


		PEB32* getPeb32Address(HANDLE hProc) {
			const HMODULE hNtdll = in::getModuleHandle("ntdll.dll");

			if (!hNtdll) return nullptr;

			const tNtQueryInformationProcess NtQueryInformationProcess = reinterpret_cast<tNtQueryInformationProcess>(in::getProcAddress(hNtdll, "NtQueryInformationProcess"));

			if (!NtQueryInformationProcess) return nullptr;

			PEB32* pPeb = nullptr;
			// returns the x86 process envrionment block for an x86 process running in the WOW64 envrionment
			NtQueryInformationProcess(hProc, ProcessWow64Information, &pPeb, sizeof(pPeb), nullptr);

			return pPeb;
		}


		static bool getDataDirFromPeHeaders(HANDLE hProc, const PeHeaders* pPeHeaders, IMAGE_DATA_DIRECTORY* pDataDir, char index) {
			if (pPeHeaders->pOptHeader64) {
				IMAGE_OPTIONAL_HEADER64 optHeader64{};
				
				if (!ReadProcessMemory(hProc, pPeHeaders->pOptHeader64, &optHeader64, sizeof(IMAGE_OPTIONAL_HEADER64), nullptr)) return false;

				if (memcpy_s(pDataDir, sizeof(IMAGE_DATA_DIRECTORY), &optHeader64.DataDirectory[index], sizeof(IMAGE_DATA_DIRECTORY))) return false;
			}
			else if (pPeHeaders->pOptHeader32) {
				IMAGE_OPTIONAL_HEADER32 optHeader32{};
				
				if (!ReadProcessMemory(hProc, pPeHeaders->pOptHeader32, &optHeader32, sizeof(IMAGE_OPTIONAL_HEADER32), nullptr)) return false;

				if (memcpy_s(pDataDir, sizeof(IMAGE_DATA_DIRECTORY), &optHeader32.DataDirectory[index], sizeof(IMAGE_DATA_DIRECTORY))) return false;
			}
			else {
				return false;
			}

			return true;
		}


		// use only with IMAGE_THUNK_DATA64 / ULONGLONG or IMAGE_THUNK_DATA32 / DWORD type combinations
		template <typename ITD, typename FLG>
		static BYTE* getIatEntryAddressFromImportDesc(HANDLE hProc, const char* funcName, const BYTE* pBase, const IMAGE_IMPORT_DESCRIPTOR* pImportDesc, FLG ordinalFlag) {
			BYTE* pIatEntry = nullptr;
			// thunks get overwritten with the function address at load time with the actual function address
			const ITD* pThunk = reinterpret_cast<const ITD*>(pBase + pImportDesc->FirstThunk);
			// original thunks do not get overwritten with the function address at load time
			const ITD* pOriginalThunk = reinterpret_cast<const ITD*>(pBase + pImportDesc->OriginalFirstThunk);
			ITD originalThunk{};

			if (!ReadProcessMemory(hProc, pOriginalThunk, &originalThunk, sizeof(ITD), nullptr)) return nullptr;

			while (originalThunk.u1.AddressOfData) {

				// not imported by ordinal
				if (!(originalThunk.u1.Ordinal & ordinalFlag)) {
					const IMAGE_IMPORT_BY_NAME* const pImportByName = reinterpret_cast<const IMAGE_IMPORT_BY_NAME*>(pBase + originalThunk.u1.AddressOfData);
					char curFuncName[MAX_PATH]{};

					if (!mem::ex::copyRemoteString(hProc, curFuncName, reinterpret_cast<const BYTE*>(pImportByName->Name), MAX_PATH)) break;
						
					if (!_stricmp(funcName, curFuncName)) {
						pIatEntry = const_cast<BYTE*>(reinterpret_cast<const BYTE*>(pThunk));

						break;
					}
					
				}

				pOriginalThunk++;
				pThunk++;

				if (!ReadProcessMemory(hProc, pOriginalThunk, &originalThunk, sizeof(ITD), nullptr)) break;

			}

			return pIatEntry;
		}

	}


	namespace in {

		FARPROC getProcAddress(HMODULE hMod, const char* funcName) {
			const BYTE* const pBase = reinterpret_cast<BYTE*>(hMod);
			PeHeaders peHeaders{};

			if (!getPeHeaders(hMod, &peHeaders) || !peHeaders.pOptHeader) return nullptr;

			const IMAGE_DATA_DIRECTORY dirEntryExport = peHeaders.pOptHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];

			const IMAGE_EXPORT_DIRECTORY* const pExportDir = reinterpret_cast<const IMAGE_EXPORT_DIRECTORY*>(pBase + dirEntryExport.VirtualAddress);

			if (!pExportDir) return nullptr;

			const DWORD* const pExportFunctionTable = reinterpret_cast<const DWORD*>(pBase + pExportDir->AddressOfFunctions);

			if (!pExportFunctionTable) return nullptr;

			const DWORD* const pExportNameTable = reinterpret_cast<const DWORD*>(pBase + pExportDir->AddressOfNames);

			if (!pExportNameTable) return nullptr;

			const WORD* const pExportOrdinalTable = reinterpret_cast<const WORD*>(pBase + pExportDir->AddressOfNameOrdinals);

			if (!pExportOrdinalTable) return nullptr;

			
			DWORD funcRva = 0;
			// export by ordinal if everything but the lowest word of name param is zero
			const bool byOrdinal = (reinterpret_cast<uintptr_t>(funcName) & 0xFFFFFFFFFFFF0000) == 0;

			if (byOrdinal) {
				const WORD index = static_cast<WORD>((reinterpret_cast<uintptr_t>(funcName) & 0xFFFF) - pExportDir->Base);
				funcRva = pExportFunctionTable[index];
			}
			else {
				
				for (DWORD i = 0; i < pExportDir->NumberOfNames; i++) {
					const char* const curFuncName = reinterpret_cast<const char*>(pBase + pExportNameTable[i]);

					if (!_stricmp(funcName, curFuncName)) {
						// the function rva is in the export table indexed by the ordinal in the ordinal table at the same index as the name in the name table
						funcRva = pExportFunctionTable[pExportOrdinalTable[i]];

						break;
					}

				}

			}

			if (!funcRva) return nullptr;

			FARPROC procAddress = nullptr;
			const bool forwarded = funcRva >= dirEntryExport.VirtualAddress && funcRva <= dirEntryExport.VirtualAddress + dirEntryExport.Size;

			if (forwarded) {
				// forward has the format "module.function"
				// it is split a the dot and the ".dll" extension is appended to the module name
				char curForward[MAX_PATH]{};
				strcpy_s(curForward, reinterpret_cast<const char*>(pBase + funcRva));

				char* forwardModName = nullptr;
				char* forwardFuncName = nullptr;

				forwardModName = strtok_s(curForward, ".", &forwardFuncName);

				char forwardModFileName[MAX_PATH]{};

				strcpy_s(forwardModFileName, forwardModName);
				strcat_s(forwardModFileName, ".dll");

				HMODULE hForwardMod = getModuleHandle(forwardModFileName);

				if (!hForwardMod) return nullptr;

				// check if exported by ordinal and looking for the forwarded funcion in the module it was forwarded to
				if (forwardFuncName[0] == '#') {
					char* forwardFuncOrdinal = reinterpret_cast<char*>(static_cast<uintptr_t>(atoi(forwardFuncName++)));
					procAddress = getProcAddress(hForwardMod, forwardFuncOrdinal);
				}
				else {
					procAddress = getProcAddress(hForwardMod, forwardFuncName);
				}

			}
			else {
				procAddress = reinterpret_cast<FARPROC>(pBase + funcRva);
			}

			return procAddress;
		}


		BYTE* getIatEntryAddress(HMODULE hImportMod, const char* exportModName, const char* funcName) {
			PeHeaders peHeaders{};
			
			if (!getPeHeaders(hImportMod, &peHeaders)) return nullptr;

			if (!peHeaders.pOptHeader) return nullptr;

			const IMAGE_DATA_DIRECTORY* const pDirEntryImport = &peHeaders.pOptHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
			const BYTE* const pBase = const_cast<BYTE*>(reinterpret_cast<const BYTE*>(peHeaders.pDosHeader));
			const IMAGE_IMPORT_DESCRIPTOR* pImportDesc = reinterpret_cast<const IMAGE_IMPORT_DESCRIPTOR*>(pBase + pDirEntryImport->VirtualAddress);

			bool found = false;
			
			while (pImportDesc->Characteristics) {
				
				if (!_stricmp(exportModName, reinterpret_cast<const char*>(pBase + pImportDesc->Name))) {
					// found the import descriptor of the module exportng the function
					found = true;

					break;
				}

				pImportDesc++;
			}

			if (!found) return nullptr;

			BYTE* pIatEntry = nullptr;
			// thunks get overwritten with the function address at load time with the actual function address
			const IMAGE_THUNK_DATA* pThunk = reinterpret_cast<const IMAGE_THUNK_DATA*>(pBase + pImportDesc->FirstThunk);
			// original thunks do not get overwritten with the function address at load time
			const IMAGE_THUNK_DATA* pOriginalThunk = reinterpret_cast<const IMAGE_THUNK_DATA*>(pBase + pImportDesc->OriginalFirstThunk);

			while (pOriginalThunk->u1.AddressOfData) {

				// not imported by ordinal
				if (!(pOriginalThunk->u1.Ordinal & IMAGE_ORDINAL_FLAG)) {
					const IMAGE_IMPORT_BY_NAME* const pImportByName = reinterpret_cast<const IMAGE_IMPORT_BY_NAME*>(pBase + pOriginalThunk->u1.AddressOfData);

					if (!_stricmp(funcName, pImportByName->Name)) {
						pIatEntry = const_cast<BYTE*>(reinterpret_cast<const BYTE*>(&pThunk->u1.Function));

						break;
					}

				}

				pOriginalThunk++;
				pThunk++;

			}

			return pIatEntry;
		}

		bool getPeHeaders(HMODULE hMod, PeHeaders* pPeHeaderPointers) {
			
			if (!hMod || !pPeHeaderPointers) return false;

			const BYTE* pBase = reinterpret_cast<const BYTE*>(hMod);
			pPeHeaderPointers->pDosHeader = reinterpret_cast<const IMAGE_DOS_HEADER*>(pBase);
			
			// make sure it is a a vaild pe 
			if (pPeHeaderPointers->pDosHeader->e_magic != IMAGE_DOS_SIGNATURE) return false;

			const DWORD ntSig = *reinterpret_cast<const DWORD*>(pBase + pPeHeaderPointers->pDosHeader->e_lfanew);
			
			// make sure it is a a vaild pe header
			if (ntSig != IMAGE_NT_SIGNATURE) return false;

			pPeHeaderPointers->pFileHeader = reinterpret_cast<const IMAGE_FILE_HEADER*>(pBase + pPeHeaderPointers->pDosHeader->e_lfanew + sizeof(IMAGE_NT_SIGNATURE));
			
			// check if binary is x64 so we can get the header addresses of a pe header in a local buffer as well
			if (pPeHeaderPointers->pFileHeader->SizeOfOptionalHeader == sizeof(IMAGE_OPTIONAL_HEADER64)) {
				pPeHeaderPointers->pNtHeaders64 = reinterpret_cast<const IMAGE_NT_HEADERS64*>(pBase + pPeHeaderPointers->pDosHeader->e_lfanew);
				pPeHeaderPointers->pOptHeader64 = reinterpret_cast<const IMAGE_OPTIONAL_HEADER64*>(reinterpret_cast<const BYTE*>(pPeHeaderPointers->pFileHeader) + sizeof(IMAGE_FILE_HEADER));
				
				#ifdef _WIN64

				// if the caller and target (buffer) architecture match we set the values of the general headers for easier usage
				pPeHeaderPointers->pNtHeaders = pPeHeaderPointers->pNtHeaders64;
				pPeHeaderPointers->pOptHeader = pPeHeaderPointers->pOptHeader64;
				
				#endif // _WIN64

			}
			else if (pPeHeaderPointers->pFileHeader->SizeOfOptionalHeader == sizeof(IMAGE_OPTIONAL_HEADER32)) {
				pPeHeaderPointers->pNtHeaders32 = reinterpret_cast<const IMAGE_NT_HEADERS32*>(pBase + pPeHeaderPointers->pDosHeader->e_lfanew);
				pPeHeaderPointers->pOptHeader32 = reinterpret_cast<const IMAGE_OPTIONAL_HEADER32*>(reinterpret_cast<const BYTE*>(pPeHeaderPointers->pFileHeader) + sizeof(IMAGE_FILE_HEADER));
				
				#ifndef _WIN64

				// if the caller and target (buffer) architecture match we set the values of the general headers for easier usage
				pPeHeaderPointers->pNtHeaders = pPeHeaderPointers->pNtHeaders32;
				pPeHeaderPointers->pOptHeader = pPeHeaderPointers->pOptHeader32;
				
				#endif // _WIN64

			}
			else {

				return false;
			}

			return true;
		}


		HMODULE getModuleHandle(const char* modName) {
			LDR_DATA_TABLE_ENTRY* pLdrEntry = getLdrDataTableEntryAddress(modName);

			if (!pLdrEntry) return nullptr;

			return static_cast<HMODULE>(pLdrEntry->DllBase);
		}


		LDR_DATA_TABLE_ENTRY* getLdrDataTableEntryAddress(const char* modName) {
			wchar_t wModName[MAX_PATH]{};
			MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, modName, -1, wModName, MAX_PATH);

			const PEB* const pPeb = getPebAddress();

			LDR_DATA_TABLE_ENTRY* pLdrTableEntry = nullptr;
			LIST_ENTRY* pNextEntry = pPeb->Ldr->InMemoryOrderModuleList.Flink;

			// walk the linked list until back at the beginning
			while (pNextEntry != &pPeb->Ldr->InMemoryOrderModuleList) {

				const LDR_DATA_TABLE_ENTRY* const pCurLdrTableEntry = CONTAINING_RECORD(pNextEntry, LDR_DATA_TABLE_ENTRY, InMemoryOrderLinks);

				if (!_wcsicmp(wModName, pCurLdrTableEntry->BaseDllName.Buffer)) {
					pLdrTableEntry = const_cast<LDR_DATA_TABLE_ENTRY* const>(pCurLdrTableEntry);

					break;
				}

				pNextEntry = pNextEntry->Flink;

			}

			return pLdrTableEntry;
		}


		PEB* getPebAddress() {
			
			#ifdef _WIN64

			PEB* pPeb = reinterpret_cast<PEB*>(__readgsqword(0x60));
			
			#else

			PEB* pPeb = reinterpret_cast<PEB*>(__readfsdword(0x30));

			#endif

			return pPeb;
		}

	}

}