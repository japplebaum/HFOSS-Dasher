// AlphabetManager.cpp
//
// Copyright (c) 2007 The Dasher Team
//
// This file is part of Dasher.
//
// Dasher is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// Dasher is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Dasher; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

#include "../Common/Common.h"

#include "AlphabetManager.h"
#include "ConversionManager.h"
#include "DasherInterfaceBase.h"
#include "DasherNode.h"
#include "Event.h"
#include "EventHandler.h"
#include "NodeCreationManager.h"


#include <vector>
#include <sstream>
#include <iostream>

using namespace Dasher;

// Track memory leaks on Windows to the line that new'd the memory
#ifdef _WIN32
#ifdef _DEBUG_MEMLEAKS
#define DEBUG_NEW new( _NORMAL_BLOCK, THIS_FILE, __LINE__ )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
#endif

CAlphabetManager::CAlphabetManager(CDasherInterfaceBase *pInterface, CNodeCreationManager *pNCManager, CLanguageModel *pLanguageModel)
  : m_pLanguageModel(pLanguageModel), m_pNCManager(pNCManager) {
  m_pInterface = pInterface;

  m_iLearnContext = m_pLanguageModel->CreateEmptyContext();

}

CAlphabetManager::CAlphNode::CAlphNode(CDasherNode *pParent, int iType,  int iOffset, unsigned int iLbnd, unsigned int iHbnd, int iColour, const string &strDisplayText, CAlphabetManager *pMgr)
: CDasherNode(pParent, iOffset, iLbnd, iHbnd, iColour, strDisplayText, iType), m_pProbInfo(NULL), m_pMgr(pMgr) {
};

CAlphabetManager::CSymbolNode::CSymbolNode(CDasherNode *pParent, int iOffset, unsigned int iLbnd, unsigned int iHbnd, CAlphabetManager *pMgr, symbol _iSymbol)
: CAlphNode(pParent, NT_SYMBOL, iOffset, iLbnd, iHbnd, pMgr->m_pNCManager->GetAlphabet()->GetColour(_iSymbol, iOffset%2), pMgr->m_pNCManager->GetAlphabet()->GetDisplayText(_iSymbol), pMgr), iSymbol(_iSymbol) {
};

CAlphabetManager::CSymbolNode::CSymbolNode(CDasherNode *pParent, int iOffset, unsigned int iLbnd, unsigned int iHbnd, int iColour, const string &strDisplayText, CAlphabetManager *pMgr, symbol _iSymbol)
: CAlphNode(pParent, NT_SYMBOL, iOffset, iLbnd, iHbnd, iColour, strDisplayText, pMgr), iSymbol(_iSymbol) {
};

CAlphabetManager::CGroupNode::CGroupNode(CDasherNode *pParent, int iOffset, unsigned int iLbnd, unsigned int iHbnd, CAlphabetManager *pMgr, SGroupInfo *pGroup)
: CAlphNode(pParent, NT_GROUP, iOffset, iLbnd, iHbnd,
            pGroup ? (pGroup->bVisible ? pGroup->iColour : pParent->getColour())
                   : pMgr->m_pNCManager->GetAlphabet()->GetColour(0, iOffset%2),
            pGroup ? pGroup->strLabel : "", pMgr), m_pGroup(pGroup) {
};

CAlphabetManager::CSymbolNode *CAlphabetManager::makeSymbol(CDasherNode *pParent, int iOffset, unsigned int iLbnd, unsigned int iHbnd, symbol iSymbol) {
  return new CSymbolNode(pParent, iOffset, iLbnd, iHbnd, this, iSymbol);
}

CAlphabetManager::CGroupNode *CAlphabetManager::makeGroup(CDasherNode *pParent, int iOffset, unsigned int iLbnd, unsigned int iHbnd, SGroupInfo *pGroup) {
  return new CGroupNode(pParent, iOffset, iLbnd, iHbnd, this, pGroup);
}

