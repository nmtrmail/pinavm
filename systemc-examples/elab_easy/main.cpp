#include <systemc.h>

//using namespace sc_core;

#define KASCPAR

#define NS * 1e-9

SC_MODULE(Component)
{
public:
#ifdef KASCPAR
 	sc_out<bool> out;
 	sc_in<bool> in;
#else	
	sc_core::sc_out<bool> out;
	sc_core::sc_in<bool> in;
#endif
	
	bool notified;
	bool isHead;

	SC_CTOR(Component):
		out("Out"),
		in("in")
		{
			notified = false;
			isHead = false;
			SC_THREAD(process);
		}
	
 	void essaiwrite();

	void process() {
		if (isHead == false) {
			while (in.read() == false) {
#ifdef KASCPAR
				wait(5, SC_NS);
#else
				sc_core::wait(5, SC_NS);
#endif
			}
		}
		notified = true;
		out.write(true);
	}
};

void Component::essaiwrite()
{
	;
}

int sc_main (int argc , char *argv[]) 
{
#ifdef KASCPAR
 	sc_signal<bool> s1;
 	sc_signal<bool> s2;
 	sc_signal<bool> s3;
#else
	sc_core::sc_signal<bool> s1("s1"), s2("s2"), s3("s3");
#endif

	Component C1("C1");
	Component C2("C2");
	Component C3("C3");
       
	C1.out(s1);
	C2.out(s2);
	C3.out(s3);

	C1.in(s3);
	C2.in(s1);
	C3.in(s2);

	C1.isHead = true;

	//Run the Simulation for "200 nanosecnds"
	sc_start(200, SC_NS);

	return 0;
}