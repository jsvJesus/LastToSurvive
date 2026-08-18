///////////////////////////////////////////////////////////////////////  
//
//  *** INTERACTIVE DATA VISUALIZATION (IDV) CONFIDENTIAL AND PROPRIETARY INFORMATION ***
//
//  This software is supplied under the terms of a license agreement or
//  nondisclosure agreement with Interactive Data Visualization, Inc. and
//  may not be copied, disclosed, or exploited except in accordance with 
//  the terms of that agreement.
//
//      Copyright (c) 2003-2016 IDV, Inc.
//      All rights reserved in all media.
//
//      IDV, Inc.
//      http://www.idvinc.com


///////////////////////////////////////////////////////////////////////
//  CCellContainer::CCellContainer

template<class TCellType>
ST_INLINE CCellContainer<TCellType>::CCellContainer( )
    : CMap<SRowCol, TCellType>(10)
    , m_fCellSize(1200.0f)
{
}


///////////////////////////////////////////////////////////////////////
//  CCellContainer::~CCellContainer

template<class TCellType>
ST_INLINE CCellContainer<TCellType>::~CCellContainer( )
{
    #ifndef NDEBUG
        m_fCellSize = -1.0f;
    #endif
}


///////////////////////////////////////////////////////////////////////
//  CCellContainer::GetCellPtrByRowCol_Add

template<class TCellType>
ST_INLINE TCellType* CCellContainer<TCellType>::GetCellPtrByRowCol_Add(st_int32 nRow, st_int32 nCol)
{
    typename CCellContainer<TCellType>::iterator iCell = GetCellItrByRowCol_Add(nRow, nCol);
    if (iCell == CMap<SRowCol, TCellType>::end( ))
        return NULL;
    else
        return &iCell->second;
}


///////////////////////////////////////////////////////////////////////
//  CCellContainer::GetCellPtrByRowCol

template<class TCellType>
ST_INLINE const TCellType* CCellContainer<TCellType>::GetCellPtrByRowCol(st_int32 nRow, st_int32 nCol) const
{
    typename CCellContainer<TCellType>::const_iterator iCell = GetCellItrByRowCol(nRow, nCol);
    if (iCell == CMap<SRowCol, TCellType>::end( ))
        return NULL;
    else
        return &iCell->second;
}


///////////////////////////////////////////////////////////////////////
//  CCellContainer::GetCellPtrByPos_Add

template<class TCellType>
ST_INLINE TCellType* CCellContainer<TCellType>::GetCellPtrByPos_Add(const Vec3& vPos)
{
    typename CCellContainer<TCellType>::TCellIterator iCell = GetCellItrByPos_Add(vPos);
    if (iCell == CMap<SRowCol, TCellType>::end( ))
        return NULL;
    else
        return &iCell->second;
}


///////////////////////////////////////////////////////////////////////
//  CCellContainer::GetCellPtrByPos

template<class TCellType>
ST_INLINE const TCellType* CCellContainer<TCellType>::GetCellPtrByPos(const Vec3& vPos) const
{
    typename CCellContainer<TCellType>::const_iterator iCell = GetCellItrByPos(vPos);
    if (iCell == CMap<SRowCol, TCellType>::end( ))
        return NULL;
    else
        return &iCell->second;
}


///////////////////////////////////////////////////////////////////////
//  CCellContainer::GetCellItrByRowCol_Add

template<class TCellType>
ST_INLINE
typename CCellContainer<TCellType>::TCellIterator
CCellContainer<TCellType>::GetCellItrByRowCol_Add(
    st_int32 nRow,
    st_int32 nCol)
{
    const SRowCol key(nRow, nCol);

    typename CCellContainer<TCellType>::TCellIterator cell =
        CMap<SRowCol, TCellType>::find(key);

    if (cell == CMap<SRowCol, TCellType>::end())
    {
        TCellType& newCell = (*this)[key];

        assert(!newCell.GetExtents().Valid());

        newCell.SetRowCol(nRow, nCol);

        cell = CMap<SRowCol, TCellType>::find(key);

        assert(
            cell !=
            CMap<SRowCol, TCellType>::end());

        assert(newCell.IsNew());
    }

    return cell;
}

///////////////////////////////////////////////////////////////////////
//  CCellContainer::GetCellItrByRowCol

template<class TCellType>
ST_INLINE
typename CCellContainer<TCellType>::TCellConstIterator
CCellContainer<TCellType>::GetCellItrByRowCol(
    st_int32 nRow,
    st_int32 nCol) const
{
    const SRowCol key(nRow, nCol);

    return CMap<SRowCol, TCellType>::find(key);
}

///////////////////////////////////////////////////////////////////////
//  CCellContainer::GetCellItrByPos_Add

template<class TCellType>
ST_INLINE
typename CCellContainer<TCellType>::TCellIterator
CCellContainer<TCellType>::GetCellItrByPos_Add(
    const Vec3& vPos)
{
    st_int32 row = 0;
    st_int32 column = 0;

    ComputeCellCoords(
        vPos,
        m_fCellSize,
        row,
        column);

    return GetCellItrByRowCol_Add(
        row,
        column);
}

///////////////////////////////////////////////////////////////////////
//  CCellContainer::GetCellItrByPos

template<class TCellType>
ST_INLINE
typename CCellContainer<TCellType>::TCellConstIterator
CCellContainer<TCellType>::GetCellItrByPos(
    const Vec3& vPos) const
{
    st_int32 row = 0;
    st_int32 column = 0;

    ComputeCellCoords(
        vPos,
        m_fCellSize,
        row,
        column);

    return GetCellItrByRowCol(
        row,
        column);
}

///////////////////////////////////////////////////////////////////////
//  CCellContainer::Erase

template<class TCellType>
ST_INLINE
typename CCellContainer<TCellType>::TCellIterator
CCellContainer<TCellType>::Erase(
    typename CCellContainer<TCellType>::TCellIterator cell)
{
    return CMap<SRowCol, TCellType>::erase(cell);
}

///////////////////////////////////////////////////////////////////////
//  CCellContainer::GetCellSize

template<class TCellType>
ST_INLINE st_float32 CCellContainer<TCellType>::GetCellSize(void) const
{
    return m_fCellSize;
}


///////////////////////////////////////////////////////////////////////
//  CCellContainer::SetCellSize

template<class TCellType>
ST_INLINE void CCellContainer<TCellType>::SetCellSize(st_float32 fCellSize)
{
    m_fCellSize = fCellSize;
}




