#include <Windows.h>
#include <vector>

using namespace std;

uintptr_t nGame = (uintptr_t)GetModuleHandle("game.dll");

#define ASM _declspec(naked)

#define WSAAPI _stdcall

typedef HANDLE WSAEVENT;

typedef struct _WSABUF {
	ULONG len;
	UCHAR* buf;
} WSABUF, * LPWSABUF;

typedef struct _WSASOCK {
	SOCKET s;
	UCHAR* buf;
} WSASOCK, * LPWSASOCK;

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
//- Variables

vector<SOCKET> sockets;
UINT position = 0;

//-
//-----------------------------------------------------------------------------

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
	return patch(nAddress, 0xe8, 1) ? patch(nAddress + 1, (uintptr_t)lpFunction - (nAddress + 5), 4) : false;
}

bool jmp(uintptr_t nAddress, LPVOID lpFunction)
{
	return patch(nAddress, 0xe9, 1) ? patch(nAddress + 1, (uintptr_t)lpFunction - (nAddress + 5), 4) : false;
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

void CALLBACK AddSocket(SOCKET s)
{
	position = 1;

	sockets.push_back(s);
}

void CALLBACK Cleaner()
{
	position = 0;

	vector<SOCKET>().swap(sockets);
}

DWORD WINAPI WSARecv_Proc(LPVOID lpParameter)
{
	LPWSASOCK lpWSASock = (LPWSASOCK)lpParameter;

	if (lpWSASock->buf[1] == 4) // If we connected to host
	{
		sockets.push_back(lpWSASock->s);
		position = lpWSASock->buf[0x28];
	}

	if (lpWSASock->buf[1] == 0x1b) // If server shutdown
		for (size_t i = 0; i < sockets.size(); i++)
			if (sockets[i] == lpWSASock->s)
				sockets.erase(sockets.begin() + i);

	if (lpWSASock->buf[1] == 0xf && lpWSASock->buf[7] == 0x11) // If we got our packet
		Beep(500, 200);

	free(lpWSASock->buf);
	
	delete[] lpWSASock;

	return 0;
}

char a[2] = { '\xF7', '\x28' };
char b[4] = { '\x01', '\x01', '\x01', '\x11' };
LPCSTR sstring = "Lolka";

DWORD WINAPI KeyBoard_Proc(LPVOID lpParameter)
{
	while (true)
	{
		if ((GetKeyState('A') & 0x8000) && position)
		{
			LPSTR buf = (LPSTR)calloc(9 + strlen(sstring), sizeof(char));

			buf[0] = (char)0xf7;
			buf[1] = position == 1 ? 0xf : 0x28;
			buf[2] = (char)(9 + strlen(sstring));
			buf[4] = 1;
			buf[5] = 1;
			buf[6] = 1;
			buf[7] = 0x11;
			memcpy(&buf[8], sstring, strlen(sstring));

			if (position == 1)
				for (size_t i = 0; i < sockets.size(); i++)
				{
					/*buf[5] = position == 1 ? 1 : (char)(i + 2);
					send(sockets[i], buf, 9 + strlen(sstring), 0);*/
				}
			else
			{
				buf[6] = position;
				send(sockets[0], buf, 9 + strlen(sstring), 0);
			}

			free(buf);

			while ((GetKeyState('A') & 0x8000))
				Sleep(100);
		}
	}

	return 0;
}

//-
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
//- Proxies

int WSAAPI WSARecvProxy(SOCKET s, LPWSABUF lpBuffers, DWORD dwBufferCount, LPDWORD lpNumberOfBytesRecvd, LPDWORD lpFlags, LPWSAOVERLAPPED lpOverlapped, LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine)
{
	LPSTR buf = (LPSTR)calloc(1460, sizeof(char));
	int ret = WSARecv(s, lpBuffers, dwBufferCount, lpNumberOfBytesRecvd, lpFlags, lpOverlapped, lpCompletionRoutine);

	if (lpBuffers->buf[0] == 0xf7)
	{
		HANDLE hThread = CreateThread(NULL, NULL, WSARecv_Proc, new WSASOCK{ s, (UCHAR*)memcpy(buf, lpBuffers->buf, lpBuffers->buf[2]) }, NULL, NULL);
		CloseHandle(hThread);
	}

	/*if (lpBuffers->buf[0] != 0xf7)
		return ret;*/

	/*HANDLE hThread = CreateThread(NULL, NULL, WSARecv_Proc, new WSASOCK{ s, lpBuffers->buf }, NULL, NULL);
	CloseHandle(hThread);*/

	/*if (lpBuffers->buf[1] == 4) // If we connected to host
	{
		sockets.push_back(s);
		position = lpBuffers->buf[0x28];
	}

	if (lpBuffers->buf[1] == 0x1b) // If server shutdown
		for (size_t i = 0; i < sockets.size(); i++)
			if (sockets[i] == s)
				sockets.erase(sockets.begin() + i);

	if (lpBuffers->buf[1] == 0xf && lpBuffers->buf[7] == 0x11) // If we got our packet
		Beep(500, 200);*/

	return ret;
}

int sendProxy(SOCKET s, LPCSTR buf, int len, int flags)
{
	Sleep(10);

	return send(s, buf, len, flags);
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
		mov [eax], ecx
		mov [eax + 4], ecx
		mov [eax + 8], ecx
		mov [eax + 0xc], ecx
		mov [eax + 0x10], ecx
		mov [eax + 0x14], ecx
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
		mov [esp + 0x1c], ecx
		mov [esp + 0x18], ecx
		mov ebx, 0x5b4
		sub ebx, edx
		lea edx, [edx + esi + 0x78]
		lea ecx, [esp + 0x1c]
		push ecx
		mov [esp + 0x28], edx
		push 1
		lea edx, [esp + 0x28]
		push edx
		push eax
		mov [esp + 0x30], ebx
		call edi
		cmp eax, -1
		pop ebx
		jne pSuccessful2
		mov eax, nGame
		add eax, 0x86d89c
		call [eax]
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
		call [eax]
		test eax, eax
		jne pSuccessful2
		mov eax, nGame
		add eax, 0x86d1e4
		call [eax]
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

void ASM f00000002()
{
	_asm
	{
		pushad
		mov ecx, [ecx + 4]
		push ecx
		call AddSocket
		popad
		test ecx, ecx
		jne pSuccessful1
		mov[esp + 4], 0x57
		mov eax, nGame
		add eax, 0x6eb5ac
		jmp eax
	pSuccessful1 :
		mov eax, [ecx]
		push esi
		mov esi, [esp + 8]
		push esi
		push edx
		mov edx, [eax + 0x2c]
		call edx
		pop esi
		ret 4
	}
}

void ASM f00000003()
{
	_asm
	{
		test ecx, ecx
		jne pSuccessful1
		mov[esp + 4], 0x57
		mov eax, nGame
		add eax, 0x6eb5ac
		jmp eax
	pSuccessful1 :
		mov eax, [ecx]
		push esi
		mov esi, [esp + 8]
		push esi
		push edx
		mov edx, [eax + 0x2c]
		call edx
		pushad
		call Cleaner
		popad
		pop esi
		ret 4
	}
}

//-
//-----------------------------------------------------------------------------

BOOL APIENTRY DllMain(HMODULE hModule, UINT ul_reason_for_call, LPVOID lpReserve)
{
	if (ul_reason_for_call == DLL_PROCESS_ATTACH && nGame)
	{
		patch(nGame + 0x86d8c0, (int)sendProxy, 4);

		jmp(nGame + 0x6da990, f00000001);

		call(nGame + 0x669cd4, f00000002);

		call(nGame + 0x677a24, f00000003);

		HANDLE hThread = CreateThread(NULL, NULL, KeyBoard_Proc, NULL, NULL, NULL);
		CloseHandle(hThread);

		return TRUE;
	}

	return FALSE;
}