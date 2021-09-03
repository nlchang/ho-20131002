// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved

#include "Private.h"
#include "DictionarySearch.h"
#include "SampleIMEBaseStructure.h"

//+---------------------------------------------------------------------------
//
// ctor
//
//----------------------------------------------------------------------------

CDictionarySearch::CDictionarySearch(LCID locale, _In_ CFile *pFile, _In_ CStringRange *pSearchKeyCode) : CDictionaryParser(locale)
{
#define HOLINE	70    //the maximum lines of index in a hotable file
#define HOFILE_HEADER_SIZE 2       //16-bit unicode bom character size=2, U+FEFF

	_pFile = pFile;
	ULONGLONG dwFileLength = pFile->GetFileSize();

	_pSearchKeyCode = pSearchKeyCode;
	_charIndex = 0;			//搜尋區段的開頭
	//-------------------------------------------------------

	wchar_t  * pEnd;
	int linecount = 0;
	int charcount = 0;
	WCHAR* pFptr = (WCHAR*)pFile->GetReadBufferPointer();
	const WCHAR* pKptr = pSearchKeyCode->Get();
	//--------------------------------------------------------------------------
	// 計算字根索引的長度
	while (!((*pFptr == L'\r') && (*(pFptr + 1) == L'\n') && (*(pFptr + 2) == L'\r') && (*(pFptr + 3) == L'\n')))
	{
		charcount++;
		pFptr++;
	}
	charcount += 4;	//origin: 4
	pFptr = (WCHAR*)pFile->GetReadBufferPointer();
	//--------------------------------------------------------------------------

	while ((*pFptr != *pKptr) && (linecount < HOLINE))
	{
		//pFile->NextLine();
		while (*pFptr != L'\r')
		{
			pFptr++;
		}
		pFptr++;
		// charcount++;
		if (*pFptr == L'\n')
		{
			linecount++;
			pFptr++;
		}
		else
		{
			linecount = HOLINE;    //table file format error
		}
	}
	if (linecount >= HOLINE)
	{
		_charIndex = 0;
	}
	else
	{
		pFptr += 2;
		_charIndex = std::wcstol(pFptr, &pEnd, 10) + charcount - HOFILE_HEADER_SIZE;	//指向搜尋區段的開頭
		//-------------------------------------------------------------------------------------------------------------
				// get table search end value
		while (*pFptr != L'\r')
		{
			pFptr++;
		}
		pFptr++;
		if (*pFptr == L'\n')
		{
			pFptr++;
		}
		else
		{
			linecount = HOLINE;    //table file format error
		}
		pFptr += 2;
		if ((*pFptr == L'\r') && (*(pFptr + 1) == L'\n'))	//如果是最後一個字根
			_tableTail = dwFileLength / sizeof(wchar_t) - charcount - HOFILE_HEADER_SIZE;	// 1 unicode char = 2 bytes
		else
			_tableTail = std::wcstol(pFptr, &pEnd, 10) + charcount - HOFILE_HEADER_SIZE;
	}
	//--------------------------------------------------------------------------------------------
}

//+---------------------------------------------------------------------------
//
// dtor
//
//----------------------------------------------------------------------------

CDictionarySearch::~CDictionarySearch()
{
}

//+---------------------------------------------------------------------------
//
// FindPhrase
//
//----------------------------------------------------------------------------

BOOL CDictionarySearch::FindPhrase(_Out_ CDictionaryResult **ppdret)
{
	return FindWorker(FALSE, ppdret, FALSE); // NO WILDCARD
}

//+---------------------------------------------------------------------------
//
// FindPhraseForWildcard
//
//----------------------------------------------------------------------------

BOOL CDictionarySearch::FindPhraseForWildcard(_Out_ CDictionaryResult **ppdret)
{
	return FindWorker(FALSE, ppdret, TRUE); // Wildcard
	//return FindWorker(FALSE, ppdret, FALSE); // NO Wildcard
}

//+---------------------------------------------------------------------------
//
// FindConvertedStringForWildcard
//
//----------------------------------------------------------------------------

BOOL CDictionarySearch::FindConvertedStringForWildcard(CDictionaryResult **ppdret)
{
	// return FindWorker(TRUE, ppdret, TRUE); // Wildcard
	return FindWorker(TRUE, ppdret, FALSE); // NO Wildcard
}

//+---------------------------------------------------------------------------
//
// FindWorker
//
//----------------------------------------------------------------------------

