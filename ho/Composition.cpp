// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved

#include "Private.h"
#include "Globals.h"
#include "ho.h"
#include "CompositionProcessorEngine.h"

extern bool Finalize;
//+---------------------------------------------------------------------------
//
// ITfCompositionSink::OnCompositionTerminated
//
// Callback for ITfCompositionSink.  The system calls this method whenever
// someone other than this service ends a composition.
//----------------------------------------------------------------------------

STDAPI CSampleIME::OnCompositionTerminated(TfEditCookie ecWrite, _In_ ITfComposition *pComposition)
{
	// Clear dummy composition
	_RemoveDummyCompositionForComposing(ecWrite, pComposition);

	// Clear display attribute and end composition, _EndComposition will release composition for us
	ITfContext* pContext = _pContext;
	if (pContext)
	{
		pContext->AddRef();
	}

	_EndComposition(_pContext);

	_DeleteCandidateList(FALSE, pContext);

	if (pContext)
	{
		pContext->Release();
		pContext = nullptr;
	}

	return S_OK;
}

//+---------------------------------------------------------------------------
//
// _IsComposing
//
//----------------------------------------------------------------------------

BOOL CSampleIME::_IsComposing()
{
	return _pComposition != nullptr;
}

//+---------------------------------------------------------------------------
//
// _SetComposition
//
//----------------------------------------------------------------------------

void CSampleIME::_SetComposition(_In_ ITfComposition *pComposition)
{
	_pComposition = pComposition;
}

//+---------------------------------------------------------------------------
//
// _AddComposingAndChar
//
//----------------------------------------------------------------------------

HRESULT CSampleIME::_AddComposingAndChar(TfEditCookie ec, _In_ ITfContext *pContext, _In_ CStringRange *pstrAddString)
{
	HRESULT hr = S_OK;

	ULONG fetched = 0;
	TF_SELECTION tfSelection;

	if (pContext->GetSelection(ec, TF_DEFAULT_SELECTION, 1, &tfSelection, &fetched) != S_OK || fetched == 0)
		return S_FALSE;

	//
	// make range start to selection
	//
	ITfRange* pAheadSelection = nullptr;
	hr = pContext->GetStart(ec, &pAheadSelection);
	if (SUCCEEDED(hr))
	{
		hr = pAheadSelection->ShiftEndToRange(ec, tfSelection.range, TF_ANCHOR_START);
		if (SUCCEEDED(hr))
		{
			ITfRange* pRange = nullptr;
			BOOL exist_composing = _FindComposingRange(ec, pContext, pAheadSelection, &pRange);

			_SetInputString(ec, pContext, pRange, pstrAddString, exist_composing);

			if (pRange)
			{
				pRange->Release();
			}
		}
	}

	tfSelection.range->Release();

	if (pAheadSelection)
	{
		pAheadSelection->Release();
	}

	return S_OK;
}

//+---------------------------------------------------------------------------
//
// _AddCharAndFinalize
//
//----------------------------------------------------------------------------

HRESULT CSampleIME::_AddCharAndFinalize(TfEditCookie ec, _In_ ITfContext *pContext, _In_ CStringRange *pstrAddString)
{
	HRESULT hr = E_FAIL;

	ULONG fetched = 0;
	TF_SELECTION tfSelection;

	if ((hr = pContext->GetSelection(ec, TF_DEFAULT_SELECTION, 1, &tfSelection, &fetched)) != S_OK || fetched != 1)
		return hr;

	// we use SetText here instead of InsertTextAtSelection because we've already started a composition
	// we don't want to the app to adjust the insertion point inside our composition
	hr = tfSelection.range->SetText(ec, 0, pstrAddString->Get(), (LONG)pstrAddString->GetLength());
	if (hr == S_OK)
	{
		// update the selection, we'll make it an insertion point just past
		// the inserted text.
		tfSelection.range->Collapse(ec, TF_ANCHOR_END);
		pContext->SetSelection(ec, 1, &tfSelection);
	}

	tfSelection.range->Release();

	return hr;
}

//+---------------------------------------------------------------------------
//
// _FindComposingRange
//
//----------------------------------------------------------------------------

BOOL CSampleIME::_FindComposingRange(TfEditCookie ec, _In_ ITfContext *pContext, _In_ ITfRange *pSelection, _Outptr_result_maybenull_ ITfRange **ppRange)
{
	if (ppRange == nullptr)
	{
		return FALSE;
	}

	*ppRange = nullptr;

	// find GUID_PROP_COMPOSING
	ITfProperty* pPropComp = nullptr;
	IEnumTfRanges* enumComp = nullptr;

	HRESULT hr = pContext->GetProperty(GUID_PROP_COMPOSING, &pPropComp);
	if (FAILED(hr) || pPropComp == nullptr)
	{
		return FALSE;
	}

	hr = pPropComp->EnumRanges(ec, &enumComp, pSelection);
	if (FAILED(hr) || enumComp == nullptr)
	{
		pPropComp->Release();
		return FALSE;
	}

	BOOL isCompExist = FALSE;
	VARIANT var;
	ULONG  fetched = 0;

	while (enumComp->Next(1, ppRange, &fetched) == S_OK && fetched == 1)
	{
		hr = pPropComp->GetValue(ec, *ppRange, &var);
		if (hr == S_OK)
		{
			if (var.vt == VT_I4 && var.lVal != 0)
			{
				isCompExist = TRUE;
				break;
			}
		}
		(*ppRange)->Release();
		*ppRange = nullptr;
	}

	pPropComp->Release();
	enumComp->Release();

	return isCompExist;
}

