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

WCHAR HOKey[] ={220,221,222,191,187,219,186,190,189,'P','L','0','O','K','9','I','J','N','8','U','H','6','T','F','C','5','R','D','X','E','S','Z','W','A','7','Y','G','1','2','3','4','V','B','M',',','.','Q',192,188};  //192='`', 188=','
//WCHAR HOKey[] ={220,221,222,191,187,219,186,190,189,'p','l','0','o','k','9','i','j','n','8','u','h','6','t','f','c','5','r','d','x','e','s','z','w','a','7','y','g'};

//WCHAR HOReadKey[]={'�t','�u','�v','�w','�x','�y','�z','�{','�|','�}','�~','��','��','��','��','��','��','��','��','��','��','��','��','��','��','��','��','��','��','��','��','��','��','��','��','��','��'};


STDAPI CKeyHandlerEditSession::DoEditSession(TfEditCookie ec)
{
    HRESULT hResult = S_OK;

    CKeyStateCategoryFactory* pKeyStateCategoryFactory = CKeyStateCategoryFactory::Instance();
    CKeyStateCategory* pKeyStateCategory = pKeyStateCategoryFactory->MakeKeyStateCategory(_KeyState.Category, _pTextService);

    if (pKeyStateCategory)
    {
		KeyHandlerEditSessionDTO keyHandlerEditSessioDTO(ec, _pContext, _uCode,_wch, _KeyState.Function);
        hResult = pKeyStateCategory->KeyStateHandler(_KeyState.Function, keyHandlerEditSessioDTO);

        pKeyStateCategory->Release();
        pKeyStateCategoryFactory->Release();
    }

    return hResult;
}
