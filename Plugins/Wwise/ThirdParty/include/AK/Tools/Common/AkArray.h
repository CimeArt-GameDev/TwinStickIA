/*******************************************************************************
The content of this file includes portions of the AUDIOKINETIC Wwise Technology
released in source code form as part of the SDK installer package.

Commercial License Usage

Licensees holding valid commercial licenses to the AUDIOKINETIC Wwise Technology
may use this file in accordance with the end user license agreement provided 
with the software or, alternatively, in accordance with the terms contained in a
written agreement between you and Audiokinetic Inc.

Apache License Usage

Alternatively, this file may be used under the Apache License, Version 2.0 (the 
"Apache License"); you may not use this file except in compliance with the 
Apache License. You may obtain a copy of the Apache License at 
http://www.apache.org/licenses/LICENSE-2.0.

Unless required by applicable law or agreed to in writing, software distributed
under the Apache License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES
OR CONDITIONS OF ANY KIND, either express or implied. See the Apache License for
the specific language governing permissions and limitations under the License.

  Version: v2019.2.5  Build: 7349
  Copyright (c) 2006-2020 Audiokinetic Inc.
*******************************************************************************/

#ifndef _AKARRAY_H
#define _AKARRAY_H

#include <AK/Tools/Common/AkObject.h>
#include <AK/Tools/Common/AkAssert.h>
#include <AK/Tools/Common/AkPlatformFuncs.h>

template <AkMemID T_MEMID>
struct AkArrayAllocatorNoAlign
{
	AkForceInline void * Alloc( size_t in_uSize )
	{
		return AkAlloc(T_MEMID, in_uSize);
	}

	AkForceInline void * ReAlloc( void * in_pCurrent, size_t in_uOldSize, size_t in_uNewSize )
	{
		return AkRealloc(T_MEMID, in_pCurrent, in_uNewSize);
	}

	AkForceInline void Free( void * in_pAddress )
	{
		AkFree(T_MEMID, in_pAddress);
	}

	AkForceInline void TransferMem(void *& io_pDest, AkArrayAllocatorNoAlign<T_MEMID> in_srcAlloc, void * in_pSrc )
	{
		io_pDest = in_pSrc;
	}
};

template <AkMemID T_MEMID>
struct AkArrayAllocatorAlignedSimd
{
	AkForceInline void * Alloc( size_t in_uSize )
	{
		return AkMalign(T_MEMID, in_uSize, AK_SIMD_ALIGNMENT);
	}

	AkForceInline void * ReAlloc(void * in_pCurrent, size_t in_uOldSize, size_t in_uNewSize)
	{
		void* pNew = Alloc(in_uNewSize);
		if (pNew && in_pCurrent)
		{
			AKPLATFORM::AkMemCpy(pNew, in_pCurrent, (AkUInt32)in_uOldSize);
			Free(in_pCurrent);
		}
		return pNew;
	}

	AkForceInline void Free( void * in_pAddress )
	{
		AkFalign(T_MEMID, in_pAddress);
	}

	AkForceInline void TransferMem(void *& io_pDest, AkArrayAllocatorAlignedSimd<T_MEMID> in_srcAlloc, void * in_pSrc )
	{
		io_pDest = in_pSrc;
	}

};

// AkHybridAllocator
//	Attempts to allocate from a small buffer of size uBufferSizeBytes, which is contained within the array type.  Useful if the array is expected to contain a small number of elements.
//	If the array grows to a larger size than uBufferSizeBytes, the the memory is allocated with the specified AkMemID.
//	NOTE: only use with types that are trivially copyable.
template< AkUInt32 uBufferSizeBytes, AkUInt8 uAlignmentSize = AK_OS_STRUCT_ALIGN, AkMemID T_MEMID = AkMemID_Object>
struct AkHybridAllocator
{
	static const AkUInt32 _uBufferSizeBytes = uBufferSizeBytes;

	AkForceInline void * Alloc(size_t in_uSize)
	{
		if (in_uSize <= uBufferSizeBytes)
			return (void *)&m_buffer;
		else
			return AkMalign(T_MEMID, in_uSize, uAlignmentSize);
	}