//+---------------------------------------------------------------------------
//
// _SetInputString
//
//----------------------------------------------------------------------------

HRESULT CSampleIME::_SetInputString(TfEditCookie ec, _In_ ITfContext *pContext, _Out_opt_ ITfRange *pRange, _In_ CStringRange *pstrAddString, BOOL exist_composing)
{
	ITfRange* pRangeInsert = nullptr;
	//WCHAR HOReadKey[]= {L'ㄅ',L'ㄆ',L'ㄇ',L'ㄈ',L'ㄉ',L'ㄊ',L'ㄋ',L'ㄌ',L'ㄍ',L'ㄎ',L'ㄏ',L'ㄐ',L'ㄑ',L'ㄒ',L'ㄓ',L'ㄔ',L'ㄕ',L'ㄖ',L'ㄗ',L'ㄘ',L'ㄙ',L'ㄧ',L'ㄨ',L'ㄩ',L'ㄚ',L'ㄛ',L'ㄜ',L'ㄝ',L'ㄞ',L'ㄟ',L'ㄠ',L'ㄡ',L'ㄢ',L'ㄣ',L'ㄤ',L'ㄥ',L'ㄦ',L'1',L'2',L'3',L'4'};
	WCHAR HOReadKey[] = { L'ㄅ',L'ㄆ',L'ㄇ',L'ㄈ',L'ㄉ',L'ㄊ',L'ㄋ',L'ㄌ',L'ㄍ',L'ㄎ',L'ㄏ',L'ㄐ',L'ㄑ',L'ㄒ',L'ㄓ',L'ㄔ',L'ㄕ',L'ㄖ',L'ㄗ',L'ㄘ',L'ㄙ',L'ㄧ',L'ㄨ',L'ㄩ',L'ㄚ',L'ㄛ',L'ㄜ',L'ㄝ',L'ㄞ',L'ㄟ',L'ㄠ',L'ㄡ',L'ㄢ',L'ㄣ',L'ㄤ',L'ㄥ',L'ㄦ',L'1',L'\u02CA',L'\u02C7',L'\u02CB' };
	WCHAR chHoTable[] = { L'\\', L']',L'\'',  L'/',  L'=',L'[',  L';',  L'.',  L'-',  L'p',  L'l', L'0',   L'o', L'k',  L'9', L'i',  L'j',   L'n', L'8', L'u',  L'h', L'7',  L'y',  L'g', L'6', L't',  L'f',   L'c',  L'5', L'r', L'd',  L'x',  L'e', L's',  L'z',  L'w', L'a', L'1', L'2',L'3',L'4' };   //virtual-key codes

	//WCHAR chHoTable[]={L'1', L'q',L'a',  L'z',  L'2',L'w',  L's',  L'x',  L'e',  L'd',  L'c', L'r',   L'f', L'v', L'5', L't', L'g',  L'b',   L'y', L'h', L'n',  L'u', L'j',  L'm',  L'8', L'i', L'k',  L',',   L'9',  L'o', L'l', L'.',  L'0',  L'p', L';',  L'/', L'-', L'1', L'2',L'3',L'4'};   //virtual-key codes

	int elementsofchHotable = sizeof(chHoTable) / sizeof(*chHoTable);

	///*
	WCHAR keyChar;
#define KeyStringSize 80			//決定輸出的字數
	WCHAR *keyString = new WCHAR[KeyStringSize];  //記錄目前輸入多少鍵
	int inputKeyNumber = 0;
	int keyIndex = 0;
	bool Phrase = false;
	//*/
	if (!exist_composing)
	{
		_InsertAtSelection(ec, pContext, pstrAddString, &pRangeInsert);
		if (pRangeInsert == nullptr)
		{
			return S_OK;
		}
		pRange = pRangeInsert;
	}
	if (pRange != nullptr)
	{
		///*
		 //-----------------------------------------------------------------------------
		if (!Finalize)		//輸入完成前, 轉換符號
		{
			inputKeyNumber = (LONG)pstrAddString->GetLength();
			if (inputKeyNumber > KeyStringSize)
				inputKeyNumber = KeyStringSize;          //limit the input key to size of keyString

			for (size_t i = 0; i < inputKeyNumber; i++)
			{
				*(keyString + i) = *((pstrAddString)->Get() + i);
				keyChar = *(keyString + i);
				if ((keyChar <= 250) && (!Phrase))                         // char in ASCII range
				{
					bool charMatched = false;

					for (int j = 0; j < elementsofchHotable; j++)
					{
						if (keyChar == chHoTable[j])
						{
							keyIndex = j;
							charMatched = true;
							break;
						}
					}
					if (charMatched)
						*(keyString + i) = HOReadKey[keyIndex];
					else
						*(keyString + i) = keyChar;
				}
				else
					Phrase = true;		//Phrase processed , no char conversion now
			}

			pRange->SetText(ec, 0, keyString, (LONG)pstrAddString->GetLength());
			//-----------------------------------------------------------------------------
//*/
		}
		else
			pRange->SetText(ec, 0, pstrAddString->Get(), (LONG)pstrAddString->GetLength());    //orig
	}
	// pstraddstring used for dictionary lookup
	_SetCompositionLanguage(ec, pContext);
	_SetCompositionDisplayAttributes(ec, pContext, _gaDisplayAttributeInput);
	// update the selection, we'll make it an insertion point just past
	// the inserted text.
	ITfRange* pSelection = nullptr;
	TF_SELECTION sel;
	if ((pRange != nullptr) && (pRange->Clone(&pSelection) == S_OK))
	{
		pSelection->Collapse(ec, TF_ANCHOR_END);
		sel.range = pSelection;
		sel.style.ase = TF_AE_NONE;
		sel.style.fInterimChar = FALSE;
		pContext->SetSelection(ec, 1, &sel);
		pSelection->Release();
	}
	if (pRangeInsert)
	{
		pRangeInsert->Release();
	}
	return S_OK;
}