BOOL CDictionarySearch::FindWorker(BOOL isTextSearch, _Out_ CDictionaryResult **ppdret, BOOL isWildcardSearch)
{
	DWORD_PTR dwTotalBufLen = GetBufferInWCharLength();        // in char
	dwTotalBufLen = _tableTail;        // in char
	if (dwTotalBufLen == 0)
	{
		return FALSE;
	}

	const WCHAR *pwch = GetBufferInWChar();
	DWORD_PTR indexTrace = 0;     // in char

	*ppdret = nullptr;
	BOOL isFound = FALSE;
	DWORD_PTR bufLenOneLine = 0;

TryAgain:
	bufLenOneLine = GetOneLine(&pwch[indexTrace], dwTotalBufLen);
	if (bufLenOneLine == 0)
	{
		goto FindNextLine;
	}
	else
	{
		CParserStringRange keyword;
		DWORD_PTR bufLen = 0;
		LPWSTR pText = nullptr;

		if (!ParseLine(&pwch[indexTrace], bufLenOneLine, &keyword))
		{
			return FALSE;    // error
		}

		if (!isTextSearch)
		{
			// Compare Dictionary key code and input key code
			if (!isWildcardSearch)
			{
				if (CStringRange::Compare(_locale, &keyword, _pSearchKeyCode) != CSTR_EQUAL)
				{
					if (bufLen)
					{
						delete[] pText;
					}
					goto FindNextLine;
				}
			}
			else
			{
				// Wildcard search
				if (!CStringRange::WildcardCompare(_locale, _pSearchKeyCode, &keyword))
				{
					if (bufLen)
					{
						delete[] pText;
					}
					goto FindNextLine;
				}
			}
		}
		else
		{
			// Compare Dictionary converted string and input string
			CSampleImeArray<CParserStringRange> convertedStrings;
			if (!ParseLine(&pwch[indexTrace], bufLenOneLine, &keyword, &convertedStrings))
			{
				if (bufLen)
				{
					delete[] pText;
				}
				return FALSE;
			}
			if (convertedStrings.Count() == 1)
			{
				CStringRange* pTempString = convertedStrings.GetAt(0);

				if (!isWildcardSearch)
				{
					if (CStringRange::Compare(_locale, pTempString, _pSearchKeyCode) != CSTR_EQUAL)
					{
						if (bufLen)
						{
							delete[] pText;
						}
						goto FindNextLine;
					}
				}
				else
				{
					// Wildcard search
					if (!CStringRange::WildcardCompare(_locale, _pSearchKeyCode, pTempString))
					{
						if (bufLen)
						{
							delete[] pText;
						}
						goto FindNextLine;
					}
				}
			}
			else
			{
				if (bufLen)
				{
					delete[] pText;
				}
				goto FindNextLine;
			}
		}

		if (bufLen)
		{
			delete[] pText;
		}

		// Prepare return's CDictionaryResult
		*ppdret = new (std::nothrow) CDictionaryResult();
		if (!*ppdret)
		{
			return FALSE;
		}

		CSampleImeArray<CParserStringRange> valueStrings;
		if (!ParseLine(&pwch[indexTrace], bufLenOneLine, &keyword, &valueStrings))
		{
			if (*ppdret)
			{
				delete *ppdret;
				*ppdret = nullptr;
			}
			return FALSE;
		}

		(*ppdret)->_FindKeyCode = keyword;
		(*ppdret)->_SearchKeyCode = *_pSearchKeyCode;

		for (UINT i = 0; i < valueStrings.Count(); i++)
		{
			CStringRange* findPhrase = (*ppdret)->_FindPhraseList.Append();
			if (findPhrase)
			{
				*findPhrase = *valueStrings.GetAt(i);
			}
		}

		// Seek to next line
		isFound = TRUE;
	}

FindNextLine:
	dwTotalBufLen -= bufLenOneLine;
	//    if (dwTotalBufLen == 0)  //orig
	//	if (dwTotalBufLen <= _charIndex)
	if (dwTotalBufLen < _charIndex)

	{
		indexTrace += bufLenOneLine;
		_charIndex += indexTrace;

		if (!isFound && *ppdret)
		{
			delete *ppdret;
			*ppdret = nullptr;
		}
		return (isFound ? TRUE : FALSE);        // End of file
	}

	indexTrace += bufLenOneLine;
	if (pwch[indexTrace] == L'\r' || pwch[indexTrace] == L'\n' || pwch[indexTrace] == L'\0')
	{
		bufLenOneLine = 1;
		goto FindNextLine;
	}

	if (isFound)
	{
		_charIndex += indexTrace;
		return TRUE;
	}

	goto TryAgain;
}