CAlphabetManager::CAlphNode *CAlphabetManager::GetRoot(CDasherNode *pParent, unsigned int iLower, unsigned int iUpper, bool bEnteredLast, int iOffset) {

  int iNewOffset(max(-1,iOffset-1));
  
  std::vector<symbol> vContextSymbols;
  // TODO: make the LM get the context, rather than force it to fix max context length as an int
  int iStart = max(0, iNewOffset - m_pLanguageModel->GetContextLength());
  
  if(pParent) {
    pParent->GetContext(m_pInterface, vContextSymbols, iStart, iNewOffset+1 - iStart);
  } else {
    std::string strContext = (iNewOffset == -1) 
      ? m_pNCManager->GetAlphabet()->GetDefaultContext()
      : m_pInterface->GetContext(iStart, iNewOffset+1 - iStart);
    m_pNCManager->GetAlphabet()->GetSymbols(vContextSymbols, strContext);
  }
  
  CAlphNode *pNewNode;
  CLanguageModel::Context iContext = m_pLanguageModel->CreateEmptyContext();
  
  std::vector<symbol>::iterator it = vContextSymbols.end();
  while (it!=vContextSymbols.begin()) {
    if (*(--it) == 0) {
      //found an impossible symbol! start after it
      ++it;
      break;
    }
  }
  if (it == vContextSymbols.end()) {
    //previous character was not in the alphabet!
    //can't construct a node "responsible" for entering it
    bEnteredLast=false;
    //instead, Create a node as if we were starting a new sentence...
    vContextSymbols.clear();
    m_pNCManager->GetAlphabet()->GetSymbols(vContextSymbols, m_pNCManager->GetAlphabet()->GetDefaultContext());
    it = vContextSymbols.begin();
    //TODO: What it the default context somehow contains symbols not in the alphabet?
  }
  //enter the symbols we could make sense of, into the LM context...
  while (it != vContextSymbols.end()) {
    m_pLanguageModel->EnterSymbol(iContext, *(it++));
  }
  
  if(!bEnteredLast) {
    pNewNode = makeGroup(pParent, iNewOffset, iLower, iUpper,  NULL);
  } else {
    const symbol iSymbol(vContextSymbols[vContextSymbols.size() - 1]);
    pNewNode = makeSymbol(pParent, iNewOffset, iLower, iUpper, iSymbol);
    //if the new node is not child of an existing node, then it
    // represents a symbol that's already happened - so we're either
    // going backwards (rebuildParent) or creating a new root after a language change
    DASHER_ASSERT (!pParent);
    pNewNode->SetFlag(NF_SEEN, true);
  }

  pNewNode->iContext = iContext;

  return pNewNode;
}

bool CAlphabetManager::CSymbolNode::IsTarget(string strTargetUtf8Char) {
  g_pLogger->Log("Comparing to: " + m_pMgr->m_pNCManager->GetAlphabet()->GetText(iSymbol));
  return (m_pMgr->m_pNCManager->GetAlphabet()->GetText(iSymbol) == strTargetUtf8Char);
}

CLanguageModel::Context CAlphabetManager::CAlphNode::CloneAlphContext(CLanguageModel *pLanguageModel) {
  if (iContext) return pLanguageModel->CloneContext(iContext);
  return CDasherNode::CloneAlphContext(pLanguageModel);
}

void CAlphabetManager::CSymbolNode::GetContext(CDasherInterfaceBase *pInterface, vector<symbol> &vContextSymbols, int iOffset, int iLength) {
  if (!GetFlag(NF_SEEN) && iOffset+iLength-1 == offset()) {
    if (iLength > 1) Parent()->GetContext(pInterface, vContextSymbols, iOffset, iLength-1);
    vContextSymbols.push_back(iSymbol);
  } else {
    CDasherNode::GetContext(pInterface, vContextSymbols, iOffset, iLength);
  }
}

symbol CAlphabetManager::CSymbolNode::GetAlphSymbol() {
  return iSymbol;
}

void CAlphabetManager::CSymbolNode::PopulateChildren() {
  m_pMgr->IterateChildGroups(this, NULL, NULL);
}
int CAlphabetManager::CAlphNode::ExpectedNumChildren() {
  return m_pMgr->m_pNCManager->GetAlphabet()->iNumChildNodes;
}

std::vector<unsigned int> *CAlphabetManager::CAlphNode::GetProbInfo() {
  if (!m_pProbInfo) {
    m_pProbInfo = new std::vector<unsigned int>();
    m_pMgr->m_pNCManager->GetProbs(iContext, *m_pProbInfo, m_pMgr->m_pNCManager->GetLongParameter(LP_NORMALIZATION));
    // work out cumulative probs in place
    for(unsigned int i = 1; i < m_pProbInfo->size(); i++) {
      (*m_pProbInfo)[i] += (*m_pProbInfo)[i - 1];
    }
  }
  return m_pProbInfo;
}

std::vector<unsigned int> *CAlphabetManager::CGroupNode::GetProbInfo() {
  if (m_pGroup && Parent() && Parent()->mgr() == mgr()) {
    DASHER_ASSERT(Parent()->offset() == offset());
    return (static_cast<CAlphNode *>(Parent()))->GetProbInfo();
  }
  //nope, no usable parent. compute here...
  return CAlphNode::GetProbInfo();
}

