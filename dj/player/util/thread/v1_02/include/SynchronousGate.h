/*
 * This synchronization mechanism is similar to a gate. What
 * makes it synchronous is that Close blocks until another
 * thread hits the gate by calling Wait.
 * 
 */
#include <cyg/kernel/kapi.h>


class CSynchronousGate {
public:
	typedef enum {
		OPEN,
		CLOSING,
		CLOSED,
	} state_t;

	CSynchronousGate(state_t);
	~CSynchronousGate();

	void Open();
	void Close();
	void AsyncClose();
//	void Release();
	void Wait();

protected:
	cyg_mutex_t _mutex;
	cyg_cond_t _wake;


	state_t _state;
};
