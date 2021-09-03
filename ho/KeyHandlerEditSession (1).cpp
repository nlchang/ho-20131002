// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved

#include "Private.h"
#include "KeyHandlerEditSession.h"
#include "EditSession.h"
#include "ho.h"
#include "CompositionProcessorEngine.h"
#include "KeyStateCategory.h"

//////////////////////////////////////////////////////////////////////
//
//    ITfEditSession
//        CEditSessionBase
// CKeyHandlerEditSession class
//
//////////////////////////////////////////////////////////////////////

//+---------------------------------------------------------------------------
//
// CKeyHandlerEditSession::DoEditSession
//
//----------------------------------------------------------------------------

WCHAR HOKey[] ={220,221,222,191,187,219,186,190,189,'P','L','0','O','K','9','I','J','N','8','U','H','6','T','F','C','5','R','D','X','E','S','Z','W','A','7','Y','G'};
//WCHAR HOReadKey[]={'£t','£u','£v','£w','£x','£y','£z','£{','£|','£}','£~','£¡','£¢','££','£¤','£¥','£¦','£§','£¨','£©','£ª','£«','£¬','£­','£®','£¯','£°','£±','£²','£³','£´','£µ','£¶','£·','£¸','£¹','£º'};


STDAPI CKeyHandlerEditSession::DoEditSession(TfEditCookie ec)
{
    HRESULT hResult = S_OK;

    CKeyStateCategoryFactory* pKeyStateCategoryFactory = CKeyStateCategoryFactory::Instance();
    CKeyStateCategory* pKeyStateCategory = pKeyStateCategoryFactory->MakeKeyStateCategory(_KeyState.Category, _pTextService);

    if (pKeyStateCategory)
    {
		for (int i=0;i < 37;i++)
		{
			if (_uCode == HOKey[i])
			{
				//_wch = 0x3105+(WCHAR)i;
				break;
			}
		}        		KeyHandlerEditSessionDTO keyHandlerEditSessioDTO(ec, _pContext, _uCode,_wch, _KeyState.Function);
        hResult = pKeyStateCategory->KeyStateHandler(_KeyState.Function, keyHandlerEditSessioDTO);

        pKeyStateCategory->Release();
        pKeyStateCategoryFactory->Release();
    }

    return hResult;
}