void CAlphabetManager::CGroupNode::PopulateChildren() {
  m_pMgr->IterateChildGroups(this, m_pGroup, NULL);
}

int CAlphabetManager::CGroupNode::ExpectedNumChildren() {
  return (m_pGroup) ? m_pGroup->iNumChildNodes : CAlphNode::ExpectedNumChildren();
}

CAlphabetManager::CGroupNode *CAlphabetManager::CreateGroupNode(CAlphNode *pParent, SGroupInfo *pInfo, unsigned int iLbnd, unsigned int iHbnd) {

  // When creating a group node...
  // ...the offset is the same as the parent...
  CGroupNode *pNewNode = makeGroup(pParent, pParent->offset(), iLbnd, iHbnd, pInfo);

  //...as is the context!
  pNewNode->iContext = m_pLanguageModel->CloneContext(pParent->iContext);

  return pNewNode;
}

CAlphabetManager::CGroupNode *CAlphabetManager::CGroupNode::RebuildGroup(CAlphNode *pParent, SGroupInfo *pInfo, unsigned int iLbnd, unsigned int iHbnd) {
  if (pInfo == m_pGroup) {
    SetRange(iLbnd, iHbnd);
    SetParent(pParent);
    //offset doesn't increase for groups...
    DASHER_ASSERT (offset() == pParent->offset());
    return this;
  }
  CGroupNode *pRet=m_pMgr->CreateGroupNode(pParent, pInfo, iLbnd, iHbnd);
  if (pInfo->iStart <= m_pGroup->iStart && pInfo->iEnd >= m_pGroup->iEnd) {
    //created group node should contain this one
    m_pMgr->IterateChildGroups(pRet,pInfo,this);
  }
  return pRet;
}

CAlphabetManager::CGroupNode *CAlphabetManager::CSymbolNode::RebuildGroup(CAlphNode *pParent, SGroupInfo *pInfo, unsigned int iLbnd, unsigned int iHbnd) {
  CGroupNode *pRet=m_pMgr->CreateGroupNode(pParent, pInfo, iLbnd, iHbnd);
  if (pInfo->iStart <= iSymbol && pInfo->iEnd > iSymbol) {
    m_pMgr->IterateChildGroups(pRet, pInfo, this);
  }
  return pRet;
}

CLanguageModel::Context CAlphabetManager::CreateSymbolContext(CAlphNode *pParent, symbol iSymbol)
{
  CLanguageModel::Context iContext = m_pLanguageModel->CloneContext(pParent->iContext);
  m_pLanguageModel->EnterSymbol(iContext, iSymbol); // TODO: Don't use symbols?
  return iContext;
}

CDasherNode *CAlphabetManager::CreateSymbolNode(CAlphNode *pParent, symbol iSymbol, unsigned int iLbnd, unsigned int iHbnd) {

  CDasherNode *pNewNode = NULL;

  //Does not invoke conversion node

  // TODO: Better way of specifying alternate roots

  // TODO: Need to fix fact that this is created even when control mode is switched off
  if(iSymbol == m_pNCManager->GetAlphabet()->GetControlSymbol()) {
      //ACL setting offset as one more than parent for consistency with "proper" symbol nodes...
      pNewNode = m_pNCManager->GetCtrlRoot(pParent, iLbnd, iHbnd, pParent->offset()+1); 

#ifdef _WIN32_WCE
      //no control manager - but (TODO!) we still try to create (0-size!) control node...
      DASHER_ASSERT(!pNewNode);
      // For now, just hack it so we get a normal root node here
      pNewNode = m_pNCManager->GetAlphRoot(pParent, iLbnd, iHbnd, false, pParent->m_iOffset+1);
#else
      DASHER_ASSERT(pNewNode);
#endif
    }
    else if(iSymbol == m_pNCManager->GetAlphabet()->GetStartConversionSymbol()) {
      //  else if(iSymbol == m_pNCManager->GetSpaceSymbol()) {

      //ACL setting m_iOffset+1 for consistency with "proper" symbol nodes...
      pNewNode = m_pNCManager->GetConvRoot(pParent, iLbnd, iHbnd, pParent->offset()+1);
    }
    else {
      // TODO: Exceptions / error handling in general

      CAlphNode *pAlphNode;
      pNewNode = pAlphNode = makeSymbol(pParent, pParent->offset()+1, iLbnd, iHbnd, iSymbol);

      //     std::stringstream ssLabel;

      //     ssLabel << m_pNCManager->GetAlphabet()->GetDisplayText(iSymbol) << ": " << pNewNode;

      //    pDisplayInfo->strDisplayText = ssLabel.str();

      pAlphNode->iContext = CreateSymbolContext(pParent, iSymbol);
  }

  return pNewNode;
}

