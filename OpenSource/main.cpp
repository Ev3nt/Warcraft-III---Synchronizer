#include <Windows.h>

uintptr_t nGame = (uintptr_t)GetModuleHandle("game.dll");

#define ASM _declspec(naked)

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

void ASM f00000001()
{
	_asm
	{
		sub esp, 0x10
		push esi
		mov esi, ecx
		mov eax, [esi]
		mov edx, [eax + 0x14]
		push edi
		call edx
		xor ecx, ecx
		lea eax, [esi + 0x634]
		mov[eax], ecx
		mov[eax + 4], ecx
		mov[eax + 8], ecx
		mov[eax + 0xc], ecx
		mov[eax + 0x10], ecx
		mov[eax + 0x14], ecx
		mov edi, WSARecvProxy
		cmp edi, ecx
		mov edx, [esi + 0x74]
		je pSuccessful1
		push ebx
		push ecx
		push eax
		lea eax, [esp + 0x14]
		push eax
		mov eax, [esi + 4]
		mov[esp + 0x1c], ecx
		mov[esp + 0x18], ecx
		mov ebx, 0x5b4
		sub ebx, edx
		lea edx, [edx + esi + 0x78]
		lea ecx, [esp + 0x1c]
		push ecx
		mov[esp + 0x28], edx
		push 1
		lea edx, [esp + 0x28]
		push edx
		push eax
		mov[esp + 0x30], ebx
		call edi
		cmp eax, -1
		pop ebx
		jne pSuccessful2
		mov eax, nGame
		add eax, 0x86d89c
		call[eax]
		cmp eax, 0x3e5
		jne pSuccessful3
	pSuccessful2 :
		pop edi
		pop esi
		add esp, 0x10
		ret
	pSuccessful1 :
		push eax
		mov eax, [esi + 4]
		push ecx
		mov ecx, 0x5b4
		sub ecx, edx
		push ecx
		lea edx, [edx + esi + 0x78]
		push edx
		push eax
		mov eax, nGame
		add eax, 0x86d1c8
		call[eax]
		test eax, eax
		jne pSuccessful2
		mov eax, nGame
		add eax, 0x86d1e4
		call[eax]
		cmp eax, 0x3e5
		je pSuccessful2
	pSuccessful3 :
		mov edx, [esi]
		mov eax, [edx + 0x24]
		mov ecx, esi
		call eax
		mov edx, [esi]
		mov eax, [edx + 0x18]
		pop edi
		mov ecx, esi
		pop esi
		add esp, 0x10
		jmp eax
	}
}

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