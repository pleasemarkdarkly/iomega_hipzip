#error "This test is currently broken"

#include <cyg/kernel/kapi.h>
#include <cyg/infra/diag.h>
#include <util/thread/ThreadedObject.h>
#include <util/thread/SynchronousEvent.h>


class WorkerThread : public IThreadedObject {
public:
	CSynchronousEvent<int,int> e;

	WorkerThread() : IThreadedObject(10, "Worker", 16384)
		{}

	void ThreadBody()  {
		int i;
		while(1) {
			if (e.Poll(&i)) {
				e.Acknowledge(i);
				diag_printf("....Processing\n");\
			}
			diag_printf("Wheee: %d\n",i);
			cyg_thread_delay(30);
		}
	}

};

WorkerThread t;


class EventTest : public IThreadedObject {
public:
	EventTest() : IThreadedObject(10, "EventTest", 16384)
		{}

	void ThreadBody()  {
		diag_printf("Hellooooooo\n");
		t.Start();
		for(int i=0; i<20; i++) {
			diag_printf("%d\n",t.e.Signal(i));
			cyg_thread_delay(47);
		}
	}
};


extern "C"
void cyg_user_start() {
	EventTest t;
	t.Start();
	cyg_scheduler_start();
}
