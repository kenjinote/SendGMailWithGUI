#pragma comment(linker,"\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#pragma comment(lib, "legacy_stdio_definitions")
#pragma comment(lib, "crypt32")
#pragma comment(lib, "libeay32")
#pragma comment(lib, "ssleay32")
#pragma comment(lib, "ws2_32")
#pragma comment(lib, "shlwapi")
#pragma comment(lib, "rpcrt4")
#pragma comment(lib, "uxtheme")

#include <winsock2.h>
#include <openssl/ssl.h>
#include <openssl/rand.h>
#include <shlwapi.h>
#include <uxtheme.h>
#include <vsstyle.h>
#include <vssym32.h>
#include <string>
#include "resource.h"

TCHAR szClassName[] = TEXT("Window");
WNDPROC DefaultEditBoxWndProc;
WNDPROC DefaultListBoxWndProc;

#define WINDOW_WIDTH		640
#define WINDOW_HEIGHT		500
#define OKBUTTON_WIDTH		128
#define MARGIN				10
#define STATIC_WIDTH		128

struct AttachmentData
{
	TCHAR szFilePath[MAX_PATH];
	HICON hIcon;
};

CHAR convtobase(const CHAR c)
{
	if (c <= 0x19)return c + 'A';
	if (c >= 0x1a && c <= 0x33)return c - 0x1a + 'a';
	if (c >= 0x34 && c <= 0x3d)return c - 0x34 + '0';
	if (c == 0x3e)return '+';
	if (c == 0x3f)return '/';
	return '=';
}

LPSTR encode(LPCSTR lpszOrg)
{
	static CHAR szReturnValue[1024];
	int i = 0, iR = 16;
	lstrcpyA(szReturnValue, "=?ISO-2022-JP?B?");
	for (;;)
	{
		if (lpszOrg[i] == '\0')
		{
			break;
		}
		szReturnValue[iR] = convtobase((lpszOrg[i]) >> 2);
		if (lpszOrg[i + 1] == '\0')
		{
			szReturnValue[iR + 1] = convtobase(((lpszOrg[i] & 0x3) << 4));
			szReturnValue[iR + 2] = '=';
			szReturnValue[iR + 3] = '=';
			szReturnValue[iR + 4] = '\0';
			break;
		}
		szReturnValue[iR + 1] = convtobase(((lpszOrg[i] & 0x3) << 4) + ((lpszOrg[i + 1]) >> 4));
		if (lpszOrg[i + 2] == '\0')
		{
			szReturnValue[iR + 2] = convtobase((lpszOrg[i + 1] & 0xf) << 2);
			szReturnValue[iR + 3] = '=';
			szReturnValue[iR + 4] = '\0';
			break;
		}
		szReturnValue[iR + 2] = convtobase(((lpszOrg[i + 1] & 0xf) << 2) + ((lpszOrg[i + 2]) >> 6));
		szReturnValue[iR + 3] = convtobase(lpszOrg[i + 2] & 0x3f);
		szReturnValue[iR + 4] = '\0';
		i += 3;
		iR += 4;
	}
	lstrcatA(szReturnValue, "?=");
	return szReturnValue;
}

LPSTR MySJisToJis(LPCWSTR lpszOrg)
{
	LPSTR MultiString = 0;
	{
		DWORD dwTextLen = WideCharToMultiByte(50220, 0, lpszOrg, -1, 0, 0, 0, 0);
		if (dwTextLen)
		{
			MultiString = (LPSTR)GlobalAlloc(GMEM_FIXED, sizeof(CHAR) * dwTextLen);
			WideCharToMultiByte(50220, 0, lpszOrg, -1, MultiString, dwTextLen, 0, 0);
		}
	}
	return MultiString;
}

void SSL_write(SSL * ssl, LPCSTR lpszStringA)
{
	SSL_write(ssl, lpszStringA, lstrlenA(lpszStringA));
#ifdef _DEBUG
	OutputDebugStringA(lpszStringA);
#endif
}

void SSL_write(SSL * ssl, LPCWSTR lpszStringW)
{
	DWORD nLength = WideCharToMultiByte(932, 0, lpszStringW, -1, 0, 0, 0, 0);
	LPSTR lpszStringA = (LPSTR)GlobalAlloc(0, nLength);
	WideCharToMultiByte(932, 0, lpszStringW, -1, lpszStringA, nLength, 0, 0);
	SSL_write(ssl, lpszStringA, nLength - 1);
	GlobalFree(lpszStringA);
#ifdef _DEBUG
	OutputDebugStringW(lpszStringW);
#endif
}

void SSL_read(SSL * ssl)
{
	CHAR szStrRcv[1024 * 50];
	int err = SSL_read(ssl, szStrRcv, sizeof(szStrRcv));
	szStrRcv[err] = '\0';
#ifdef _DEBUG
	OutputDebugStringA(szStrRcv);
#endif
}

LPSTR base64encode(LPBYTE lpData, DWORD dwSize)
{
	DWORD dwResult = 0;
	if (CryptBinaryToStringA(lpData, dwSize, CRYPT_STRING_BASE64, 0, &dwResult))
	{
		LPSTR lpszBase64 = (LPSTR)GlobalAlloc(GMEM_FIXED, dwResult);
		if (CryptBinaryToStringA(lpData, dwSize, CRYPT_STRING_BASE64, lpszBase64, &dwResult))
		{
			*(LPWORD)(lpszBase64 + dwResult - 2) = 0;
			return lpszBase64;
		}
	}
	return 0;
}

