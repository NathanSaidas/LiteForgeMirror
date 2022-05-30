// ********************************************************************
// Copyright (c) 2020 Nathan Hanlan
// 
// Permission is hereby granted, free of charge, to any person obtaining a 
// copy of this software and associated documentation files(the "Software"), 
// to deal in the Software without restriction, including without limitation 
// the rights to use, copy, modify, merge, publish, distribute, sublicense, 
// and / or sell copies of the Software, and to permit persons to whom the 
// Software is furnished to do so, subject to the following conditions :
// 
// The above copyright notice and this permission notice shall be included in 
// all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE 
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, 
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
// ********************************************************************
#pragma once

#include "Core/Reflection/Object.h"
#include "Runtime/Reflection/ReflectionTypes.h"
#include "AbstractEngine/World/EntityCollection.h"

namespace lf
{
    class World;

    // Dummy Type, for working with System Tuples
    struct SystemTupleVoid {};

    // Typedef for component tuple types 
    template<typename ComponentT>
    using TComponentTupleType = TVector< TVector<typename ComponentT::ComponentDataType>*>;

    // Recursive template type to count the amount of arguments in a System Tuple
    template<typename ComponentT, typename ... ArgsT>
    struct TSystemTupleCount
    {
        static constexpr SizeT VALUE = TSystemTupleCount<ArgsT...>::VALUE + 1;
    };
    template<>
    struct TSystemTupleCount<SystemTupleVoid>
    {
        static constexpr SizeT VALUE = 0;
    };

    template<typename ComponentT, typename ... ArgsT>
    struct TComponentSystemTupleBase
    {
        using TupleType = TComponentSystemTupleBase<ComponentT, ArgsT...>;
        using ComponentType = ComponentT;
        using ComponentDataType = typename ComponentT::ComponentDataType;
        using ArrayType = TVector<ComponentDataType>;
        static constexpr SizeT ARG_COUNT = TSystemTupleCount<ComponentT, ArgsT...>::VALUE;

        TComponentSystemTupleBase()
            : mComponents()
            , mNext()
        {
        }

        // Builds based off an existing array of entity collections
        void Initialize(TVector<EntityCollection*>& collections)
        {
            for (EntityCollection* collection : collections)
            {
                mCollections.push_back(collection->GetCurrentArray<ComponentT>());
            }
            mNext.Initialize(collections);
        }

        // Gets all the types 
        void GetTypes(TVector<const Type*>& includeTypes)
        {
            includeTypes.push_back(typeof(ComponentT));
            mNext.GetTypes(includeTypes);
        }

        // Query collections and bind to them
        void Bind(World* world, const TVector<const Type*>& excludeTypes)
        {
            Clear();

            TVector<const Type*> includeTypes;
            includeTypes.reserve(ARG_COUNT);

            GetTypes(includeTypes);
            TVector<EntityCollection*> collections = world->FindCollections(includeTypes, excludeTypes);
            Initialize(collections);
        }

        SizeT CollectionCount() const { return mCollections.size(); }
        SizeT Count(SizeT collectionID) const { return mCollections[collectionID]->size(); }
        SizeT Count() const
        {
            SizeT count = 0;
            for (SizeT i = 0; i < mCollections.size(); ++i)
            {
                count += mCollections[i]->size();
            }
            return count;
        }

        // Return pointer to item.
        ComponentDataType* GetItem(SizeT collectionID, SizeT itemID)
        {
            CriticalAssert(collectionID < CollectionCount() && itemID < Count(collectionID));
            return &(*mCollections[collectionID])[itemID];
        }

        // Recursive invoke function that invokes the callback given with the component data pointers
        // in the arrays;.
        template<typename CallbackT, typename ... ArgsT>
        void InvokeWithItems(const CallbackT& callback, SizeT collectionID, SizeT itemID, ArgsT ... args)
        {
            mNext.InvokeWithItems(callback, collectionID, itemID, args..., GetItem(collectionID, itemID));
        }

        template<typename CallbackT, typename ... ArgsT>
        void InvokeWithEntityItems(const CallbackT& callback, SizeT collectionID, SizeT itemID)
        {
            EntityId id = GetEntityId(collectionID, itemID);
            InvokeWithItems(callback, collectionID, itemID, id);
        }

        EntityId GetEntityId(SizeT collectionID, SizeT itemID)
        {
            return mNext.GetEntityId(collectionID, itemID);
        }

        void Clear()
        {
            mCollections.clear();
            mNext.Clear();
        }

        TVector<ArrayType*> mCollections;
        TComponentSystemTupleBase<ArgsT...> mNext;
    };
    // dummy template
    template<>
    struct TComponentSystemTupleBase<SystemTupleVoid>
    {
        TComponentSystemTupleBase()
        {}

        void Initialize(TVector<EntityCollection*>& collections) 
        {
            mCollections = collections;
        }
        void GetTypes(TVector<const Type*>&) {}

        template<typename CallbackT, typename ... ArgsT>
        void InvokeWithItems(const CallbackT& callback, SizeT, SizeT, ArgsT ... args)
        {
            callback(args...);
        }

        EntityId GetEntityId(SizeT collectionID, SizeT itemID)
        {
            return mCollections[collectionID]->GetEntity(itemID);
        }

        void Clear(){}
        TVector<EntityCollection*> mCollections;
    };

    // Typedef this 'Builder' in your tuple type, this will allow you easy tuple creation/iteration/manipulation.
    template<typename ... ArgsT>
    using TComponentSystemTuple = TComponentSystemTupleBase<ArgsT..., SystemTupleVoid>;
}
