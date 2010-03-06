#ifndef XL_VIEW_CLASS_WITH_THREADS_H
#define XL_VIEW_CLASS_WITH_THREADS_H
#include <Windows.h>
#include "libxl/include/string.h"

/**
 * usage:
 * class YourClass : public ClassWithThreadT<YourClass, N>
 * {
	friend class ClassWithThreadT<YourClass, N>;
   private:
  	const xl::tchar* getThreadName();
  	void assignThreadProc();
	void markThreadExit();
	void _Lock();
	void _Unlock();
 * }
 */

template <class T, int THREAD_COUNT>
class ClassWithThreadT
{
	static const int dummy = sizeof(int[THREAD_COUNT]);// make sure the count > 0
protected:
	typedef unsigned (__stdcall *_ThreadProc) (void *);

	HANDLE m_hEvents[THREAD_COUNT];
	HANDLE m_hThreads[THREAD_COUNT];
	_ThreadProc m_procThreads[THREAD_COUNT];

	ClassWithThreadT ()
	{
		for (int i = 0; i < THREAD_COUNT; ++ i) {
			m_hEvents[i] = NULL;
			m_hThreads[i] = INVALID_HANDLE_VALUE;
			m_procThreads[i] = NULL;
		}
	}

	void _CreateThreads () {
		int dummy = sizeof(int[(THREAD_COUNT == T::THREAD_COUNT) ? 1 : 0]);
		dummy = dummy;
		xl::tchar name[MAX_PATH];
		
		T *p = (T *)this;
		p->_AssignThreadProc();
		const xl::tchar *eventName = p->_GetThreadName();
		p->_Lock();
		for (int i = 0; i < THREAD_COUNT; ++ i) {
			assert(m_hEvents[i] == NULL);
			assert(m_hThreads[i] == INVALID_HANDLE_VALUE);
			assert(m_procThreads[i] != NULL);

			_stprintf_s(name, MAX_PATH,
				_T("Local\\%s[%d] [created at %d]"), eventName, i, ::GetTickCount());
			m_hEvents[i] = ::CreateEvent(NULL, FALSE, FALSE, name);
			assert(m_hEvents[i] != NULL);
			m_hThreads[i] = (HANDLE)_beginthreadex(NULL, 0, m_procThreads[i], (T *)this, 0, NULL);
			assert(m_hThreads[i] != INVALID_HANDLE_VALUE);
		}
		p->_Unlock();
	}

	void _TerminateThreads () {
		T *p = (T *)this;
		p->_Lock();
		p->_MarkThreadExit();
#ifndef NDEBUG
		for (int i = 0; i < THREAD_COUNT; ++ i) {
			assert(m_hEvents[i] != NULL);
			assert(m_hThreads[i] != INVALID_HANDLE_VALUE);
		}
#endif

		for (int i = 0; i < THREAD_COUNT; ++ i) {
			::SetEvent(m_hEvents[i]);
		}

		p->_Unlock(); // the thread maybe want the lock, so we unlock it

		if (::WaitForMultipleObjects(THREAD_COUNT, m_hThreads, TRUE, 3000) == WAIT_TIMEOUT) {
			for (int i = 0; i < THREAD_COUNT; ++ i) {
				if (::WaitForSingleObject(m_hThreads[i], 0) == WAIT_TIMEOUT) {
					::TerminateThread(m_hThreads[i], (DWORD)-1);
					XLTRACE(_T("%s [%d] does not exit normally\n"), p->_GetThreadName(), i);
				}
			}
		}

		for (int i = 0; i < THREAD_COUNT; ++ i) {
			CloseHandle(m_hEvents[i]);
			m_hEvents[i] = NULL;
			CloseHandle(m_hThreads[i]);
			m_hThreads[i] = INVALID_HANDLE_VALUE;
		}
	}

	void _RunThread (size_t i) {
		T *p = (T *)this;
		p->_Lock();
		assert(i < THREAD_COUNT);
		assert(m_hEvents[i] != NULL);

		::SetEvent(m_hEvents[i]);
		p->_Unlock();
	}
};


#endif