LPWSTR CreateGuid()
{
	static WCHAR szGUID[37] = L"E698B9EF-3E28-410B-B51B-9F180701E642";
	GUID guid = GUID_NULL;
	HRESULT hr = UuidCreate(&guid);
	if (HRESULT_CODE(hr) != RPC_S_OK)
	{
#ifdef _DEBUG
		OutputDebugStringW(L"UuidCreate関数が失敗しました。");
#endif
		return szGUID;
	}
	if (guid == GUID_NULL)
	{
#ifdef _DEBUG
		OutputDebugStringW(L"新しいGUIDは生成されませんでした。");
#endif
		return szGUID;
	}
	if (HRESULT_CODE(hr) == RPC_S_UUID_NO_ADDRESS)
	{
#ifdef _DEBUG
		OutputDebugStringW(L"UUIDを作成するために使用できるネットワークアドレスがありません。");
#endif
	}
	if (HRESULT_CODE(hr) == RPC_S_UUID_LOCAL_ONLY)
	{
#ifdef _DEBUG
		OutputDebugStringW(L"このコンピュータでのみ有効なUUIDが割り当てられました。");
#endif
	}
	wsprintf(szGUID, TEXT("%08lX-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X"), guid.Data1, guid.Data2, guid.Data3, guid.Data4[0], guid.Data4[1], guid.Data4[2], guid.Data4[3], guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]);
	return szGUID;
}