CDasherNode *CAlphabetManager::CSymbolNode::RebuildSymbol(CAlphNode *pParent, symbol iSymbol, unsigned int iLbnd, unsigned int iHbnd) {
  if(iSymbol == this->iSymbol) {
    SetRange(iLbnd, iHbnd);
    SetParent(pParent);
    DASHER_ASSERT(offset() == pParent->offset() + 1);
    return this;
  }
  return m_pMgr->CreateSymbolNode(pParent, iSymbol, iLbnd, iHbnd);
}

CDasherNode *CAlphabetManager::CGroupNode::RebuildSymbol(CAlphNode *pParent, symbol iSymbol, unsigned int iLbnd, unsigned int iHbnd) {
  return m_pMgr->CreateSymbolNode(pParent, iSymbol, iLbnd, iHbnd);
}

void CAlphabetManager::IterateChildGroups(CAlphNode *pParent, SGroupInfo *pParentGroup, CAlphNode *buildAround) {
  std::vector<unsigned int> *pCProb(pParent->GetProbInfo());
  const int iMin(pParentGroup ? pParentGroup->iStart : 1);
  const int iMax(pParentGroup ? pParentGroup->iEnd : pCProb->size());
  // TODO: Think through alphabet file formats etc. to make this class easier.
  // TODO: Throw a warning if parent node already has children
  
  // Create child nodes and add them
  
  int i(iMin); //lowest index of child which we haven't yet added
  SGroupInfo *pCurrentNode(pParentGroup ? pParentGroup->pChild : m_pNCManager->GetAlphabet()->m_pBaseGroup);
  // The SGroupInfo structure has something like linked list behaviour
  // Each SGroupInfo contains a pNext, a pointer to a sibling group info
  while (i < iMax) {
    CDasherNode *pNewChild;
    bool bSymbol = !pCurrentNode //gone past last subgroup
                  || i < pCurrentNode->iStart; //not reached next subgroup
    const int iStart=i, iEnd = (bSymbol) ? i+1 : pCurrentNode->iEnd;
    //uint64 is platform-dependently #defined in DasherTypes.h as an (unsigned) 64-bit int ("__int64" or "long long int")
    unsigned int iLbnd = (((*pCProb)[iStart-1] - (*pCProb)[iMin-1]) *
                          (uint64)(m_pNCManager->GetLongParameter(LP_NORMALIZATION))) /
                         ((*pCProb)[iMax-1] - (*pCProb)[iMin-1]);
    unsigned int iHbnd = (((*pCProb)[iEnd-1] - (*pCProb)[iMin-1]) *
                          (uint64)(m_pNCManager->GetLongParameter(LP_NORMALIZATION))) /
                         ((*pCProb)[iMax-1] - (*pCProb)[iMin-1]);
    //loop for eliding groups with single children (see below).
    // Variables store necessary properties of any elided groups:
    std::string groupPrefix=""; int iOverrideColour=-1;
    SGroupInfo *pInner=pCurrentNode;
    while (true) {
      if (bSymbol) {
        pNewChild = (buildAround) ? buildAround->RebuildSymbol(pParent, i, iLbnd, iHbnd) : CreateSymbolNode(pParent, i, iLbnd, iHbnd);
        i++; //make one symbol at a time - move onto next symbol in next iteration of (outer) loop
        break; //exit inner (group elision) loop
      } else if (pInner->iNumChildNodes>1) { //in/reached nontrivial subgroup - do make node for entire group:
        pNewChild= (buildAround) ? buildAround->RebuildGroup(pParent, pInner, iLbnd, iHbnd) : CreateGroupNode(pParent, pInner, iLbnd, iHbnd);
        i = pInner->iEnd; //make one group at a time - so move past entire group...
        pCurrentNode = pCurrentNode->pNext; //next sibling of _original_ pCurrentNode (above)
                                     // (maybe not of pCurrentNode now, which might be a subgroup filling the original)
        break; //exit inner (group elision) loop
      }
      //were about to create a group node, which would have only one child
      // (eventually, if the group node were PopulateChildren'd).
      // Such a child would entirely fill it's parent (the group), and thus,
      // creation/destruction of the child would cause the node's colour to flash
      // between that for parent group and child.
      // Hence, instead we elide the group node and create the child _here_...
      
      //1. however we also have to take account of the appearance of the elided group. Hence:
      groupPrefix += pInner->strLabel;
      if (pInner->bVisible) iOverrideColour=pInner->iColour;
      //2. now go into the group...
      pInner = pInner->pChild;
      bSymbol = (pInner==NULL); //which might contain a single subgroup, or a single symbol
      if (bSymbol) pCurrentNode = pCurrentNode->pNext; //if a symbol, we've still moved past the outer (elided) group
      DASHER_ASSERT(iEnd == (bSymbol ? i+1 : pInner->iEnd)); //probability calcs still ok
      //3. loop round inner loop...
    }
    //created a new node - symbol or (group which will have >1 child).
    DASHER_ASSERT(pParent->GetChildren().back()==pNewChild);
    //now adjust the node we've actually created, to take account of any elided group(s)...
    // tho not if we've reused the existing node, assume that's been adjusted already
    if (pNewChild && pNewChild!=buildAround) pNewChild->PrependElidedGroup(iOverrideColour, groupPrefix);
  }

  pParent->SetFlag(NF_ALLCHILDREN, true);
}