	AkForceInline void * ReAlloc( void * in_pCurrent, size_t in_uOldSize, size_t in_uNewSize )
	{
		void* pNew = Alloc(in_uNewSize);
		if (pNew != in_pCurrent && pNew && in_pCurrent)
		{
			AKPLATFORM::AkMemCpy(pNew, in_pCurrent, (AkUInt32)in_uOldSize);
			Free(in_pCurrent);
		}
		return pNew;
	}

	AkForceInline void Free(void * in_pAddress)
	{
		if (&m_buffer != in_pAddress)
			AkFalign(T_MEMID, in_pAddress);
	}

	AkForceInline void TransferMem(void *& io_pDest, AkHybridAllocator<uBufferSizeBytes, uAlignmentSize, T_MEMID>& in_srcAlloc, void * in_pSrc)
	{
		if (&in_srcAlloc.m_buffer == in_pSrc)
		{
			AKPLATFORM::AkMemCpy(m_buffer, in_srcAlloc.m_buffer, uBufferSizeBytes);
			io_pDest = m_buffer;
		}
		else
		{
			io_pDest = in_pSrc;
		}
	}
	
	AK_ALIGN(char m_buffer[uBufferSizeBytes], uAlignmentSize);
};

template <class T>
struct AkAssignmentMovePolicy
{
	// By default the assignment operator is invoked to move elements of an array from slot to slot.  If desired,
	//	a custom 'Move' operation can be passed into TMovePolicy to transfer ownership of resources from in_Src to in_Dest.
	static AkForceInline void Move( T& in_Dest, T& in_Src )
	{
		in_Dest = in_Src;
	}

	static AkForceInline bool IsTrivial()
	{
		return true;
	}
};

// Can be used as TMovePolicy to create arrays of arrays.
template <class T>
struct AkTransferMovePolicy
{
	static AkForceInline void Move( T& in_Dest, T& in_Src )
	{
		in_Dest.Transfer(in_Src); //transfer ownership of resources.
	}

	static AkForceInline bool IsTrivial()
	{
		return false;
	}
};

// Common allocators:
typedef AkArrayAllocatorNoAlign<AkMemID_Object> ArrayPoolDefault;
typedef AkArrayAllocatorNoAlign<AkMemID_Processing> ArrayPoolLEngineDefault;
typedef AkArrayAllocatorAlignedSimd<AkMemID_Processing> ArrayPoolLEngineDefaultAlignedSimd;

struct AkGrowByPolicy_Legacy
{
	static AkUInt32 GrowBy( AkUInt32 /*in_CurrentArraySize*/ ) { return 1; }
};

struct AkGrowByPolicy_NoGrow
{
	static AkUInt32 GrowBy( AkUInt32 /*in_CurrentArraySize*/ ) { return 0; }
};

struct AkGrowByPolicy_Proportional
{
	static AkUInt32 GrowBy( AkUInt32 in_CurrentArraySize )
	{
		if ( in_CurrentArraySize == 0 )
			return 1;
		else
			return in_CurrentArraySize + ( in_CurrentArraySize >> 1 );
	}
};

//#define AkGrowByPolicy_DEFAULT AkGrowByPolicy_Legacy
#define AkGrowByPolicy_DEFAULT AkGrowByPolicy_Proportional