BOOL SendMail(
	LPWSTR lpszToAddress,
	LPCWSTR lpszFromAddress,
	LPCWSTR lpszPassword,
	LPCWSTR lpszSubject,
	LPWSTR lpszMessage,
	LPWSTR*lpszAttachment,
	const int nAttachmentCount
)
{
	BOOL bRet = FALSE;
	WCHAR szTextW[1024];
	CHAR szTextA[1024];
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(1, 1), &wsaData))
	{
#ifdef _DEBUG
		OutputDebugStringW(L"エラー: WinSockの初期化に失敗しました。\r\n");
#endif
		goto END0;
	}
	LPSTR*lpszBase64Code;
	lpszBase64Code = (LPSTR*)GlobalAlloc(GMEM_ZEROINIT, sizeof(LPSTR)*nAttachmentCount);
	if (lpszBase64Code == 0)
	{
#ifdef _DEBUG
		OutputDebugStringW(L"エラー: 添付の初期化に失敗しました。\r\n");
#endif
		goto END1;
	}
	for (int i = 0; i < nAttachmentCount; i++)
	{
		HANDLE hFile1 = CreateFileW(lpszAttachment[i], GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
		if (hFile1 == INVALID_HANDLE_VALUE)
		{
#ifdef _DEBUG
			OutputDebugStringW(L"エラー: 添付ファイルが開けませんでした。\r\n");
			OutputDebugStringW(PathFindFileNameW(lpszAttachment[i]));
			OutputDebugStringW(L"\r\n");
#endif
			goto END2;
		}
		LARGE_INTEGER FileSize;
		SecureZeroMemory(&FileSize, sizeof(FileSize));
		if (!GetFileSizeEx(hFile1, &FileSize))
		{
			CloseHandle(hFile1);
#ifdef _DEBUG
			OutputDebugStringW(L"エラー: 添付ファイルサイズを取得できませんでした。\r\n");
			OutputDebugStringW(PathFindFileNameW(lpszAttachment[i]));
			OutputDebugStringW(L"\r\n");
#endif
			goto END2;
		}
		if (FileSize.HighPart || FileSize.LowPart > 1024 * 1024 * 25 || FileSize.LowPart == 0)
		{
			CloseHandle(hFile1);
#ifdef _DEBUG
			OutputDebugStringW(L"エラー: 添付ファイルサイズが異常です。\r\n");
			OutputDebugStringW(PathFindFileNameW(lpszAttachment[i]));
			OutputDebugStringW(L"\r\n");
#endif
			goto END2;
		}
		LPBYTE p; p = (LPBYTE)GlobalAlloc(GMEM_FIXED, FileSize.LowPart);
		if (p == NULL)
		{
			CloseHandle(hFile1);
#ifdef _DEBUG
			OutputDebugStringW(L"エラー: 添付ファイルのためのメモリが確保できませんでした。\r\n");
			OutputDebugStringW(PathFindFileNameW(lpszAttachment[i]));
			OutputDebugStringW(L"\r\n");
#endif
			goto END2;
		}
		DWORD end1;
		ReadFile(hFile1, p, FileSize.LowPart, &end1, NULL);
		CloseHandle(hFile1);
		if (end1 == 0)
		{
			GlobalFree(p);
#ifdef _DEBUG
			OutputDebugStringW(L"エラー: 添付ファイルからデータを読み込めませんでした。\r\n");
			OutputDebugStringW(PathFindFileNameW(lpszAttachment[i]));
			OutputDebugStringW(L"\r\n");
#endif
			goto END2;
		}
		lpszBase64Code[i] = base64encode(p, FileSize.LowPart);
		GlobalFree(p);
		if (lpszBase64Code[i] == 0)
		{
#ifdef _DEBUG
			OutputDebugStringW(L"エラー: 添付ファイルのBase64エンコードに失敗しました。\r\n");
			OutputDebugStringW(PathFindFileNameW(lpszAttachment[i]));
			OutputDebugStringW(L"\r\n");
#endif
			goto END2;
		}
	}
	LPHOSTENT lpHost = gethostbyname("smtp.gmail.com");
	if (lpHost == 0)
	{
#ifdef _DEBUG
		OutputDebugStringW(L"エラー: メールサーバーが見つかりませんでした。\r\n");
#endif
		goto END2;
	}
	SOCKET s = socket(PF_INET, SOCK_STREAM, 0);
	if (s == INVALID_SOCKET)
	{
#ifdef _DEBUG
		OutputDebugStringW(L"エラー: ソケットをオープンできませんでした。\r\n");
#endif
		goto END2;
	}
	SOCKADDR_IN sockadd;
	sockadd.sin_family = AF_INET;
	sockadd.sin_port = htons(465);
	sockadd.sin_addr = *((LPIN_ADDR)*lpHost->h_addr_list);
	if (connect(s, (PSOCKADDR)&sockadd, sizeof(sockadd)))
	{
#ifdef _DEBUG
		OutputDebugStringW(L"エラー: サーバーソケットに接続に失敗しました。\r\n");
#endif
		goto END3;
	}
	SSLeay_add_ssl_algorithms();
	SSL_load_error_strings();
	SSL_library_init();
	char qrandomstring[15];
	srand((unsigned int)time(0));
	wsprintfA(qrandomstring, "%d", rand());
	RAND_seed(qrandomstring, (int)strlen(qrandomstring));
	SSL_CTX *ctx; ctx = SSL_CTX_new(SSLv23_client_method());
	SSL *ssl; ssl = SSL_new(ctx);
	SSL_set_fd(ssl, (int)s);
	if (SSL_connect(ssl) <= 0)
	{
#ifdef _DEBUG
		OutputDebugStringW(L"エラー: SSLの初期化に失敗しました。\r\n");
#endif
		goto END4;
	}
	SSL_write(ssl, L"EHLO localhost\r\n");
	SSL_read(ssl);
	SSL_read(ssl);
	SSL_write(ssl, L"AUTH LOGIN\r\n");
	SSL_read(ssl);
	{
		DWORD nLength = WideCharToMultiByte(932, 0, lpszFromAddress, -1, 0, 0, 0, 0);
		LPSTR lpszFromAddressA = (LPSTR)GlobalAlloc(0, nLength);
		WideCharToMultiByte(932, 0, lpszFromAddress, -1, lpszFromAddressA, nLength, 0, 0);
		LPSTR p = base64encode((LPBYTE)lpszFromAddressA, lstrlenA(lpszFromAddressA));
		wsprintfA(szTextA, "%s\r\n", p);
		GlobalFree(p);
		GlobalFree(lpszFromAddressA);
		SSL_write(ssl, szTextA);
	}
	SSL_read(ssl);
	{
		DWORD nLength = WideCharToMultiByte(932, 0, lpszPassword, -1, 0, 0, 0, 0);
		LPSTR lpszzPasswordA = (LPSTR)GlobalAlloc(0, nLength);
		WideCharToMultiByte(932, 0, lpszPassword, -1, lpszzPasswordA, nLength, 0, 0);
		LPSTR p = base64encode((LPBYTE)lpszzPasswordA, lstrlenA(lpszzPasswordA));
		wsprintfA(szTextA, "%s\r\n", p);
		GlobalFree(p);
		GlobalFree(lpszzPasswordA);
		SSL_write(ssl, szTextA);
	}
	SSL_read(ssl);
	wsprintfW(szTextW, L"MAIL FROM: <%s>\r\n", lpszFromAddress);
	SSL_write(ssl, szTextW);
	SSL_read(ssl);
	LPCWSTR lpszDelimiter1 = L",";
	LPWSTR token = _wcstok(lpszToAddress, lpszDelimiter1);
	while (token)
	{
		LPWSTR szText = (LPWSTR)GlobalAlloc(0, (lstrlenW(token) + 1) * sizeof(WCHAR));
		lstrcpyW(szText, token);
		StrTrimW(szText, L" ");
		wsprintfW(szTextW, L"RCPT TO: <%s>\r\n", szText);
		GlobalFree(szText);
		SSL_write(ssl, szTextW);
		SSL_read(ssl);
		token = _wcstok(0, lpszDelimiter1);
	}
	SSL_write(ssl, L"DATA\r\n");
	SSL_read(ssl);
	wsprintfW(szTextW, L"From: <%s>\r\n", lpszFromAddress);
	SSL_write(ssl, szTextW);
	wsprintfW(szTextW, L"To: <%s>\r\n", lpszToAddress);
	SSL_write(ssl, szTextW);
	LPSTR lpTemp = MySJisToJis(lpszSubject);
	if (lpTemp)
	{
		wsprintfA(szTextA, "Subject: %s\r\n", encode(lpTemp));
		SSL_write(ssl, szTextA);
		GlobalFree(lpTemp);
	}
	WCHAR szBoundary[128];
	lstrcpy(szBoundary, CreateGuid());
	SSL_write(ssl, L"MIME-Version: 1.0\r\n");
	wsprintfW(szTextW, L"Content-Type: multipart/mixed; boundary=\"%s\"\r\n", szBoundary);
	SSL_write(ssl, szTextW);
	SSL_write(ssl, L"\r\n");
	wsprintfW(szTextW, L"--%s\r\n", szBoundary);
	SSL_write(ssl, szTextW);
	SSL_write(ssl, L"Content-Type: text/plain; charset=shift_jis\r\n");
	SSL_write(ssl, L"\r\n");
	LPCWSTR lpszDelimiter2 = L"\n";
	token = _wcstok(lpszMessage, lpszDelimiter2);
	while (token)
	{
		if (token != lpszMessage)
		{
			if (token[0] == L'.')
			{
				SSL_write(ssl, L".");
			}
		}
		SSL_write(ssl, token);
		SSL_write(ssl, lpszDelimiter2);
		token = _wcstok(0, lpszDelimiter2);
	}
	for (int i = 0; i < nAttachmentCount; i++)
	{
		wsprintfW(szTextW, L"--%s\r\n", szBoundary);
		SSL_write(ssl, szTextW);
		SSL_write(ssl, L"Content-Type: application/octet-stream\r\n");
		lpTemp = MySJisToJis(PathFindFileNameW(lpszAttachment[i]));
		if (lpTemp)
		{
			wsprintfA(szTextA, "Content-Disposition: attachment; filename=\"%s\"\r\n", encode(lpTemp));
			SSL_write(ssl, szTextA);
			GlobalFree(lpTemp);
		}
		SSL_write(ssl, L"Content-Transfer-Encoding: base64\r\n");
		SSL_write(ssl, L"\r\n");
		DWORD j, nLen;
		nLen = lstrlenA(lpszBase64Code[i]);
		for (j = 0; j < nLen; j += 66)
		{
			if (nLen - j > 66)
			{
				CopyMemory(szTextA, lpszBase64Code[i] + j, 66);
				szTextA[66] = 0;
			}
			else
			{
				lstrcpyA(szTextA, lpszBase64Code[i] + j);
				lstrcatA(szTextA, "\r\n");
			}
			SSL_write(ssl, szTextA);
		}
	}
	wsprintfW(szTextW, L"--%s--\r\n", szBoundary);
	SSL_write(ssl, szTextW);
	SSL_write(ssl, "\x0d\x0a.\x0d\x0a");
	SSL_read(ssl);
	SSL_write(ssl, "QUIT\r\n");
	SSL_read(ssl);
	bRet = TRUE;
END4:
	SSL_shutdown(ssl);
	SSL_free(ssl);
	SSL_CTX_free(ctx);
END3:
	shutdown(s, SD_BOTH);
	closesocket(s);
END2:
	for (int i = 0; i < nAttachmentCount; i++)
	{
		GlobalFree(lpszBase64Code[i]);
	}
	GlobalFree(lpszBase64Code);
END1:
	WSACleanup();
END0:
	return bRet;
}

