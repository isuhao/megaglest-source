// ==============================================================
//	This file is part of Glest Shared Library (www.glest.org)
//
//	Copyright (C) 2009-2010 Titus Tscharntke (info@titusgames.de) and
//                          Mark Vejvoda (mark_vejvoda@hotmail.com)
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#ifndef _SHARED_PLATFORMCOMMON_CACHEMANAGER_H_
#define _SHARED_PLATFORMCOMMON_CACHEMANAGER_H_

#include "thread.h"
#include <map>
#include <string>
#include <stdexcept>
#include "platform_util.h"
#include "leak_dumper.h"

using namespace std;
using namespace Shared::Platform;

namespace Shared { namespace PlatformCommon {

// =====================================================
//	class BaseThread
// =====================================================

class CacheManager
{
public:

static const char *getFolderTreeContentsCheckSumRecursivelyCacheLookupKey1;
static const char *getFolderTreeContentsCheckSumRecursivelyCacheLookupKey2;
static const char *getFolderTreeContentsCheckSumListRecursivelyCacheLookupKey1;
static const char *getFolderTreeContentsCheckSumListRecursivelyCacheLookupKey2;

protected:
	static std::map<string, Mutex *> itemCacheMutexList;
	static Mutex mutexMap;
	typedef enum {
		cacheItemGet,
		cacheItemSet
	} CacheAccessorType;

	template <typename T>
	static Mutex & manageCachedItemMutex(string cacheKey) {
		if(itemCacheMutexList.find(cacheKey) == itemCacheMutexList.end()) {
			MutexSafeWrapper safeMutex(&mutexMap);
			if(itemCacheMutexList.find(cacheKey) == itemCacheMutexList.end()) {
				itemCacheMutexList[cacheKey] = new Mutex(CODE_AT_LINE);
			}
			safeMutex.ReleaseLock();
		}
		Mutex *mutex = itemCacheMutexList[cacheKey];
		return *mutex;
	}

	template <typename T>
	static T & manageCachedItem(string cacheKey, T *value,CacheAccessorType accessor) {
		// Here is the actual type-safe instantiation
		static std::map<string, T > itemCache;
		if(accessor == cacheItemSet) {
			if(value == NULL) {
				try {
					Mutex &mutexCache = manageCachedItemMutex<T>(cacheKey);
					MutexSafeWrapper safeMutex(&mutexCache);
					if(itemCache.find(cacheKey) != itemCache.end()) {
						itemCache.erase(cacheKey);
					}
					safeMutex.ReleaseLock();
				}
				catch(const std::exception &ex) {
					throw megaglest_runtime_error(ex.what());
				}

			}
			if(value != NULL) {
				try {
					Mutex &mutexCache = manageCachedItemMutex<T>(cacheKey);
					MutexSafeWrapper safeMutex(&mutexCache);
					itemCache[cacheKey] = *value;
					safeMutex.ReleaseLock();
				}
				catch(const std::exception &ex) {
					throw megaglest_runtime_error(ex.what());
				}
			}
		}
		// If this is the first access we return a default object of the type
		Mutex &mutexCache = manageCachedItemMutex<T>(cacheKey);
		MutexSafeWrapper safeMutex(&mutexCache);

		return itemCache[cacheKey];
	}

public:

	CacheManager() { }
	static void cleanupMutexes() {
		MutexSafeWrapper safeMutex(&mutexMap);
		for(std::map<string, Mutex *>::iterator iterMap = itemCacheMutexList.begin();
			iterMap != itemCacheMutexList.end(); iterMap++) {
			delete iterMap->second;
			iterMap->second = NULL;
		}
		itemCacheMutexList.clear();
		safeMutex.ReleaseLock();
	}
	~CacheManager() {
		CacheManager::cleanupMutexes();
	}

	template <typename T>
	static void setCachedItem(string cacheKey, const T value) {
		manageCachedItem<T>(cacheKey,value,cacheItemSet);
	}
	template <typename T>
	static T & getCachedItem(string cacheKey) {
		return manageCachedItem<T>(cacheKey,NULL,cacheItemGet);
	}
	template <typename T>
	static void clearCachedItem(string cacheKey) {
		return manageCachedItem<T>(cacheKey,NULL,cacheItemSet);
	}

	template <typename T>
	static Mutex & getMutexForItem(string cacheKey) {
		return manageCachedItemMutex<T>(cacheKey);
	}
};

}}//end namespace

#endif