/// Specific implementation of array
template <class T, class ARG_T, class TAlloc = ArrayPoolDefault, class TGrowBy = AkGrowByPolicy_DEFAULT, class TMovePolicy = AkAssignmentMovePolicy<T> > class AkArray : public TAlloc
{
public:
	/// Constructor
	AkArray()
		: m_pItems( 0 )
		, m_uLength( 0 )
		, m_ulReserved( 0 )
	{
	}

	/// Destructor
	~AkArray()
	{
		AKASSERT( m_pItems == 0 );
		AKASSERT( m_uLength == 0 );
		AKASSERT( m_ulReserved == 0 );
	}

// Workaround for SWIG to parse nested structure: 
// Bypass this inner struct and use a proxy in a separate header.
#ifndef SWIG
	/// Iterator
	struct Iterator
	{
		T* pItem;	///< Pointer to the item in the array.

		/// + operator
		Iterator operator+(AkUInt32 inc) const
		{
			AKASSERT( pItem );
			Iterator returnedIt;
			returnedIt.pItem = pItem + inc;
			return returnedIt;
		}

        /// - operator
        AkUInt32 operator-(Iterator const& rhs) const
        {
			AKASSERT((pItem && rhs.pItem)||(!pItem && !rhs.pItem));
			return (AkUInt32)(pItem - rhs.pItem);
        }

		/// ++ operator
		Iterator& operator++()
		{
			AKASSERT( pItem );
			++pItem;
			return *this;
		}

		/// -- operator
        Iterator& operator--()
		{
			AKASSERT( pItem );
			--pItem;
			return *this;
		}

		/// * operator
		T& operator*()
		{
			AKASSERT( pItem );
			return *pItem;
		}

		/// == operator
		bool operator ==( const Iterator& in_rOp ) const
		{
			return ( pItem == in_rOp.pItem );
		}

		/// != operator
		bool operator !=( const Iterator& in_rOp ) const
		{
			return ( pItem != in_rOp.pItem );
		}
	};
#endif // #ifndef SWIG

	/// Returns the iterator to the first item of the array, will be End() if the array is empty.
	Iterator Begin() const
	{
		Iterator returnedIt;
		returnedIt.pItem = m_pItems;
		return returnedIt;
	}

	/// Returns the iterator to the end of the array
	Iterator End() const
	{
		Iterator returnedIt;
		returnedIt.pItem = m_pItems + m_uLength;
		return returnedIt;
	}

	/// Returns the iterator th the specified item, will be End() if the item is not found
	Iterator FindEx( ARG_T in_Item ) const
	{
		Iterator it = Begin();

		for ( Iterator itEnd = End(); it != itEnd; ++it )
		{
			if ( *it == in_Item )
				break;
		}

		return it;
	}

	/// Returns the iterator of the specified item, will be End() if the item is not found
	/// The array must be in ascending sorted order.
	Iterator BinarySearch( ARG_T in_Item ) const
	{
		AkUInt32 uNumToSearch = Length();
		T* pBase = m_pItems;
		T* pPivot;

		while ( uNumToSearch > 0 )
		{
			pPivot = pBase + ( uNumToSearch >> 1 );
			if ( in_Item == *pPivot )
			{
				Iterator result;
				result.pItem = pPivot;
				return result;
			}

			if ( in_Item > *pPivot )
			{
				pBase = pPivot + 1;
				uNumToSearch--;
			}
			uNumToSearch >>= 1;
		}

		return End();
	}

	/// Erase the specified iterator from the array
	Iterator Erase( Iterator& in_rIter )
	{
		AKASSERT( m_pItems != 0 );

		// Move items by 1

		T * pItemLast = m_pItems + m_uLength - 1;

		for ( T * pItem = in_rIter.pItem; pItem < pItemLast; pItem++ )
			TMovePolicy::Move( pItem[ 0 ], pItem[ 1 ] );

		// Destroy the last item

		pItemLast->~T();

		m_uLength--;

		return in_rIter;
	}

	/// Erase the item at the specified index
    void Erase( unsigned int in_uIndex )
	{
		AKASSERT( m_pItems != 0 );

		// Move items by 1

		T * pItemLast = m_pItems + m_uLength - 1;

		for ( T * pItem = m_pItems+in_uIndex; pItem < pItemLast; pItem++ )
			TMovePolicy::Move( pItem[ 0 ], pItem[ 1 ] );

		// Destroy the last item

		pItemLast->~T();

		m_uLength--;
	}

	/// Erase the specified iterator in the array. but it dos not guarantee the ordering in the array.
	/// This version should be used only when the order in the array is not an issue.
	Iterator EraseSwap( Iterator& in_rIter )
	{
		AKASSERT( m_pItems != 0 );

		if ( Length( ) > 1 )
		{
			// Swap last item with this one.
			TMovePolicy::Move( *in_rIter.pItem, Last( ) );
		}

		// Destroy.
		AKASSERT( Length( ) > 0 );
		Last( ).~T();

		m_uLength--;

		return in_rIter;
	}

	bool IsGrowingAllowed()
	{
		return TGrowBy::GrowBy( 1 ) != 0;
	}

	/// Pre-Allocate a number of spaces in the array
	AKRESULT Reserve( AkUInt32 in_ulReserve )
	{
		AKASSERT( m_pItems == 0 && m_uLength == 0 );
		AKASSERT( in_ulReserve || IsGrowingAllowed() );

		if ( in_ulReserve )
		{
			m_pItems = (T *) TAlloc::Alloc( sizeof( T ) * in_ulReserve );
			if ( m_pItems == 0 )
				return AK_InsufficientMemory;

			m_ulReserved = in_ulReserve;
		}

		return AK_Success;
	}

	AkUInt32 Reserved() const { return m_ulReserved; }

	/// Term the array. Must be called before destroying the object.
	void Term()
	{
		if ( m_pItems )
		{
			RemoveAll();
			TAlloc::Free( m_pItems );
			m_pItems = 0;
			m_ulReserved = 0;
		}
	}

	/// Returns the numbers of items in the array.
	AkForceInline AkUInt32 Length() const
	{
		return m_uLength;
	}

	/// Returns a pointer to the first item in the array.
	AkForceInline T * Data() const
	{
		return m_pItems;
	}

	/// Returns true if the number items in the array is 0, false otherwise.
	AkForceInline bool IsEmpty() const
	{
		return m_uLength == 0;
	}
	
	/// Returns a pointer to the specified item in the list if it exists, 0 if not found.
	AkForceInline T* Exists(ARG_T in_Item) const
	{
		Iterator it = FindEx( in_Item );
		return ( it != End() ) ? it.pItem : 0;
	}

	/// Add an item in the array, without filling it.
	/// Returns a pointer to the location to be filled.
	AkForceInline T * AddLast()
	{
		size_t cItems = Length();

#if defined(_MSC_VER)
#pragma warning( push )
#pragma warning( disable : 4127 )
#endif
		if ( ( cItems >= m_ulReserved ) && IsGrowingAllowed() )
		{
			if ( !GrowArray() ) 
				return 0;
		}
#if defined(_MSC_VER)
#pragma warning( pop )
#endif

		// have we got space for a new one ?
		if(  cItems < m_ulReserved )
		{
			T * pEnd = m_pItems + m_uLength++;
			AkPlacementNew( pEnd ) T; 
			return pEnd;
		}

		return 0;
	}

	/// Add an item in the array, and fills it with the provided item.
	AkForceInline T * AddLast(ARG_T in_rItem)
	{
		T * pItem = AddLast();
		if ( pItem )
			*pItem = in_rItem;
		return pItem;
	}

	/// Returns a reference to the last item in the array.
	T& Last()
	{
		AKASSERT( m_uLength );

		return *( m_pItems + m_uLength - 1 );
	}

	/// Removes the last item from the array.
    void RemoveLast()
    {
        AKASSERT( m_uLength );
        ( m_pItems + m_uLength - 1 )->~T();
        m_uLength--;
    }

	/// Removes the specified item if found in the array.
	AKRESULT Remove(ARG_T in_rItem)
	{
		Iterator it = FindEx( in_rItem );
		if ( it != End() )
		{
			Erase( it );
			return AK_Success;
		}

		return AK_Fail;
	}

	/// Fast remove of the specified item in the array.
	/// This method do not guarantee keeping ordering of the array.
	AKRESULT RemoveSwap(ARG_T in_rItem)
	{
		Iterator it = FindEx( in_rItem );
		if ( it != End() )
		{
			EraseSwap( it );
			return AK_Success;
		}

		return AK_Fail;
	}

	/// Removes all items in the array
	void RemoveAll()
	{
		for ( Iterator it = Begin(), itEnd = End(); it != itEnd; ++it )
			(*it).~T();
		m_uLength = 0;
	}

	/// Operator [], return a reference to the specified index.
	AkForceInline T& operator[](unsigned int uiIndex) const
    {
        AKASSERT( m_pItems );
        AKASSERT( uiIndex < Length() );
        return m_pItems[uiIndex];
    }

	/// Insert an item at the specified position without filling it.
	/// Returns the pointer to the item to be filled.
	T * Insert(unsigned int in_uIndex)
	{
        AKASSERT( in_uIndex <= Length() );

		size_t cItems = Length();

#if defined(_MSC_VER)
#pragma warning( push )
#pragma warning( disable : 4127 )
#endif
		if ( ( cItems >= m_ulReserved ) && IsGrowingAllowed() )
		{
			if ( !GrowArray() ) 
				return 0;
		}
#if defined(_MSC_VER)
#pragma warning( pop )
#endif

		// have we got space for a new one ?
		if(  cItems < m_ulReserved )
		{
			T * pItemLast = m_pItems + m_uLength++;
			AkPlacementNew( pItemLast ) T; 

			// Move items by 1

			for ( T * pItem = pItemLast; pItem > ( m_pItems + in_uIndex ); --pItem )
				TMovePolicy::Move( pItem[ 0 ], pItem[ -1 ] );

			// Reinitialize item at index

			( m_pItems + in_uIndex )->~T();
			AkPlacementNew( m_pItems + in_uIndex ) T; 

			return m_pItems + in_uIndex;
		}

		return 0;
	}

	bool GrowArray()
	{
		// If no size specified, growing by the declared growth policy of the array.
		return GrowArray( TGrowBy::GrowBy( m_ulReserved ) );
	}

	/// Resize the array.
	bool GrowArray( AkUInt32 in_uGrowBy )
	{
		AKASSERT( in_uGrowBy );

		AkUInt32 ulNewReserve = m_ulReserved + in_uGrowBy;
		T * pNewItems = NULL;
		size_t cItems = Length();
		if (TMovePolicy::IsTrivial())
		{
			pNewItems = (T *)TAlloc::ReAlloc(m_pItems, sizeof(T) * cItems, sizeof(T) * ulNewReserve);
			if (!pNewItems)
				return false;
		}
		else
		{
			pNewItems = (T *)TAlloc::Alloc(sizeof(T) * ulNewReserve);
			if (!pNewItems)
				return false;

			// Copy all elements in new array, destroy old ones
			if (m_pItems && m_pItems != pNewItems /*AkHybridAllocator may serve up same memory*/)
			{
				for (size_t i = 0; i < cItems; ++i)
				{
					AkPlacementNew(pNewItems + i) T;

					TMovePolicy::Move(pNewItems[i], m_pItems[i]);

					m_pItems[i].~T();
				}

				TAlloc::Free(m_pItems);
			}
		}

		m_pItems = pNewItems;
		m_ulReserved = ulNewReserve;
		return true;
	}

	/// Resize the array to the specified size.
	bool Resize(AkUInt32 in_uiSize)
	{
		AkUInt32 cItems = Length();
		if (in_uiSize < cItems)
		{
			//Destroy superfluous elements
			for(AkUInt32 i = in_uiSize - 1 ; i < cItems; i++)
			{
				m_pItems[ i ].~T();
			}
			m_uLength = in_uiSize;
			return true;
		}

		if ( in_uiSize > m_ulReserved )
		{
			if ( !GrowArray(in_uiSize - cItems) ) 
				return false;
		}

		//Create the missing items.
		for(size_t i = cItems; i < in_uiSize; i++)
		{
			AkPlacementNew( m_pItems + i ) T; 
		}

		m_uLength = in_uiSize;
		return true;
	}

	void Transfer(AkArray<T,ARG_T,TAlloc,TGrowBy,TMovePolicy>& in_rSource)
	{
		Term();

		TAlloc::TransferMem( (void*&)m_pItems, in_rSource, (void*)in_rSource.m_pItems );
		m_uLength = in_rSource.m_uLength;
		m_ulReserved = in_rSource.m_ulReserved;

		in_rSource.m_pItems = NULL;
		in_rSource.m_uLength = 0;
		in_rSource.m_ulReserved = 0;
	}

	AKRESULT Copy(const AkArray<T, ARG_T, TAlloc, TGrowBy, TMovePolicy>& in_rSource)
	{
		Term();

		if (Resize(in_rSource.Length()))
		{
			for (AkUInt32 i = 0; i < in_rSource.Length(); ++i)
				m_pItems[i] = in_rSource.m_pItems[i];
			return AK_Success;
		}
		return AK_Fail;
	}

protected:

	T *         m_pItems;		///< pointer to the beginning of the array.
	AkUInt32    m_uLength;		///< number of items in the array.
	AkUInt32	m_ulReserved;	///< how many we can have at most (currently allocated).
};


#endif
