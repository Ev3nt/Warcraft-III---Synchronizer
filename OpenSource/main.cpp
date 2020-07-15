#include <Windows.h>

uintptr_t nGame = (uintptr_t)GetModuleHandle("game.dll");

#define WSAAPI _stdcall

typedef HANDLE WSAEVENT;

typedef struct _WSABUF {
	ULONG len;
	UCHAR* buf;
} WSABUF, * LPWSABUF;

typedef struct _WSAOVERLAPPED {
	DWORD    Internal;
	DWORD    InternalHigh;
	DWORD    Offset;
	DWORD    OffsetHigh;
	WSAEVENT hEvent;
} WSAOVERLAPPED, * LPWSAOVERLAPPED;

typedef void (CALLBACK* WSAOVERLAPPED_COMPLETION_ROUTINE)();
typedef WSAOVERLAPPED_COMPLETION_ROUTINE* LPWSAOVERLAPPED_COMPLETION_ROUTINE;

//-----------------------------------------------------------------------------
//- Memory

template <typename Return, typename Function, typename ...Arguments>
Return inline stdcall(Function function, Arguments ...arguments)
{
	return reinterpret_cast<Return(CALLBACK*)(Arguments...)>(function)(arguments...);
}

bool patch(uintptr_t nAddress, DWORD dwBYTES, size_t nSize)
{
	DWORD dwOldProtect;
	if (VirtualProtect((LPVOID)nAddress, nSize, PAGE_EXECUTE_READWRITE, &dwOldProtect))
	{
		memcpy((LPVOID)nAddress, (LPVOID)&dwBYTES, nSize);
		VirtualProtect((LPVOID)nAddress, nSize, dwOldProtect, NULL);

		return true;
	}

	return false;
}

bool call(uintptr_t nAddress, LPVOID lpFunction)
{
	return patch(nAddress, 0xE8, 1) ? patch(nAddress + 1, (uintptr_t)lpFunction - (nAddress + 5), 4) : false;
}

bool jmp(uintptr_t nAddress, LPVOID lpFunction)
{
	return patch(nAddress, 0xE9, 1) ? patch(nAddress + 1, (uintptr_t)lpFunction - (nAddress + 5), 4) : false;
}

bool fill(uintptr_t nAddress, DWORD dwBYTE, size_t nSize)
{
	bool ret = true;
	for (size_t i = 0; i < nSize && ret; i++)
		ret = patch(nAddress + i, dwBYTE, 1);
	
	return ret;
}
//-
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
//- WSA Functions

int WSAAPI WSARecv(SOCKET s, LPWSABUF lpBuffers, DWORD dwBufferCount, LPDWORD lpNumberOfBytesRecvd, LPDWORD lpFlags, LPWSAOVERLAPPED lpOverlapped, LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine)
{
	return stdcall<int>(*(uintptr_t*)(nGame + 0xada624), s, lpBuffers, dwBufferCount, lpNumberOfBytesRecvd, lpFlags, lpOverlapped, lpCompletionRoutine);
}

int WSAAPI send(SOCKET s, LPCSTR buf, int len, int flags)
{
	return stdcall<int>(*(uintptr_t*)(nGame + 0x86d8c0), s, buf, len, flags);
}

//-
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
//- Functions

//-
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
//- Proxies

int WSAAPI WSARecvProxy(SOCKET s, LPWSABUF lpBuffers, DWORD dwBufferCount, LPDWORD lpNumberOfBytesRecvd, LPDWORD lpFlags, LPWSAOVERLAPPED lpOverlapped, LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine)
{
	return WSARecv(s, lpBuffers, dwBufferCount, lpNumberOfBytesRecvd, lpFlags, lpOverlapped, lpCompletionRoutine);
}

//-
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
//- ASM

//-
//-----------------------------------------------------------------------------

BOOL APIENTRY DllMain(HMODULE hModule, UINT ul_reason_for_call, LPVOID lpReserve)
{
	if (ul_reason_for_call == DLL_PROCESS_ATTACH && nGame)
	{
		return TRUE;
	}

	return FALSE;
}