void ReplaceAll(std::wstring& str, const std::wstring& from, const std::wstring& to)
{
	size_t start_pos = 0;
	while ((start_pos = str.find(from, start_pos)) != std::wstring::npos)
	{
		str.replace(start_pos, from.length(), to);
		start_pos += to.length();
	}
}

LRESULT CALLBACK ListBoxProc1(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_COMMAND:
		if (LOWORD(wParam) == 1000)
		{
			int nSelectedItemsCount = (int)SendMessage(hWnd, LB_GETSELCOUNT, 0, 0);
			int *pSelectedItems = new int[nSelectedItemsCount];
			SendMessage(hWnd, LB_GETSELITEMS, nSelectedItemsCount, (LPARAM)pSelectedItems);
			for (int i = nSelectedItemsCount - 1; i >= 0; i--)
			{
				AttachmentData * pData = (AttachmentData *)SendMessage(hWnd, LB_GETITEMDATA, pSelectedItems[i], 0);
				DestroyIcon(pData->hIcon);
				delete pData;
				SendMessage(hWnd, LB_DELETESTRING, pSelectedItems[i], 0);
			}
			delete[] pSelectedItems;
		}
		break;
	case WM_RBUTTONDOWN:
		{
			DWORD dwPos = GetMessagePos();
			POINT pt = { LOWORD(dwPos), HIWORD(dwPos) };
			ScreenToClient(hWnd, &pt);
			// カーソル位置のインデックス値を求める
			int nCursorIndex = (int)SendMessage(hWnd, LB_ITEMFROMPOINT, 0, MAKELPARAM(pt.x, pt.y));
			if (nCursorIndex == LB_ERR) break;
			// 選択されていたら何もしない
			if (SendMessage(hWnd, LB_GETSEL, nCursorIndex, 0)) break;
			// 選択されていない場合は、選択をすべて解除した後に
			int nSelectedItemsCount = (int)SendMessage(hWnd, LB_GETSELCOUNT, 0, 0);
			int *pSelectedItems = new int[nSelectedItemsCount];
			SendMessage(hWnd, LB_GETSELITEMS, nSelectedItemsCount, (LPARAM)pSelectedItems);
			for (int i =  0; i < nSelectedItemsCount; ++i)
			{
				SendMessage(hWnd, LB_SETSEL, FALSE, pSelectedItems[i]);
			}
			delete[] pSelectedItems;
			// カーソル位置を選択
			SendMessage(hWnd, LB_SETSEL, TRUE, nCursorIndex);
		}
		break;
	case WM_KEYDOWN:
		if (wParam == VK_DELETE)
		{
			PostMessage(hWnd, WM_COMMAND, 1000, 0);
		}
		break;
	case WM_CONTEXTMENU:
		{
			int nSelectedItemsCount = (int)SendMessage(hWnd, LB_GETSELCOUNT, 0, 0);
			HMENU hMenuPopup = CreatePopupMenu();
			AppendMenu(hMenuPopup, MF_STRING | (nSelectedItemsCount == 0 ? MF_DISABLED : 0), 1000, (LPCTSTR)TEXT("削除(&D)\tDelete"));
			DWORD dwPos = GetMessagePos();
			POINT pt = { LOWORD(dwPos), HIWORD(dwPos) };
			TrackPopupMenuEx(hMenuPopup, TPM_LEFTALIGN | TPM_RIGHTBUTTON | TPM_NOANIMATION, pt.x, pt.y, hWnd, NULL);
			DestroyMenu(hMenuPopup);
		}
		break;
	default:
		break;
	}
	return CallWindowProc(DefaultListBoxWndProc, hWnd, msg, wParam, lParam);
}

