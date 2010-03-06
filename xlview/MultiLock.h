#ifndef XL_VIEW_MULTILOCK_H
#define XL_VIEW_MULTILOCK_H
/**
 * For avoiding deadlock, we should make sure that
 * all the locks be acquired in the same order, in
 * the application wise, so there is CMultiLock.
 *
 * The order is, first external lock, than it self
 */
#include <assert.h>
#include "libxl/include/common.h"
#include "libxl/include/lockable.h"

class CMultiLock : private xl::CUserLock {
	xl::ILockable    *m_pExternalLock;

public:
	CMultiLock (xl::ILockable *pExternalLock)
		: m_pExternalLock(pExternalLock)
	{
		assert(m_pExternalLock != NULL);
	}

	void lockAll () const {
		m_pExternalLock->lock();
		lock();
	}

	void unlockAll () const {
		unlock();
		m_pExternalLock->unlock();
	}

	void lockMe () const {
		lock();
	}

	void unlockMe () const {
		unlock();
	}

	using CUserLock::getLockLevel;
};

class CScopeMultiLock {
	CMultiLock       *m_pLock;
	bool              m_lockAll;
public:
	CScopeMultiLock (CMultiLock * pLock, bool lockAll)
		: m_pLock(NULL)
		, m_lockAll(true)
	{
		lock (pLock, lockAll);
	}

	~CScopeMultiLock () {
		if (m_pLock) {
			unlock();
		}
	}

	void lock (CMultiLock *pLock, bool lockAll) {
		assert(m_pLock == NULL);
		assert(pLock != NULL);
		m_pLock = pLock;
		m_lockAll = lockAll;
		m_lockAll ? m_pLock->lockAll() : m_pLock->lockMe();
	}

	void unlock () {
		assert(m_pLock != NULL);
		m_lockAll ? m_pLock->unlockAll() : m_pLock->unlockMe();
		m_pLock = NULL;
	}
};


#endif