CAlphabetManager::CAlphNode::~CAlphNode() {
  delete m_pProbInfo;
  m_pMgr->m_pLanguageModel->ReleaseContext(iContext);
}

const std::string &CAlphabetManager::CSymbolNode::outputText() {
  return mgr()->m_pNCManager->GetAlphabet()->GetText(iSymbol);
}

void CAlphabetManager::CSymbolNode::Output(Dasher::VECTOR_SYMBOL_PROB* pAdded, int iNormalization) {
  //std::cout << this << " " << Parent() << ": Output at offset " << m_iOffset << " *" << m_pMgr->m_pNCManager->GetAlphabet()->GetText(t) << "* " << std::endl;

  Dasher::CEditEvent oEvent(1, outputText(), offset());
  m_pMgr->m_pNCManager->InsertEvent(&oEvent);

  // Track this symbol and its probability for logging purposes
  if (pAdded != NULL) {
    Dasher::SymbolProb sItem;
    sItem.sym    = iSymbol;
    sItem.prob   = Range() / (double)iNormalization;

    pAdded->push_back(sItem);
  }
}

void CAlphabetManager::CSymbolNode::Undo(int *pNumDeleted) {
  Dasher::CEditEvent oEvent(2, outputText(), offset());
  m_pMgr->m_pNCManager->InsertEvent(&oEvent);
  if (pNumDeleted) (*pNumDeleted)++;
}

CDasherNode *CAlphabetManager::CGroupNode::RebuildParent() {
  // CAlphNode's always have a parent, they inserted a symbol; CGroupNode's
  // with an m_pGroup have a container i.e. the parent group, unless
  // m_pGroup==NULL => "root" node where Alphabet->m_pBaseGroup is the *first*child*...
  if (m_pGroup == NULL) return NULL;
  //offset of group node is same as parent...
  return CAlphNode::RebuildParent(offset());
}

CDasherNode *CAlphabetManager::CSymbolNode::RebuildParent() {
  //parent's offset is one less than this.
  return CAlphNode::RebuildParent(offset()-1);
}

CDasherNode *CAlphabetManager::CAlphNode::RebuildParent(int iNewOffset) {
  //possible that we have a parent, as RebuildParent() rebuilds back to closest AlphNode.
  if (Parent()) return Parent();
  
  CAlphNode *pNewNode = m_pMgr->GetRoot(NULL, 0, 0, iNewOffset!=-1, iNewOffset+1);
  
  //now fill in the new node - recursively - until it reaches us
  m_pMgr->IterateChildGroups(pNewNode, NULL, this);

  //finally return our immediate parent (pNewNode may be an ancestor rather than immediate parent!)
  DASHER_ASSERT(Parent() != NULL);

  //although not required, we believe only NF_SEEN nodes are ever requested to rebuild their parents...
  DASHER_ASSERT(GetFlag(NF_SEEN));
  //so set NF_SEEN on all created ancestors (of which pNewNode is the last)
  CDasherNode *pNode = this;
  do {
    pNode = pNode->Parent();
    pNode->SetFlag(NF_SEEN, true);
  } while (pNode != pNewNode);
  
  return Parent();
}

// TODO: Shouldn't there be an option whether or not to learn as we write?
// For want of a better solution, game mode exemption explicit in this function
void CAlphabetManager::CSymbolNode::SetFlag(int iFlag, bool bValue) {
  CDasherNode::SetFlag(iFlag, bValue);
  switch(iFlag) {
  case NF_COMMITTED:
    if(bValue && !GetFlag(NF_GAME) && m_pMgr->m_pInterface->GetBoolParameter(BP_LM_ADAPTIVE))
      m_pMgr->m_pLanguageModel->LearnSymbol(m_pMgr->m_iLearnContext, iSymbol);
    break;
  }
}