LRESULT CALLBACK EditBoxProc1(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_PASTE:
		{
			OpenClipboard(NULL);
			HANDLE hText = GetClipboardData(CF_UNICODETEXT);
			if (hText)
			{
				LPWSTR lpszBuf = (LPWSTR)GlobalLock(hText);
				{
					std::wstring strClipboardText(lpszBuf);
					ReplaceAll(strClipboardText, L"\r\n", L"\n");
					ReplaceAll(strClipboardText, L"\r", L"\n");
					ReplaceAll(strClipboardText, L"\n", L"\r\n");
					SendMessage(hWnd, EM_REPLACESEL, 0, (LPARAM)strClipboardText.c_str());
				}
				GlobalUnlock(hText);
			}
			CloseClipboard();
		}
		return 0;
	default:
		break;
	}
	return CallWindowProc(DefaultEditBoxWndProc, hWnd, msg, wParam, lParam);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	static HWND hEdit1, hEdit2, hEdit3, hEdit4, hEdit5, hButton, hList, hStatic;
	static HFONT hFont;
	static DOUBLE dControlHeight = 32.0;
	static DWORD dwIconSize = 32;
	switch (msg)
	{
	case WM_CREATE:
		{
			HTHEME hTheme = OpenThemeData(hWnd, VSCLASS_AEROWIZARD);
			LOGFONT lf = { 0 };
			GetThemeFont(hTheme, NULL, AW_HEADERAREA, 0, TMT_FONT, &lf);
			hFont = CreateFontIndirectW(&lf);
			dControlHeight = abs(lf.lfHeight * 1.8);
			CloseThemeData(hTheme);
			TCHAR szExePath[MAX_PATH];
			GetModuleFileName(((LPCREATESTRUCT)lParam)->hInstance, szExePath, _countof(szExePath));
			ICONINFO iconInfo = { 0 };
			WORD nIcon = 0;
			HICON hIcon = ExtractAssociatedIcon(((LPCREATESTRUCT)lParam)->hInstance, szExePath, &nIcon);
			GetIconInfo(hIcon, &iconInfo);
			if (iconInfo.hbmColor)
			{
				BITMAP bitmap;
				GetObject(iconInfo.hbmMask, sizeof(bitmap), &bitmap);
				dwIconSize = bitmap.bmWidth;
			}
			else
			{
				BITMAP bitmap;
				GetObject(iconInfo.hbmMask, sizeof(bitmap), &bitmap);
				dwIconSize = bitmap.bmWidth;
			}
			DestroyIcon(hIcon);
		}
		SendMessage(CreateWindowW(L"STATIC", L"宛先(&A):", WS_VISIBLE | WS_CHILD | SS_RIGHT, MARGIN, (int)(MARGIN + (dControlHeight + MARGIN) * 0), STATIC_WIDTH, (int)dControlHeight, hWnd, 0, ((LPCREATESTRUCT)lParam)->hInstance, 0), WM_SETFONT, (WPARAM)hFont, 0);
		hEdit1 = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", 0, WS_VISIBLE | WS_CHILD | WS_TABSTOP | ES_AUTOHSCROLL, MARGIN * 2 + STATIC_WIDTH, (int)(MARGIN + (dControlHeight + MARGIN) * 0), 512, (int)dControlHeight, hWnd, (HMENU)100, ((LPCREATESTRUCT)lParam)->hInstance, 0);
		SendMessage(hEdit1, EM_LIMITTEXT, 0, 0);
		SendMessage(hEdit1, WM_SETFONT, (WPARAM)hFont, 0);
		SendMessage(CreateWindowW(L"STATIC", L"差出人(&F):", WS_VISIBLE | WS_CHILD | SS_RIGHT, MARGIN, (int)(MARGIN + (dControlHeight + MARGIN) * 1), STATIC_WIDTH, (int)dControlHeight, hWnd, 0, ((LPCREATESTRUCT)lParam)->hInstance, 0), WM_SETFONT, (WPARAM)hFont, 0);
		hEdit4 = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", 0, WS_VISIBLE | WS_CHILD | WS_TABSTOP | ES_AUTOHSCROLL, MARGIN * 2 + STATIC_WIDTH, (int)(MARGIN + (dControlHeight + MARGIN) * 1), 512, (int)dControlHeight, hWnd, (HMENU)103, ((LPCREATESTRUCT)lParam)->hInstance, 0);
		SendMessage(hEdit4, EM_LIMITTEXT, 0, 0);
		SendMessage(hEdit4, WM_SETFONT, (WPARAM)hFont, 0);
		SendMessage(CreateWindowW(L"STATIC", L"パスワード(&P):", WS_VISIBLE | WS_CHILD | SS_RIGHT, MARGIN, (int)(MARGIN + (dControlHeight + MARGIN) * 2), STATIC_WIDTH, (int)dControlHeight, hWnd, 0, ((LPCREATESTRUCT)lParam)->hInstance, 0), WM_SETFONT, (WPARAM)hFont, 0);
		hEdit5 = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", 0, WS_VISIBLE | WS_CHILD | WS_TABSTOP | ES_AUTOHSCROLL | ES_PASSWORD, MARGIN * 2 + STATIC_WIDTH, (int)(MARGIN + (dControlHeight + MARGIN) * 2), 512, (int)dControlHeight, hWnd, (HMENU)104, ((LPCREATESTRUCT)lParam)->hInstance, 0);
		SendMessage(hEdit5, EM_LIMITTEXT, 0, 0);
		SendMessage(hEdit5, WM_SETFONT, (WPARAM)hFont, 0);	
		SendMessage(CreateWindowW(L"STATIC", L"件名(&T):", WS_VISIBLE | WS_CHILD | SS_RIGHT, MARGIN, (int)(MARGIN + (dControlHeight + MARGIN) * 3), STATIC_WIDTH, (int)dControlHeight, hWnd, 0, ((LPCREATESTRUCT)lParam)->hInstance, 0), WM_SETFONT, (WPARAM)hFont, 0);
		hEdit2 = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", 0, WS_VISIBLE | WS_CHILD | WS_TABSTOP | ES_AUTOHSCROLL, MARGIN * 2 + STATIC_WIDTH, (int)(MARGIN + (dControlHeight + MARGIN) * 3), 512, (int)dControlHeight, hWnd, (HMENU)101, ((LPCREATESTRUCT)lParam)->hInstance, 0);
		SendMessage(hEdit2, EM_LIMITTEXT, 0, 0);
		SendMessage(hEdit2, WM_SETFONT, (WPARAM)hFont, 0);
		SendMessage(CreateWindowW(L"STATIC", L"本文(&B):", WS_VISIBLE | WS_CHILD | SS_RIGHT, MARGIN, (int)(MARGIN + (dControlHeight + MARGIN) * 4), STATIC_WIDTH, (int)dControlHeight, hWnd, 0, ((LPCREATESTRUCT)lParam)->hInstance, 0), WM_SETFONT, (WPARAM)hFont, 0);
		hEdit3 = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", 0, WS_VISIBLE | WS_CHILD | WS_TABSTOP | WS_VSCROLL | ES_AUTOVSCROLL | ES_MULTILINE | ES_WANTRETURN, MARGIN * 2 + STATIC_WIDTH, (int)(MARGIN + (dControlHeight + MARGIN) * 4), 512, (int)(dControlHeight * 10), hWnd, (HMENU)102, ((LPCREATESTRUCT)lParam)->hInstance, 0);
		SendMessage(hEdit3, EM_LIMITTEXT, 0, 0);
		SendMessage(hEdit3, WM_SETFONT, (WPARAM)hFont, 0);
		DefaultEditBoxWndProc = (WNDPROC)SetWindowLongPtr(hEdit3, GWLP_WNDPROC, (LONG_PTR)EditBoxProc1);
		hStatic = CreateWindowW(L"STATIC", L"添付(&C):", WS_VISIBLE | WS_CHILD | SS_RIGHT, MARGIN, (int)(MARGIN + (dControlHeight + MARGIN) * 4), STATIC_WIDTH, (int)dControlHeight, hWnd, 0, ((LPCREATESTRUCT)lParam)->hInstance, 0);
		SendMessage(hStatic, WM_SETFONT, (WPARAM)hFont, 0);
		hList = CreateWindowExW(WS_EX_CLIENTEDGE, L"LISTBOX", 0, WS_VISIBLE | WS_CHILD | WS_TABSTOP | WS_VSCROLL | LBS_OWNERDRAWFIXED | LBS_NOINTEGRALHEIGHT | LBS_MULTIPLESEL | LBS_EXTENDEDSEL, MARGIN * 2 + STATIC_WIDTH, (int)(MARGIN + (dControlHeight + MARGIN) * 4), 512, (int)(dControlHeight * 10), hWnd, (HMENU)102, ((LPCREATESTRUCT)lParam)->hInstance, 0);
		SendMessage(hList, WM_SETFONT, (WPARAM)hFont, 0);
		DefaultListBoxWndProc = (WNDPROC)SetWindowLongPtr(hList, GWLP_WNDPROC, (LONG_PTR)ListBoxProc1);
		hButton = CreateWindowW(L"BUTTON", L"送信(F5)", WS_VISIBLE | WS_CHILD | WS_TABSTOP | BS_DEFPUSHBUTTON, MARGIN, (int)(WINDOW_HEIGHT - (dControlHeight + MARGIN)), OKBUTTON_WIDTH, (int)dControlHeight, hWnd, (HMENU)IDOK, ((LPCREATESTRUCT)lParam)->hInstance, 0);
		SendMessage(hButton, WM_SETFONT, (WPARAM)hFont, 0);
		SendMessage(hEdit3, WM_PASTE, 0, 0);
		SendMessage(hEdit3, EM_SETSEL, 0, -1);
		SetFocus(hEdit3);
		DragAcceptFiles(hWnd, TRUE);
		break;
	case WM_DROPFILES:
		{
			const UINT nFileCount = DragQueryFile((HDROP)wParam, 0xFFFFFFFF, NULL, 0);
			for (UINT i = 0; i < nFileCount; ++i)
			{
				TCHAR szFilePath[MAX_PATH];
				DragQueryFile((HDROP)wParam, i, szFilePath, _countof(szFilePath));
				AttachmentData * pData = new AttachmentData;
				lstrcpy(pData->szFilePath, szFilePath);
				WORD nIcon;
				pData->hIcon = ExtractAssociatedIcon(GetModuleHandle(0), szFilePath, &nIcon);
				const int nIndex = SendMessage(hList, LB_ADDSTRING, 0, (LPARAM)szFilePath);
				SendMessage(hList, LB_SETITEMDATA, nIndex, (LPARAM)pData);
			}
			DragFinish((HDROP)wParam);
		}
		break;
	case WM_SIZE:
		MoveWindow(hEdit1, MARGIN * 2 + STATIC_WIDTH, (int)(MARGIN + (dControlHeight + MARGIN) * 0), LOWORD(lParam) - (MARGIN * 3 + STATIC_WIDTH), (int)dControlHeight, TRUE);
		MoveWindow(hEdit4, MARGIN * 2 + STATIC_WIDTH, (int)(MARGIN + (dControlHeight + MARGIN) * 1), LOWORD(lParam) - (MARGIN * 3 + STATIC_WIDTH), (int)dControlHeight, TRUE);
		MoveWindow(hEdit5, MARGIN * 2 + STATIC_WIDTH, (int)(MARGIN + (dControlHeight + MARGIN) * 2), LOWORD(lParam) - (MARGIN * 3 + STATIC_WIDTH), (int)dControlHeight, TRUE);
		MoveWindow(hEdit2, MARGIN * 2 + STATIC_WIDTH, (int)(MARGIN + (dControlHeight + MARGIN) * 3), LOWORD(lParam) - (MARGIN * 3 + STATIC_WIDTH), (int)dControlHeight, TRUE);
		MoveWindow(hEdit3, MARGIN * 2 + STATIC_WIDTH, (int)(MARGIN + (dControlHeight + MARGIN) * 4), LOWORD(lParam) - (MARGIN * 3 + STATIC_WIDTH), (int)(HIWORD(lParam) - (MARGIN * 2 + (dControlHeight + MARGIN) * 4) - (dControlHeight * 4 + MARGIN * 1)), TRUE);
		MoveWindow(hStatic, MARGIN, (int)(HIWORD(lParam) - (dControlHeight * 4 + MARGIN)), STATIC_WIDTH, (int)(dControlHeight), TRUE);
		MoveWindow(hList, MARGIN * 2 + STATIC_WIDTH, (int)(HIWORD(lParam) - (dControlHeight * 4 + MARGIN)), LOWORD(lParam) - (MARGIN * 3 + STATIC_WIDTH), (int)(dControlHeight * 4), TRUE);
		MoveWindow(hButton, MARGIN, (int)(HIWORD(lParam) - (dControlHeight + MARGIN)), OKBUTTON_WIDTH, (int)dControlHeight, TRUE);
		break;
	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK)
		{
			HWND hEdit;
			for (int i = 100; i < 105; i++)
			{
				hEdit = GetDlgItem(hWnd, i);
				if (!GetWindowTextLengthW(hEdit)) { SetFocus(hEdit); return 0; }
			}
			for (int i = 100; i < 105; i++)
			{
				hEdit = GetDlgItem(hWnd, i);
				EnableWindow(hEdit, FALSE);
			}
			EnableWindow(GetDlgItem(hWnd, IDOK), FALSE);
			EnableWindow(GetDlgItem(hWnd, IDCANCEL), FALSE);
			DWORD dwToLength = GetWindowTextLengthW(hEdit1);
			LPWSTR lpszTo = (LPWSTR)GlobalAlloc(0, (dwToLength + 1) * sizeof(WCHAR));
			GetWindowTextW(hEdit1, lpszTo, dwToLength + 1);
			DWORD dwFromLength = GetWindowTextLengthW(hEdit4);
			LPWSTR lpszFrom = (LPWSTR)GlobalAlloc(0, (dwFromLength + 1) * sizeof(WCHAR));
			GetWindowTextW(hEdit4, lpszFrom, dwFromLength + 1);
			DWORD dwPasswordLength = GetWindowTextLengthW(hEdit5);
			LPWSTR lpszPassword = (LPWSTR)GlobalAlloc(0, (dwPasswordLength + 1) * sizeof(WCHAR));
			GetWindowTextW(hEdit5, lpszPassword, dwPasswordLength + 1);
			DWORD dwTitleLength = GetWindowTextLengthW(hEdit2);
			LPWSTR lpszTitle = (LPWSTR)GlobalAlloc(0, (dwTitleLength + 1) * sizeof(WCHAR));
			GetWindowTextW(hEdit2, lpszTitle, dwTitleLength + 1);
			DWORD dwMessageLength = GetWindowTextLengthW(hEdit3);
			LPWSTR lpszMessage = (LPWSTR)GlobalAlloc(0, (dwMessageLength + 1) * sizeof(WCHAR));
			GetWindowTextW(hEdit3, lpszMessage, dwMessageLength + 1);
			ShowWindow(hWnd, SW_HIDE);
			int nAttachmentFileCount = 0;
			LPWSTR *ppszAttachmentFilePathList = 0;
			{
				nAttachmentFileCount = SendMessage(hList, LB_GETCOUNT, 0, 0);
				ppszAttachmentFilePathList = (LPWSTR*)GlobalAlloc(0, sizeof(LPWSTR) * nAttachmentFileCount);
				for (int i = 0; i < nAttachmentFileCount; ++i)
				{
					AttachmentData * pData = (AttachmentData *)SendMessage(hList, LB_GETITEMDATA, i, 0);
					ppszAttachmentFilePathList[i] = pData->szFilePath;
				}
			}
			if (!SendMail(lpszTo, lpszFrom, lpszPassword, lpszTitle, lpszMessage, ppszAttachmentFilePathList, nAttachmentFileCount))
			{
				ShowWindow(hWnd, SW_SHOW);
				MessageBoxW(hWnd, L"送信に失敗しました。", 0, 0);
			}
			else
			{
				PostMessage(hWnd, WM_CLOSE, 0, 0);
			}
			GlobalFree(ppszAttachmentFilePathList);
			GlobalFree(lpszTo);
			GlobalFree(lpszFrom);
			GlobalFree(lpszPassword);
			GlobalFree(lpszTitle);
			GlobalFree(lpszMessage);
			for (int i = 100; i < 105; i++)
			{
				hEdit = GetDlgItem(hWnd, i);
				EnableWindow(hEdit, TRUE);
			}
			EnableWindow(GetDlgItem(hWnd, IDOK), TRUE);
			EnableWindow(GetDlgItem(hWnd, IDCANCEL), TRUE);
		}
		else if (LOWORD(wParam) == IDCANCEL)
		{
			DestroyWindow(hWnd);
			return 1;
		}
		break;
	case WM_CLOSE:
		DestroyWindow(hWnd);
		break;
	case WM_MEASUREITEM:
		{
			PMEASUREITEMSTRUCT pmis = (PMEASUREITEMSTRUCT)lParam;
			pmis->itemHeight = (int)(dwIconSize + dControlHeight / 4.0);
		}
		break;
	case WM_DRAWITEM:
		{
			PDRAWITEMSTRUCT pdis = (PDRAWITEMSTRUCT)lParam;
			if (pdis->itemID == -1)
			{
				break;
			}
			switch (pdis->itemAction)
			{
			case ODA_SELECT:
			case ODA_DRAWENTIRE:
				{
					AttachmentData * pData = (AttachmentData *)SendMessage(pdis->hwndItem, LB_GETITEMDATA, pdis->itemID, 0);
					RECT rect;
					rect.left = pdis->rcItem.left;
					rect.top = pdis->rcItem.top;
					rect.right = pdis->rcItem.right;
					rect.bottom = pdis->rcItem.bottom;
					if (pdis->itemState & ODS_SELECTED)
					{
						HBRUSH hBrush = CreateSolidBrush(GetSysColor(COLOR_HIGHLIGHT));
						FillRect(pdis->hDC, &rect, (HBRUSH)hBrush);
						DeleteObject(hBrush);
						DrawFocusRect(pdis->hDC, &rect);
						SetTextColor(pdis->hDC, GetSysColor(COLOR_HIGHLIGHTTEXT));
					}
					else
					{
						HBRUSH hBrush = CreateSolidBrush(GetSysColor(COLOR_WINDOW));
						FillRect(pdis->hDC, &rect, (HBRUSH)hBrush);
						DeleteObject(hBrush);
						SetTextColor(pdis->hDC, GetSysColor(COLOR_WINDOWTEXT));
					}
					DrawIcon(pdis->hDC, (int)(pdis->rcItem.left + dControlHeight / 8.0), (int)(pdis->rcItem.top + dControlHeight / 8.0), pData->hIcon);
					TextOut(pdis->hDC, (int)(pdis->rcItem.left + dwIconSize + dControlHeight / 4.0), (int)(pdis->rcItem.top + dControlHeight / 8.0), pData->szFilePath, lstrlen(pData->szFilePath));
				}
				break;
			case ODA_FOCUS:
				break;
			}
		}
		break;
	case WM_DESTROY:
		{
			const int nListItemMax = SendMessage(hList, LB_GETCOUNT, 0, 0);
			for (int i = 0; i < nListItemMax; ++i)
			{				
				AttachmentData * pData = (AttachmentData *)SendMessage(hList, LB_GETITEMDATA, i, 0);
				DestroyIcon(pData->hIcon);
				delete pData;
			}
		}
		DeleteObject(hFont);
		PostQuitMessage(0);
		break;
	default:
		return(DefDlgProc(hWnd, msg, wParam, lParam));
	}
	return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPreInst, LPSTR pCmdLine, int nCmdShow)
{
	MSG msg;
	WNDCLASS wndclass = {
		0,
		WndProc,
		0,
		DLGWINDOWEXTRA,
		hInstance,
		LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1)),
		LoadCursor(0,IDC_ARROW),
		0,
		0,
		szClassName
	};
	RegisterClass(&wndclass);
	HWND hWnd = CreateWindow(
		szClassName,
		TEXT("GMail でメールを送る"),
		WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
		CW_USEDEFAULT,
		0,
		CW_USEDEFAULT,
		0,
		0,
		0,
		hInstance,
		0
	);
	ShowWindow(hWnd, SW_SHOWDEFAULT);
	UpdateWindow(hWnd);
	ACCEL Accel[] = { { FVIRTKEY,VK_F5,IDOK } };
	HACCEL hAccel = CreateAcceleratorTable(Accel, _countof(Accel));
	while (GetMessage(&msg, 0, 0, 0))
	{
		if (!TranslateAccelerator(hWnd, hAccel, &msg) && !IsDialogMessage(hWnd, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	DestroyAcceleratorTable(hAccel);
	return (int)msg.wParam;
}