//+---------------------------------------------------------------------------
//
// _InsertAtSelection
//
//----------------------------------------------------------------------------

HRESULT CSampleIME::_InsertAtSelection(TfEditCookie ec, _In_ ITfContext *pContext, _In_ CStringRange *pstrAddString, _Outptr_ ITfRange **ppCompRange)
{
	ITfRange* rangeInsert = nullptr;
	ITfInsertAtSelection* pias = nullptr;
	HRESULT hr = S_OK;

	if (ppCompRange == nullptr)
	{
		hr = E_INVALIDARG;
		goto Exit;
	}

	*ppCompRange = nullptr;

	hr = pContext->QueryInterface(IID_ITfInsertAtSelection, (void **)&pias);
	if (FAILED(hr))
	{
		goto Exit;
	}

	hr = pias->InsertTextAtSelection(ec, TF_IAS_QUERYONLY, pstrAddString->Get(), (LONG)pstrAddString->GetLength(), &rangeInsert);

	if (FAILED(hr) || rangeInsert == nullptr)
	{
		rangeInsert = nullptr;
		pias->Release();
		goto Exit;
	}

	*ppCompRange = rangeInsert;
	pias->Release();
	hr = S_OK;

Exit:
	return hr;
}

//+---------------------------------------------------------------------------
//
// _RemoveDummyCompositionForComposing
//
//----------------------------------------------------------------------------

HRESULT CSampleIME::_RemoveDummyCompositionForComposing(TfEditCookie ec, _In_ ITfComposition *pComposition)
{
	HRESULT hr = S_OK;

	ITfRange* pRange = nullptr;

	if (pComposition)
	{
		hr = pComposition->GetRange(&pRange);
		if (SUCCEEDED(hr))
		{
			pRange->SetText(ec, 0, nullptr, 0);
			pRange->Release();
		}
	}

	return hr;
}

//+---------------------------------------------------------------------------
//
// _SetCompositionLanguage
//
//----------------------------------------------------------------------------

BOOL CSampleIME::_SetCompositionLanguage(TfEditCookie ec, _In_ ITfContext *pContext)
{
	HRESULT hr = S_OK;
	BOOL ret = TRUE;

	CCompositionProcessorEngine* pCompositionProcessorEngine = nullptr;
	pCompositionProcessorEngine = _pCompositionProcessorEngine;

	LANGID langidProfile = 0;
	pCompositionProcessorEngine->GetLanguageProfile(&langidProfile);

	ITfRange* pRangeComposition = nullptr;
	ITfProperty* pLanguageProperty = nullptr;

	// we need a range and the context it lives in
	hr = _pComposition->GetRange(&pRangeComposition);
	if (FAILED(hr))
	{
		ret = FALSE;
		goto Exit;
	}

	// get our the language property
	hr = pContext->GetProperty(GUID_PROP_LANGID, &pLanguageProperty);
	if (FAILED(hr))
	{
		ret = FALSE;
		goto Exit;
	}

	VARIANT var;
	var.vt = VT_I4;   // we're going to set DWORD
	var.lVal = langidProfile;

	hr = pLanguageProperty->SetValue(ec, pRangeComposition, &var);
	if (FAILED(hr))
	{
		ret = FALSE;
		goto Exit;
	}

	pLanguageProperty->Release();
	pRangeComposition->Release();

Exit:
	return ret;
}