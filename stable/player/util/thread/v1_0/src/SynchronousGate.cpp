#include <util/thread/SynchronousGate.h>
#include <cyg/kernel/kapi.h>
#include <cyg/infra/diag.h>

CSynchronousGate::CSynchronousGate(state_t state)
{
	cyg_mutex_init(&_mutex);
	cyg_cond_init(&_wake, &_mutex);
	_state = state;
}

CSynchronousGate::~CSynchronousGate()
{
	cyg_mutex_lock(&_mutex);
	cyg_cond_broadcast(&_wake);
	cyg_cond_destroy(&_wake);
	cyg_mutex_unlock(&_mutex);
	cyg_mutex_destroy(&_mutex);
}

void
CSynchronousGate::Open()
{
	cyg_mutex_lock(&_mutex);
	_state = OPEN;
	cyg_cond_broadcast(&_wake);
	cyg_mutex_unlock(&_mutex);
}

void
CSynchronousGate::Close()
{
	cyg_mutex_lock(&_mutex);
	_state = CLOSING;
	cyg_cond_broadcast(&_wake);

	while (_state == CLOSING)
		cyg_cond_wait(&_wake);

	_state = CLOSED;
	cyg_mutex_unlock(&_mutex);
}

void
CSynchronousGate::AsyncClose()
{
	cyg_mutex_lock(&_mutex);
	_state = CLOSED;
	cyg_cond_broadcast(&_wake);
	cyg_mutex_unlock(&_mutex);
}

void
CSynchronousGate::Wait()
{
	cyg_mutex_lock(&_mutex);
	while (_state != OPEN) {
		if (_state == CLOSING) {
			_state = CLOSED;
			cyg_cond_broadcast(&_wake);
		}
		cyg_cond_wait(&_wake);
	}

	cyg_mutex_unlock(&_mutex);
}
